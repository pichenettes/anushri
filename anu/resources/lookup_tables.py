#!/usr/bin/python2.5
#
# Copyright 2012 Olivier Gillet.
#
# Author: Olivier Gillet (ol.gillet@gmail.com)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# -----------------------------------------------------------------------------
#
# Lookup table definitions.

import numpy

"""----------------------------------------------------------------------------
LFO and envelope increments.
----------------------------------------------------------------------------"""

lookup_tables = []
lookup_tables_32 = []

control_rate = 20000000 / 510 / 8.0 / 2
min_frequency = 1.0 / 16.0  # Hertz
max_frequency = 100.0  # Hertz

num_values = 256
min_increment = 65536 * 65536.0 * min_frequency / control_rate
max_increment = 65536 * 65536.0 * max_frequency / control_rate

rates = numpy.linspace(numpy.log(min_increment),
                       numpy.log(max_increment), num_values)
lfo_increments = numpy.exp(rates).astype(int)
lfo_increments[0:2] = 0
lookup_tables_32.append(
    ('lfo_increments', lfo_increments)
)


# Create lookup table for envelope times.
max_time = 12.0  # seconds
min_time = 3.0 / control_rate  # seconds
gamma = 0.175
min_increment = 65536 * 65536.0 / (max_time * control_rate)
max_increment = 65536 * 65536.0 / (min_time * control_rate)

rates = numpy.linspace(numpy.power(max_increment, -gamma),
                       numpy.power(min_increment, -gamma), num_values)

values = numpy.power(rates, -1 / gamma).astype(int)
lookup_tables_32.append(
    ('env_increments', values)
)


# Create lookup table for glide times.
max_time = 6.0
min_time = 3.0 / control_rate
gamma = 0.1
min_increment = 65536 / (max_time * control_rate)
max_increment = 65536 / (min_time * control_rate)

rates = numpy.linspace(numpy.power(max_increment, -gamma),
                       numpy.power(min_increment, -gamma), num_values)

values = numpy.power(rates, -1 / gamma).astype(int)
lookup_tables.append(
    ('glide_increments', values)
)

# Create lookup table for drum envelope times
control_rate = 20000000 / 510 / 32.0

max_time = 4.0  # seconds
min_time = 4.0 / control_rate
gamma = 0.25
min_increment = 65536 / (max_time * control_rate)
max_increment = 65536 / (min_time * control_rate)

rates = numpy.linspace(numpy.power(max_increment, -gamma),
                       numpy.power(min_increment, -gamma), num_values)

values = numpy.power(rates, -1 / gamma).astype(int)
lookup_tables.append(
    ('drm_env_increments', values)
)

# Create pitch increment table for drum synth
sample_rate = 20000000 / 510
frequency = 55 * 2 ** ((numpy.arange(257.0) - 69) / 24)
lookup_tables.append(
    ('drm_phase_increments', 65536 * frequency / sample_rate)
)



"""----------------------------------------------------------------------------
DCO pitch table.
----------------------------------------------------------------------------"""

notes = numpy.arange(0, 12 * 128 + 16, 16) / 128.0 + 16
frequencies = 2 ** ((notes - 69) / 12) * 440.0
values = 20000000.0 / (2 * 8 * frequencies)
lookup_tables.append(
    ('dco_pitch', numpy.round(values))
)

"""----------------------------------------------------------------------------
Envelope curves
-----------------------------------------------------------------------------"""

env_linear = numpy.arange(0, 257.0) / 256.0
env_linear[-1] = env_linear[-2]
env_quartic = 1.0 - (1.0 - env_linear) ** 2.0
env_expo = 1.0 - numpy.exp(-4 * env_linear)

# lookup_tables.append(('env_linear', env_linear / env_linear.max() * 65535.0))
# lookup_tables.append(('env_quartic', env_quartic / env_quartic.max() * 65535.0))
lookup_tables.append(('env_expo', env_expo / env_expo.max() * 65535.0))

"""----------------------------------------------------------------------------
Groove templates
----------------------------------------------------------------------------"""

def ConvertGrooveTemplate(values):
  # Center
  values = numpy.array(values)
  values -= values.mean()
  # Scale
  scale = numpy.abs(values).max() / 127.0
  values /= scale
  values = values.astype(int)
  # Fix rounding error
  values[8] -= values.sum()
  return values

lookup_tables.extend([
    ('groove_swing', ConvertGrooveTemplate(
      [1, 1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1])),
    ('groove_shuffle', ConvertGrooveTemplate(
      [1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1])),
    ('groove_push', ConvertGrooveTemplate(
      [-0.5, -0.5, 1, 0, -1, 0, 0, 0.7, 0, 0, 0.7, -0.4, -0.7, 0, 0.7, 0.0])),
    ('groove_lag', ConvertGrooveTemplate(
      [0.1, 0.2, 0.4, 0, 0.15, -0.2, -0.35, -0.5,
       0.5, 0.15, -0.4, -0.2, 0.45, -0.2, 0.4, -0.2])),
    ('groove_human', ConvertGrooveTemplate(
      [0.7, -0.8, 0.85, -0.75, 0.7, -0.7,  0.4, -0.3,
       0.5, -0.7, 0.8, -0.75, 0.8, -1, 0.5, -0.25])),
    ('groove_monkey', ConvertGrooveTemplate(
      [0.5, -0.6, 0.6, -0.8, 0.6, -0.7, 0.8, -0.7,
       0.4, -0.5, 0.9, -0.6, 0.9, -0.8, 0.6, -0.6]))])

"""----------------------------------------------------------------------------
Arpeggiator patterns
----------------------------------------------------------------------------"""

def XoxTo16BitInt(pattern):
  uint16 = 0
  i = 0
  for char in pattern:
    if char == 'o':
      uint16 += (2 ** i)
      i += 1
    elif char == '-':
      i += 1
  assert i == 16
  return uint16


def ConvertPatterns(patterns):
  return [XoxTo16BitInt(pattern) for pattern in patterns]


lookup_tables.append(
  ('arpeggiator_patterns', ConvertPatterns([
      'o-o- o-o- o-o- o-o-',
      'o-o- oooo o-o- oooo',
      'ooo- ooo- ooo- ooo-',
      'o--o --o- -o-- o-o-',
      'oo-o -oo- oo-o -oo-',
      'oooo -oo- oooo -oo-'])))