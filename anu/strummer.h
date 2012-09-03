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
//
// Strummer (note generator controlled from 5-pots).

#ifndef ANU_STRUMMER_H_
#define ANU_STRUMMER_H_

#include "avrlib/base.h"

#include "anu/midi_dispatcher.h"

namespace anu {

struct StrummerData {
  uint8_t root_note;
  uint8_t octave_shift;
  uint8_t pentatonic_shift;
  uint8_t bhairav_shift;
  uint8_t chromatic_shift;
};

class Strummer {
 public:
  Strummer() { }
  
  static void Start();
  static void Update(uint8_t pot, uint8_t value);
  static void PlayNote();

 private:
  static uint8_t current_note_;
  static StrummerData data_;
  
  DISALLOW_COPY_AND_ASSIGN(Strummer);
};

extern Strummer strummer;

}  // namespace anu

#endif  // ANU_STRUMMER_H_
