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

#include "anu/clock.h"

#include "anu/resources.h"

namespace anu {

/* <static> */
uint16_t Clock::clock_counter_;
uint16_t Clock::tick_duration_table_[kNumStepsInGroovePattern];
uint8_t Clock::tick_count_;
uint8_t Clock::step_count_;
uint8_t Clock::prescaler_;
uint8_t Clock::prescaler_counter_ = 0;
uint16_t Clock::tick_duration_ = 0;
volatile uint8_t Clock::num_clock_events_ = 0;
/* </static> */

static const uint32_t kSampleRateNum = 2000000L;
static const uint32_t kSampleRateDen = 51L;
static const int32_t kTempoFactor = \
    (4 * kSampleRateNum * 60L / 24L / kSampleRateDen);

/* static */
void Clock::Update(
    uint8_t bpm,
    uint8_t groove_template,
    uint8_t groove_amount,
    uint8_t prescaler) {
  STATIC_ASSERT(kTempoFactor == 392156L);

  int32_t rounding = 2 * static_cast<int32_t>(bpm);
  int32_t denominator = 4 * static_cast<int32_t>(bpm);
  int16_t base_tick_duration = (kTempoFactor + rounding) / denominator;
  for (uint8_t i = 0; i < kNumStepsInGroovePattern; ++i) {
    int32_t swing_direction = static_cast<int16_t>(pgm_read_word(
        lookup_table_table[LUT_RES_GROOVE_SWING + groove_template] + i));
    swing_direction *= base_tick_duration;
    swing_direction *= groove_amount;
    int16_t swing = swing_direction >> 16;
    tick_duration_table_[i] = base_tick_duration + swing;
  }
  tick_duration_ = tick_duration_table_[0];
  prescaler_ = prescaler;
}

/* extern */
Clock clock;

}  // namespace anu
