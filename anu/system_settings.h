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

#ifndef ANU_SYSTEM_SETTINGS_H_
#define ANU_SYSTEM_SETTINGS_H_

#include "avrlib/base.h"

#include "anu/storage.h"

namespace anu {

enum MidiOutBitmask {
  MIDI_OUT_TX_INPUT_MESSAGES = 1,
  MIDI_OUT_TX_GENERATED_MESSAGES = 2,
  MIDI_OUT_TX_GENERATED_DRUM_MESSAGES = 4,
  MIDI_OUT_TX_TRANSPORT = 8,
  MIDI_OUT_TX_CONTROLLERS = 16
};

struct SystemSettingsData {
  uint8_t midi_channel;
  uint8_t midi_out_mode;
  uint8_t clock_ppqn;
  uint8_t reference_note;
  uint8_t padding[2];
  
  uint16_t vco_cv_offset;
  uint16_t vco_cv_scale_low;
  uint16_t vco_cv_scale_high;
  uint8_t more_padding[3];
};

typedef SystemSettingsData PROGMEM prog_SystemSettingsData;
extern const prog_SystemSettingsData init_settings PROGMEM;

template<>
struct StorageLayout<SystemSettingsData> {
  static uint8_t* eeprom_address() { 
    return (uint8_t*)(1000);
  }
  static const prog_char* init_data() {
    return (prog_char*)&init_settings;
  }
};

class SystemSettings {
 public:
  SystemSettings() { }
  
  static void Init() {
    storage.Load(&data_);
  }
  
  static void ResetToFactoryDefaults() {
    storage.ResetToFactoryDefaults(&data_);
  }
  
  static inline uint8_t receive_channel(uint8_t channel) {
    return channel == data_.midi_channel;
  }
  
  static inline uint8_t midi_channel() {
    return data_.midi_channel;
  }
  
  static inline uint8_t midi_out_mode() { return data_.midi_out_mode; }
  static inline uint8_t clock_ppqn() { return data_.clock_ppqn; }
  static inline uint8_t reference_note() { return data_.reference_note; }
  static inline int16_t vco_cv_offset() { return data_.vco_cv_offset; }
  static inline uint16_t vco_cv_scale_low() { return data_.vco_cv_scale_low; }
  static inline uint16_t vco_cv_scale_high() { return data_.vco_cv_scale_high; }

  static void ChangePpqn() {
    ++data_.clock_ppqn;
    if (data_.clock_ppqn > 2) {
      data_.clock_ppqn = 0;
    }
    storage.Save(data_);
  }
  
  static void set_calibration_data(uint16_t a, uint16_t b, uint16_t c) {
    data_.vco_cv_offset = a;
    data_.vco_cv_scale_low = b;
    data_.vco_cv_scale_high = c;
    storage.Save(data_);
  }
  
  static void set_midi_channel(uint8_t channel, uint8_t note) {
    data_.reference_note = note;
    data_.midi_channel = channel;
    storage.Save(data_);
  }
  
  static void set_midi_out_mode(uint8_t mode) {
    data_.midi_out_mode = mode;
    storage.Save(data_);
  }
  
  static void set_midi_channel(uint8_t channel) {
    data_.midi_channel = channel;
    storage.Save(data_);
  }
  
  static SystemSettingsData* mutable_data() {
    return &data_;
  }
  
  static void Save() {
    storage.Save(data_);
  }

 private:
  static SystemSettingsData data_;
};

extern SystemSettings system_settings;

};  // namespace anu

#endif  // ANU_SYSTEM_SETTINGS_H_
