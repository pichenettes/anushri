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

#include "anu/voice_controller.h"

#include <avr/pgmspace.h>

#include "avrlib/op.h"
#include "midi/midi.h"

#include "anu/clock.h"
#include "anu/midi_dispatcher.h"
#include "anu/parameter.h"
#include "anu/storage.h"
#include "anu/system_settings.h"

namespace anu {
  
using namespace avrlib;

/* <static> */
SequencerSettings VoiceController::seq_settings_;
Sequence VoiceController::sequence_;
Voice VoiceController::voice_;

bool VoiceController::ignore_note_off_messages_;
uint8_t VoiceController::clock_pulse_;
uint8_t VoiceController::clock_counter_;

NoteStack<16> VoiceController::pressed_keys_;

bool VoiceController::clock_running_;
bool VoiceController::sequencer_running_;
bool VoiceController::sequencer_recording_;
uint8_t VoiceController::sequencer_note_;
int8_t VoiceController::sequencer_transposition_;
uint8_t VoiceController::sequencer_velocity_;

uint8_t VoiceController::previous_generated_note_;
uint16_t VoiceController::arp_pattern_mask_;
uint8_t VoiceController::arp_pattern_step_;
int8_t VoiceController::arp_direction_;
int8_t VoiceController::arp_step_;
int8_t VoiceController::arp_octave_;
uint8_t VoiceController::lfo_sync_counter_;

uint8_t VoiceController::drum_sequencer_step_;
uint8_t VoiceController::drum_sequencer_perturbation_[3];
uint8_t VoiceController::drum_remote_control_current_instrument_;

bool VoiceController::dirty_;
/* </static> */

typedef SequencerSettings PROGMEM prog_SequencerSettings;
typedef Sequence PROGMEM prog_Sequence;

static const prog_SequencerSettings init_seq_settings PROGMEM = {
  // Arp range and pattern
  0, 0,
  
  // Tempo and swing
  120, 0, 
  
  // Drum parameters
  0, 0,
  0, 0,
  0, 0,
  0, 0,
  128, 255,
  
  0, 0, 0, 0,
  
  // Acidity
  0,
  
  // Padding
  0, 0,
};

static const prog_Sequence init_sequence PROGMEM = {
  4,
  { 60, 60, 48, 48 }
};

static const prog_uint8_t* drum_map[3][3] = {
  { wav_res_drum_map_node_8, wav_res_drum_map_node_3, wav_res_drum_map_node_6 },
  { wav_res_drum_map_node_2, wav_res_drum_map_node_4, wav_res_drum_map_node_0 },
  { wav_res_drum_map_node_7, wav_res_drum_map_node_1, wav_res_drum_map_node_5 },
};

template<>
struct StorageLayout<SequencerSettings> {
  static uint8_t* eeprom_address() { 
    return (uint8_t*)(192);
  }
  static const prog_char* init_data() {
    return (prog_char*)&init_seq_settings;
  }
};

template<>
struct StorageLayout<Sequence> {
  static uint8_t* eeprom_address() { 
    return (uint8_t*)(256);
  }
  static const prog_char* init_data() {
    return (prog_char*)&init_sequence;
  }
};

uint8_t clock_divisions[] = { 1, 2, 6 };
uint8_t clock_internal_rate_compensation[] = { 6, 3, 1 };

/* static */
void VoiceController::Init() {
  STATIC_ASSERT(sizeof(SequencerSettings) == PRM_SEQ_LAST);
  
  storage.Load(&seq_settings_);
  storage.Load(&sequence_);
  pressed_keys_.Init();
  voice_.Init();

  ignore_note_off_messages_ = false;
  sequencer_running_ = false;
  sequencer_transposition_ = 0;
  sequencer_velocity_ = 64;

  // At boot, set the VCO to the reference note (C3).
  voice_.set_note(system_settings.reference_note());
  TouchClock();
  dirty_ = false;

  RefreshDrumSynthSettings();
  RefreshDrumSynthMixing();

  drum_remote_control_current_instrument_ = 0;
}

/* static */
void VoiceController::RefreshDrumSynthSettings() {
  for (uint8_t i = 0; i < kNumDrumParts; ++i) {
    drum_synth.MorphPatch(i, seq_settings_.drums_tone[i]);
  }
}

/* static */
void VoiceController::RefreshDrumSynthMixing() {
  drum_synth.SetBalance(seq_settings_.drums_balance);
  drum_synth.SetBandwidth(seq_settings_.drums_bandwidth);
}

/* static */
void VoiceController::TouchClock() {
  clock.Update(
      seq_settings_.tempo,
      1,
      seq_settings_.swing >> 1,
      clock_internal_rate_compensation[system_settings.clock_ppqn()]);
}

/* static */
void VoiceController::SetValue(uint8_t offset, uint8_t value) {
  uint8_t* bytes;
  bytes = static_cast<uint8_t*>(static_cast<void*>(&seq_settings_));
  uint8_t previous_value = bytes[offset];
  bytes[offset] = value;
  if (value != previous_value) {
    dirty_ = true;
    if (offset == PRM_SEQ_ARP_MODE) {
      arp_direction_ = \
          seq_settings_.arp_direction() == ARPEGGIO_DIRECTION_DOWN ? -1 : 1;
    } else if (offset >= PRM_SEQ_TEMPO && offset <= PRM_SEQ_SWING) {
      TouchClock();
    } else if (offset >= PRM_SEQ_DRUMS_BD_TONE && \
               offset <= PRM_SEQ_DRUMS_HH_TONE) {
      RefreshDrumSynthSettings();
    }
    else if (offset >= PRM_SEQ_DRUMS_BALANCE && \
             offset <= PRM_SEQ_DRUMS_BANDWIDTH) {
      RefreshDrumSynthMixing();
    }
  }
}

/* static */
void VoiceController::NoteOn(uint8_t note, uint8_t velocity) {
  if (velocity == 0) {
    NoteOff(note);
  } else {
    if (sequencer_running_ && sequence_.num_notes) {
      // Note is interpreted as transposition/velocity for sequencer.
      sequencer_transposition_ = note - 60;
      sequencer_velocity_ = velocity;
    } else {
      pressed_keys_.NoteOn(note, velocity);
      if (seq_settings_.arp_mode == 0) {
        // If the arpeggiator is off, actually trigger the note!
        voice_.NoteOn(note, velocity, 0, 0, pressed_keys_.size() != 1);
      } else {
        if (pressed_keys_.size() == 1 && !clock_running_) {
          StartClock();
          StartArpeggiator();
        }
      }
    }
    if (sequencer_recording_) {
      sequence_.notes[sequence_.num_notes++] = note;
      if (sequence_.num_notes == 128) {
        sequencer_recording_ = false;
      }
    }
  }
}

/* static */
void VoiceController::NoteOff(uint8_t note) {
  // Note off messages are ignored when the sequencer is running.
  if (!(sequencer_running_ && sequence_.num_notes)
      && !ignore_note_off_messages_) {
    if (seq_settings_.arp_mode == 0) {
      // Normal play mode.
      uint8_t top_note = pressed_keys_.most_recent_note().note;
      pressed_keys_.NoteOff(note);
      if (pressed_keys_.size() == 0) {
        // No key is pressed, we trigger the release segment.
        voice_.NoteOff(note);
      } else {
        // We have released the most recent pressed key, but some other keys
        // are still pressed. Retrigger the most recent of them.
        if (top_note == note) {
          voice_.NoteOn(
              pressed_keys_.most_recent_note().note,
              pressed_keys_.most_recent_note().velocity,
              0,
              0,
              true);
        }
      }
    } else {
      pressed_keys_.NoteOff(note);
      if (pressed_keys_.size() == 0) {
        StopArpeggiator();
        if (!sequencer_running_) {
          StopClock();
        }
      }
    }
  }
}

/* static */
void VoiceController::ReleaseAllHeldNotes() {
  ignore_note_off_messages_ = false;
  while (pressed_keys_.size()) {
    NoteOff(pressed_keys_.most_recent_note().note);
  }
}

/* static */
void VoiceController::ControlChange(uint8_t controller, uint8_t value) {
  if (controller == midi::kModulationWheelMsb &&
      sequencer_recording_ &&
      value > 0x40)  {
    uint8_t accent_slide_index = sequence_.num_notes >> 3;
    uint8_t accent_slide_mask = 1 << (sequence_.num_notes & 0x7);
    sequence_.accents[accent_slide_index] |= accent_slide_mask;
  }
  voice_.ControlChange(controller, value);
  switch (controller) {
    case midi::kHoldPedal:
      if (value >= 64) {
        ignore_note_off_messages_ = true;
      } else {
        ReleaseAllHeldNotes();
      }
      break;
  }
  uint8_t parameter_number = parameter_manager.LookupCCMap(controller);
  if (parameter_number != 0xff) {
    parameter_manager.SetScaled(parameter_number, value << 1);
  }
}

/* static */
void VoiceController::PitchBend(uint16_t pitch_bend) {
  voice_.PitchBend(pitch_bend);
  if (sequencer_recording_ &&
      (pitch_bend > 8192 + 2048 || pitch_bend < 8192 - 2048)) {
    uint8_t accent_slide_index = sequence_.num_notes >> 3;
    uint8_t accent_slide_mask = 1 << (sequence_.num_notes & 0x7);
    sequence_.slides[accent_slide_index] |= accent_slide_mask;
  }
}

/* static */
void VoiceController::Aftertouch(uint8_t velocity) {
  voice_.Aftertouch(velocity);
}

/* static */
void VoiceController::AllSoundOff() {
  pressed_keys_.Clear();
  voice_.AllSoundOff();
}

/* static */
void VoiceController::ResetAllControllers() {
  voice_.ResetAllControllers();
}

/* static */
void VoiceController::AllNotesOff() {
  pressed_keys_.Clear();
  voice_.GateOff();
}

/* static */
void VoiceController::Clock(bool midi_generated) {
  clock_pulse_ = 8;
  voice_.set_lfo_pll_target_phase(lfo_sync_counter_);
  if (!clock_counter_) {
    ClockArpeggiator();
    ClockSequencer();
    ClockDrumMachine();
  }
  midi_dispatcher.OnClock(midi_generated);
  ++clock_counter_;
  if (clock_counter_ >= clock_divisions[system_settings.clock_ppqn()]) {
    clock_counter_ = 0;
  }
  ++lfo_sync_counter_;
  if (lfo_sync_counter_ == 24) {
    lfo_sync_counter_ = 0;
  }
}

/* static */
void VoiceController::StartClock() {
  clock.Reset();
  clock_counter_ = 0;
  lfo_sync_counter_ = 0;
  clock_running_ = true;
  midi_dispatcher.OnStart();
}


/* static */
void VoiceController::StartSequencer() {
  if (!(pressed_keys_.size() && sequence_.num_notes == 0)) {
    AllSoundOff();
  }
  if (sequencer_recording_) {
    StopRecording();
  }
  sequencer_note_ = 0;
  sequencer_running_ = true;
}

/* static */
void VoiceController::StartArpeggiator() {
  voice_.AllSoundOff();
  previous_generated_note_ = 0xff;
  arp_pattern_mask_ = 0x1;
  arp_pattern_step_ = 0;
  arp_direction_ = \
      seq_settings_.arp_direction() == ARPEGGIO_DIRECTION_DOWN ? -1 : 1;
  ResetArpeggiatorPattern();
}

/* static */
void VoiceController::StartDrumMachine() {
  drum_sequencer_step_ = 0;
  // No need to send a MIDI start event since one of the arpeggiator or
  // sequencer has been started.
}

/* static */
void VoiceController::StopClock() {
  clock_running_ = false;
  midi_dispatcher.OnStop();
}

/* static */
void VoiceController::StopSequencer() {
  AllSoundOff();
  midi_dispatcher.OnInternalNoteOff(previous_generated_note_);
  previous_generated_note_ = 0xff;
  sequencer_running_ = false;
}

/* static */
void VoiceController::StopArpeggiator() {
  AllNotesOff();
  midi_dispatcher.OnInternalNoteOff(previous_generated_note_);
  previous_generated_note_ = 0xff;
}

/* static */
void VoiceController::ClockSequencer() {
  if (!sequence_.num_notes || !sequencer_running_) {
    return;
  }
  
  int16_t note = sequence_.notes[sequencer_note_];
  if (note == 0xff) {
    // Rest.
    voice_.NoteOff(previous_generated_note_);
    midi_dispatcher.OnInternalNoteOff(previous_generated_note_);
    previous_generated_note_ = 0xff;
  } else if (note == 0xfe) {
    // Tie: do nothing!
  } else {
    uint8_t accent_slide_index = sequencer_note_ >> 3;
    uint8_t accent_slide_mask = 1 << (sequencer_note_ & 0x7);
    note += sequencer_transposition_;
    bool slid = sequence_.slides[accent_slide_index] & accent_slide_mask;
    bool accented = sequence_.accents[accent_slide_index] & accent_slide_mask;
    
    // When the sequencer note is slid, the note emitted on the MIDI out will
    // overlap with the previous note, to allow an external sound module to
    // reproduce a slide too.
    if (!slid) {
      midi_dispatcher.OnInternalNoteOff(previous_generated_note_);
    }
    voice_.NoteOn(
        note,
        sequencer_velocity_,
        slid ? 0x60 : 0,
        accented ? 0x70 : 0,
        slid);
    midi_dispatcher.OnInternalNoteOn(
        note,
        accented ? 0x7f : sequencer_velocity_);
    if (previous_generated_note_ != 0xff && slid) {
      midi_dispatcher.OnInternalNoteOff(previous_generated_note_);
    }
    previous_generated_note_ = note;
  }
  
  ++sequencer_note_;
  if (sequencer_note_ >= sequence_.num_notes) {
    sequencer_note_ = 0;
  }
}

uint8_t drums_midi_notes[] = { 36, 38, 42 };

/* static */
void VoiceController::ClockDrumMachine() {
  uint16_t step_mask = 1 << drum_sequencer_step_;
  uint8_t override_mask = 1;
  if (has_drums()) {
    uint8_t x = seq_settings_.drums_x;
    uint8_t y = seq_settings_.drums_y;
    for (uint8_t i = 0; i < kNumDrumParts; ++i) {
      uint8_t level = ReadDrumMap(drum_sequencer_step_, i, x, y);
      if (level < 255 - drum_sequencer_perturbation_[i]) {
        level += drum_sequencer_perturbation_[i];
      }
      uint8_t threshold = ~seq_settings_.drums_density[i];
      
      // Override generative sequencer with pre-programmed pattern.
      if (seq_settings_.drums_override & override_mask) {
        level = (seq_settings_.drums_pattern[i] & step_mask) ? 255 : 0;
        threshold = 0;
      }
      
      if (level > threshold) {
        uint8_t level = 128 + (level >> 1);
        drum_synth.Trigger(i, level);
        midi_dispatcher.OnDrumNote(drums_midi_notes[i], level >> 1);
      }
      override_mask <<= 1;
    }
  }
  ++drum_sequencer_step_;
  if (drum_sequencer_step_ >= 16) {
    drum_sequencer_step_ = 0;
    for (uint8_t i = 0; i < kNumDrumParts; ++i) {
      drum_sequencer_perturbation_[i] = Random::GetByte() >> 3;
    }
  }
}

const prog_uint8_t slide_probability[] PROGMEM = {
  1, 0, 4, 4, 2, 0, 8, 6, 1, 1, 0, 0, 0, 0, 15, 15,
};

const prog_uint8_t accent_level[] PROGMEM = {
  16, 0, 2, 0, 1, 0, 0, 0, 8, 0, 1, 0, 4, 0, 2, 2,
};

/* static */
void VoiceController::ClockArpeggiator() {
  if (has_arpeggiator()) {
    uint16_t pattern = pgm_read_word(
        lut_res_arpeggiator_patterns + seq_settings_.arp_pattern);
    uint8_t has_arpeggiator_note = (arp_pattern_mask_ & pattern) ? 255 : 0;

    // Trigger notes only if the arp is on, and if keys are pressed.
    if (has_arpeggiator_note) {
      StepArpeggiator();
      const NoteEntry& arpeggio_note = pressed_keys_.sorted_note(arp_step_);
      uint8_t note = arpeggio_note.note;
      uint8_t velocity = arpeggio_note.velocity;
      note += 12 * arp_octave_;
      while (note > 127) {
        note -= 12;
      }
      uint8_t random = Random::GetByte();
      uint8_t slide_threshold = U8U8Mul(
          pgm_read_byte(slide_probability + arp_pattern_step_),
          seq_settings_.acidity);
      // Slide less frequently to first note.
      if (arp_step_ == 0) {
        slide_threshold >>= 1;
      }
      uint8_t accent = U8U8Mul(
          pgm_read_byte(accent_level + arp_pattern_step_),
          seq_settings_.acidity) >> 1;
      accent = (accent + U8U8MulShift8(accent, random)) >> 1;
      bool slid = random < slide_threshold;
      if (!slid) {
        midi_dispatcher.OnInternalNoteOff(previous_generated_note_);
      }
      voice_.NoteOn(note, velocity, slid ? 0x60 : 0, accent, slid);
      if (slid) {
        midi_dispatcher.OnInternalNoteOff(previous_generated_note_);
      }
      midi_dispatcher.OnInternalNoteOn(note, velocity);
      previous_generated_note_ = note;
    } else {
      voice_.NoteOff(previous_generated_note_);
      midi_dispatcher.OnInternalNoteOff(previous_generated_note_);
      previous_generated_note_ = 0xff;
    }
  }
  ++arp_pattern_step_;
  arp_pattern_mask_ <<= 1;
  if (!arp_pattern_mask_) {
    arp_pattern_mask_ = 1;
    arp_pattern_step_ = 0;
  }
}

/* static */
void VoiceController::ResetArpeggiatorPattern() {
  if (arp_direction_ == 1) {
    arp_octave_ = 0;
    arp_step_ = 0;
  } else {
    arp_step_ = pressed_keys_.size() - 1;
    arp_octave_ = seq_settings_.arp_range() - 1;
  }
}

/* static */
void VoiceController::StepArpeggiator() {
  uint8_t num_notes = pressed_keys_.size();
  if (seq_settings_.arp_direction() == ARPEGGIO_DIRECTION_RANDOM) {
    uint8_t random_byte = Random::GetByte();
    arp_octave_ = random_byte & 0xf;
    arp_step_ = (random_byte & 0xf0) >> 4;
    while (arp_octave_ >= seq_settings_.arp_range()) {
      arp_octave_ -= seq_settings_.arp_range();
    }
    while (arp_step_ >= num_notes) {
      arp_step_ -= num_notes;
    }
  } else {
    arp_step_ += arp_direction_;
    uint8_t change_octave = 0;
    if (arp_step_ >= num_notes) {
      arp_step_ = 0;
      change_octave = 1;
    } else if (arp_step_ < 0) {
      arp_step_ = num_notes - 1;
      change_octave = 1;
    }
    if (change_octave) {
      arp_octave_ += arp_direction_;
      if (arp_octave_ >= seq_settings_.arp_range() || arp_octave_ < 0) {
        if (seq_settings_.arp_direction() == ARPEGGIO_DIRECTION_UP_DOWN) {
          arp_direction_ = -arp_direction_;
          ResetArpeggiatorPattern();
          if (num_notes > 1 || seq_settings_.arp_range() > 1) {
            StepArpeggiator();
          }
        } else {
          ResetArpeggiatorPattern();
        }
      }
    }
  }
}

/* static */
void VoiceController::StartRecording() {
  Stop();
  StopArpeggiator();
  seq_settings_.arp_mode = 0;
  sequencer_recording_ = true;
  memset(&sequence_, 0, sizeof(Sequence));
}

/* static */
void VoiceController::StopRecording() {
  Stop();
  sequencer_recording_ = false;
  sequencer_transposition_ = 0;
  sequencer_velocity_ = 64;
  SaveSequence();
}

/* static */
void VoiceController::SaveSequence() {
  storage.Save(sequence_);
}

/* static */
void VoiceController::SavePatch() {
  if (dirty_) {
    storage.Save(seq_settings_);
  }
  voice_.SavePatch();
  dirty_ = false;
}

/* static */
void VoiceController::ResetToFactoryDefaults() {
  storage.ResetToFactoryDefaults(&seq_settings_);
  storage.ResetToFactoryDefaults(&sequence_);
  voice_.ResetToFactoryDefaults();
}

/* static */
uint8_t VoiceController::ReadDrumMap(
    uint8_t step,
    uint8_t instrument,
    uint8_t x,
    uint8_t y) {
  uint8_t i = x >> 7;
  uint8_t j = y >> 7;
  const prog_uint8_t* a_map = drum_map[i][j];
  const prog_uint8_t* b_map = drum_map[i + 1][j];
  const prog_uint8_t* c_map = drum_map[i][j + 1];
  const prog_uint8_t* d_map = drum_map[i + 1][j + 1];
  uint8_t offset = U8ShiftLeft4(instrument) + step;
  uint8_t a = pgm_read_byte(a_map + offset);
  uint8_t b = pgm_read_byte(b_map + offset);
  uint8_t c = pgm_read_byte(c_map + offset);
  uint8_t d = pgm_read_byte(d_map + offset);
  return U8Mix(U8Mix(a, b, x << 1), U8Mix(c, d, x << 1), y << 1);
}

const prog_uint8_t white_keys[] PROGMEM = {
  0, 2, 4, 5, 7, 9, 11,
  12, 14, 16, 17, 19, 21, 23,
  24, 26
};

/* static */
void VoiceController::RemoteControlDrumSequencer(uint8_t note) {
  if (note == 37) {
    // C#: enable override.
    seq_settings_.drums_override ^= 0xff;
  } else if (note == 39) {
    // D#: clear current pattern.
    seq_settings_.drums_pattern[drum_remote_control_current_instrument_] = 0;
  } else {
    uint16_t step_mask = 1;
    // F# / G# / A#: select part.
    for (uint8_t i = 0; i < kNumDrumParts; ++i) {
      if (note == 42 + (i << 1)) {
        drum_remote_control_current_instrument_ = i;
        return;
      }
    }
    // White keys: program pattern.
    for (uint8_t i = 0; i < 16; ++i) {
      if (note == 36 + pgm_read_byte(white_keys + i)) {
        seq_settings_.drums_pattern[
            drum_remote_control_current_instrument_] ^= step_mask;
        return;
      }
      step_mask <<= 1;
    }
  }
  dirty_ = true;
}

/* extern */
VoiceController voice_controller;

};  // namespace anu
