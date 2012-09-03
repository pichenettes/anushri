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
// LFO (cheap oscillator).

#include "anu/lfo.h"

#include "avrlib/random.h"

#include "anu/dsp_utils.h"

namespace anu {

using avrlib::Random;

uint16_t Lfo::Render() {
  uint16_t value;
  switch (shape_) {
    case LFO_SHAPE_TRIANGLE:
      value = (phase_ & 0x80000000)
        ? phase_ >> 15
        : ~(phase_ >> 15);
      break;
    
    case LFO_SHAPE_SQUARE:
      value = (phase_ & 0x80000000) ? 0xffff : 0;
      break;

    case LFO_SHAPE_RAMP_UP:
      value = phase_ >> 16;
      break;
    
    case LFO_SHAPE_RAMP_DOWN:
      value = ~(phase_ >> 16);
      break;
        
    case LFO_SHAPE_S_H:
      if (looped_) {
        value_ = Random::GetWord();
      }
      value = value_;
      break;

    case LFO_SHAPE_BERNOUILLI:
      if (looped_) {
        value_ = (Random::GetWord() & 1) ? 65535 : 0;
      }
      value = value_;
      break;

    case LFO_SHAPE_LINES:
      if (looped_) {
        value_ = next_value_;
        next_value_ = Random::GetWord();
      }
      value = Mix(value_, next_value_, phase_ >> 16);
      break;
      
    case LFO_SHAPE_NOISE:
      value = Random::GetWord();
      break;

  }
  phase_ += phase_increment_;
  looped_ = phase_ < phase_increment_;
  return value;
}

}  // namespace anu
