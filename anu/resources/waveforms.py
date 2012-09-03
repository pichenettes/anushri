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
# Custom LFO waveshapes.

import numpy

def scale(x, min=1, max=254, center=True, dithering=0):
  mx = x.max()
  mn = x.min()
  x = (x - mn) / (mx - mn)
  x = numpy.round(x * (max - min) + min)
  target_type = numpy.uint8
  x[x < numpy.iinfo(target_type).min] = numpy.iinfo(target_type).min
  x[x > numpy.iinfo(target_type).max] = numpy.iinfo(target_type).max
  return x.astype(target_type)


ramp_up = numpy.linspace(0, 1.0, 124) ** 3.2
deadband = numpy.hstack((-ramp_up[::-1], [0] * 8, ramp_up))
waveforms = [('deadband', scale(deadband, 0, 255))]


pitch_up = numpy.linspace(0, 12.0, 40)
pitch_up = numpy.hstack(([0] * 20, pitch_up))
pitch_up = numpy.hstack((
    pitch_up - 12,
    pitch_up,
    pitch_up + 12,
    pitch_up + 24,
    pitch_up + 36))[:256]
waveforms.append(('pitch_deadband', pitch_up))

expo_decay = numpy.linspace(0, 1.0, 257)
expo_decay = numpy.exp(-1.75 * expo_decay)
waveforms.append(('drm_envelope', scale(expo_decay, min=0, max=255)))

# Sine wave.
sine = -numpy.sin(numpy.arange(257) / float(257) * 2 * numpy.pi) * 127.5 + 127.5
waveforms.append(('sine', scale(sine) + 128))

# HH
hh = map(ord, file('anu/resources/hh_linn.raw').read())
hh = hh[:4097]
waveforms.append(('hh', hh))

# DrumMap nodes
nodes = [[236, 0, 0, 138, 0, 0, 208, 0, 58, 28, 174, 0, 104, 0, 58, 0, 10, 66, 0, 8, 232, 0, 0, 38, 0, 148, 0, 14, 198, 0, 114, 0, 154, 98, 244, 34, 160, 108, 192, 24, 160, 98, 228, 20, 160, 92, 194, 44],
[246, 10, 88, 14, 214, 10, 62, 8, 250, 8, 40, 14, 198, 14, 160, 120, 16, 186, 44, 52, 230, 12, 116, 18, 22, 154, 10, 18, 246, 88, 72, 58, 136, 130, 220, 64, 130, 120, 156, 32, 128, 112, 220, 32, 126, 106, 184, 88],
[224, 0, 98, 0, 0, 68, 0, 198, 0, 136, 174, 0, 46, 28, 116, 12, 0, 94, 0, 0, 224, 160, 20, 34, 0, 52, 0, 0, 194, 0, 16, 118, 228, 104, 138, 90, 122, 102, 108, 76, 196, 160, 182, 160, 96, 36, 202, 22],
[240, 204, 42, 0, 86, 108, 66, 104, 190, 22, 224, 0, 14, 148, 0, 36, 0, 0, 112, 62, 232, 180, 0, 34, 0, 48, 26, 18, 214, 18, 138, 38, 232, 186, 224, 182, 108, 60, 80, 62, 142, 42, 24, 34, 136, 14, 170, 26],
[228, 14, 36, 24, 74, 54, 122, 26, 186, 14, 96, 34, 18, 30, 48, 12, 2, 0, 46, 38, 226, 0, 68, 0, 2, 0, 92, 30, 232, 166, 116, 22, 64, 12, 236, 128, 160, 30, 202, 74, 68, 28, 228, 120, 160, 28, 188, 82],
[236, 24, 14, 54, 0, 0, 106, 0, 202, 220, 0, 178, 0, 160, 140, 8, 134, 82, 114, 160, 224, 0, 22, 44, 66, 40, 0, 0, 192, 22, 14, 158, 174, 86, 230, 58, 124, 64, 210, 58, 160, 76, 224, 22, 124, 34, 194, 26],
[236, 0, 226, 0, 0, 0, 160, 0, 0, 0, 188, 0, 0, 0, 210, 0, 26, 188, 0, 62, 242, 102, 8, 160, 22, 216, 0, 48, 200, 112, 30, 22, 230, 212, 222, 228, 180, 14, 114, 32, 160, 38, 66, 12, 154, 22, 88, 36],
[226, 0, 42, 0, 66, 0, 226, 14, 238, 0, 126, 0, 84, 10, 170, 22, 0, 0, 54, 0, 182, 0, 128, 36, 6, 10, 84, 10, 238, 8, 158, 26, 240, 46, 218, 24, 232, 0, 96, 0, 240, 28, 204, 30, 214, 0, 64, 0],
[228, 0, 212, 0, 14, 0, 214, 0, 160, 52, 218, 0, 0, 0, 134, 32, 104, 0, 22, 84, 230, 22, 0, 58, 6, 0, 138, 20, 220, 18, 176, 34, 230, 26, 52, 24, 82, 28, 52, 118, 154, 26, 52, 24, 202, 212, 186, 196]]

for i, p in enumerate(nodes):
  waveforms.append(('drum_map_node_%d' % i, p))
