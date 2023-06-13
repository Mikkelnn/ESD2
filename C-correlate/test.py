import numpy as np

x = [1, 2, 3, 4, 5]
y = [4, 5, 6, 7, 8]

valid = np.correlate(x,y,"valid")
same = np.correlate(x[2:],y[2:],"same")
full = np.correlate(x,y,"full")

print(f'valid: {valid.argmax()}, {valid}')
print(f'same: {same.argmax() - int((len(y[2:])-1)/2)}, {same}')
print(f'full: {full.argmax() - (len(y)-1)}, {full}')



t1 = [0,0,0,0,0,0,0,0,0,0,0,0]
s = 3
e = 7
print(f'esti: {e-s}, actu: {len(t1[s:e])}')