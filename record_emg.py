
import time
import serial
import csv
import multiprocessing
import os
import asyncio
def write_to_csv(file_name):
    port = serial.Serial('COM7', 9600, timeout=0.5)
    time.sleep(0.1)
    data = []
    file = open(file_name, 'w', newline='')
    spamwriter = csv.writer(file, delimiter=' ',
                            quotechar='|', quoting=csv.QUOTE_MINIMAL)
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
        if os.path.exists(os.path.join(path,file_name)):
            file_name = file_name + str(i)
            i = i +1
        else:
            return file_name

async def run_for_n_seconds(path,seconds):
    await asyncio.gather(asyncio.sleep(seconds),write_to_csv(path))

while True:
    path = r'C:\Users\madwill\Desktop\data'
    duration = input('Enter recording length \n')
    type = input('Enter recording type: flexion 0 extention 1 \n')
    try:
        duration = int(duration)
        type = int(type)
        if type ==0:
            name = 'flexion'
        else:
            name = 'extention'
        name = next_name(path,name)
        full_path = os.path.join(path,name+'.csv')
        run_for_n_seconds(full_path,duration)
        
    except:
        print('error or invalid input')
