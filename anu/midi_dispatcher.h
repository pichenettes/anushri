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
//
// -----------------------------------------------------------------------------

#ifndef ANU_MIDI_DISPATCHER_H_
#define ANU_MIDI_DISPATCHER_H_

#include "avrlib/base.h"
#include "avrlib/ring_buffer.h"

#include "anu/drum_synth.h"
#include "anu/sysex_handler.h"
#include "anu/system_settings.h"
#include "anu/voice_controller.h"

#include "midi/midi.h"

namespace anu {

struct LowPriorityBufferSpecs {
  enum {
    buffer_size = 128,
    data_size = 8,
  };
  typedef avrlib::DataTypeForSize<data_size>::Type Value;
};

struct HighPriorityBufferSpecs {
  enum {
    buffer_size = 16,
    data_size = 8,
  };
  typedef avrlib::DataTypeForSize<data_size>::Type Value;
};

class MidiDispatcher : public midi::MidiDevice {
 public:
  typedef avrlib::RingBuffer<LowPriorityBufferSpecs> OutputBufferLowPriority;
  typedef avrlib::RingBuffer<HighPriorityBufferSpecs> OutputBufferHighPriority;

  MidiDispatcher() { }

  // ------ MIDI in handling ---------------------------------------------------

  // Forwarded to the controller.
  static inline void NoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    voice_controller.NoteOn(note, velocity);
  }
  static inline void NoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    voice_controller.NoteOff(note);
  }

  // Handled.
  static inline void ControlChange(
      uint8_t channel,
      uint8_t controller,
      uint8_t value) {
    voice_controller.ControlChange(controller, value);
  }
  static inline void PitchBend(uint8_t channel, uint16_t pitch_bend) {
    voice_controller.PitchBend(pitch_bend);
  }
  static void Aftertouch(uint8_t channel, uint8_t note, uint8_t velocity) {
    voice_controller.Aftertouch(velocity);
  }
  static void Aftertouch(uint8_t channel, uint8_t velocity) {
    voice_controller.Aftertouch(velocity);
  }
  static void AllSoundOff(uint8_t channel) {
    voice_controller.AllSoundOff();
  }
  static void ResetAllControllers(uint8_t channel) {
    voice_controller.ResetAllControllers();
  }
  static void AllNotesOff(uint8_t channel) {
    voice_controller.AllNotesOff();
  }
  static void OmniModeOff(uint8_t channel) {
    system_settings.set_midi_channel(channel);
  }
  static void OmniModeOn(uint8_t channel) { }
  
  static void ProgramChange(uint8_t channel, uint8_t program) {
    // No fucking presets.
  }
  
  static void Reset() { }
  static void Clock() { 
    if (!voice_controller.internal_clock()) {
      voice_controller.Clock(true);
    }
  }
  static void Start() {
    if (!voice_controller.internal_clock()) {
      voice_controller.Start();
    }
  }
  static void Stop() { 
    if (!voice_controller.internal_clock()) {
      voice_controller.Stop();
    }
  }
  static void Continue() { Start(); }
  
  static void SysExStart() {
    ProcessSysEx(0xf0);
  }
  static void SysExByte(uint8_t sysex_byte) {
    ProcessSysEx(sysex_byte);
  }
  static void SysExEnd() {
    ProcessSysEx(0xf7);
  }
  
  static uint8_t CheckChannel(uint8_t channel) {
    return system_settings.receive_channel(channel);
  }
  
  static void RawMidiData(
      uint8_t status,
      uint8_t* data,
      uint8_t data_size,
      uint8_t accepted_channel) {
    if (mode() & MIDI_OUT_TX_INPUT_MESSAGES) {
      Send(status, data, data_size);
    }
    
    // Change RX channel when in learning mode.
    if (learning_midi_channel_ && (status & 0xf0) == 0x90) {
      uint8_t channel = status & 0xf;
      // Channel 10 is forbidden.
      if (channel != 0x9) {
        system_settings.set_midi_channel(channel, data[0]);
        learning_midi_channel_ = false;
      }
    }
    
    // Manually handles drum sound triggering from MIDI.
    if (status == 0x99) {
      uint8_t note = data[0];
      uint8_t velocity = data[1] << 1;
      if (velocity) {
        if (note == 36) {
          drum_synth.Trigger(0, velocity);
        } else if (note == 38) {
          drum_synth.Trigger(1, velocity);
        } else if (note == 42) {
          drum_synth.Trigger(2, velocity);
        }
        seen_midi_drum_events_ = true;
      }
    }
    
    // Manually handles CCs for the drum section from MIDI.
    if (status == 0xb9) {
      uint8_t cc = data[0];
      uint8_t value = data[1];
      drum_synth.SetParameterCc(cc, value);
    }
    
    // Manually handles drum programming from MIDI.
    if (status == 0x9f) {
      uint8_t note = data[0];
      uint8_t velocity = data[1] << 1;
      if (velocity) {
        voice_controller.RemoteControlDrumSequencer(note);
      }
    }
  }
  
  static uint8_t readable_high_priority() {
    return OutputBufferHighPriority::readable();
  }
  
  static uint8_t readable_low_priority() {
    return OutputBufferLowPriority::readable();
  }

  static uint8_t ImmediateReadHighPriority() {
    return OutputBufferHighPriority::ImmediateRead();
  }
  
  static uint8_t ImmediateReadLowPriority() {
    return OutputBufferLowPriority::ImmediateRead();
  }
  
  static void LearnChannel() {
    learning_midi_channel_ = true;
  }
  
  static void ResetDrumEventMonitor() {
    seen_midi_drum_events_ = false;
  }
  
  static bool learning_midi_channel() {
    return learning_midi_channel_;
  }
  
  static bool seen_midi_drum_events() {
    return seen_midi_drum_events_;
  }

  // ------ Generation of MIDI out messages ------------------------------------
  static inline void OnInternalNoteOff(uint8_t note) {
    if (note != 0xff) {
      uint8_t channel = system_settings.midi_channel();
      if (mode() & MIDI_OUT_TX_GENERATED_MESSAGES) {
        Send3(0x80 | channel, note, 0);
      }
    }
  }
  
  static inline void OnInternalNoteOn(uint8_t note, uint8_t velocity) {
    uint8_t channel = system_settings.midi_channel();
    if (mode() & MIDI_OUT_TX_GENERATED_MESSAGES) {
      Send3(0x90 | channel, note, velocity);
    }
  }

  static inline void OnDrumNote(uint8_t note, uint8_t velocity) {
    if (note != 0xff) {
      if (mode() & MIDI_OUT_TX_GENERATED_DRUM_MESSAGES) {
        Send3(0x99, note, velocity);
        Send3(0x89, note, 0);
      }
    }
  }
  
  static inline void OnStart() {
    if (mode() & MIDI_OUT_TX_TRANSPORT) {
      SendNow(0xfa);
    }
  }

  static inline void OnStop() {
    if (mode() & MIDI_OUT_TX_TRANSPORT) {
      SendNow(0xfc);
    }
  }
  
  static inline void OnClock(bool midi_generated) {
    // No need to duplicate a MIDI clock message.
    if (mode() & MIDI_OUT_TX_INPUT_MESSAGES && midi_generated) {
      return;
    }
    if (mode() & MIDI_OUT_TX_TRANSPORT) {
      SendNow(0xf8);
    }
  }
  
  static void Send3(uint8_t status, uint8_t a, uint8_t b);
  static void SendBlocking(uint8_t byte);

 private:
  static bool learning_midi_channel_;
  static bool seen_midi_drum_events_;
   
  static void Send(uint8_t status, uint8_t* data, uint8_t size);
  static void SendNow(uint8_t byte);
  static inline uint8_t mode() { return system_settings.midi_out_mode(); }
  
  static void ProcessSysEx(uint8_t byte) {
    if (mode() & MIDI_OUT_TX_INPUT_MESSAGES) {
      Send(byte, NULL, 0);
    }
    sysex_handler.Receive(byte);
  }
  
  DISALLOW_COPY_AND_ASSIGN(MidiDispatcher);
};

extern MidiDispatcher midi_dispatcher;

}  // namespace anu

#endif  // ANU_MIDI_DISPATCHER_H_
