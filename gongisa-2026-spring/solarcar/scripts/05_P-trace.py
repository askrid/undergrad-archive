from core.drv8833 import DRV8833
from core.mux04 import LineSensor
import time

sensor = LineSensor()
motor = DRV8833()

kp = 1.6
base_speed = 100

while True:
    channels = sensor.read_channels()
    weights = [-55, -30, -20, -10, 10, 20, 30, 55]
    error_sum = 0
    active_count = 0
    for i in range(8):
        if channels[i] == 1:  # 라인을 감지했을 때
            error_sum += weights[i]
            active_count += 1
            steering = error_sum * kp
            print("steering : ", steering)
            motor.set_speed(base_speed + steering, base_speed - steering)

    # 라인을 완전히 벗어난 경우
    if active_count == 0:
        print("No Line!")
    time.sleep(0.1)
