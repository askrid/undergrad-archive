from core.mux04 import LineSensor
import time

sensor = LineSensor()

while True:
    channels = sensor.read_channels()
    channels = [1 - c for c in channels]
    print("channels : ", channels)
    time.sleep(0.1)
