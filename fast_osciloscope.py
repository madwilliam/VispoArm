
from PyQt5 import QtCore, QtWidgets
import pyqtgraph as pg
import numpy as np
import serial
import time
class MyWidget(pg.GraphicsLayoutWidget):

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.port = serial.Serial('COM3', 115200, timeout=0.5)
        time.sleep(0.1)
        self.x = np.zeros(1000)
        self.all = []
        self.mainLayout = QtWidgets.QVBoxLayout()
        self.setLayout(self.mainLayout)
        self.timer = QtCore.QTimer(self)
        self.timer.setInterval(10) # in milliseconds
        self.timer.start()
        self.timer.timeout.connect(self.onNewData)
        self.plotItem = self.addPlot(title="Lidar points")
        self.plotDataItem = self.plotItem.plot([], pen=None, 
            symbolBrush=(255,0,0), symbolSize=5, symbolPen=None)

    def setData(self, x):
        self.plotDataItem.setData(x)

    def onNewData(self):
        self.x = np.roll(self.x,1,0)
        self.port.write(b's') #handshake with Arduino
        if (self.port.inWaiting()):# if the arduino replies
            value = self.port.readline()# read the reply
            number = int(str(value[:-2])[2:-1]) #convert received data to integer 
            self.x[-1] = number
            self.all.append(number)
            self.setData(self.x)
        else:
            print('no response')
        
def main():
    app = QtWidgets.QApplication([])
    pg.setConfigOptions(antialias=False) # True seems to work as well
    win = MyWidget()
    win.show()
    win.resize(800,600) 
    win.raise_()
    app.exec_()

if __name__ == "__main__":
    main()