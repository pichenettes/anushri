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
// Global clock.

#ifndef ANU_CLOCK_H_
#define ANU_CLOCK_H_

#include "avrlib/base.h"
#include "avrlib/gpio.h"

namespace anu {

static const uint8_t kNumStepsInGroovePattern = 16;
static const uint8_t kNumTicksPerStep = 6;

class Clock {
 public:
  static inline void Reset() {
    clock_counter_ = 0;
    tick_count_ = 0;
    step_count_ = 0;
    tick_duration_ = tick_duration_table_[0];
    num_clock_events_ = 0;
  }
  
  static inline void Tick() {
    ++clock_counter_;
    if (clock_counter_ >= tick_duration_) {
      ++num_clock_events_;
      clock_counter_ = 0;
    }
  }
  
  static inline uint8_t CountEvents() {
    uint8_t count = 0;
    while (num_clock_events_) {
      ++tick_count_;
      if (tick_count_ == kNumTicksPerStep) {
        tick_count_ = 0;
        ++step_count_;
        if (step_count_ == kNumStepsInGroovePattern) {
          step_count_ = 0;
        }
        tick_duration_ = tick_duration_table_[step_count_];
      }
      ++prescaler_counter_;
      if (prescaler_counter_ >= prescaler_) {
        ++count;
        prescaler_counter_ = 0;
      }
      --num_clock_events_;
    }
    return count;
  }
  
  static void set_prescaler(uint8_t prescaler) {
    prescaler_ = prescaler;
  }
  
  static void Update(
      uint8_t bpm,
      uint8_t groove_template,
      uint8_t groove_amount,
      uint8_t prescaler);

 private:
  static uint16_t clock_counter_;
  static uint16_t tick_duration_table_[kNumStepsInGroovePattern];
  static uint16_t tick_duration_;
  static uint8_t tick_count_;
  static uint8_t step_count_;
  static uint8_t prescaler_;
  static uint8_t prescaler_counter_;
  static volatile uint8_t num_clock_events_;
};

extern Clock clock;

}  // namespace anu

#endif // ANU_CLOCK_H_
