from core.drv8833 import DRV8833
import time

motor = DRV8833()

# 동작 테스트
motor.set_speed(100, 100)  # 전진
