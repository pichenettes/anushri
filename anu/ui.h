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

#ifndef ANU_UI_H_
#define ANU_UI_H_

#include "avrlib/base.h"
#include "avrlib/ui/event_queue.h"

#include "anu/hardware_config.h"

namespace anu {

enum InputBits {
  INPUT_UTIL_BUTTON,
  INPUT_SEQ_BUTTON,
  INPUT_RUN_STOP_BUTTON,
  INPUT_KBD_BUTTON,
  INPUT_REC_BUTTON,
  INPUT_ADSR_BUTTON,
  INPUT_GATE,
  INPUT_TRIG
};

enum OutputBits {
  OUTPUT_ROW_3_LED,
  OUTPUT_ROW_2_LED,
  OUTPUT_ROW_1_LED,
  OUTPUT_LFO_LED,
  OUTPUT_GATE_LED,
  OUTPUT_CLOCK_LED,
  OUTPUT_GATE,
  OUTPUT_TRIG
};

enum Controls {
  CONTROL_SEQ_BUTTON,
  CONTROL_HOLD_BUTTON,
  CONTROL_KBD_BUTTON,
  CONTROL_RUN_STOP_BUTTON,
  CONTROL_ADSR_BUTTON,
  CONTROL_REC_BUTTON,

  CONTROL_SHIFT_SEQ_BUTTON,
  CONTROL_SHIFT_UTIL_BUTTON,
  CONTROL_SHIFT_KBD_BUTTON,
  CONTROL_SHIFT_RUN_STOP_BUTTON,
  CONTROL_SHIFT_ADSR_BUTTON,
  CONTROL_SHIFT_REC_BUTTON,
  
  CONTROL_SHIFT_LONG_PRESS,
  CONTROL_RUN_STOP_LONG_PRESS
};

enum DisplayMode {
  DISPLAY_MODE_LFO_GATE_CLOCK,
  DISPLAY_MODE_PPQN_SETTING,
  DISPLAY_MODE_MIDI_CHANNEL,
  DISPLAY_MODE_MIDI_FILTER,
};

const uint8_t kNumPots = 14;
const uint8_t kNumSwitches = 6;

class Ui {
 public:
  Ui() { }
  
  static void Init();
  static void Poll();
  static void DoEvents();
  static void FlushEvents() {
    queue_.Flush();
  }
  static uint8_t led_pattern() {
    return led_pattern_;
  }
  static void UpdateLeds();
  
 private:
  static void TrySavingSettings();
  static void HandleSwitchEvent(uint8_t index);
  static void HandlePotEvent(uint8_t index, uint8_t value);
  static void UnlockPot(uint8_t index);
  static void LockPots(bool snap);
  
  static uint16_t adc_values_[kNumPots];
  static uint8_t adc_thresholds_[kNumPots];
  static bool snapped_[kNumPots];
  static int16_t snap_position_cache_[kNumPots];
  static uint8_t active_row_;
  static uint8_t led_pattern_;
  static uint8_t pwm_cycle_;
  static uint8_t inhibit_switch_;
  static uint8_t display_mode_;
  static uint8_t scanned_pot_;
  static uint8_t pot_scanning_warm_up_;
  static bool busy_;
  static int8_t display_snap_delta_;
  static avrlib::EventQueue<16> queue_;
  static uint8_t disable_switch_sensing_;
  static uint16_t long_press_counter_;
  static bool strummer_enabled_;
   
  DISALLOW_COPY_AND_ASSIGN(Ui);
};

extern Ui ui;
extern Inputs inputs;
extern Outputs outputs;

}  // namespace anu

#endif  // ANU_UI_H_
