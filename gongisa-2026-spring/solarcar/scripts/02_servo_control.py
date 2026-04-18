from myservo import myServo
import time

servo = myServo(pin=10)

while True:
    servo.myServoWriteAngle(45, 10)  # (angle, speed)
    time.sleep(1)
    servo.myServoWriteAngle(90, 10)
    time.sleep(1)
    servo.myServoWriteAngle(135, 10)
    time.sleep(1)
    servo.myServoWriteAngle(90, 10)
    time.sleep(1)
