from time import sleep
import utime
import machine

Servo1 = machine.PWM(machine.Pin(5),freq=40)
Servo2 = machine.PWM(machine.Pin(4),freq=40)
Servo3 = machine.PWM(machine.Pin(12),freq=40)
Servo4 = machine.PWM(machine.Pin(15),freq=40)
Senser = machine.Pin(13, machine.Pin.IN, machine.Pin.PULL_UP)

#servos 15-107
def Contaner1():
    print('contaner1')
    Servo1.duty(15)
    sleep(0.5)
    Servo1.duty(100)

def Contaner2():
    print('contaner2')
    Servo2.duty(15)
    sleep(0.5)
    Servo2.duty(100)

def Contaner3():
    print('contaner3')
    Servo3.duty(15)
    sleep(0.5)
    Servo3.duty(100)
    
def Contaner4():
    print('contaner4')
    Servo4.duty(15)
    sleep(0.5)
    Servo4.duty(100)


