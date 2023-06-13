import serial
import easygui
import numpy as np
import correlation
import time

ser = serial.Serial(
    port='/COM3',
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS
)


# select csv file
file_path = easygui.fileopenbox()
# file_path = "T:\Repoes\AAU\ESD2\Project\FFT_python\lyd_lab\\20230509-105904-(human side center 2m).csv"
data = np.genfromtxt(file_path, delimiter=',')

# get number of channels from data
N_channels = data.shape[1]

if not ser.is_open:
  ser.open()

time.sleep(1)
print("writing....")

ser.write(b'l') # start loading data
for s in data:
  for ch in s:
    ser.write(bytes([int(ch)]))

print("done")
time.sleep(1)
ser.close()

correlation.calculate(file_path)


