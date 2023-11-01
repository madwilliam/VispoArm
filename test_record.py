
list
import time
import serial
import csv
import subprocess
import os
from threading import Thread
import sys

port = serial.Serial('/dev/tty.usbserial-B00054LQ', 9600, timeout=0.5)
time.sleep(0.1)
data = []
print('recording')
while True:
    port.write(b's') 
    try:
        value = port.readline()
        value = int(value.decode())
    except:
        value = None