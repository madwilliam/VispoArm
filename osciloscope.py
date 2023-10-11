import time
import matplotlib.pyplot as plt
from drawnow import *
import serial
import numpy as np
val = [ ]
cnt = 0
data_stream = np.zeros(50)
port = serial.Serial('/dev/tty.usbserial-B00054LQ', 115200, timeout=0.5)
plt.figure()
p = plt.plot(data_stream, 'ro-', label='Channel 0')
plt.ylim(-1023,1023)
plt.title('Osciloscope')
plt.grid(True)
plt.ylabel('ADC outputs')
plt.legend(loc='lower right')
plt.ion()     
plt.show()
plt.pause(.0001)
while (True):
    port.write(b's') #handshake with Arduino
    if (port.inWaiting()):# if the arduino replies
        data_stream = np.roll(data_stream,1,0)
        value = port.readline()# read the reply
        number = float(str(value[:-2])[2:-1]) #convert received data to integer 
        time.sleep(0.01)
        data_stream[-1] = number
        p[0].set_ydata(data_stream)
        p[0].set_xdata(list(range(len(data_stream))))
        plt.draw()
        plt.pause(.0001)
        cnt = cnt+1