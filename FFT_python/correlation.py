import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, lfilter
import easygui
import math

# visualize data
from threading import Thread
import visual

# Calculate the frequencies
sampling_rate = 80000  # Assuming unit sampling rate, modify if needed
sampel_period_us = (1.0 / sampling_rate) * math.pow(10, 6)
sampels_per_ms = sampling_rate / 1000
sound_speed_cm_per_s = 34300 # speed of sound in cm/s
mic_dist_cm = 7.5 # distance between microphones in cm
pulse_width_ms = 1

low = 38200
high = 38700
order = 5
ms = 200 # number of milliseconds of data to display 

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
  return sidx - 90, smallest

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



def calculate(file_path):
  # Load data from the CSV file
  data = np.genfromtxt(file_path, delimiter=',')

  # get number of channels from data
  N_channels = data.shape[1]


  # Separate the columns
  channels = []
  filtered = []
  for i in range(0, N_channels):
    channels.append(data[int(pulse_width_ms * sampels_per_ms):, i]) # remove pulse at start
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

  # Calculate cross correlation
  filteredCrossCorrelation_full = np.correlate(filtered[used_channels[1]], filtered[used_channels[0]], mode='full')
  filteredCrossCorrelation_same = np.correlate(filtered[used_channels[1]], filtered[used_channels[0]], mode='same')
  
  print('filteredCrossCorrelation (full):')
  fshift_full = filteredCrossCorrelation_full.argmax() - (len(filteredCrossCorrelation_full) / 2)
  print(f'idx shift: {fshift_full}, delay (us): {fshift_full * sampel_period_us}, AoA: {angleFromShift(fshift_full)[0]}')

  print('filteredCrossCorrelation (same):')
  fshift_same = filteredCrossCorrelation_same.argmax() - (len(filteredCrossCorrelation_same) / 2)
  print(f'idx shift: {fshift_same}, delay (us): {fshift_same * sampel_period_us}, AoA: {angleFromShift(fshift_same)[0]}')

  blockShift_0 = np.correlate(filtered[used_channels[0]], signal_block, mode='valid').argmax() + int(pulse_width_ms * sampels_per_ms)
  blockShift_1 = np.correlate(filtered[used_channels[1]], signal_block, mode='valid').argmax() + int(pulse_width_ms * sampels_per_ms)

  print(f'blockShift_0: {blockShift_0}, blockShift_1: {blockShift_1}, diff: {blockShift_1 - blockShift_0}, AoA: {angleFromShift(blockShift_1 - blockShift_0)[0]}')

  # determine the closest channel i.e the channel first recieving the signal
  refSignal = used_channels[0] if fshift_same > 0 else used_channels[1]
  signalDelay = np.correlate(filtered[refSignal], signal_block, mode='valid').argmax() + int(pulse_width_ms * sampels_per_ms) # compåensate for the start signal
  distance_cm = signalDelay * sampel_period_us * 0.000001 * sound_speed_cm_per_s
  print(f'estimated distance (cm): {distance_cm * 0.5}')

  # draw visualization of data
  Thread(target=visual.drawObj, args=(angleFromShift(fshift_same)[0], distance_cm * 0.5, mic_dist_cm, N_used_channels)).start()
  
  # Plot the spectra
  Channel_plots = 2
  N_plots = (N_used_channels * Channel_plots)

  plt.figure(figsize=(12, 4*N_plots))

  for i in range(0, N_used_channels):

    plt.subplot(N_plots, 1, (i*Channel_plots)+1)
    plt.plot(channels[used_channels[i]][:int(sampels_per_ms * ms)])
    plt.title(f'Channel {used_channels[i]} Raw signal')
    plt.xlabel(f'Time ({ms} ms)')
    plt.ylabel('Amplitude')

    # Filtered signal
    plt.subplot(N_plots, 1, (i*Channel_plots)+2)
    plt.plot(filtered[used_channels[i]][:int(sampels_per_ms * ms)])
    plt.title(f'Channel {used_channels[i]} Filtered signal (Filter (low={low}, high={high}, order={order}))')
    plt.xlabel(f'Time ({ms} ms)')
    plt.ylabel('Amplitude')

  # # correlation plots
  # plt.subplot(N_plots, 1, N_plots-1)
  # plt.plot(crossCorrelation)
  # plt.title(f'Crosscorrelation raw data')
  # plt.xlabel(f'Sample shift')
  # plt.ylabel('Similarity')

  # plt.subplot(N_plots, 1, N_plots)
  # plt.plot(filteredCrossCorrelation)
  # plt.title(f'Crosscorrelation filtered data')
  # plt.xlabel(f'Sample shift')
  # plt.ylabel('Similarity')

  plt.tight_layout()
  plt.show()



file_path = easygui.fileopenbox()
print(file_path + ':')
calculate(file_path)

# import os
# def get_files(path):
#     for file in os.listdir(path):
#         filePath = os.path.join(path, file)
#         if os.path.isfile(filePath):
#             yield file, filePath

# for file_path in get_files(easygui.diropenbox()):
#   print(file_path[0] + ':')
#   calculate(file_path[1])
#   print('')