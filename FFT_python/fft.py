import numpy as np
import matplotlib.pyplot as plt

# Load data from the CSV file
data = np.genfromtxt('wavegen-39khz.txt', delimiter=',')

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

# Perform FFT on each column
fft1 = np.fft.fft(column1)
fft2 = np.fft.fft(column2)
fft3 = np.fft.fft(column3)
fft4 = np.fft.fft(column4)

# Calculate the frequencies
sampling_rate = 80000  # Assuming unit sampling rate, modify if needed
n = len(data)
freq_resolution = sampling_rate / n
frequencies = np.fft.fftfreq(n, d=1/sampling_rate) * sampling_rate

# Define y-axis range
y_range = (0, 1e4)  # Adjust as per your signal's amplitude range

# Plot the spectra
plt.figure(figsize=(12, 8))

plt.subplot(2, 2, 1)
plt.plot(frequencies, np.abs(fft1))
plt.title('Column 1 Spectrum')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude')
plt.ylim(y_range)

plt.subplot(2, 2, 2)
plt.plot(frequencies, np.abs(fft2))
plt.title('Column 2 Spectrum')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude')
plt.ylim(y_range)

plt.subplot(2, 2, 3)
plt.plot(frequencies, np.abs(fft3))
plt.title('Column 3 Spectrum')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude')
plt.ylim(y_range)

plt.subplot(2, 2, 4)
plt.plot(frequencies, np.abs(fft4))
plt.title('Column 4 Spectrum')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude')
plt.ylim(y_range)

plt.tight_layout()
plt.show()
