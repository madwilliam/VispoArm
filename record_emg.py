list
import time
import serial
import csv
import subprocess
import os
from threading import Thread
import sys
from subprocess import Popen, PIPE

def write_to_csv(file_name):
    port = serial.Serial('/dev/tty.usbserial-B00054LQ', 9600, timeout=0.5)
    time.sleep(0.1)
    data = []
    file = open(file_name, 'w', newline='')
    spamwriter = csv.writer(file, delimiter=' ',
                            quotechar='|', quoting=csv.QUOTE_MINIMAL)
    print('recording')
    while True:
        port.write(b's') 
        try:
            value = port.readline()
            value = int(value.decode())
            spamwriter.writerow({value})
            file.flush()
        except:
            value = None

def next_name(path,file_name):
    i=0
    while True:
        if os.path.exists(os.path.join(path,file_name+'.csv')):
            new_file_name = file_name + str(i)
            i = i +1
        else:
            new_file_name = file_name
        return new_file_name

while True:
    path = r'/Users/william/Desktop/data/emg/new recording/'
    duration = input('Enter recording length \n')
    type = input('Enter recording type: flexion 0 extention 1 \n')
    duration = int(duration)
    type = int(type)
    if type ==0:
        name = 'flexion'
    else:
        name = 'extention'
    name = next_name(path,name)
    full_path = os.path.join(path,name+'.csv')
    script_directory = os.path.dirname(os.path.abspath(sys.argv[0]))
    script_path = f"{os.path.join(script_directory,'write_to_csv.py')}"
    script_path_2 = f"{os.path.join(script_directory,'mark_data.py')}"
    p1 = Popen(["python",script_path,full_path], stdout=PIPE, stderr=PIPE)
    p2 = Popen(["python",script_path_2], stdout=PIPE, stderr=PIPE)
    # time.sleep(duration)
    # p1.kill()    
    # p2.kill()    