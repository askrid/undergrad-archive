import time
from machine import Pin, PWM


class myServo(object):
    def __init__(self, pin: int = 15, hz: int = 50):
        self._servo = PWM(Pin(pin), hz)
        self.current_angle = 90.0
        # 초기화 시에는 내부 함수를 써서 즉시 정중앙(90도)으로 맞춥니다.
        self._write_angle(self.current_angle)

    def myServoWriteDuty(self, duty):
        if duty <= 26:
            duty = 26
        if duty >= 128:
            duty = 128
        self._servo.duty(duty)

    # --- [내부 전용 함수] 실제 하드웨어 각도를 세팅하는 로직 ---
    # 파이썬에서 이름 앞에 '_'를 붙이면 내부에서만 쓴다는 의미입니다.
    def _write_angle(self, pos):
        if pos <= 0:
            pos = 0
        if pos >= 180:
            pos = 180
        pos_buffer = (pos / 180.0) * (128 - 26)
        self._servo.duty(int(pos_buffer) + 26)
        self.current_angle = float(pos)

    def myServoWriteTime(self, us):
        if us <= 500:
            us = 500
        if us >= 2500:
            us = 2500
        pos_buffer = (1024 * us) / 20000
        self._servo.duty(int(pos_buffer))

    # --- [통합된 외부용 함수] speed 인자를 기본값 50으로 받음 ---
    def myServoWriteAngle(self, target_pos, speed=50):
        if target_pos < 0:
            target_pos = 0
        if target_pos > 180:
            target_pos = 180

        # 속도 범위 제한 (1~100)
        if speed < 1:
            speed = 1
        if speed > 100:
            speed = 100

        # speed(1~100)를 한 번에 움직일 '각도 량(step)'으로 변환
        step_size = 0.8 + ((speed - 1) / 99.0) * 4.2
        update_interval = 15

        direction = 1 if target_pos > self.current_angle else -1

        while True:
            # 소수점 단위로 다음 각도 계산
            next_angle = self.current_angle + (direction * step_size)

            # 목표 지점에 도달했거나 넘어섰는지 확인
            if (direction == 1 and next_angle >= target_pos) or (
                direction == -1 and next_angle <= target_pos
            ):
                self._write_angle(target_pos)  # 마지막으로 정확한 목표치에 세팅
                break

            # 잘게 쪼개진 각도로 모터 갱신 (내부 함수 호출!)
            self._write_angle(next_angle)
            time.sleep_ms(update_interval)

    def deinit(self):
        self._servo.deinit()
