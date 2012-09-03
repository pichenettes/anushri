import numpy

k = 8.6173324e-5
temperature = 273.13 + 30

def ssm2164gain(cv):
  return numpy.exp(- cv / 60.0 / (temperature * k))


def svf_resonance_gain(value):
  cv = value * 5.0
  return ssm2164gain(cv)


def anu_resonance_gain(value):
  return 0.5 / (0.5 + value * value * 10.0)
  

print svf_resonance_gain(0.0), anu_resonance_gain(0.0)
print svf_resonance_gain(0.5), anu_resonance_gain(0.5)
print svf_resonance_gain(1.0), anu_resonance_gain(1.0)
