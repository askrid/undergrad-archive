import network
import socket
import time
from machine import I2C, Pin

from core.ina226 import INA226

SSID = "Team 03"  # "WiFi_이름"
PASSWORD = "03030303"  # "WiFi_비밀번호"
SERVER_IP = "192.168.137.1"
SERVER_PORT = 5005


def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(False)
    time.sleep(0.5)
    wlan.active(True)
    try:
        wlan.config(txpower=10)
    except:
        pass
    if not wlan.isconnected():
        print(f"{SSID} 연결 중...")
        wlan.connect(SSID, PASSWORD)
        timeout = 10
        while not wlan.isconnected() and timeout > 0:
            time.sleep(1)
            timeout -= 1
            print(".", end="")
    if wlan.isconnected():
        print("\nWi-Fi 연결 성공! IP:", wlan.ifconfig()[0])
        return True
    return False


ina_0x40 = INA226(address=0x40)
ina_0x41 = INA226(address=0x41)

try:
    ina_0x40.configure(avg=4, busConvTime=4, shuntConvTime=4, mode=7)
    ina_0x41.configure(avg=4, busConvTime=4, shuntConvTime=4, mode=7)
    ina_0x40.calibrate(rShuntValue=0.1, iMaxExpected=2.0)
    ina_0x41.calibrate(rShuntValue=0.1, iMaxExpected=2.0)
except Exception as e:
    print("Init Error:", e)

if connect_wifi():
    while True:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((SERVER_IP, SERVER_PORT))
            while True:
                v1 = ina_0x40.read_bus_voltage()
                ma1 = ina_0x40.read_shunt_current()
                v2 = ina_0x41.read_bus_voltage()
                ma2 = ina_0x41.read_shunt_current()

                if None not in (v1, ma1, v2, ma2):
                    msg = f"{v1:.3f},{ma1*1000:.1f},{v2:.3f},{ma2*1000:.1f}\n"
                    s.send(msg.encode("utf-8"))
                time.sleep(0.5)
        except Exception as e:
            print("Error:", e)
            if "s" in locals():
                s.close()
            time.sleep(2)
            if not network.WLAN(network.STA_IF).isconnected():
                connect_wifi()
