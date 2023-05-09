import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, lfilter
import easygui
import math

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


# Load data from the CSV file
file_path = easygui.fileopenbox()
data = np.genfromtxt(file_path, delimiter=',')

# get number of channels from data
N_channels = data.shape[1]


# Calculate the frequencies
n = len(data)
sampling_rate = 80000  # Assuming unit sampling rate, modify if needed
sampels_per_ms = sampling_rate / 1000

low = 38200
high = 39000
order = 5
ms = 20

# Separate the columns
channels = []
filtered = []
for i in range(0, N_channels):
  channels.append(data[:, i])
  channels[i] = channels[i] - np.mean(channels[i]) # Remove DC offset
  filtered.append(butter_bandpass_filter(channels[i], low, high, sampling_rate, order))


# Find channels with data - i.e non zeros
used_channels = []
for i in range(0, N_channels):
   if np.sum(np.abs(channels[i])) > 0:
      used_channels.append(i)

print(f'channels used: {used_channels}')
if (len(used_channels) < 2):
   print('only one channel! - can´t cross correlate.....')
   exit()

# Calculate cross correlation
crossCorrelation = np.correlate(channels[used_channels[0]], channels[used_channels[1]], mode='full')
filteredCrossCorrelation = np.correlate(filtered[used_channels[0]], filtered[used_channels[1]], mode='full')

print('crossCorrelation:')
rcmax = crossCorrelation.argmax() - len(data) # - endIdx
print(f'idx: {rcmax}, delay (us): {rcmax * 12.5}')
print('filteredCrossCorrelation:')
fcmax = filteredCrossCorrelation.argmax() - len(data) # - endIdx
print(f'idx: {fcmax}, delay (us): {fcmax * 12.5}')

micDistCm = 7.5
# rangle = math.asin(((rcmax * 12.5 * 0.000001)*34300)/micDistCm)
# print(f'raw angle: {np.rad2deg(rangle)}')

fangle = math.asin(((fcmax * 12.5 * 0.000001)*34300)/micDistCm)
print(f'filtered angle: {np.rad2deg(fangle)}')


# Plot the spectra
Channel_plots = 2
N_plots = (N_channels * Channel_plots) + 2

plt.figure(figsize=(12, 4*N_plots))

for i in range(0, N_channels):

  plt.subplot(N_plots, 1, (i*Channel_plots)+1)
  plt.plot(channels[i][:int(sampels_per_ms * ms)])
  plt.title(f'Channel {i} Raw signal')
  plt.xlabel(f'Time ({ms} ms)')
  plt.ylabel('Amplitude')

  # Filtered signal
  plt.subplot(N_plots, 1, (i*Channel_plots)+2)
  plt.plot(filtered[i][:int(sampels_per_ms * ms)])
  plt.title(f'Channel {i} Filtered signal (Filter (low={low}, high={high}, order={order}))')
  plt.xlabel(f'Time ({ms} ms)')
  plt.ylabel('Amplitude')

# correlation plots
plt.subplot(N_plots, 1, N_plots-1)
plt.plot(crossCorrelation)
plt.title(f'Crosscorrelation raw data')
plt.xlabel(f'Sample shift')
plt.ylabel('Similarity')

plt.subplot(N_plots, 1, N_plots)
plt.plot(filteredCrossCorrelation)
plt.title(f'Crosscorrelation filtered data')
plt.xlabel(f'Sample shift')
plt.ylabel('Similarity')

plt.tight_layout()
plt.show()