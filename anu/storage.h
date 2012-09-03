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

#ifndef ANU_STORAGE_H_
#define ANU_STORAGE_H_

#include "avrlib/base.h"

#include <avr/pgmspace.h>
#include <avr/eeprom.h>

namespace anu {

template<typename T>
struct StorageLayout {
  // Crash-guard: these templates must be specialized.
  static uint8_t* eeprom_address() { while(1); }
  static const prog_char* init_data() { while(1); }
};

class Storage {
 public:
  template<typename T>
  static void Save(const T& data) {
    Save(&data, StorageLayout<T>::eeprom_address(), sizeof(T));
  };
  
  template<typename T>
  static void Load(T* data) {
    Load(
        data,
        StorageLayout<T>::eeprom_address(),
        static_cast<uint16_t>(sizeof(T)),
        StorageLayout<T>::init_data(),
        false);
  };

  template<typename T>
  static void ResetToFactoryDefaults(T* data) {
    Load(
        data,
        StorageLayout<T>::eeprom_address(),
        static_cast<uint16_t>(sizeof(T)),
        StorageLayout<T>::init_data(),
        true);
    Save(*data);
  };

  static void Save(const void* data, uint8_t* address, uint16_t size) {
    uint8_t checksum = Checksum(data, size);
    eeprom_write_block(data, address, size);
    eeprom_write_byte(address + size, checksum);
  };

  static void Load(
      void* data,
      uint8_t* address,
      uint16_t size,
      const prog_char* default_data,
      bool force_reinitialization) {
    eeprom_read_block(data, address, size);
    uint16_t checksum = eeprom_read_byte(address + size);
    if (checksum != Checksum(data, size) || force_reinitialization) {
      memcpy_P(data, default_data, size);
    }
  };
  
 private:
  static uint8_t Checksum(const void* data, uint16_t size) {
    uint8_t s = 0;
    const uint8_t* d = static_cast<const uint8_t*>(data);
    while (size--) {
      s += *d++;
    }
    return s;
  }
};

extern Storage storage;

};  // namespace anu

#endif  // ANU_STORAGE_H_
