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
            value = int(value.decode())
        except:
            value = None
        return value