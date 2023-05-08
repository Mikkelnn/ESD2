import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, lfilter
import easygui

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

for i in range(0, N_channels):
  channels.append(data[:, i])
  channels[i] = channels[i] - np.mean(channels[i]) # Remove DC offset


# Calculate the frequencies
n = len(data)
sampling_rate = 80000  # Assuming unit sampling rate, modify if needed
sampels_per_ms = sampling_rate / 1000
frequencies = np.fft.rfftfreq(n, d=1/sampling_rate)

low = 38200
high = 39000
order = 5
ms = 5



Channel_plots = 4
N_plots = N_channels * Channel_plots

# Plot the spectra
plt.figure(figsize=(12, 4*N_plots))

for i in range(0, N_channels):
  filtered = butter_bandpass_filter(data[:, i], low, high, sampling_rate, order)
  # Perform FFT on each column
  fft_raw = np.fft.rfft(channels[i])
  fft_filtered = np.fft.rfft(filtered)

  plt.subplot(N_plots, 1, (i*Channel_plots)+1)
  plt.plot(frequencies, np.abs(fft_raw))
  plt.title(f'Channel {i} Spectrum (Raw)')
  plt.xlabel('Frequency (Hz)')
  plt.ylabel('Amplitude')

  plt.subplot(N_plots, 1, (i*Channel_plots)+2)
  plt.plot(channels[i][:int(sampels_per_ms * ms)])
  plt.title(f'Channel {i} Raw signal')
  plt.xlabel(f'Time ({ms} ms)')
  plt.ylabel('Amplitude')

  # Filtered signal
  plt.subplot(N_plots, 1, (i*Channel_plots)+3)
  plt.plot(frequencies, np.abs(fft_filtered))
  plt.title(f'Channel {i} Filtered Spectrum (Filter (low={low}, high={high}, order={order}))')
  plt.xlabel('Frequency (Hz)')
  plt.ylabel('Amplitude')

  plt.subplot(N_plots, 1, (i*Channel_plots)+4)
  plt.plot(filtered[:int(sampels_per_ms * ms)])
  plt.title(f'Channel {i} Filtered signal (Filter (low={low}, high={high}, order={order}))')
  plt.xlabel(f'Time ({ms} ms)')
  plt.ylabel('Amplitude')

plt.tight_layout()
plt.show()