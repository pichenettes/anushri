import numpy
# import pylab

k = 8.6173324e-5
R_tempco = 68400
R_in = 4320
R_div = 460.0
V_ref = 4.110
ratio = R_div / (R_in + R_div)
V_tempco = R_div / (R_div + R_in + R_tempco) * V_ref / (ratio)

def ssm2164gain(cv, temperature):
  return numpy.exp(- cv / 10.4 / (temperature * k))

def hz2midi(hz):
  return 69 + 12 * numpy.log2(hz / 440.0)


r_dac = 100000
r_shift = 72133
r_input = 200000
r_fb = 101733
r_vco = 22000.0
c_vco = 2.2e-9
room_temp = 273.13 + 25

def f_vco(cv_dac, cv_input, temperature):
  i1 = cv_dac / r_dac + cv_input / r_input - 4.096 / r_shift
  v1 = -ssm2164gain(V_tempco, temperature) * r_fb * i1
  f_max = 2.0 / (r_vco * c_vco)
  return f_max * ssm2164gain(v1, temperature)

print f_vco(2.048 + 0.5, 0, room_temp) / f_vco(2.048, 0, room_temp)
print 'Pitch at C3', hz2midi(f_vco(2.048, 0, room_temp))
