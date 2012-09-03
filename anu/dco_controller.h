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

#ifndef ANU_DCO_CONTROLLER_H_
#define ANU_DCO_CONTROLLER_H_

#include "avrlib/base.h"
#include "avrlib/op.h"

#include "anu/hardware_config.h"
#include "anu/resources.h"

namespace anu {

using namespace avrlib;

static const uint16_t kOctave = 12 << 7;
static const uint16_t kFirstNote = 16 << 7;

class DcoController {
 public:
  DcoController() { }
  ~DcoController() { }
  
  static void Start() {
    // Configure the 16-bit timer for the "PWM, Phase and Frequency Correct"
    // mode, TOP set by OCR1A.
    TuningTimer::set_mode(_BV(WGM10), _BV(WGM13), 2);
    // Enable square wave generation of OC1A.
    Dco::Start();
  }

  static void Stop() {
    Dco::Stop();
  }
  
  static void Mute() {
    Dco::set_frequency(0);
  }
  
  static void set_note(int16_t note) {
    // Lowest note: E0.
    note -= kFirstNote;
    // Transpose the lowest octave up.
    while (note < 0) {
      note += kOctave;
    }
    uint8_t shifts = 0;
    while (note >= kOctave) {
      note -= kOctave;
      ++shifts;
    }
    uint16_t index_integral = U16ShiftRight4(note);
    uint16_t index_fractional = U8U8Mul(note & 0xf, 16);
    uint16_t count = pgm_read_word(lut_res_dco_pitch + index_integral);
    uint16_t next = pgm_read_word(lut_res_dco_pitch + index_integral + 1);
    count -= U16U8MulShift8(count - next, index_fractional);
    while (shifts--) {
      count >>= 1;
    }
    Dco::set_frequency(count);
  }
  
 private:
  DISALLOW_COPY_AND_ASSIGN(DcoController);
};

extern DcoController dco_controller;

};  // namespace anu

#endif  // ANU_DCO_CONTROLLER_H_
