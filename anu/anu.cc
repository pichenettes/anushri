// Copyright 2012 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "avrlib/boot.h"
#include "avrlib/gpio.h"
#include "avrlib/parallel_io.h"
#include "avrlib/timer.h"
#include "avrlib/watchdog_timer.h"

#include "anu/audio_buffer.h"
#include "anu/clock.h"
#include "anu/dco_controller.h"
#include "anu/hardware_config.h"
#include "anu/midi_dispatcher.h"
#include "anu/parameter.h"
#include "anu/ui.h"
#include "anu/voice_controller.h"
#include "anu/voice_tuner.h"

#include "midi/midi.h"

using namespace avrlib;
using namespace anu;
using namespace midi;


DAC1SS dac_1_ss;
DAC2SS dac_2_ss;
DACInterface dac;

PitchCalibrationInput pitch_calibration_input;
DcoOut dco_out;

MidiIO midi_io;
MidiBuffer midi_in_buffer;
MidiStreamParser<MidiDispatcher> midi_parser;

Timer<0> dac_timer;
Timer<2> clock_timer;
TuningTimer tuning_timer;

PwmOutput<3> audio_out;

volatile uint8_t refresh_counter = 0;
volatile uint16_t clock_ticks = 0;
volatile uint8_t num_external_clock_events = 0;

inline void FlushMidiOut() {
  // Try to flush the high priority buffer first.
  if (midi_dispatcher.readable_high_priority()) {
    if (midi_io.writable()) {
      midi_io.Overwrite(midi_dispatcher.ImmediateReadHighPriority());
    }
  } else {
    if (midi_dispatcher.readable_low_priority()) {
      if (midi_io.writable()) {
        midi_io.Overwrite(midi_dispatcher.ImmediateReadLowPriority());
      }
    }
  }
}

inline void PollMidiIn() {
  if (midi_io.readable()) {
    midi_in_buffer.NonBlockingWrite(midi_io.ImmediateRead());
  }
}

inline bool UpdateDACs() {
  Word dac_value;
  voice_controller.mutable_voice()->ReadDACStateSample();
  const DACState& dac_state = voice_controller.voice().dac_state();
  
  // Write VCO CV.
  // 0.5V / Oct.
  // 2.048V = Midi note 60 = 261.625
  dac_value.value = 0x1000 | dac_state.vco_cv;
  dac_1_ss.Low();
  dac.Send(dac_value.bytes[1]);
  dac.Send(dac_value.bytes[0]);
  dac_1_ss.High();
  
  // Write VCF CV.
  // 0.5V / Oct.
  dac_value.value = 0x9000 | dac_state.vcf_cv;
  dac_1_ss.Low();
  dac.Send(dac_value.bytes[1]);
  dac.Send(dac_value.bytes[0]);
  dac_1_ss.High();
  
  // Write VCA CV.
  dac_value.value = 0x1000 | dac_state.vca_cv;
  dac_2_ss.Low();
  dac.Send(dac_value.bytes[1]);
  dac.Send(dac_value.bytes[0]);
  dac_2_ss.High();
  
  // Write PWM CV.
  dac_value.value = 0x9000 | dac_state.pw_cv;
  dac_2_ss.Low();
  dac.Send(dac_value.bytes[1]);
  dac.Send(dac_value.bytes[0]);
  dac_2_ss.High();
  
  return dac_state.vca_cv != 0;
}

static uint8_t cycle = 0;
static uint8_t previous_in = 0xff;

// 4.9kHz: MIDI in/out; gate & clock poll.
// 2.45kHz: DAC & DCO refresh
// 0.61kHz: UI poll
ISR(TIMER0_OVF_vect, ISR_NOBLOCK) {
  if (cycle & 1) {
    if (UpdateDACs()) {
      dco_controller.set_note(voice_controller.voice().dco_pitch());
    } else {
      // Mute the DCO when no note is playing to avoid signal bleed through the
      // drums channel.
      dco_controller.Mute();
    }
  }
  PollMidiIn();
  FlushMidiOut();
  
  // Read the input shift register without updating the switch debounce state.
  uint8_t in = inputs.ReadRegister();
  
  // Detect raising edges on the Trig line.
  if ((in & (1 << INPUT_TRIG)) && !(previous_in & (1 << INPUT_TRIG))) {
    ++num_external_clock_events;
  }
  // Detect raising and falling edges on the Gate line.
  if ((in & (1 << INPUT_GATE)) && !(previous_in & (1 << INPUT_GATE))) {
    voice_controller.GateOn();
  }
  if (!(in & (1 << INPUT_GATE)) && (previous_in & (1 << INPUT_GATE))) {
    voice_controller.GateOff();
  }
  previous_in = in;
  
  // Writes to the output shift register (LED and Gate/Trig).
  uint8_t out = ui.led_pattern();
  if (voice_controller.voice().gate() &&
      !voice_controller.voice().retriggered()) {
    out |= _BV(OUTPUT_GATE);
  }
  voice_controller.mutable_voice()->ClearRetriggeredFlag();
  if (voice_controller.clock_pulse()) {
    out |= _BV(OUTPUT_TRIG);
    voice_controller.ClearClockPulse();
  }
  
  if ((cycle & 0x7) == 0) {
    TickSystemClock();
    inputs.Process(in);
    ui.Poll();
  }
  outputs.Write(out);
  ++cycle;
}

// 2.5 MHz timebase used for oscillators calibration, and for DCO.

volatile uint8_t num_overflows;

ISR(TIMER1_CAPT_vect) {
  static uint16_t previous_time = 0;
  uint16_t current_time = ICR1;
  uint32_t interval = current_time + \
      (static_cast<uint32_t>(num_overflows) << 16) - previous_time;
  previous_time = current_time;
  num_overflows = 0;
  voice_tuner.UpdatePitchMeasurement(interval);
}

ISR(TIMER1_OVF_vect, ISR_NOBLOCK) {
  ++num_overflows;
}

// 39kHz clock used for the tempo counter.
ISR(TIMER2_OVF_vect, ISR_NOBLOCK) {
  clock.Tick();
  audio_out.Write(audio_buffer.ImmediateRead());
}

inline void Init() {
  sei();
  UCSR0B = 0;
  ResetWatchdog();
  
  midi_io.Init();
  system_settings.Init();
  drum_synth.Init();
  voice_controller.Init();
  voice_tuner.Init();
  
  dac.Init();
  dac_1_ss.set_mode(DIGITAL_OUTPUT);
  dac_2_ss.set_mode(DIGITAL_OUTPUT);
  dco_out.set_mode(DIGITAL_OUTPUT);
  
  pitch_calibration_input.set_mode(DIGITAL_INPUT);
  pitch_calibration_input.Low();
  
  ui.Init();
  if (!(inputs.ReadRegister() & 0x4)) {
    voice_controller.ResetToFactoryDefaults();
    system_settings.ResetToFactoryDefaults();
  }
  
  dac_timer.set_prescaler(2);
  dac_timer.set_mode(TIMER_PWM_PHASE_CORRECT);
  dac_timer.Start();
  
  clock_timer.set_prescaler(1);
  clock_timer.set_mode(TIMER_PWM_PHASE_CORRECT);
  clock_timer.Start();
  
  audio_out.Init();
  
  dco_controller.Start();
}

int main(void) {
  Init();
  ui.FlushEvents();
  while (1) {
    // Fill some samples for the DACs.
    voice_controller.mutable_voice()->Refresh();
    
    // Fill some samples for the PWM out. To avoid getting the 40kHz PWM carrier 
    // when unnecessary, we set the output to 0 unless:
    // - The drum machine is configured to play a pattern.
    // - We have received a note message on MIDI channel 10, which is a hint
    //   that an external sequencer might trigger Anushri's drum synth.
    if (voice_controller.has_drums() ||
        midi_dispatcher.seen_midi_drum_events() ||
        drum_synth.playing()) {
      drum_synth.Render();
    } else {
      drum_synth.FillWithSilence();
    }
    
    // If we have not received any event on channel 10 for 5 mins, we consider
    // that no further event will come and we preventively disable the drum
    // engine.
    if (drum_synth.idle_time_ms() > 300000) {
      midi_dispatcher.ResetDrumEventMonitor();
    }

    // Count how many clock ticks have elapsed since the last refresh.
    if (voice_controller.internal_clock()) {
      uint8_t num_events = clock.CountEvents();
      while (num_events) {
        voice_controller.Clock(false);
        --num_events;
      }
      num_external_clock_events = 0;
    } else {
      while (num_external_clock_events) {
        voice_controller.Clock(false);
        --num_external_clock_events;
      }
    }
    
    // Check if there is some MIDI data to process. If so, decode the MIDI
    // bytestream.
    while (midi_in_buffer.readable()) {
      midi_parser.PushByte(midi_in_buffer.ImmediateRead());
    }
    
    // Update the voice tuner state machine.
    voice_tuner.Refresh();
    if (num_overflows > 32) {
      voice_tuner.Abort();
    }
    
    // Handle UI events
    ui.DoEvents();
  }
}
