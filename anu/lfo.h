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
//
// Contrary to oscillators which are templatized "static singletons", to
// generate the fastest, most specialized code, LFOs are less
// performance-sensitive and are thus implemented as a traditional class.

#ifndef ANU_LFO_H_
#define ANU_LFO_H_

#include "avrlib/base.h"

#include <avr/pgmspace.h>

namespace anu {

extern const prog_uint8_t wav_res_lfo_waveforms[];

enum LfoShape {
  LFO_SHAPE_TRIANGLE,
  LFO_SHAPE_SQUARE,
  LFO_SHAPE_RAMP_UP,
  LFO_SHAPE_RAMP_DOWN,
  
  LFO_SHAPE_S_H,
  LFO_SHAPE_BERNOUILLI,
  LFO_SHAPE_LINES,
  LFO_SHAPE_NOISE,
  LFO_SHAPE_LAST
};

class Lfo {
 public:
  Lfo() { }
  ~Lfo() { }

  uint16_t Render();
  
  inline void set_target_phase(uint16_t target_phase) {
    // This is a simple P.D. corrector to get the LFO to track the phase and
    // frequency of the pseudo-sawtooth given by the MIDI clock.
    uint16_t phi = phase_ >> 16;
    int16_t d_error = (65536 / 24) - (phi - previous_phase_);
    int16_t p_error = target_phase - phi;
    int32_t error = 0;
    error += d_error;
    error += p_error;
    error <<= 8;
    phase_increment_ += error;
    // The tracking range is 15 BPM .. 960 BPM.
    if (phase_increment_ < 109523) {
      phase_increment_ = 109523;
    } else if (phase_increment_ > 7009509) {
      phase_increment_ = 7009509;
    }
    previous_phase_ = phi;
  }
  
  inline void set_phase(uint32_t phase) {
    looped_ = phase < phase_;
    phase_ = phase;
  }

  inline void set_phase_increment(uint32_t phase_increment) {
    phase_increment_ = phase_increment;
  }
  
  inline void set_shape(LfoShape shape) {
    shape_ = shape;
  }
  
  bool looped() const { return looped_; }

 private:
  uint32_t phase_increment_;
  uint32_t phase_;
  uint16_t previous_phase_;
  uint16_t value_;
  uint16_t next_value_;
  uint32_t filtered_value_;
  
  uint8_t shape_;
  bool looped_;
  
  DISALLOW_COPY_AND_ASSIGN(Lfo);
};

}  // namespace anu

#endif  // ANU_LFO_H_
