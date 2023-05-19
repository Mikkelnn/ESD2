import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, lfilter
import easygui
import math

sampling_rate = 80000  # Assuming unit sampling rate, modify if needed
sampels_per_ms = sampling_rate / 1000

low = 38000
high = 39000
order = 5
ms = 10

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

N_used_channels = len(used_channels)


# Calculate the frequencies
n = len(data)
frequencies = np.fft.rfftfreq(n, d=1/sampling_rate)

Channel_plots = 4
N_plots = N_used_channels * Channel_plots

# Plot the spectra
plt.figure(figsize=(12, 4*N_plots))

for i in range(0, N_used_channels):
  # Perform FFT on each column
  fft_raw = np.fft.rfft(channels[used_channels[i]])
  fft_filtered = np.fft.rfft(filtered[used_channels[i]])

  plt.subplot(N_plots, 1, (i*Channel_plots)+1)
  plt.plot(frequencies, 20 * np.log10(np.abs(fft_raw)))
  plt.title(f'Channel {used_channels[i]} Spectrum (Raw)')
  plt.xlabel('Frequency (Hz)')
  plt.ylabel('Amplitude (dB)')

  plt.subplot(N_plots, 1, (i*Channel_plots)+2)
  plt.plot(channels[used_channels[i]][:int(sampels_per_ms * ms)])
  plt.title(f'Channel {used_channels[i]} Raw signal')
  plt.xlabel(f'Time ({ms} ms)')
  plt.ylabel('Amplitude')

  # Filtered signal
  plt.subplot(N_plots, 1, (i*Channel_plots)+3)
  plt.plot(frequencies, 20 * np.log10(np.abs(fft_filtered)))
  plt.title(f'Channel {used_channels[i]} Filtered Spectrum (Filter (low={low}, high={high}, order={order}))')
  plt.xlabel('Frequency (Hz)')
  plt.ylabel('Amplitude (dB)')

  plt.subplot(N_plots, 1, (i*Channel_plots)+4)
  plt.plot(filtered[used_channels[i]][:int(sampels_per_ms * ms)])
  plt.title(f'Channel {used_channels[i]} Filtered signal (Filter (low={low}, high={high}, order={order}))')
  plt.xlabel(f'Time ({ms} ms)')
  plt.ylabel('Amplitude')

plt.tight_layout()
plt.show()