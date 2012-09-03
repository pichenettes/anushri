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

#include "anu/parameter.h"

#include <avr/pgmspace.h>

#include "avrlib/op.h"

#include "anu/drum_synth.h"
#include "anu/midi_dispatcher.h"
#include "anu/voice.h"
#include "anu/voice_controller.h"

namespace anu {
  
using namespace avrlib;
  
uint8_t Parameter::Scale(uint8_t value_8bits) const {
  uint8_t scaled_value;
  if (unit == UNIT_RAW || unit == UNIT_CROSSFADE) {
    scaled_value = value_8bits;
  } else if (unit == UNIT_QUANTIZED_PITCH) {
    scaled_value = pgm_read_byte(wav_res_pitch_deadband + value_8bits);
  } else {
    uint8_t range = max_value - min_value + 1;
    scaled_value = U8U8MulShift8(range, value_8bits);
    scaled_value += min_value;
    if (unit == UNIT_TEMPO) {
      if (scaled_value >= 239) {
        scaled_value = 240;
      }
      if (scaled_value < 40) {
        scaled_value = 0;
      }
    }
  }
  return scaled_value;
}

uint8_t Parameter::Unscale(uint8_t value) const {
  if (unit == UNIT_RAW || unit == UNIT_CROSSFADE) {
    return value;
  } else {
    for (uint8_t i = 0; i < 255; ++i) {
      if (Scale(i) == value) {
        return i;
      }
    }
  }
}

typedef Parameter PROGMEM prog_Parameter;

static const prog_uint8_t cc_map[116] PROGMEM = {
  // 0 to 7
  0xff, 0xff, 0xff, 0xff, 0xff, PARAMETER_GLIDE, 0xff, 0xff, 
  // 8 to 15
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  // 16 to 23
  PARAMETER_VCO_ENV_AMOUNT, PARAMETER_VCO_LFO_AMOUNT,
  PARAMETER_PW_ENV_AMOUNT, PARAMETER_PW_LFO_AMOUNT,
  PARAMETER_CUTOFF_ENV_AMOUNT, PARAMETER_CUTOFF_LFO_AMOUNT,
  PARAMETER_CUTOFF_BIAS, PARAMETER_CUTOFF_TRACKING, 
  // 24 to 31
  PARAMETER_ENV_ATTACK, PARAMETER_ENV_DECAY,
  PARAMETER_ENV_SUSTAIN, PARAMETER_ENV_RELEASE,
  PARAMETER_ENV_VCA_MORPH, PARAMETER_VCF_VELOCITY_AMOUNT,
  PARAMETER_VCA_VELOCITY_AMOUNT, PARAMETER_ENV_LEGATO_MODE,
  // 32 to 39
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  // 40 to 47
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  // 48 to 55
  PARAMETER_VCO_DCO_RANGE,PARAMETER_VCO_DCO_FINE,
  PARAMETER_VCO_DETUNE, PARAMETER_VCO_FINE,
  PARAMETER_VIBRATO_RATE, PARAMETER_VIBRATO_DESTINATION,
  PARAMETER_LFO_SHAPE, PARAMETER_LFO_RATE, 
  // 56 to 63
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  // 64 to 71
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  // 72 to 79
  PARAMETER_ENV_RELEASE, PARAMETER_ENV_ATTACK,
  PARAMETER_CUTOFF_BIAS, PARAMETER_ENV_DECAY,
  PARAMETER_LFO_RATE, PARAMETER_VCO_LFO_AMOUNT,
  0xff, 0xff, 
  // 80 to 87
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  // 88 to 95
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  // 96 to 103
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, PARAMETER_DRUMS_X, PARAMETER_DRUMS_Y, 
  // 104 to 111
  PARAMETER_DRUMS_BD_DENSITY,
  PARAMETER_DRUMS_SD_DENSITY,
  PARAMETER_DRUMS_HH_DENSITY,
  PARAMETER_DRUMS_BD_TONE,
  PARAMETER_DRUMS_SD_TONE,
  PARAMETER_DRUMS_HH_TONE,
  PARAMETER_DRUMS_BALANCE,
  PARAMETER_DRUMS_BANDWIDTH,
  // 112 to 115
  PARAMETER_SWING,
  PARAMETER_ARP_MODE,
  PARAMETER_ARP_PATTERN,
  PARAMETER_ARP_ACIDITY
};

static const prog_Parameter parameters[PARAMETER_LAST] PROGMEM = {
  // PARAMETER_VCO_MOD_BALANCE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_VCO_ENV_AMOUNT, UNIT_CROSSFADE, 0, 0 },

  // PARAMETER_VCO_ENV_AMOUNT
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_VCO_ENV_AMOUNT, UNIT_RAW, 0, 0 },

  // PARAMETER_VCO_LFO_AMOUNT
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_VCO_LFO_AMOUNT, UNIT_RAW, 0, 0 },

  // PARAMETER_PW_MOD_BALANCE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_PW_ENV_AMOUNT, UNIT_CROSSFADE, 0, 0 },

  // PARAMETER_PW_ENV_AMOUNT
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_PW_ENV_AMOUNT, UNIT_RAW, 0, 0 },

  // PARAMETER_PW_LFO_AMOUNT
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_PW_LFO_AMOUNT, UNIT_RAW, 0, 0 },

  // PARAMETER_CUTOFF_BIAS
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_CUTOFF_BIAS, UNIT_RAW, 0, 0 },

  // PARAMETER_CUTOFF_TRACKING
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_CUTOFF_TRACKING, UNIT_RAW, 0, 0 },
  
  // PARAMETER_CUTOFF_ENV_AMOUNT
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_CUTOFF_ENV_AMOUNT, UNIT_RAW, 0, 0 },
  
  // PARAMETER_CUTOFF_LFO_AMOUNT
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_CUTOFF_LFO_AMOUNT, UNIT_RAW, 0, 0 },
  
  // PARAMETER_GLIDE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_KBD_GLIDE, UNIT_RAW, 0, 0 },

  // PARAMETER_VCO_DETUNE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_VCO_DETUNE, UNIT_QUANTIZED_PITCH, 0, 0 },

  // PARAMETER_VCO_FINE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_VCO_FINE, UNIT_INT8, -32, 32 },
  
  // PARAMETER_LFO_SHAPE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_LFO_SHAPE, UNIT_UINT8,
    LFO_SHAPE_TRIANGLE, LFO_SHAPE_NOISE },

  // PARAMETER_LFO_RATE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_LFO_RATE, UNIT_RAW, 0, 0 },

  // PARAMETER_ENV_ATTACK
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_ENV_ATTACK, UNIT_RAW, 0, 0 },

  // PARAMETER_ENV_DECAY
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_ENV_DECAY, UNIT_RAW, 0, 0 },

  // PARAMETER_ENV_SUSTAIN
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_ENV_SUSTAIN, UNIT_RAW, 0, 0 },

  // PARAMETER_ENV_RELEASE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_ENV_RELEASE, UNIT_RAW, 0, 0 },

  // PARAMETER_ENV_VCA_MORPH
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_ENV_VCA_MORPH, UNIT_RAW, 0, 0 },

  // PARAMETER_ENV_LEGATO_MODE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_ENV_LEGATO_MODE, UNIT_RAW, 0, 0 },

  // PARAMETER_SEQ_TEMPO
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_TEMPO, UNIT_TEMPO, 20, 240 },

  // PARAMETER_SEQ_SWING
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_SWING, UNIT_RAW, 0, 0 },

  // PARAMETER_ARP_MODE
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_ARP_MODE, UNIT_UINT8, 0, 8 },

  // PARAMETER_ARP_PATTERN
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_ARP_PATTERN, UNIT_UINT8, 0, 5 },

  // PARAMETER_ARP_ACIDITY
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_ARP_ACIDITY, UNIT_UINT8, 0, 15 },

  // PARAMETER_VCO_DCO_RANGE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_VCO_DCO_RANGE, UNIT_INT8, -2, 2 },

  // PARAMETER_VCO_DCO_FINE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_VCO_DCO_FINE, UNIT_INT8, -127, 127 },

  // PARAMETER_VELOCITY_MOD
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_KBD_VCF_VELOCITY_AMOUNT, UNIT_CROSSFADE,
    0, 0 },

  // PARAMETER_VCF_VELOCITY_AMOUNT
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_KBD_VCF_VELOCITY_AMOUNT, UNIT_RAW, 0, 0 },

  // PARAMETER_VCA_VELOCITY_AMOUNT
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_KBD_VCA_VELOCITY_AMOUNT, UNIT_RAW, 0, 0 },

  // PARAMETER_VIBRATO_RATE
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_VIBRATO_RATE, UNIT_RAW, 0, 0 },

  // PARAMETER_VIBRATO_DESTINATION
  { PARAMETER_DOMAIN_PATCH, PRM_PATCH_VIBRATO_DESTINATION, UNIT_RAW, 0, 0 },

  // PARAMETER_DRUMS_X
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_DRUMS_X, UNIT_RAW, 0, 0 },

  // PARAMETER_DRUMS_Y
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_DRUMS_Y, UNIT_RAW, 0, 0 },

  // PARAMETER_DRUMS_BD_DENSITY
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_DRUMS_BD_DENSITY, UNIT_RAW, 0, 0 },

  // PARAMETER_DRUMS_SD_DENSITY
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_DRUMS_SD_DENSITY, UNIT_RAW, 0, 0 },

  // PARAMETER_DRUMS_HH_DENSITY
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_DRUMS_HH_DENSITY, UNIT_RAW, 0, 0 },

  // PARAMETER_DRUMS_BD_TONE
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_DRUMS_BD_TONE, UNIT_RAW, 0, 0 },

  // PARAMETER_DRUMS_SD_TONE
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_DRUMS_SD_TONE, UNIT_RAW, 0, 0 },

  // PARAMETER_DRUMS_HH_TONE
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_DRUMS_HH_TONE, UNIT_RAW, 0, 0 },
  
  // PARAMETER_DRUMS_BALANCE
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_DRUMS_BALANCE, UNIT_RAW, 0, 0 },

  // PARAMETER_DRUMS_BANDWIDTH
  { PARAMETER_DOMAIN_SEQUENCER, PRM_SEQ_DRUMS_BANDWIDTH, UNIT_RAW, 0, 0 },
  
  // PARAMETER_UNASSIGNED,
  { PARAMETER_DOMAIN_SEQUENCER, 0, UNIT_RAW, 0, 0 },
};

/* static */
Parameter ParameterManager::cached_definition_;

/* static */
uint8_t ParameterManager::cached_index_ = 0xff;

/* static */
const Parameter& ParameterManager::parameter(uint8_t index) {
  if (index != cached_index_) {
    ResourcesManager::Load(parameters, index, &cached_definition_);
    cached_index_ = index;
  }
  return cached_definition_;
}

/* static */
uint8_t ParameterManager::LookupCCMap(uint8_t cc) {
  return cc < sizeof(cc_map) ? pgm_read_byte(cc_map + cc) : 0xff;
}

/* static */
void ParameterManager::SetValue(const Parameter& p, uint8_t value) {
  if (p.domain == PARAMETER_DOMAIN_PATCH) {
    if (p.unit == UNIT_CROSSFADE) {
      value = pgm_read_byte(wav_res_deadband + value);
      if (value >= 128) {
        voice_controller.mutable_voice()->SetValue(
            p.offset,
            0);
        voice_controller.mutable_voice()->SetValue(
            p.offset + 1,
            (value - 128) << 1);
      } else {
        voice_controller.mutable_voice()->SetValue(
            p.offset,
            255 - (value << 1));
        voice_controller.mutable_voice()->SetValue(
            p.offset + 1,
            0);
      }
    } else {
      voice_controller.mutable_voice()->SetValue(p.offset, value);
    }
  } else if (p.domain == PARAMETER_DOMAIN_SEQUENCER) {
    voice_controller.SetValue(p.offset, value);
  }
}

/* static */
uint8_t ParameterManager::GetValue(const Parameter& p) {
  if (p.domain == PARAMETER_DOMAIN_PATCH) {
    return voice_controller.voice().GetValue(p.offset);
  } else if (p.domain == PARAMETER_DOMAIN_SEQUENCER) {
    return voice_controller.GetValue(p.offset);
  }
}

/* extern */
ParameterManager parameter_manager;

};  // namespace anu
