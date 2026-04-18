import sys

sys.path.insert(0, "/core")

import time
import network
import socket

from core.drv8833 import DRV8833
from core.mux04 import LineSensor
from core.ina226 import INA226
from core.myservo import myServo

# ============================================================
# CONFIGURATION
# ============================================================

# -- Line following (PD controller) --
KP = 1.2
KD = 0.8
WEIGHTS = [-100, -30, -20, -10, 10, 20, 30, 100]

# -- Speed (0-100 scale) --
SPEED_HIGH = 100
SPEED_NORMAL = 100
SPEED_LOW = 100
SPEED_CRITICAL = 75
MIN_SPEED = 45  # motor dead zone threshold

# -- Battery thresholds (under load) --
V_NORMAL = 3.3  # above: high speed
V_LOW = 3.0  # above: normal speed
V_CRITICAL = 2.8  # above: low speed, below: critical

# -- Charging --
# When motor stops, voltage rises 0.2-0.3V due to no-load.
# All charging voltage comparisons use this offset.
V_STOP_OFFSET = 0.3
V_CHARGE_TRIGGER = 3.0  # trigger charging below this (under load)
V_CHARGE_TARGET = 3.1  # charge target (under load)
CHARGE_MAX_SEC = 120
CHARGE_MIN_SEC = 12

# -- Servo (solar panel) --
SERVO_SCAN_STEP = 5
SERVO_SCAN_INTERVAL_MS = 50

# -- Competition --
COMPETITION_DURATION_MS = 20 * 60 * 1000

# -- Intervals --
ENERGY_INTERVAL_MS = 1000
TELEMETRY_INTERVAL_MS = 500

# -- Line-loss recovery (iterations, ~10ms each) --
LINE_LOST_TURN_ITERS = 50
LINE_LOST_SPIN_ITERS = 150

# -- Telemetry --
SSID = "Team 03"
PASSWORD = "03030303"
PC_IP = "192.168.137.1"
PC_PORT = 5005

# -- States --
STATE_DRIVING = "DRIVING"
STATE_CHARGING = "CHARGING"

# ============================================================
# Helpers
# ============================================================


def drive(left, right):
    def fix(v):
        if v > 100:
            return 100
        if v < -100:
            return -100
        if 0 < v < MIN_SPEED:
            return MIN_SPEED
        if -MIN_SPEED < v < 0:
            return -MIN_SPEED
        return v

    motor.set_speed(fix(left), fix(right))


def read_ina(ina):
    v = ina.read_bus_voltage()
    i = ina.read_shunt_current()
    return (v or 0, (i or 0) * 1000)  # (volts, milliamps)


def start_wifi():
    """Start WiFi connection without blocking. check_wifi() opens socket later."""
    global wlan
    wlan = network.WLAN(network.STA_IF)
    wlan.active(False)
    time.sleep(0.1)
    wlan.active(True)
    try:
        wlan.config(txpower=10)
    except Exception:
        pass
    if not wlan.isconnected():
        print(f"{SSID} connecting (async)...")
        wlan.connect(SSID, PASSWORD)


def check_wifi():
    """Open socket once WiFi connects. Called from main loop."""
    global sock
    if sock or not wlan.isconnected():
        return
    print("WiFi OK:", wlan.ifconfig()[0])
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


# ============================================================
# Telemetry
# ============================================================

charge_elapsed_sec = 0
charge_total_sec = 0


def send_telemetry():
    if not sock:
        return
    try:
        comp_e = time.ticks_diff(time.ticks_ms(), competition_start) // 1000
        comp_r = max(0, COMPETITION_DURATION_MS // 1000 - comp_e)
        msg = (
            f"{solar_v:.3f},{solar_ma:.1f},"
            f"{motor_v:.3f},{motor_ma:.1f},"
            f"{base_speed:.0f},"
            f"{''.join(str(c) for c in channels)},"
            f"{state},{comp_e},{comp_r},"
            f"{charge_elapsed_sec},{charge_total_sec}"
        )
        sock.sendto(msg.encode(), (PC_IP, PC_PORT))
    except Exception:
        pass


# ============================================================
# Charging
# ============================================================


def calc_charge_duration(voltage, remaining_ms):
    voltage_gap = V_CHARGE_TARGET - voltage
    if voltage_gap <= 0:
        return 0

    base_sec = (voltage_gap / (V_CHARGE_TARGET - V_CRITICAL)) * CHARGE_MAX_SEC

    remaining_sec = remaining_ms / 1000
    if remaining_sec < 120:
        return 0
    elif remaining_sec < 300:
        base_sec *= (remaining_sec - 120) / 180

    return max(CHARGE_MIN_SEC, min(CHARGE_MAX_SEC, int(base_sec)))


def find_best_solar_angle():
    best_angle = 90
    best_power = 0.0
    for angle in range(0, 181, SERVO_SCAN_STEP):
        servo._write_angle(angle)
        time.sleep_ms(SERVO_SCAN_INTERVAL_MS)
        try:
            v, ma = read_ina(ina_0x40)
            p = v * abs(ma / 1000)
            if p > best_power:
                best_power = p
                best_angle = angle
        except Exception:
            pass
    servo._write_angle(best_angle)
    print(f"Best angle: {best_angle}deg {best_power:.4f}W")
    return best_angle


def update_sensors():
    global solar_v, solar_ma, motor_v, motor_ma
    try:
        solar_v, solar_ma = read_ina(ina_0x40)
    except Exception:
        pass
    try:
        motor_v, motor_ma = read_ina(ina_0x41)
    except Exception:
        pass


def do_charging(remaining_ms):
    global state, charge_elapsed_sec, charge_total_sec

    state = STATE_CHARGING
    motor.stop()

    # Wait briefly for voltage to settle after motor stop
    time.sleep_ms(300)
    update_sensors()
    print(f"CHARGING: motor_v={motor_v:.2f}V (no-load)")

    # Check if charging is worthwhile before expensive solar sweep
    load_v = motor_v - V_STOP_OFFSET
    charge_sec = calc_charge_duration(load_v, remaining_ms)
    if charge_sec == 0:
        print("CHARGING: skipped")
        _exit_charging()
        return

    find_best_solar_angle()

    # Target voltage at rest = under-load target + offset
    rest_target = V_CHARGE_TARGET + V_STOP_OFFSET
    start_v = motor_v
    charge_total_sec = charge_sec
    print(f"CHARGING: {charge_sec}s | {start_v:.2f}V -> {rest_target:.2f}V (rest)")

    charge_start = time.ticks_ms()
    while True:
        elapsed_ms = time.ticks_diff(time.ticks_ms(), charge_start)
        charge_elapsed_sec = elapsed_ms // 1000

        update_sensors()
        gained = motor_v - start_v
        left = charge_sec - charge_elapsed_sec
        print(
            f"  [{charge_elapsed_sec}/{charge_sec}s left:{left}s] "
            f"bat:{motor_v:.2f}V (+{gained:.3f}V) "
            f"solar:{solar_v:.2f}V {solar_ma:.0f}mA"
        )
        send_telemetry()

        if motor_v >= rest_target:
            print(
                f"CHARGE DONE: target reached {motor_v:.2f}V (+{gained:.3f}V in {charge_elapsed_sec}s)"
            )
            break
        if elapsed_ms >= charge_sec * 1000:
            print(
                f"CHARGE DONE: timeout {charge_sec}s bat={motor_v:.2f}V (+{gained:.3f}V)"
            )
            break

        time.sleep_ms(1000)

    _exit_charging()


def _exit_charging():
    global state, charge_elapsed_sec, charge_total_sec
    charge_elapsed_sec = 0
    charge_total_sec = 0
    state = STATE_DRIVING
    # Drive forward to clear the horizontal charge line
    drive(60, 60)
    time.sleep_ms(500)


# ============================================================
# Hardware init
# ============================================================

ina_0x40 = INA226(address=0x40)
ina_0x41 = INA226(address=0x41)
try:
    for ina in (ina_0x40, ina_0x41):
        ina.configure(avg=4, busConvTime=4, shuntConvTime=4, mode=7)
        ina.calibrate(rShuntValue=0.1, iMaxExpected=2.0)
    print("INA226 OK")
except Exception as e:
    print("INA226 init error:", e)

sensor = LineSensor()
motor = DRV8833()
servo = myServo(pin=10)

wlan = None
sock = None
start_wifi()

# ============================================================
# State
# ============================================================

prev_error = 0.0
last_known_error = 0.0
line_lost_count = 0
base_speed = SPEED_NORMAL
state = STATE_DRIVING

solar_v = 0.0
solar_ma = 0.0
motor_v = 3.6
motor_ma = 0.0
channels = [0] * 8

last_energy_time = time.ticks_ms()
last_telemetry_time = time.ticks_ms()
last_charge_time = 0
competition_start = time.ticks_ms()

# ============================================================
# Main loop
# ============================================================

print("Starting competition run")

try:
    while True:
        now = time.ticks_ms()
        remaining_ms = COMPETITION_DURATION_MS - time.ticks_diff(now, competition_start)
        if remaining_ms <= 0:
            print("Competition time up!")
            break

        # --- READ LINE SENSOR ---
        channels = sensor.read_channels()
        channels = [1 - c for c in channels]
        active_count = 0
        error_sum = 0
        for i in range(8):
            if channels[i] == 1:
                error_sum += WEIGHTS[i]
                active_count += 1

        # --- CHARGING TRIGGER ---
        if active_count == 8 and motor_v < V_CHARGE_TRIGGER:
            do_charging(remaining_ms)
            last_charge_time = time.ticks_ms()
            prev_error = 0.0
            last_known_error = 0.0
            line_lost_count = 0
            continue

        # --- LINE FOLLOWING ---
        if active_count > 0:
            error = error_sum / active_count
            derivative = error - prev_error
            prev_error = error
            last_known_error = error
            line_lost_count = 0

            abs_err = abs(error)
            scale = 1.0 + max(0, abs_err - 50) / 50.0
            steering = (KP * error + KD * derivative) * scale
            speed = max(MIN_SPEED, base_speed * max(0.3, 1.0 - abs_err / 100.0))
            drive(speed + steering, speed - steering)

        # --- LINE LOST RECOVERY ---
        else:
            line_lost_count += 1
            d = 1 if last_known_error >= 0 else -1
            if line_lost_count < LINE_LOST_TURN_ITERS:
                drive(60 if d > 0 else 0, 0 if d > 0 else 60)
            elif line_lost_count < LINE_LOST_SPIN_ITERS:
                drive(55 * d, -50 * d)
            else:
                drive(50 * d, -50 * d)

        # --- ENERGY / SPEED ADAPTATION ---
        if time.ticks_diff(now, last_energy_time) > ENERGY_INTERVAL_MS:
            update_sensors()
            if motor_v > V_NORMAL:
                target = SPEED_HIGH
            elif motor_v > V_LOW:
                target = SPEED_NORMAL
            elif motor_v > V_CRITICAL:
                target = SPEED_LOW
            else:
                target = SPEED_CRITICAL
            base_speed = max(MIN_SPEED, base_speed + 0.3 * (target - base_speed))
            last_energy_time = now

        # --- TELEMETRY ---
        if time.ticks_diff(now, last_telemetry_time) > TELEMETRY_INTERVAL_MS:
            check_wifi()
            send_telemetry()
            last_telemetry_time = now

except KeyboardInterrupt:
    pass
finally:
    print("Stopping.")
    motor.stop()
    if sock:
        sock.close()
