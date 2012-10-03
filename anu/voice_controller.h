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

#ifndef ANU_VOICE_CONTROLLER_H_
#define ANU_VOICE_CONTROLLER_H_

#include "avrlib/base.h"
#include "avrlib/random.h"

#include "anu/envelope.h"
#include "anu/lfo.h"
#include "anu/note_stack.h"
#include "anu/voice.h"

namespace anu {

static const uint8_t kNumDrumParts = 3;

enum ArpeggiatorDirection {
  ARPEGGIO_DIRECTION_UP = 0,
  ARPEGGIO_DIRECTION_DOWN,
  ARPEGGIO_DIRECTION_UP_DOWN,
  ARPEGGIO_DIRECTION_RANDOM,
  ARPEGGIO_DIRECTION_LAST
};

enum SequencerSettingsParameter {
  PRM_SEQ_ARP_MODE,
  PRM_SEQ_ARP_PATTERN,
  PRM_SEQ_TEMPO,
  PRM_SEQ_SWING,
  PRM_SEQ_DRUMS_X,
  PRM_SEQ_DRUMS_Y,
  PRM_SEQ_DRUMS_BD_DENSITY,
  PRM_SEQ_DRUMS_SD_DENSITY,
  PRM_SEQ_DRUMS_HH_DENSITY,
  PRM_SEQ_DRUMS_BD_TONE,
  PRM_SEQ_DRUMS_SD_TONE,
  PRM_SEQ_DRUMS_HH_TONE,
  PRM_SEQ_DRUMS_BALANCE,
  PRM_SEQ_DRUMS_BANDWIDTH,
  PRM_SEQ_DRUMS_OVERRIDE,
  PRM_SEQ_DRUMS_BD_PATTERN_L,
  PRM_SEQ_DRUMS_BD_PATTERN_H,
  PRM_SEQ_DRUMS_SD_PATTERN_L,
  PRM_SEQ_DRUMS_SD_PATTERN_H,
  PRM_SEQ_DRUMS_HH_PATTERN_L,
  PRM_SEQ_DRUMS_HH_PATTERN_H,
  PRM_SEQ_ARP_ACIDITY,
  PRM_SEQ_PADDING_1,
  PRM_SEQ_PADDING_2,
  PRM_SEQ_LAST
};

struct SequencerSettings {
  uint8_t arp_mode;
  uint8_t arp_pattern;
  uint8_t tempo;
  uint8_t swing;
  
  uint8_t drums_x;
  uint8_t drums_y;
  uint8_t drums_density[kNumDrumParts];
  uint8_t drums_tone[kNumDrumParts];
  uint8_t drums_balance;
  uint8_t drums_bandwidth;
  
  uint8_t drums_override;
  uint16_t drums_pattern[kNumDrumParts];
  
  uint8_t acidity;
  uint8_t padding[2];
  
  inline uint8_t arp_range() const { return ((arp_mode - 1) & 0x01) + 1; }
  inline uint8_t arp_direction() const { return (arp_mode - 1) >> 1; }
  inline bool has_drums() const {
    return (drums_density[0] > 1) || \
        (drums_density[1] > 1) || \
        (drums_density[2] > 1) || drums_override;
  }
};

struct Sequence {
  uint8_t num_notes;
  uint8_t notes[128];
  uint8_t accents[16];
  uint8_t slides[16];
};

class VoiceController {
 public:
  VoiceController() { }
  ~VoiceController() { }
  static void Init();
  static void NoteOn(uint8_t note, uint8_t velocity);
  static void NoteOff(uint8_t note);
  static void ControlChange(uint8_t controller, uint8_t value);
  static void PitchBend(uint16_t pitch_bend);
  static void Aftertouch(uint8_t velocity);
  static void AllSoundOff();
  static void ResetAllControllers();
  static void AllNotesOff();
  static void Clock(bool midi_generated);
  
  static inline void Start() {
    StartClock();
    StartArpeggiator();
    StartSequencer();
    StartDrumMachine();
  }
  static inline void Stop() {
    StopSequencer();
    StopClock();
  }
  
  static inline void InsertRest() {
    sequence_.notes[sequence_.num_notes++] = 0xff;
    if (sequence_.num_notes == 128) {
      StopRecording();
    }
  };
  
  static inline void InsertTie() {
    sequence_.notes[sequence_.num_notes++] = 0xfe;
    if (sequence_.num_notes == 128) {
      StopRecording();
    }
  };
  
  static inline void HoldNotes() {
    if (ignore_note_off_messages_) {
      ReleaseAllHeldNotes();
    } else {
      if (pressed_keys_.size()) {
        ignore_note_off_messages_ = true;
      }
    }
  }
  
  static void StopRecording();
  static void SaveSequence();
  static void RemoteControlDrumSequencer(uint8_t note);
  static void StartRecording();
  
  static void GateOn() { voice_.GateOn(); }
  static void GateOff() { voice_.GateOff(); }
  
  static inline const Voice& voice() { return voice_; }
  static inline Voice* mutable_voice() { return &voice_; }
  static inline Sequence* mutable_sequence() { return &sequence_; }
  static inline SequencerSettings* mutable_sequencer_settings() {
    return &seq_settings_;
  }

  static inline bool sequencer_running() { return sequencer_running_; }
  static inline bool sequencer_recording() { return sequencer_recording_; }
  static inline bool hold_state() { return ignore_note_off_messages_; }
  static inline uint8_t clock_pulse() { return clock_pulse_; }
  static inline uint8_t sequencer_step() { return sequencer_note_; }
  static inline uint8_t sequence_length() { return sequence_.num_notes; }
  static inline uint8_t internal_clock() { return seq_settings_.tempo >= 40; }
  static inline bool at_rest() {
    return pressed_keys_.size() == 0 && !sequencer_running_ && voice_.at_rest();
  }
  static inline bool has_drums() {
    return (sequencer_running_ && seq_settings_.has_drums());
  }
  static inline bool has_arpeggiator() {
    return (seq_settings_.arp_mode && pressed_keys_.size());
  }
  
  static inline void ClearClockPulse() {
    --clock_pulse_;
  }
  
  static void SetValue(uint8_t address, uint8_t value);
  
  static uint8_t GetValue(uint8_t address) {
    const uint8_t* bytes;
    bytes = static_cast<const uint8_t*>(static_cast<const void*>(&seq_settings_));
    return bytes[address];
  }
  static void SavePatch();
  static void ResetToFactoryDefaults();
  static void ReleaseAllHeldNotes();
  
  static void Touch() {
    RefreshDrumSynthSettings();
    RefreshDrumSynthMixing();
    voice_.Touch();
  }
  static void TouchClock();
  
 private:
  static void ResetArpeggiatorPattern();

  static void StartClock();
  static void StopClock();
  static void StartArpeggiator();
  static void ClockArpeggiator();
  static void StopArpeggiator();
  static void StepArpeggiator();

  static void StartSequencer();
  static void ClockSequencer();
  static void StopSequencer();
  
  static void StartDrumMachine();
  static void ClockDrumMachine();
  
  static void RefreshDrumSynthSettings();
  static void RefreshDrumSynthMixing();
  
  static uint8_t ReadDrumMap(
      uint8_t step,
      uint8_t instrument,
      uint8_t x,
      uint8_t y);
  
  static SequencerSettings seq_settings_;
  static Sequence sequence_;
  static Voice voice_;
  
  static bool ignore_note_off_messages_;
  static uint8_t clock_pulse_;
  static uint8_t clock_counter_;
  static uint8_t lfo_sync_counter_;
  
  static NoteStack<16> pressed_keys_;

  static bool sequencer_running_;
  static bool sequencer_recording_;
  static bool clock_running_;
  static uint8_t sequencer_note_;
  static int8_t sequencer_transposition_;
  static uint8_t sequencer_velocity_;
  
  static uint8_t previous_generated_note_;
  static uint16_t arp_pattern_mask_;
  static uint8_t arp_pattern_step_;
  static int8_t arp_direction_;
  static int8_t arp_step_;
  static int8_t arp_octave_;
  
  static uint8_t drum_sequencer_step_;
  static uint8_t drum_sequencer_perturbation_[kNumDrumParts];
  
  static uint8_t drum_remote_control_current_instrument_;
  
  static bool dirty_;
  
  DISALLOW_COPY_AND_ASSIGN(VoiceController);
};

extern VoiceController voice_controller;

};  // namespace anu

#endif  // ANU_VOICE_CONTROLLER_H_
