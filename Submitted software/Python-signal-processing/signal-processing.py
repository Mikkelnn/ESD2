from scipy.signal import butter, lfilter
from PIL import Image
import numpy as np
import math
import io
import getData # get data from MCU
import visual # visualize data


# Calculate the frequencies
sampling_rate = 80000  # Assuming unit sampling rate, modify if needed
sampel_period_us = (1.0 / sampling_rate) * math.pow(10, 6)
sampels_per_ms = sampling_rate / 1000
sound_speed_cm_per_s = 34300 # speed of sound in cm/s
mic_dist_cm = 7.5 # distance between microphones in cm
pulse_width_ms = 1
pulse_width_sampels = int(pulse_width_ms * sampels_per_ms)

low = 38200 # low = 38200
high = 38700
order = 5

# simulates the sent signal
signal_block = []
for i in range (0, int(pulse_width_ms * sampels_per_ms)):
  signal_block.append(255)

# idx 0: -90deg, idx 179: +90deg
angleLookup = []
for i in range(-90, 91):
   distance = mic_dist_cm * math.sin(np.deg2rad(i))
   delay_us = (distance / sound_speed_cm_per_s) * math.pow(10, 6)
   angleLookup.append(delay_us)

def angleFromShift(shift):
  delay_us = shift * sampel_period_us
  sidx = None
  smallest = None
  for idx, val in enumerate(angleLookup):
    current = math.pow(val-delay_us, 2)
    if (smallest == None or current < smallest):
      smallest = current
      sidx = idx
  # 90 is subtracted to denote zero degrees is perpendicular to the microphone array
  return sidx - 90

def butter_bandpass(lowcut, highcut, fs, order=5):
    nyq = 0.5 * fs
    low = lowcut / nyq
    high = highcut / nyq
    b, a = butter(order, [low, high], btype='band')
    return b, a

def butter_bandpass_filter(data, lowcut, highcut, fs, order=5):
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = lfilter(b, a, data)
    return y


def butter_highpass(cut, fs, order=5):
    nyq = 0.5 * fs
    ny_cut = cut / nyq
    b, a = butter(order, [ny_cut], btype='highpass')
    return b, a

def butter_highpass_filter(data, cut, fs, order=5):
    b, a = butter_highpass(cut, fs, order=order)
    y = lfilter(b, a, data)
    return y

def calculate(csv_data):
  # Load data from the CSV file
  data = np.genfromtxt(io.StringIO(csv_data), delimiter=',')

  # get number of channels from data
  N_channels = data.shape[1]

  # Separate the columns
  channels = []
  filtered = []
  for i in range(0, N_channels):
    channels.append(data[pulse_width_sampels:, i]) # remove pulse at start
    channels[i] = channels[i] - np.mean(channels[i]) # Remove DC offset
    filtered.append(butter_bandpass_filter(channels[i], low, high, sampling_rate, order))

  # Find channels with data - i.e non zeros
  used_channels = []
  for i in range(0, N_channels):
    if np.sum(np.abs(channels[i])) > 0:
      used_channels.append(i)

  N_used_channels = len(used_channels)
  print(f'channels used: {used_channels}')
  if (N_used_channels < 2):
    print('only one channel! - can´t cross correlate.....')
    return

  # determine the first occurence of the pulse recieved  
  blockShift_0 = np.correlate(filtered[used_channels[0]], signal_block, mode='valid').argmax()
  blockShift_1 = np.correlate(filtered[used_channels[1]], signal_block, mode='valid').argmax()
  signalDelay = blockShift_0 if blockShift_0 < blockShift_1 else blockShift_1
  # calculate the distance from the microphone array to the object
  distance_cm = (signalDelay + pulse_width_sampels) * sampel_period_us * 0.000001 * sound_speed_cm_per_s * 0.5

  # detrmine the max and min index for correlation
  max_sampels_between_mics = math.ceil(((mic_dist_cm / sound_speed_cm_per_s) * 1000000) / sampel_period_us)
  margin = (2 * max_sampels_between_mics)
  max_correlation_sampel = min(len(used_channels[0]) - 1, int(signalDelay + pulse_width_sampels + margin))
  min_correlation_sampel = max(0, int(signalDelay - margin))

  # Calculate cross correlation
  x_section = filtered[used_channels[1]][min_correlation_sampel:max_correlation_sampel]
  y_section = filtered[used_channels[0]][min_correlation_sampel:max_correlation_sampel]
  max_correlation_idx = np.correlate(x_section, y_section, mode='same').argmax()
  relative_shift = max_correlation_idx - int((max_correlation_sampel-min_correlation_sampel-1) * 0.5)

  print(f'min_correlation_sampel: {min_correlation_sampel}, max_correlation_sampel: {max_correlation_sampel}')
  print('filteredCrossCorrelation (same):')
  print(f'idx shift: {relative_shift}, delay (us): {relative_shift * sampel_period_us}, AoA: {angleFromShift(relative_shift)}')
  print(f'estimated distance (cm): {distance_cm}')

  # draw visualization of data
  illustration = visual.drawObj(angleFromShift(relative_shift), distance_cm, mic_dist_cm, N_used_channels)
  ps = illustration.postscript(colormode = 'color')
  illu_im = Image.open(io.BytesIO(ps.encode('utf-8')))
  illu_im.show()
  illu_im.close()

csv = getData.get_data()
calculate(csv)