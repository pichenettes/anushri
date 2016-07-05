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

#include "anu/ui.h"

#include "avrlib/adc.h"

#include "anu/hardware_config.h"
#include "anu/midi_dispatcher.h"
#include "anu/parameter.h"
#include "anu/strummer.h"
#include "anu/sysex_handler.h"
#include "anu/system_settings.h"
#include "anu/voice_controller.h"
#include "anu/voice_tuner.h"

namespace anu {

static const uint8_t kNumSoftPots = 10;
static const uint8_t kNumRows = 3;

using namespace avrlib;

/* <static> */
uint8_t Ui::led_pattern_;
avrlib::EventQueue<16> Ui::queue_;
uint8_t Ui::active_row_ = 0;
uint8_t Ui::inhibit_switch_ = 0;
uint8_t Ui::pwm_cycle_;
uint8_t Ui::display_mode_;
uint16_t Ui::adc_values_[kNumPots];
uint8_t Ui::adc_thresholds_[kNumPots];
uint8_t Ui::scanned_pot_;
uint8_t Ui::pot_scanning_warm_up_ = 16;
bool Ui::busy_ = false;
bool Ui::snapped_[kNumPots];
int16_t Ui::snap_position_cache_[kNumPots];
int8_t Ui::display_snap_delta_ = 0;
uint8_t Ui::disable_switch_sensing_ = 0;
uint16_t Ui::long_press_counter_ = 0;
bool Ui::strummer_enabled_ = false;
/* </static> */

Adc adc;

AdcMuxBank1SS mux_bank1_ss;
AdcMuxBank2SS mux_bank2_ss;
AdcMuxAddress mux_address;

/* static */
void Ui::Init() {
  inputs.Init();
  outputs.Init();
  display_mode_ = DISPLAY_MODE_LFO_GATE_CLOCK;

  mux_bank1_ss.set_mode(DIGITAL_OUTPUT);
  mux_bank1_ss.High();
  mux_bank2_ss.set_mode(DIGITAL_OUTPUT);
  mux_bank2_ss.High();
  mux_address.set_mode(DIGITAL_OUTPUT);
  
  adc.set_alignment(ADC_RIGHT_ALIGNED);
  adc.set_reference(ADC_DEFAULT);
  adc.Init();
  adc.StartConversion(kAdcInputMux);
  memset(adc_thresholds_, 4, kNumPots);
  memset(snapped_, true, kNumPots);
  parameter_manager.Init();
}

// This is used to convert a pot index into addresses for the ADC muxes.
const uint8_t pots_layout[] = {
  // Bottom row
   6,  0,  4,  2,  1,
   3,  7,  5,  9, 11, 
  // Upper row
  14, 10, 12,  8,
};

/* static */
void Ui::Poll() {
  UpdateLeds();
  if (disable_switch_sensing_) {
    --disable_switch_sensing_;
  } else {
    // Scan switches
    uint8_t mask = 1;
    for (uint8_t i = 0; i < kNumSwitches; ++i) {
      if (inputs.raised(i + 2)) {
        if ((inhibit_switch_ & mask)) {
          inhibit_switch_ ^= mask;
        } else {
          uint8_t control_index = CONTROL_SEQ_BUTTON;
          if (inputs.low(3)) {
            inhibit_switch_ = _BV(1);
            control_index = CONTROL_SHIFT_SEQ_BUTTON;
          }
          control_index += i;
          queue_.AddEvent(CONTROL_SWITCH, control_index, 1);
          // In the next 100ms, no switch event may be generated.
          disable_switch_sensing_ = 61;
          long_press_counter_ = 0;
        }
      }
      mask <<= 1;
    }
    // Detect the long press on the run/stop and the hold buttons.
    if (inputs.lowered(3) || inputs.lowered(5)) {
      long_press_counter_ = 0;
    }
    if (inputs.low(3) || inputs.low(5)) {
      ++long_press_counter_;
    }
    if (long_press_counter_ == 1224) {
      if (inputs.low(3)) {
        queue_.AddEvent(CONTROL_SWITCH, CONTROL_SHIFT_LONG_PRESS, 1);
        inhibit_switch_ = _BV(1);
      } else if (inputs.low(5)) {
        queue_.AddEvent(CONTROL_SWITCH, CONTROL_RUN_STOP_LONG_PRESS, 1);
        inhibit_switch_ = _BV(3);
      }
    }
  }
  
  // Read pot value and launch ADC scan.
  uint16_t adc_value = adc.ReadOut();
  int16_t delta = adc_values_[scanned_pot_] - adc_value;
  if (delta < 0) {
    delta = -delta;
  }
  if (delta >= adc_thresholds_[scanned_pot_]) {
    adc_values_[scanned_pot_] = adc_value;
    // Do not post events in the queue until 16 scanning cycles have been
    // performed.
    if (!pot_scanning_warm_up_) {
      queue_.AddEvent(CONTROL_POT, scanned_pot_, adc_value >> 2);
    }
  }
  mux_bank1_ss.High();
  mux_bank2_ss.High();
  ++scanned_pot_;
  if (scanned_pot_ == kNumPots) {
    scanned_pot_ = 0;
    if (pot_scanning_warm_up_) {
      --pot_scanning_warm_up_;
    }
  }
  uint8_t address = pots_layout[scanned_pot_];
  if (address & 0x08) {
    mux_bank2_ss.Low();
  } else {
    mux_bank1_ss.Low();
  }
  mux_address.Write(address & 0x07);
  
  adc.StartConversion(kAdcInputMux);
}

/* static */
void Ui::UnlockPot(uint8_t index) {
  if (index < kNumSoftPots) {
    // Soft pots have a higher threshold than hardwired ones.
    adc_thresholds_[index] = 8;
  } else {
    adc_thresholds_[index] = 4;
  }
}

/* static */
void Ui::LockPots(bool snap) {
  queue_.Flush();
  memset(adc_thresholds_, 16, kNumSoftPots);
  memset(snapped_, !snap, kNumSoftPots);
  memset(snap_position_cache_, 0xff, sizeof(snap_position_cache_));
  display_snap_delta_ = 0;
}

/* static */
void Ui::HandleSwitchEvent(uint8_t index) {
  if (display_mode_ == DISPLAY_MODE_MIDI_FILTER) {
    uint8_t mask = 1 << index;
    system_settings.set_midi_out_mode(system_settings.midi_out_mode() ^ mask);
    return;
  }
  switch (index) {
    case CONTROL_ADSR_BUTTON:
      active_row_ = 0;
      LockPots(false);
      break;
    
    case CONTROL_KBD_BUTTON:
      active_row_ = 1;
      LockPots(true);
      break;
      
    case CONTROL_SEQ_BUTTON:
      if (voice_controller.sequencer_recording()) {
        voice_controller.InsertRest();
      } else {
        active_row_ = 2;
        LockPots(false);
      }
      break;
      
    case CONTROL_RUN_STOP_BUTTON:
      if (voice_controller.sequencer_running()) {
        voice_controller.Stop();
      } else {
        voice_controller.Start();
      }
      break;
      
    case CONTROL_REC_BUTTON:
      if (voice_controller.sequencer_recording()) {
        busy_ = true;
        voice_controller.StopRecording();
        busy_ = false;
        strummer_enabled_ = false;
      } else {
        voice_controller.StartRecording();
      }
      break;
      
    case CONTROL_HOLD_BUTTON:
      if (voice_controller.sequencer_recording()) {
        voice_controller.InsertTie();
      } else if (strummer_enabled_) {
        strummer_enabled_ = false;
        voice_controller.ReleaseAllHeldNotes();
      } else {
        voice_controller.HoldNotes();
      }
      break;
      
    case CONTROL_SHIFT_ADSR_BUTTON:
      midi_dispatcher.LearnChannel();
      break;

    case CONTROL_SHIFT_KBD_BUTTON:
      display_mode_ = DISPLAY_MODE_MIDI_FILTER;
      break;
      
    case CONTROL_SHIFT_SEQ_BUTTON:
      system_settings.ChangePpqn();
      voice_controller.TouchClock();
      display_mode_ = DISPLAY_MODE_PPQN_SETTING;
      break;
    
    case CONTROL_SHIFT_RUN_STOP_BUTTON:
      voice_tuner.StartTuning();
      break;
      
    case CONTROL_SHIFT_REC_BUTTON:
      system_settings.set_calibration_data(60 * 128, 64000 / 3, 64000 / 3);
      break;
      
    case CONTROL_SHIFT_LONG_PRESS:
      strummer.Start();
      strummer_enabled_ = true;
      break;
      
    case CONTROL_RUN_STOP_LONG_PRESS:
      sysex_handler.BulkDump();
      break;
  }
}

const prog_uint8_t pot_to_parameter_map[16 * kNumRows] PROGMEM = {
  // active_row_ == 0
  PARAMETER_GLIDE,
  PARAMETER_VCO_DETUNE,
  PARAMETER_VCO_FINE,
  PARAMETER_LFO_SHAPE,
  PARAMETER_LFO_RATE,
  PARAMETER_ENV_ATTACK,
  PARAMETER_ENV_DECAY,
  PARAMETER_ENV_SUSTAIN,
  PARAMETER_ENV_RELEASE,
  PARAMETER_ENV_VCA_MORPH,
  
  PARAMETER_VCO_MOD_BALANCE,
  PARAMETER_PW_MOD_BALANCE,
  PARAMETER_CUTOFF_ENV_AMOUNT,
  PARAMETER_CUTOFF_LFO_AMOUNT,
  PARAMETER_UNASSIGNED,
  PARAMETER_UNASSIGNED,

  // active_row_ == 1
  PARAMETER_TEMPO,
  PARAMETER_SWING,
  PARAMETER_ARP_MODE,
  PARAMETER_ARP_PATTERN,
  PARAMETER_ARP_ACIDITY,
  PARAMETER_VCO_DCO_RANGE,
  PARAMETER_VCO_DCO_FINE,
  PARAMETER_VIBRATO_RATE,
  PARAMETER_VIBRATO_DESTINATION,
  PARAMETER_VELOCITY_MOD,

  PARAMETER_VCO_MOD_BALANCE,
  PARAMETER_PW_MOD_BALANCE,
  PARAMETER_CUTOFF_ENV_AMOUNT,
  PARAMETER_CUTOFF_LFO_AMOUNT,
  PARAMETER_UNASSIGNED,
  PARAMETER_UNASSIGNED,
  
  // active_row_ == 2
  PARAMETER_DRUMS_X,
  PARAMETER_DRUMS_BD_DENSITY,
  PARAMETER_DRUMS_SD_DENSITY,
  PARAMETER_DRUMS_HH_DENSITY,
  PARAMETER_DRUMS_BALANCE,
  PARAMETER_DRUMS_Y,
  PARAMETER_DRUMS_BD_TONE,
  PARAMETER_DRUMS_SD_TONE,
  PARAMETER_DRUMS_HH_TONE,
  PARAMETER_DRUMS_BANDWIDTH,

  PARAMETER_VCO_MOD_BALANCE,
  PARAMETER_PW_MOD_BALANCE,
  PARAMETER_CUTOFF_ENV_AMOUNT,
  PARAMETER_CUTOFF_LFO_AMOUNT,
  PARAMETER_UNASSIGNED,
  PARAMETER_UNASSIGNED,
};

/* static */
void Ui::HandlePotEvent(uint8_t index, uint8_t value) {
  if (strummer_enabled_ && index >= 5 && index < 10) {
    strummer.Update(index - 5, value);
    return;
  }

  UnlockPot(index);
  uint8_t parameter_index = pgm_read_byte(
      pot_to_parameter_map + index + active_row_ * 16);

  if (!snapped_[index]) {
    // Unlock pot until target value is reached.
    if (snap_position_cache_[index] == -1) {
      snap_position_cache_[index] = \
          parameter_manager.GetScaled(parameter_index);
    }
    int16_t distance = snap_position_cache_[index] - value;
    if (distance < 8 && distance > -8) {
      snapped_[index] = true;
      display_snap_delta_ = 0;
    } else {
      display_snap_delta_ = distance > 0 ? 1 : -1;
    }
  }
  if (snapped_[index]) {
    parameter_manager.SetScaled(parameter_index, value);
  }
}

/* static */
void Ui::DoEvents() {
  while (queue_.available()) {
    Event e = queue_.PullEvent();
    if (e.control_type == CONTROL_SWITCH) {
      HandleSwitchEvent(e.control_id);
    } else if (e.control_type == CONTROL_POT) {
      HandlePotEvent(e.control_id, e.value);
    }
    queue_.Touch();
  }
  if (midi_dispatcher.learning_midi_channel()) {
    display_mode_ = DISPLAY_MODE_MIDI_CHANNEL;
  }
  if (queue_.idle_time_ms() > 2500) {
    display_mode_ = DISPLAY_MODE_LFO_GATE_CLOCK;
    display_snap_delta_ = 0;
  }
  if (queue_.idle_time_ms() > 10000) {
    TrySavingSettings();
    queue_.Touch();
  }
}

/* static */
void Ui::TrySavingSettings() {
  if (voice_controller.at_rest() && drum_synth.idle_time_ms() > 10000) {
    busy_ = true;
    voice_controller.SavePatch();
    busy_ = false;
  }
}

/* static */
void Ui::UpdateLeds() {
  uint8_t led_pattern = 0;
  pwm_cycle_ += 32;
  
  // 3 horizontal LEDs.
  
  // Default status: Lfo / Gate / Clock blinkers.
  if (display_mode_ == DISPLAY_MODE_LFO_GATE_CLOCK) {
    if (voice_controller.voice().lfo() > pwm_cycle_) {
      led_pattern |= _BV(OUTPUT_LFO_LED);
    }
    if (voice_controller.voice().gate()) {
      led_pattern |= _BV(OUTPUT_GATE_LED);
    }
    if (voice_controller.sequencer_running() &&
        !((voice_controller.sequencer_step() - 1) & 0x03)) {
      led_pattern |= _BV(OUTPUT_CLOCK_LED);
    }
  // A bunch of special cases in MIDI learn mode.
  } else if (display_mode_ == DISPLAY_MODE_PPQN_SETTING) {
    led_pattern |= _BV(OUTPUT_LFO_LED) << system_settings.clock_ppqn();
  } else if (display_mode_ == DISPLAY_MODE_MIDI_CHANNEL) {
    led_pattern |= (system_settings.midi_channel() & 0x07) << OUTPUT_LFO_LED;
  }
  if (voice_controller.sequencer_recording()) {
    uint8_t bar = voice_controller.sequence_length() & 0x03;
    if (bar) {
      led_pattern = (1 << (bar - 1)) << OUTPUT_LFO_LED;
    } else {
      led_pattern = 0;
    }
  }
  if (display_snap_delta_) {
    led_pattern = display_snap_delta_ > 0
        ? _BV(OUTPUT_CLOCK_LED)
        : _BV(OUTPUT_LFO_LED);
  }
  
  // 3 vertical LEDs.
  
  // Default status: current page.
  if (voice_controller.sequencer_recording() || busy_) {
    led_pattern |= \
        _BV(OUTPUT_ROW_1_LED) | _BV(OUTPUT_ROW_2_LED) | _BV(OUTPUT_ROW_3_LED);
  } else {
    led_pattern |= _BV(OUTPUT_ROW_1_LED) >> active_row_;
  }
  if (display_mode_ == DISPLAY_MODE_MIDI_FILTER) {
    led_pattern = (system_settings.midi_out_mode()) & 0x3f;
  }
  
  // Dim the LEDs 50% when hold mode is enabled.
  if ((voice_controller.hold_state() || strummer_enabled_) && pwm_cycle_) {
    led_pattern = 0;
  }
  
  led_pattern_ = led_pattern;
}

/* extern */
Ui ui;

/* extern */
Inputs inputs;

/* extern */
Outputs outputs;

}  // namespace anu
