import time
import serial
import numpy as np
from io import StringIO
import matplotlib.pyplot as plt

import csv

ser = serial.Serial(
    port='/COM3',
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS
)

# ser.open()
if not ser.is_open:
  ser.open()

init = ser.read(ser.in_waiting).decode()
print(init)

ser.write(b'r') # start sampeling
time.sleep(0.5) # wait to finish
ser.write(b'd') # start data dump

time.sleep(0.1)
print('loading....')
data = ''
# get data
while ser.in_waiting:
  data += ser.read(ser.in_waiting).decode()
  if ser.in_waiting <= 0:
    time.sleep(0.001)

ser.close()

print(f'data: {len(data)}')
f = open(f'csv_data/{time.strftime("%Y%m%d-%H%M%S")}.csv', 'w')
f.write(data.replace('\r\n', '\n'))
f.close()

# time.sleep(2)
# print(data)

# exit()
data = np.genfromtxt(StringIO(data), delimiter=',')
# data = np.genfromtxt('test2.txt', delimiter=',')


sampling_rate = 80000
sampels_per_ms = sampling_rate / 1000 
ms = 5

N_channels = 4

# Separate the columns
channels = []

for i in range(0, N_channels):
  channels.append(data[:, i])
  channels[i] = channels[i] - np.mean(channels[i]) # Remove DC offset


# Plot the spectra
plt.figure(figsize=(12, 8))

for i in range(0, N_channels):
  plt.subplot(N_channels, 1, i+1)
  plt.plot(channels[i][:int(sampels_per_ms * ms)])
  plt.title(f'Channel {i} raw signal')
  plt.xlabel(f'Time ({ms} ms)')
  plt.ylabel('Amplitude')


plt.tight_layout()
plt.show()