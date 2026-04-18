from machine import I2C, Pin

from core.myservo import myServo
from core.ina226 import INA226
import time

servo = myServo(pin=10)

ina_0x40 = INA226(address=0x40)
ina_0x41 = INA226(address=0x41)

try:
    ina_0x40.configure(avg=4, busConvTime=4, shuntConvTime=4, mode=7)
    ina_0x41.configure(avg=4, busConvTime=4, shuntConvTime=4, mode=7)
    ina_0x40.calibrate(rShuntValue=0.1, iMaxExpected=2.0)
    ina_0x41.calibrate(rShuntValue=0.1, iMaxExpected=2.0)
except Exception as e:
    print("Error:", e)

data = {}

print("\n" + "=" * 55)
print(
    f"{'Angle':^7} | {'V1 (V)':^9} | {'mA1 (mA)':^10} | {'V2 (V)':^9} | {'mA2 (mA)':^10}"
)
print("-" * 55)

try:
    for i in range(90):
        angle = 45 + i
        servo.myServoWriteAngle(angle, 10)

        v1 = ina_0x40.read_bus_voltage()
        ma1 = ina_0x40.read_shunt_current()
        v2 = ina_0x41.read_bus_voltage()
        ma2 = ina_0x41.read_shunt_current()

        data[angle] = {"v1": v1, "ma1": ma1, "v2": v2, "ma2": ma2}

        print(f"{angle:7d} | {v1:9.3f} | {ma1:10.2f} | {v2:9.3f} | {ma2:10.2f}")

        time.sleep(0.1)

    print("-" * 55)
    print("Done")

    # v1 값이 가장 높은 angle 찾기
    max_angle = max(data, key=lambda k: data[k]["v1"])
    print(f"Moving servo to angle {max_angle} with highest v1: {data[max_angle]['v1']}")
    servo.myServoWriteAngle(max_angle, 15)

except KeyboardInterrupt:
    print("\nStopped")
