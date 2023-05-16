import numpy as np
import matplotlib.pyplot as plt
import math

def getSignal(out, paddingStart, paddingEnd, freq, periods):
    for _ in range(0, paddingStart):
      out.append(0)
    
    for _ in range(0, periods):
      for i in range(-int(math.pi*100),int(math.pi*100), 40*freq):
        out.append(10 * math.sin((i/100)))

    for _ in range(0, paddingEnd):
      out.append(0)

def signalsCorr():
  x = []
  getSignal(x, 0, 10, 1, 1)

  y = []
  getSignal(y, 5, 5, 1, 1)

  # while (len(y) < len(x)):
  #   y.append(0)


  xyCorrelation = np.correlate(x,y,mode='full')


  # for i in range(-(len(y)-1), len(x)):
  #   if i <= 0:
  #     print(y[abs(i):])
  #   else:
  #     print(np.zeros(i).tolist() + y[:-i])

  plt.rcParams.update({'font.size': 20})
  N_plot = 3
  plt.figure(figsize=(12, 4*N_plot))

  plt.subplot(N_plot, 1, 1)
  plt.stem(x, basefmt='')
  plt.title("Signal x")
  plt.xlabel('n')
  plt.ylabel('x[n]')

  plt.subplot(N_plot, 1, 2)
  plt.stem(y, basefmt='')
  plt.title("Signal y")
  plt.xlabel('n')
  plt.ylabel('y[n]')

  plt.subplot(N_plot, 1, 3)
  plt.stem(range(-(len(y)-1), len(x)), xyCorrelation, basefmt='')
  plt.title("Cross correlation of x and y")
  plt.xlabel('y shift relative to x')
  plt.ylabel('Correlation')

  N_plot = 8
  Title_size = None #10
  fig, subplts = plt.subplots(N_plot, 1, sharex='all', sharey='all')
  subplts[0].stem(x, basefmt='g',markerfmt='go')
  subplts[0].set_title('Signal x', size=Title_size)
  subplts[1].stem(y, basefmt='')
  subplts[1].set_title('Signal y shifted 0', size=Title_size)

  for idx, i in enumerate(range(-(len(y)-1), len(x), 10)):
    subplts[idx+2].set_title(f'Signal y shifted {i}{" (equal to x)" if i == -5 else ""}', size=Title_size)
    if i <= 0:
      y_part = y[abs(i):]
      subplts[idx+2].stem(y_part + np.zeros(len(x)-len(y_part)).tolist(), basefmt=('r' if i == -5 else ''), markerfmt=('ro' if i == -5 else 'o'))
    else:
      subplts[idx+2].stem(np.zeros(i).tolist() + y[:-i], basefmt='')    

  plt.xlabel('Sampel')
  plt.ylabel('Amplitude')

  plt.tight_layout()
  plt.show()


x = [1, 1, 1, 1, 1]
y = [2, 2, 2, 2]

print(f'x: {x}')
print(f'y: {y}')

full = np.correlate(x,y, mode='full')
print(f'full:  {full}')

valid = np.correlate(x,y, mode='valid')
print(f'valid: {valid}')

same = np.correlate(x,y, mode='same')
print(f'same:  {same}')