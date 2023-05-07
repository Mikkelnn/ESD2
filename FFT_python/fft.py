import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, lfilter

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
# data = np.genfromtxt('t:/Repoes/AAU/ESD2/Project/FFT_python/wavegen-39khz.txt', delimiter=',')
data = np.genfromtxt('t:/Repoes/AAU/ESD2/Project/FFT_python/test1.txt', delimiter=',')

# Separate the columns
column1 = data[:, 0]
column2 = data[:, 1]
column3 = data[:, 2]
column4 = data[:, 3]

# Remove DC offset
column1 = column1 - np.mean(column1)
column2 = column2 - np.mean(column2)
column3 = column3 - np.mean(column3)
column4 = column4 - np.mean(column4)

# Calculate the frequencies
n = len(data)
sampling_rate = 80000  # Assuming unit sampling rate, modify if needed
sampels_per_ms = sampling_rate / 1000
frequencies = np.fft.rfftfreq(n, d=1/sampling_rate)

low = 38200
high = 39000
order = 5
filtered = butter_bandpass_filter(data[:, 0], low, high, sampling_rate, order)
ms = 5

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