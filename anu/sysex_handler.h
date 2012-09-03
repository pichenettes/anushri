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

#ifndef ANU_SYSEX_HANDLER_H_
#define ANU_SYSEX_HANDLER_H_

#include "avrlib/base.h"

namespace anu {
  
enum SysExReceptionState {
  RECEIVING_HEADER,
  RECEIVING_COMMAND,
  RECEIVING_DATA,
  RECEIVING_FOOTER,
  RECEPTION_OK,
  RECEPTION_ERROR,
};

enum SysExObjectType {
  SYSEX_OBJECT_TYPE_SYSTEM_SETTINGS,
  SYSEX_OBJECT_TYPE_PATCH,
  SYSEX_OBJECT_TYPE_SEQUENCER_SETTINGS,
  SYSEX_OBJECT_TYPE_SEQUENCE_BLOCK_1,
  SYSEX_OBJECT_TYPE_SEQUENCE_BLOCK_2,
  SYSEX_OBJECT_TYPE_LAST
};

class SysExHandler {
 public:
  static void BulkDump();
  static void Receive(uint8_t sysex_rx_byte);
  
 private:
  static void ParseCommand();
  static void AcceptBuffer();

  static void* GetObjectAddress(SysExObjectType type);
  static uint8_t GetObjectSize(SysExObjectType type);
  
  static uint8_t rx_buffer_[129];
  static uint8_t* rx_destination_;
  static uint16_t rx_bytes_received_;
  static uint16_t rx_expected_size_;
  static SysExReceptionState rx_state_;
  static uint8_t rx_checksum_;
  static uint8_t rx_command_[2];
  
  DISALLOW_COPY_AND_ASSIGN(SysExHandler);
};

extern SysExHandler sysex_handler;

};  // namespace anu

#endif  // ANU_SYSEX_HANDLER_H_
