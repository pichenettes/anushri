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

#include "anu/system_settings.h"

#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "anu/resources.h"

namespace anu {

/* static */
SystemSettingsData SystemSettings::data_;

uint8_t midi_channel;
uint8_t midi_out_mode;
uint8_t clock_ppqn;
uint8_t reference_note;
uint8_t padding[2];

uint16_t vco_cv_offset;
uint16_t vco_cv_scale_low;
uint16_t vco_cv_scale_high;
uint8_t more_padding[3];

/* extern */
const prog_SystemSettingsData init_settings PROGMEM = {
  // MIDI channel
  0,
  
  // MIDI out filter
  0xff,
  
  // Clock PPQN
  2,
  
  // Reference note
  60,
  
  // Padding
  0, 0,
  
  // VCO calibration data
  60 * 128,
  64000 / 3, 
  64000 / 3,
  
  // Padding
  0, 0, 0
};

/* extern */
SystemSettings system_settings;

};  // namespace anu

