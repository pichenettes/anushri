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

#ifndef ANU_HARDWARE_CONFIG_H_
#define ANU_HARDWARE_CONFIG_H_

#include "avrlib/base.h"
#include "avrlib/devices/shift_register.h"
#include "avrlib/devices/switch.h"
#include "avrlib/gpio.h"
#include "avrlib/parallel_io.h"
#include "avrlib/serial.h"
#include "avrlib/spi.h"
#include "avrlib/timer.h"

namespace anu {

using avrlib::DebouncedSwitches;
using avrlib::Gpio;
using avrlib::MSB_FIRST;
using avrlib::PARALLEL_NIBBLE_HIGH;
using avrlib::PARALLEL_TRIPLE_LOW;
using avrlib::ParallelPort;
using avrlib::PortB;
using avrlib::PortC;
using avrlib::PortD;
using avrlib::PwmChannel1B;
using avrlib::RingBuffer;
using avrlib::SerialInput;
using avrlib::SerialPort0;
using avrlib::ShiftRegisterOutput;
using avrlib::SpiMaster;
using avrlib::SpiSS;
using avrlib::Timer;

// MIDI
typedef avrlib::Serial<
    SerialPort0,
    31250,
    avrlib::POLLED,
    avrlib::POLLED> MidiIO;
typedef RingBuffer<SerialInput<SerialPort0> > MidiBuffer;

// IO
typedef Gpio<PortD, 6> IOClockLine;
typedef Gpio<PortD, 5> IOInputLine;
typedef Gpio<PortD, 4> IOOutputLine;
typedef Gpio<PortD, 2> IOEnableLine;
typedef DebouncedSwitches<IOEnableLine, IOClockLine, IOInputLine, 8> Inputs;
typedef ShiftRegisterOutput<
    IOEnableLine,
    IOClockLine,
    IOOutputLine,
    8,
    MSB_FIRST> Outputs;

typedef Gpio<PortB, 0> PitchCalibrationInput;
typedef Gpio<PortB, 2> DcoOut;
typedef PwmChannel1B Dco;

// DAC
typedef Gpio<PortB, 1> DAC1SS;
typedef Gpio<PortD, 7> DAC2SS;
typedef SpiMaster<SpiSS, MSB_FIRST, 2> DACInterface;

// ADC
typedef Gpio<PortC, 3> AdcMuxBank1SS;
typedef Gpio<PortC, 4> AdcMuxBank2SS;
typedef ParallelPort<PortC, PARALLEL_TRIPLE_LOW> AdcMuxAddress;

// Timer used for tuning
typedef Timer<1> TuningTimer;

const uint8_t kAdcInputMux = 5;

}  // namespace anu

#endif  // ANU_HARDWARE_CONFIG_H_
