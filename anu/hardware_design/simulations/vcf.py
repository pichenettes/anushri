import numpy

k = 8.6173324e-5
temperature = 273.13 + 27

def ssm2164gain(cv):
  return numpy.exp(- cv / 10.4 / (temperature * k))

r_fb = 37300
r_ref = 62000
r_input = 200000
r_pot = 200000
r_dac = 49900

r_vcf = 33000.0
c_vcf = 220.0 * 10 ** (-12)


def f_vcf(cv_dac, cv_input, cv_pot):
  i1 = cv_dac / r_dac + cv_input / r_input + cv_pot / r_pot - 4.096 / r_ref
  v1 = -i1 * r_fb
  print v1
  f_max = 1 / (2 * numpy.pi * r_vcf * c_vcf)
  return f_max * ssm2164gain(v1)


print f_vcf(2.048 + 0.0, 0.0, -5.0)
print f_vcf(2.048 + 0.0, 0.0, 5.0)