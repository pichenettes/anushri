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

#ifndef ANU_PARAMETERS_H_
#define ANU_PARAMETERS_H_

#include "avrlib/base.h"

namespace anu {

enum ParameterDomain {
  PARAMETER_DOMAIN_PATCH,
  PARAMETER_DOMAIN_SEQUENCER,
  PARAMETER_DOMAIN_UNASSIGNED
};

enum ParameterUnit {
  UNIT_RAW,
  UNIT_UINT8,
  UNIT_INT8,
  UNIT_CROSSFADE,
  UNIT_QUANTIZED_PITCH,
  UNIT_TEMPO
};

enum Parameters {
  // First row
  PARAMETER_VCO_MOD_BALANCE,
  PARAMETER_VCO_ENV_AMOUNT,
  PARAMETER_VCO_LFO_AMOUNT,
  PARAMETER_PW_MOD_BALANCE,
  PARAMETER_PW_ENV_AMOUNT,
  PARAMETER_PW_LFO_AMOUNT,
  PARAMETER_CUTOFF_BIAS,
  PARAMETER_CUTOFF_TRACKING,
  PARAMETER_CUTOFF_ENV_AMOUNT,
  PARAMETER_CUTOFF_LFO_AMOUNT,
  
  // Second row, ADSRLFO mode
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
  PARAMETER_ENV_LEGATO_MODE,

  // Second row, keyboard/play mode
  PARAMETER_TEMPO,
  PARAMETER_SWING,
  PARAMETER_ARP_MODE,
  PARAMETER_ARP_PATTERN,
  PARAMETER_ARP_ACIDITY,
  PARAMETER_VCO_DCO_RANGE,
  PARAMETER_VCO_DCO_FINE,
  PARAMETER_VELOCITY_MOD,
  PARAMETER_VCF_VELOCITY_AMOUNT,
  PARAMETER_VCA_VELOCITY_AMOUNT,
  PARAMETER_VIBRATO_RATE,
  PARAMETER_VIBRATO_DESTINATION,
  
  PARAMETER_DRUMS_X,
  PARAMETER_DRUMS_Y,
  PARAMETER_DRUMS_BD_DENSITY,
  PARAMETER_DRUMS_SD_DENSITY,
  PARAMETER_DRUMS_HH_DENSITY,
  PARAMETER_DRUMS_BD_TONE,
  PARAMETER_DRUMS_SD_TONE,
  PARAMETER_DRUMS_HH_TONE,
  PARAMETER_DRUMS_BALANCE,
  PARAMETER_DRUMS_BANDWIDTH,
  
  PARAMETER_UNASSIGNED,
  PARAMETER_LAST
};

struct Parameter {
  uint8_t domain;
  uint8_t offset;
  uint8_t unit;
  uint8_t min_value;
  uint8_t max_value;
  uint8_t padding;
  
  uint8_t Scale(uint8_t value_8bit) const;
  uint8_t Unscale(uint8_t value) const;
};

class ParameterManager {
 public:
  ParameterManager() { }
  ~ParameterManager() { }

  static const Parameter& parameter(uint8_t index);
  
  static uint8_t GetValue(const Parameter& parameter);
  static void SetValue(const Parameter& parameter, uint8_t value_8bit);
  
  static void SetScaled(uint8_t index, uint8_t value_8bit) {
    const Parameter& p = parameter(index);
    SetValue(p, p.Scale(value_8bit));
  }

  static uint8_t GetScaled(uint8_t index) {
    const Parameter& p = parameter(index);
    return p.Unscale(GetValue(p));
  }
  
  static uint8_t LookupCCMap(uint8_t cc);
  
  static void Init() { }
  
 private:
  static Parameter cached_definition_;
  static uint8_t cached_index_;
  
  DISALLOW_COPY_AND_ASSIGN(ParameterManager);
};

extern ParameterManager parameter_manager;

};  // namespace anu

#endif  // ANU_PARAMETERS_H_
