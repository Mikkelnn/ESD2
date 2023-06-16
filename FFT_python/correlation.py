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
pulse_width_sampels = int(pulse_width_ms * sampels_per_ms)

low = 38200 # low = 38200
high = 38700
order = 5
ms = 20 # number of milliseconds of data to display 

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

def calculate(file_path):
  # Load data from the CSV file
  file_name = file_path.split('\\')[-1]
  data = np.genfromtxt(file_path, delimiter=',')

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
  max_correlation_sampel = min(len(filtered[used_channels[0]]) - 1, int(signalDelay + pulse_width_sampels + margin))
  min_correlation_sampel = max(0, int(signalDelay - margin))

  # Calculate cross correlation
  x_section = filtered[used_channels[1]][min_correlation_sampel:max_correlation_sampel]
  y_section = filtered[used_channels[0]][min_correlation_sampel:max_correlation_sampel]
  max_correlation_idx = np.correlate(x_section, y_section, mode='same').argmax()
  relative_shift = max_correlation_idx - int((max_correlation_sampel-min_correlation_sampel-1) * 0.5)

  print(f'Python: AoA: {angleFromShift(relative_shift)}, estimated distance (cm): {distance_cm}')
  print(f'signalDelay: {signalDelay}')
  print(f'min_correlation_sampel: {min_correlation_sampel}, max_correlation_sampel: {max_correlation_sampel}')
  print(f'idx shift: {relative_shift}, delay (us): {relative_shift * sampel_period_us}, AoA: {angleFromShift(relative_shift)}')
  print(f'blockShift_0: {blockShift_0}, blockShift_1: {blockShift_1}, diff: {blockShift_1 - blockShift_0}, AoA: {angleFromShift(blockShift_1 - blockShift_0)}')

  # print("x_section:y_section")
  # for i,x in enumerate(x_section):
  #    print(f'{x_section[i]}:{y_section[i]}')

  return

  # draw visualization of data
  illustration = visual.drawObj(angleFromShift(relative_shift), distance_cm, mic_dist_cm, N_used_channels, file_name)


  print(f'min_correlation_sampel: {min_correlation_sampel}, max_correlation_sampel: {max_correlation_sampel}')
  print('filteredCrossCorrelation (same):')
  print(f'idx shift: {relative_shift}, delay (us): {relative_shift * sampel_period_us}, AoA: {angleFromShift(relative_shift)}')
  print(f'blockShift_0: {blockShift_0}, blockShift_1: {blockShift_1}, diff: {blockShift_1 - blockShift_0}, AoA: {angleFromShift(blockShift_1 - blockShift_0)}')
  print(f'estimated distance (cm): {distance_cm}')

  
  # return

  # Plot the spectra
  Channel_plots = 2
  N_plots = (N_used_channels * Channel_plots)

  plt.figure(figsize=(12, 4*N_plots))

  for i in range(0, N_used_channels):

    plt.subplot(N_plots, 1, (i*Channel_plots)+1)
    plt.plot(channels[used_channels[i]][min_correlation_sampel-20:max_correlation_sampel+20]) # [:int(sampels_per_ms * ms)]
    plt.title(f'Channel {used_channels[i]} Raw signal')
    plt.xlabel(f'Time ({ms} ms)')
    plt.ylabel('Amplitude')

    # Filtered signal
    plt.subplot(N_plots, 1, (i*Channel_plots)+2)
    plt.plot(filtered[used_channels[i]][min_correlation_sampel-20:max_correlation_sampel+20]) # [:int(sampels_per_ms * ms)]
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
  # plt.show()

  from PIL import Image
  import io

  plt_buf = io.BytesIO()
  plt.savefig(plt_buf, format='jpg')
  plt_buf.seek(0)
  plt_im = Image.open(plt_buf)

  plt_im.save(f'output_images_report\\{file_name.split(".")[0]}.jpg', 'JPEG')
  plt.close()
  plt_buf.close()
  plt_im.close()
  return

  ps = illustration.postscript(colormode = 'color')
  illu_im = Image.open(io.BytesIO(ps.encode('utf-8')))

  x_spacing = 25
  x_size = plt_im.size[0] + illu_im.size[0] + x_spacing
  y_size = max(plt_im.size[1], illu_im.size[1])
  
  print(f'size: ({x_size}, {y_size})')

  new_image = Image.new('RGB',(x_size, y_size), (255,255,255))
  new_image.paste(plt_im, (0, 0))
  new_image.paste(illu_im, (plt_im.size[0] + x_spacing, 0))
  new_image.save(f'output_images\\{file_name.split(".")[0]}.jpg', 'JPEG')
  # new_image.show()

  # cleanup
  plt.close()
  plt_buf.close()
  plt_im.close()
  illu_im.close()
  # illustration.destroy()

file_path =  "T:\Repoes\AAU\ESD2\Project\FFT_python\lyd_lab\\20230509-105904-(human side center 2m).csv"
# file_path = easygui.fileopenbox()
print(file_path + ':')
calculate(file_path)

# process = ["lyd_lab\\20230509-095839.csv", "lyd_lab\\20230509-100420.csv", "lyd_lab\\20230509-100828.csv"]
# for file_path in process:
#   calculate(file_path)


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