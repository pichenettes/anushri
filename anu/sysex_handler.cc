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

#include "anu/sysex_handler.h"

#include "anu/midi_dispatcher.h"
#include "anu/storage.h"
#include "anu/system_settings.h"
#include "anu/voice_controller.h"

namespace anu {

/* static */
uint8_t SysExHandler::rx_buffer_[129];

/* static */
uint8_t* SysExHandler::rx_destination_;

/* static */
uint16_t SysExHandler::rx_bytes_received_;

/* static */
uint16_t SysExHandler::rx_expected_size_;

/* static */
SysExReceptionState SysExHandler::rx_state_;

/* static */
uint8_t SysExHandler::rx_checksum_;

/* static */
uint8_t SysExHandler::rx_command_[2];

static const prog_uint8_t header[] PROGMEM = {
  0xf0,  // <SysEx>
  0x00, 0x21, 0x02,  // Mutable Instruments manufacturer ID.
  0x00, 0x08,  // Product ID for Anushri.
  // Then:
  // * Command byte:
  // - 0x01: Data structure dump
  // * Argument byte:
  // - 0x00: System Settings
  // - 0x01: Patch
  // - 0x02: SequencerSettings
  // - 0x03: Sequence (first block of 128 bytes)
  // - 0x04: Sequence (second block of remaining bytes)
};

static const prog_uint8_t block_sizes[] PROGMEM = {
  sizeof(SystemSettingsData),
  sizeof(Patch),
  sizeof(SequencerSettings),
  128,
  sizeof(Sequence) - 128
};

/* static */
void* SysExHandler::GetObjectAddress(SysExObjectType type) {
  uint8_t* p;
  switch (type) {
    case SYSEX_OBJECT_TYPE_SYSTEM_SETTINGS:
      return system_settings.mutable_data();
      
    case SYSEX_OBJECT_TYPE_PATCH:
      return voice_controller.mutable_voice()->mutable_patch();
    
    case SYSEX_OBJECT_TYPE_SEQUENCER_SETTINGS:
      return voice_controller.mutable_sequencer_settings();
    
    case SYSEX_OBJECT_TYPE_SEQUENCE_BLOCK_1:
      return voice_controller.mutable_sequence();
      
    case SYSEX_OBJECT_TYPE_SEQUENCE_BLOCK_2:
      return static_cast<uint8_t*>(
          static_cast<void*>(voice_controller.mutable_sequence())) + 128;
  }
}

/* static */
uint8_t SysExHandler::GetObjectSize(SysExObjectType type) {
  return pgm_read_byte(block_sizes + static_cast<uint8_t>(type));
}

/* static */
void SysExHandler::ParseCommand() {
  rx_bytes_received_ = 0;
  rx_state_ = RECEIVING_DATA;
  rx_destination_ = rx_buffer_;
  switch (rx_command_[0]) {
    case 0x01:  // Data structure transfer
      {
        SysExObjectType type = static_cast<SysExObjectType>(rx_command_[1]);
        rx_expected_size_ = GetObjectSize(type);
      }
      break;
    
    case 0x11:  // Data structure dump request
      rx_expected_size_ = 0;
      break;

    default:
      rx_state_ = RECEIVING_FOOTER;
      break;
  }
}

/* static */
void SysExHandler::BulkDump() {
  for (uint8_t object = 0; object < SYSEX_OBJECT_TYPE_LAST; ++object) {
    SysExObjectType type = static_cast<SysExObjectType>(object);
    const uint8_t* data = static_cast<uint8_t*>(GetObjectAddress(type));
    uint8_t size = GetObjectSize(type);
    
    // Header.
    for (uint8_t i = 0; i < sizeof(header); ++i) {
      midi_dispatcher.SendBlocking(pgm_read_byte(header + i));
    }
    
    // Command and argument.
    midi_dispatcher.SendBlocking(0x01);
    midi_dispatcher.SendBlocking(object);
    
    // Outputs the data.
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < size; ++i) {
      checksum += data[i];
      midi_dispatcher.SendBlocking(U8ShiftRight4(data[i]));
      midi_dispatcher.SendBlocking(data[i] & 0x0f);
    }
    // Outputs a checksum.
    midi_dispatcher.SendBlocking(U8ShiftRight4(checksum));
    midi_dispatcher.SendBlocking(checksum & 0x0f);

    // End of SysEx block.
    midi_dispatcher.SendBlocking(0xf7);
  }
}

/* static */
void SysExHandler::AcceptBuffer() {
  switch (rx_command_[0]) {
    case 0x01:  // Transfer
      {
        SysExObjectType type = static_cast<SysExObjectType>(rx_command_[1]);
        void* address = GetObjectAddress(type);
        uint8_t size = GetObjectSize(type);
        memcpy(address, rx_buffer_, size);
        if (type == SYSEX_OBJECT_TYPE_SEQUENCE_BLOCK_2) {
          voice_controller.SaveSequence();
        } else if (type == SYSEX_OBJECT_TYPE_SYSTEM_SETTINGS) {
          system_settings.Save();
        }
        voice_controller.Touch();
      };
      break;
    case 0x11:  // Request
      BulkDump();
      break;
  }
}

/* static */
void SysExHandler::Receive(uint8_t rx_byte) {
  if (rx_byte == 0xf0) {
    rx_checksum_ = 0;
    rx_bytes_received_ = 0;
    rx_state_ = RECEIVING_HEADER;
  }
  switch (rx_state_) {
    case RECEIVING_HEADER:
      if (pgm_read_byte(header + rx_bytes_received_) == \
          rx_byte) {
        rx_bytes_received_++;
        if (rx_bytes_received_ >= sizeof(header)) {
          rx_state_ = RECEIVING_COMMAND;
          rx_bytes_received_ = 0;
        }
      } else {
        rx_state_ = RECEIVING_FOOTER;
      }
      break;

    case RECEIVING_COMMAND:
      rx_command_[rx_bytes_received_++] = rx_byte;
      if (rx_bytes_received_ == 2) {
        ParseCommand();
      }
      break;

    case RECEIVING_DATA:
      {
        uint16_t i = rx_bytes_received_ >> 1;
        if (rx_bytes_received_ & 1) {
          rx_destination_[i] |= rx_byte & 0xf;
          if (i < rx_expected_size_) {
            rx_checksum_ += rx_destination_[i];
          }
        } else {
          rx_destination_[i] = U8ShiftLeft4(rx_byte);
        }
        rx_bytes_received_++;
        if (rx_bytes_received_ >= (1 + rx_expected_size_) * 2) {
          rx_state_ = RECEIVING_FOOTER;
        }
      }
    break;

  case RECEIVING_FOOTER:
    if (rx_byte == 0xf7 &&
        rx_checksum_ == rx_destination_[rx_expected_size_]) {
      AcceptBuffer();
    } else {
      rx_state_ = RECEPTION_ERROR;
    }
    break;
  }
}

};  // namespace anu
