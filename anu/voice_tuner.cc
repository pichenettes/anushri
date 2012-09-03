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

#include "anu/voice_tuner.h"

#include <avr/interrupt.h>
#include <math.h>

#include "anu/dco_controller.h"
#include "anu/system_settings.h"
#include "anu/voice_controller.h"

namespace anu {
  
/* static */
uint8_t VoiceTuner::tuning_state_;

/* static */
uint32_t VoiceTuner::pitch_measurements_;

/* static */
uint8_t VoiceTuner::num_pitch_measurements_;

/* static */
float VoiceTuner::pitch_c1_;

/* static */
float VoiceTuner::pitch_c3_;

/* static */
float VoiceTuner::pitch_c5_;

/* static */
TuningTimer VoiceTuner::tuning_timer_;

/* static */
void VoiceTuner::SetTuningState(uint8_t state) {
  tuning_state_ = state;
  cli();
  num_pitch_measurements_ = 0;
  pitch_measurements_ = 0;
  sei();
}

/* static */
float VoiceTuner::GetPitch() {
  return static_cast<float>(num_pitch_measurements_ - 8) \
      * (F_CPU / 8) / static_cast<float>(pitch_measurements_);
}

/* static */
void VoiceTuner::StartTuning() {
  dco_controller.Stop();
  tuning_timer_.set_mode(0, 0, 2);
  tuning_timer_.Start();
  tuning_timer_.StartInputCapture();
  SetTuningState(TUNING_PROBING_C1);
}

void VoiceTuner::Refresh() {
  switch (tuning_state_) {
    case TUNING_PROBING_C1:
      voice_controller.mutable_voice()->Lock(1048, 0, 4095, 128);
      if (num_pitch_measurements_ >= 8 + 8) {
        pitch_c1_ = GetPitch();
        SetTuningState(TUNING_PROBING_C3);
      };
      break;
    
    case TUNING_PROBING_C3:
      voice_controller.mutable_voice()->Lock(2048, 0, 4095, 128);
      if (num_pitch_measurements_ >= 8 + 16) {
        pitch_c3_ = GetPitch();
        SetTuningState(TUNING_PROBING_C5);
      }
      break;
      
    case TUNING_PROBING_C5:
      voice_controller.mutable_voice()->Lock(3048, 0, 4095, 128);
      if (num_pitch_measurements_ >= 8 + 32) {
        pitch_c5_ = GetPitch();
        tuning_timer_.StopInputCapture();
        tuning_timer_.Stop();
        dco_controller.Start();
        SetTuningState(TUNING_COMPUTING_RESPONSE);
      }
      break;
    
    case TUNING_COMPUTING_RESPONSE:
      {
        // 2 * 64000.0 * math.log(2) / 3 = 29574.28
        // 128.0 * 12.0 / math.log(2) = 2215.98
        // midi2hz(60) = 261.625
        uint16_t scale_low = static_cast<uint16_t>(
            29574.28f / logf(pitch_c3_ / pitch_c1_));
        uint16_t scale_high = static_cast<uint16_t>(
            29574.28f / logf(pitch_c5_ / pitch_c3_));
        uint16_t offset = 60 * 128 + static_cast<uint16_t>(
            2215.98f * logf(pitch_c3_ / 261.625));
        system_settings.set_calibration_data(offset, scale_low, scale_high);
        // Fall through!
      }
      
    case TUNING_ABORT:
      voice_controller.mutable_voice()->Unlock();
      tuning_state_ = TUNING_OFF;
      break;
  }
}

/* extern */
VoiceTuner voice_tuner;

};  // namespace anu
