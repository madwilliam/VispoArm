import time
import serial
import csv
import argparse

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

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("file_name",default=2,type=str)
    args = parser.parse_args()
    file_name = args.file_name
    write_to_csv(file_name)
