
from ArduinoInterface import ArduinoInterface
import time
interface = ArduinoInterface('COM7')
while True:
    print(interface.get_data())
    time.sleep(0.001)