import time
import matplotlib.pyplot as plt
# from drawnow import *
import serial
import numpy as np
val = [ ]
cnt = 0
sampling_rate_hz = 2000 
duration_s = 3
nsamples = duration_s*sampling_rate_hz
data_stream = np.zeros(nsamples)
port = serial.Serial('/dev/tty.usbserial-B00054LQ', 115200, timeout=0.5)
print('recording')
for i in range(nsamples):
    port.write(b's') #handshake with Arduino
    if (port.inWaiting()):# if the arduino replies
        data_stream = np.roll(data_stream,1,0)
        value = port.readline()# read the reply
        number = float(str(value[:-2])[2:-1]) #convert received data to integer 
        data_stream[cnt] = number
        cnt = cnt+1
    time.sleep(1/sampling_rate_hz)
print('done')
plt.plot(data_stream)
plt.show()