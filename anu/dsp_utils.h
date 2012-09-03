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
// A set of basic operands, especially useful for fixed-point arithmetic, with
// fast ASM implementations.

#ifndef ANU_DSP_UTILS_H_
#define ANU_DSP_UTILS_H_

#include "avrlib/base.h"
#include "avrlib/op.h"

#include <avr/pgmspace.h>

#define CLIP(x) if (x < -32767) x = -32767; if (x > 32767) x = 32767;

namespace anu {
  
using namespace avrlib;

inline int16_t Mix(int16_t a, int16_t b, uint16_t balance) {
  if (b > a) {
    return a + U16U16MulShift16(b - a, balance);
  } else {
    return a - U16U16MulShift16(a - b, balance);
  }
}

inline uint16_t Mix(uint16_t a, uint16_t b, uint16_t balance) {
  if (b > a) {
    return a + U16U16MulShift16(b - a, balance);
  } else {
    return a - U16U16MulShift16(a - b, balance);
  }
}

inline uint16_t InterpolateIncreasing(
    const prog_uint16_t* table,
    uint16_t phase) {
  uint16_t a = pgm_read_word(table + (phase >> 8));
  uint16_t b = pgm_read_word(table + (phase >> 8) + 1);
  return a + U16U8MulShift8(b - a, phase);
}

}  // namespace anu

#endif  // ANU_DSP_UTILS_H__