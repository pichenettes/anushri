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
// Instance of the audio buffer class.

#ifndef ANU_AUDIO_BUFFER_H_
#define ANU_AUDIO_BUFFER_H_

#include "avrlib/ring_buffer.h"

namespace anu {

struct AudioBufferSpecs {
  typedef uint8_t Value;
  enum {
    buffer_size = 128,
    data_size = 8,
  };
};

static const uint8_t kAudioBlockSize = 32;

extern avrlib::RingBuffer<AudioBufferSpecs> audio_buffer;

}  // namespace anu

#endif  // ANU_AUDIO_BUFFER_H_
