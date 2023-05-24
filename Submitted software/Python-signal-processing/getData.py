import time
import serial

def get_data():
  ser = serial.Serial(
      port='/COM3',
      baudrate=115200,
      parity=serial.PARITY_NONE,
      stopbits=serial.STOPBITS_ONE,
      bytesize=serial.EIGHTBITS
  )

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

  return data