from machine import Pin, PWM


class DRV8833:
    # 매개변수에 기본값을 할당합니다 (in1=0, in2=1, in3=4, in4=3)
    def __init__(self, in1=0, in2=1, in3=4, in4=3, freq=1000):
        # 모터 A (왼쪽) 핀 설정
        self.motorA_in1 = PWM(Pin(in1), freq=freq)
        self.motorA_in2 = PWM(Pin(in2), freq=freq)
        # 모터 B (오른쪽) 핀 설정
        self.motorB_in3 = PWM(Pin(in3), freq=freq)
        self.motorB_in4 = PWM(Pin(in4), freq=freq)

        # 초기 상태: 정지
        self.stop()

    def _calculate_duty(self, speed):
        abs_speed = abs(speed)
        if abs_speed > 100:
            abs_speed = 100
        # ESP32/MicroPython 표준 10비트 PWM 범위: 0 ~ 1023
        return int((abs_speed / 100) * 1023)

    def set_speed(self, left_speed, right_speed):
        # 왼쪽 모터 제어
        l_duty = self._calculate_duty(left_speed)
        if left_speed > 0:
            self.motorA_in1.duty(l_duty)
            self.motorA_in2.duty(0)
        elif left_speed < 0:
            self.motorA_in1.duty(0)
            self.motorA_in2.duty(l_duty)
        else:
            self.motorA_in1.duty(0)
            self.motorA_in2.duty(0)

        # 오른쪽 모터 제어
        r_duty = self._calculate_duty(right_speed)
        if right_speed > 0:
            self.motorB_in3.duty(r_duty)
            self.motorB_in4.duty(0)
        elif right_speed < 0:
            self.motorB_in3.duty(0)
            self.motorB_in4.duty(r_duty)
        else:
            self.motorB_in3.duty(0)
            self.motorB_in4.duty(0)

    def stop(self):
        self.set_speed(0, 0)

    def deinit(self):
        self.stop()
        self.motorA_in1.deinit()
        self.motorA_in2.deinit()
        self.motorB_in3.deinit()
        self.motorB_in4.deinit()
