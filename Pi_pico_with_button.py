from machine import Pin, PWM, ADC,
from time import sleep
import utime

button0 = Pin(6, Pin.IN, Pin.PULL_DOWN)
Vibe_check = Pin(16, Pin.IN, Pin.PULL_DOWN)

servo0Pin = PWM(Pin(17))
servo0Pin.freq(40)

servo1Pin = PWM(Pin(18))
servo1Pin.freq(40)

servo2Pin = PWM(Pin(19))
servo2Pin.freq(40)

def dropcheck():
    if int(Vibe_check.value()) == 1:
        print('yes')
        return True
    else:
        print('no')
        return False

def servo0():
    print('servo0')
    servo0Pin.duty_u16(6866)
    sleep(1)
    servo0Pin.duty_u16(2333)
    sleep(1)
    servo0Pin.duty_u16(1700)
    sleep(1)
    servo0Pin.duty_u16(1000)
    sleep(0.6)
    servo0Pin.duty_u16(0)
    if dropcheck() == False:
        servo0()
    else:
        return

def servo1():
    print('servo1')
    servo1Pin.duty_u16(6866)
    sleep(1)
    servo1Pin.duty_u16(2333)
    sleep(1)
    servo1Pin.duty_u16(1700)
    sleep(1)
    servo1Pin.duty_u16(1000)
    sleep(0.6)
    servo1Pin.duty_u16(0)
    if dropcheck() == False:
        servo1()
    else:
        return

def servo2():
    print('servo2')
    servo2Pin.duty_u16(6866)
    sleep(1)
    servo2Pin.duty_u16(2333)
    sleep(1)
    servo2Pin.duty_u16(1700)
    sleep(1)
    servo2Pin.duty_u16(1000)
    sleep(0.6)
    servo2Pin.duty_u16(0)
    if dropcheck() == False:
        servo2()
    else:
        return

while True:
    if button0.value():
       servo0()
       servo1()
       servo2()
    sleep(0.5)
 
