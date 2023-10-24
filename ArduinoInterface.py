import serial
import time
class ArduinoInterface:
    def __init__(self,serial_port):
        self.port = serial.Serial(serial_port, 115200, timeout=0.5)
        time.sleep(0.1)
    
    def get_data(self):
        self.port.write(b's') 
        try:
            value = self.port.readline()
            # value = value.decode('unicode_escape')
            value = int.from_bytes(value, byteorder='big', signed=True)
        except:
            value = None
        time.sleep(0.001)
        return value