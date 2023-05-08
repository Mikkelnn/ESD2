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


N_channels = 4

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
plt.figure(figsize=(3*N_plots, 8))

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



exit()




# Perform FFT on each column
fft1 = np.fft.rfft(column1)
fft2 = np.fft.rfft(filtered)
fft3 = np.fft.rfft(column3)
fft4 = np.fft.rfft(column4)

# Plot the spectra
plt.figure(figsize=(12, 8))

plt.subplot(4, 1, 1)
plt.plot(frequencies, np.abs(fft1))
plt.title('Channel 1 Spectrum (Raw)')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude')

plt.subplot(4, 1, 2)
plt.plot(column1[:int(sampels_per_ms * ms)])
plt.title('Raw signal')
plt.xlabel(f'Time ({ms} ms)')
plt.ylabel('Amplitude')



plt.subplot(4, 1, 3)
plt.plot(frequencies, np.abs(fft2))
plt.title(f'Channel 1 Filtered Spectrum (Filter (low={low}, high={high}, order={order}))')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude')

plt.subplot(4, 1, 4)
plt.plot(filtered[:int(sampels_per_ms * ms)])
plt.title(f'Filtered signal (Filter (low={low}, high={high}, order={order}))')
plt.xlabel(f'Time ({ms} ms)')
plt.ylabel('Amplitude')

# plt.subplot(2, 2, 3)
# plt.plot(frequencies, np.abs(fft3))
# plt.title('Column 3 Spectrum')
# plt.xlabel('Frequency (Hz)')
# plt.ylabel('Amplitude')

# plt.subplot(2, 2, 4)
# plt.plot(frequencies, np.abs(fft4))
# plt.title('Column 4 Spectrum')
# plt.xlabel('Frequency (Hz)')
# plt.ylabel('Amplitude')


print('Raw:')
for i, x in enumerate(np.abs(fft1)):
  if x > 70000:
    print(f'Frequency {frequencies[i]}Hz; {x}')

print('Filtered:')
for i, x in enumerate(np.abs(fft2)):
  if x > 30000:
    print(f'Frequency {frequencies[i]}Hz; {x}')




plt.tight_layout()
plt.show()