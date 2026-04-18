from machine import I2C, Pin
import struct

_shared_i2c = None


def get_shared_i2c(i2c_bus=0, freq=400000):
    global _shared_i2c
    if _shared_i2c is not None:
        return _shared_i2c
    pin_configs = [(6, 7), (7, 6)]
    for sda_pin, scl_pin in pin_configs:
        try:
            temp_i2c = I2C(i2c_bus, sda=Pin(sda_pin), scl=Pin(scl_pin), freq=freq)
            if len(temp_i2c.scan()) > 0:
                print(f"센서 연결 성공! (SDA={sda_pin}, SCL={scl_pin})")
                _shared_i2c = temp_i2c
                return _shared_i2c
        except Exception:
            pass
    print("[I2C 경고] 센서 응답이 없습니다. 배선을 확인하세요.")
    _shared_i2c = I2C(i2c_bus, sda=Pin(6), scl=Pin(7), freq=freq)
    return _shared_i2c


class INA226:
    # 레지스터 주소 정의
    REG_CONFIG = 0x00
    REG_SHUNTVOLTAGE = 0x01
    REG_BUSVOLTAGE = 0x02
    REG_POWER = 0x03
    REG_CURRENT = 0x04
    REG_CALIBRATION = 0x05

    def __init__(self, address=0x40):
        self.address = address
        self.i2c = get_shared_i2c()
        self._current_lsb = 0.0
        self._power_lsb = 0.0

    def _write_register(self, reg, value):
        data = struct.pack(">H", int(value))
        self.i2c.writeto_mem(self.address, reg, data)

    def _read_register(self, reg):
        data = self.i2c.readfrom_mem(self.address, reg, 2)
        return struct.unpack(">H", data)[0]

    def _read_register_signed(self, reg):
        data = self.i2c.readfrom_mem(self.address, reg, 2)
        return struct.unpack(">h", data)[0]

    def configure(self, avg=0b100, busConvTime=0b100, shuntConvTime=0b100, mode=0b111):
        config = (avg << 9) | (busConvTime << 6) | (shuntConvTime << 3) | mode
        self._write_register(self.REG_CONFIG, config)

    def calibrate(self, rShuntValue=0.1, iMaxExpected=2.0):
        self._current_lsb = iMaxExpected / 32768.0
        self._power_lsb = self._current_lsb * 25.0
        cal_value = 0.00512 / (self._current_lsb * rShuntValue)
        self._write_register(self.REG_CALIBRATION, cal_value)

    def read_bus_voltage(self):
        val = self._read_register(self.REG_BUSVOLTAGE)
        return val * 0.00125

    def read_shunt_voltage(self):
        val = self._read_register_signed(self.REG_SHUNTVOLTAGE)
        return val * 0.0000025

    def read_shunt_current(self):
        val = self._read_register_signed(self.REG_CURRENT)
        return val * self._current_lsb

    def read_bus_power(self):
        val = self._read_register(self.REG_POWER)
        return val * self._power_lsb
