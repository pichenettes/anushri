import math

scale = 21769
bias = 7510

def midi_to_hz(midi):
  return 440 * 2 ** ((midi - 69) / 12.0)

def midi_pitch_to_cv(x):
  return ((x - bias) * scale / 65536 + 2048) / 1000.0

def cv_to_pitch(cv):
  return 15 * 2 ** (2 * cv * 0.98)

print midi_to_hz(60)

target_c1 = midi_to_hz(36)

fc1 = cv_to_pitch(midi_pitch_to_cv(36 * 128))
fc3 = cv_to_pitch(midi_pitch_to_cv(60 * 128))
fc5 = cv_to_pitch(midi_pitch_to_cv(84 * 128))

c1_pitch = cv_to_pitch(1.048)
c5_pitch = cv_to_pitch(3.048)

scale_constant = 256000.0 * math.log(2) / 3
offset_constant_a = 128 * 12 * 0.5 / math.log(2)
offset_constant_b = midi_to_hz(60) * midi_to_hz(60)

estimated_scale = scale_constant / (math.log(c5_pitch / c1_pitch))
estimated_bias = 60 * 128 + offset_constant_a * math.log(c5_pitch * c1_pitch / offset_constant_b)

print offset_constant_a, offset_constant_b

# def v2hz(v):
#   return 57.6 * 2 ** ((v - 1048) / 469.6)
# 
# scale = 64000.0 / 3
# offset = 60 * 128
# 
# print midi_to_hz(60)
# 
# print (36 * 128 - offset) * scale / 65536 + 2048, v2hz(1048)
# print (60 * 128 - offset) * scale / 65536 + 2048, v2hz(2048)
# 
# pitch_c1 = 57.6
# pitch_c3 = 252.0
# 
# scale = 29574.28 / math.log(pitch_c3 / pitch_c1)
# offset = 60 * 128 + 128 * 12 / math.log(2.0) * math.log(pitch_c3 / midi_to_hz(60))
# 
# print v2hz((36 * 128 - offset) * scale / 65536 + 2048)
# print v2hz((60 * 128 - offset) * scale / 65536 + 2048)
