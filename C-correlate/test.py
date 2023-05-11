import numpy as np

x = [1, 0, 0, 1]
y = [0, 1, 0]

valid = np.correlate(x,y,"valid")
same = np.correlate(x,y,"same")
full = np.correlate(x,y,"full")

print(f'valid: {valid.argmax()}, {valid}')
print(f'same: {same.argmax() - int((len(y)-1)/2)}, {same}')
print(f'full: {full.argmax() - (len(y)-1)}, {full}')



t1 = [0,0,0,0,0,0,0,0,0,0,0,0]
s = 5
e = 6
print(f'esti: {e-s}, actu: {len(t1[s:e])}')