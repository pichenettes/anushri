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

#ifndef ANU_VOICE_TUNER_H_
#define ANU_VOICE_TUNER_H_

#include "avrlib/base.h"

#include "anu/hardware_config.h"

namespace anu {

enum TuningState {
  TUNING_OFF,
  TUNING_PROBING_C1,
  TUNING_PROBING_C3,
  TUNING_PROBING_C5,
  TUNING_COMPUTING_RESPONSE,
  TUNING_ABORT
};

class VoiceTuner {
 public:
  VoiceTuner() { }
  ~VoiceTuner() { }
  static void Init() { tuning_state_ = TUNING_OFF; }
  static void Refresh();
  
  static void UpdatePitchMeasurement(uint32_t pitch_measurement) {
    ++num_pitch_measurements_;
    if (num_pitch_measurements_ > 8) {
      pitch_measurements_ += pitch_measurement;
    }
  }
  
  static void Abort() {
    tuning_state_ = TUNING_ABORT;
  }
  static void StartTuning();
  static uint8_t tuning_state() { return tuning_state_; }

 private:
  static void SetTuningState(uint8_t index);
  static void Process();
  static float GetPitch();
   
  static uint8_t tuning_state_;
  static uint32_t pitch_measurements_;
  static uint8_t num_pitch_measurements_;
  
  static float pitch_c1_;
  static float pitch_c3_;
  static float pitch_c5_;
  
  static TuningTimer tuning_timer_;
  
  DISALLOW_COPY_AND_ASSIGN(VoiceTuner);
};

extern VoiceTuner voice_tuner;

};  // namespace anu

#endif  // ANU_VOICE_TUNER_H_
