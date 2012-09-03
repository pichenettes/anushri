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
// Strummer.

#include "anu/strummer.h"

#include "anu/voice_controller.h"

namespace anu {

/* static */
uint8_t Strummer::current_note_;

/* static */
StrummerData Strummer::data_ = { 0, 2, 5, 7, 4 };

/* static */
void Strummer::Start() {
  current_note_ = 0xff;
  PlayNote();
}

const prog_uint8_t octave_shift[] PROGMEM = {
  -24, -12, 0, 12, 24
};

const prog_uint8_t pentatonic_shift[] PROGMEM = {
  -12, -3, -5, -8, -10, 0, 2, 4, 7, 9, 12
};

const prog_uint8_t bhairav_shift[] PROGMEM = {
  -12, -11, -8, -7, -5, -4, -1, 0, 1, 4, 5, 7, 8, 11, 12
};

const prog_uint8_t chromatic_shift[] PROGMEM = {
  -12, -5 , -2, -1, 0, 1, 2, 7, 12
};

const prog_uint8_t ranges[] PROGMEM = {
  12,
  5,
  sizeof(pentatonic_shift),
  sizeof(bhairav_shift),
  sizeof(chromatic_shift)
};

void Strummer::Update(uint8_t pot, uint8_t value) {
  value = U8U8MulShift8(pgm_read_byte(ranges + pot), value);
  uint8_t* address = static_cast<uint8_t*>(static_cast<void*>(&data_));
  address[pot] = value;
  PlayNote();
};

/* static */
void Strummer::PlayNote() {
  int16_t note = data_.root_note + 48;
  note += static_cast<int8_t>(
      pgm_read_byte(octave_shift + data_.octave_shift));
  note += static_cast<int8_t>(
      pgm_read_byte(pentatonic_shift + data_.pentatonic_shift));
  note += static_cast<int8_t>(
      pgm_read_byte(bhairav_shift + data_.bhairav_shift));
  note += static_cast<int8_t>(
      pgm_read_byte(chromatic_shift + data_.chromatic_shift));
  while (note < 12) {
    note += 12;
  }
  while (note > 96) {
    note -= 12;
  }
  if (note != current_note_) {
    voice_controller.NoteOn(note, 100);
    if (current_note_ != 0xff) {
      voice_controller.NoteOff(current_note_);
    }
    current_note_ = note;
  }
}

/* extern */
Strummer strummer;

}  // namespace anu
