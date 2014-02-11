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

#ifndef ANU_DRUM_SYNTH_H_
#define ANU_DRUM_SYNTH_H_

#include <string.h>

#include "avrlib/base.h"

namespace anu {

static const uint8_t kNumDrumInstruments = 3;

struct DrumPatch {
  uint8_t pitch;
  uint8_t pitch_decay;
  uint8_t pitch_mod;
  uint8_t amp_decay;
  uint8_t crunchiness;
  uint8_t level;
};

struct DrumState {
  uint16_t phase;
  uint16_t phase_increment;
  uint16_t pitch_env_phase;
  uint16_t pitch_env_increment;
  uint16_t amp_env_phase;
  uint16_t amp_env_increment;
  uint8_t amp_level;
  uint8_t amp_level_noise;
  uint8_t level;
};

class DrumSynth {
 public:
  DrumSynth() { }
  ~DrumSynth() { }
  static void Init();
  static void Trigger(uint8_t instrument, uint8_t velocity);
  static void SetParameterCc(uint8_t cc, uint8_t value);
  static void MorphPatch(uint8_t instrument, uint8_t value);
  static void SetBalance(uint8_t value);
  static void SetBandwidth(uint8_t bandwidth);
  static void Render();
  static void FillWithSilence();
  static uint32_t idle_time_ms();
  static bool playing() { return playing_; }
  
 private:
  static void UpdateModulations();
  
  static DrumPatch patch_[kNumDrumInstruments];
  static DrumState state_[kNumDrumInstruments];
  
  static uint8_t sample_;
  static uint8_t sample_counter_;
  static uint8_t sample_rate_;
  static uint8_t fade_counter_;
  static uint32_t last_event_time_;
  static bool playing_;
  
  DISALLOW_COPY_AND_ASSIGN(DrumSynth);
};

extern DrumSynth drum_synth;

};  // namespace anu

#endif  // ANU_DRUM_SYNTH_H_
