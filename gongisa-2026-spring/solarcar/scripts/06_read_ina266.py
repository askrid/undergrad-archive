from core.ina226 import INA226
import time

ina_0x40 = INA226(address=0x40)
ina_0x41 = INA226(address=0x41)


try:
    ina_0x40.configure(avg=4, busConvTime=4, shuntConvTime=4, mode=7)
    ina_0x41.configure(avg=4, busConvTime=4, shuntConvTime=4, mode=7)
    ina_0x40.calibrate(rShuntValue=0.1, iMaxExpected=2.0)
    ina_0x41.calibrate(rShuntValue=0.1, iMaxExpected=2.0)
except Exception as e:
    print("초기화 실패 (센서 연결 확인):", e)

while True:
    try:
        # 1번 센서 데이터 읽기
        v1 = ina_0x40.read_bus_voltage()
        ma1 = ina_0x40.read_shunt_current()

        # 2번 센서 데이터 읽기
        v2 = ina_0x41.read_bus_voltage()
        ma2 = ina_0x41.read_shunt_current()

        if v1 is None or ma1 is None or v2 is None or ma2 is None:
            print("데이터를 읽지 못했습니다. 센서 배선을 확인하세요.")
            time.sleep(1)
            continue

        print("[Sensor 0x40] {:.3f}V, {:.1f}mA".format(v1, ma1 * 1000))
        print("[Sensor 0x41] {:.3f}V, {:.1f}mA".format(v2, ma2 * 1000))
        print("-" * 40)

    except KeyboardInterrupt:
        print("I2C 통신 물리적 에러:")

    time.sleep(0.5)
