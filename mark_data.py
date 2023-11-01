# for data collection markers

import time
from getkey import getkey, keys
import csv
from generateIndexes import gen_index

def main():
    FLEXION = 0
    EXTENSION = 1
    SUSTAIN = 2
    REST = 3

    # get current time for filename
    t = time.localtime()
    filename = time.strftime("%H:%M:%S_%m:%d:%y", t)

    print(f'Starting file time {filename}')

    with open(filename + '.csv', 'a') as fn:
        csv_write = csv.writer(fn)
        while(1):
            key = getkey()
            if key == 'x':
                print('done')
                fn.close()
                exit()
            else:
                checkKey(key, csv_write)
    return

def checkKey(key, csv_write):
    
    if key == 'f':
        print(f'{time.strftime("%H:%M:%S", time.localtime())} FLEXION RECORDED')
        csv_write.writerow([0, time.strftime("%H:%M:%S", time.localtime())])
    elif key == 'e':
        print(f'{time.strftime("%H:%M:%S", time.localtime())} EXTENSION RECORDED')
        csv_write.writerow([1, time.strftime("%H:%M:%S", time.localtime())])
    elif key == 's':
        print(f'{time.strftime("%H:%M:%S", time.localtime())} SUSTAIN RECORDED')
        csv_write.writerow([2, time.strftime("%H:%M:%S", time.localtime())])
    elif key == 'r':
        print(f'{time.strftime("%H:%M:%S", time.localtime())} REST RECORDED')
        csv_write.writerow([3, time.strftime("%H:%M:%S", time.localtime())])

if __name__ == '__main__':
    main()