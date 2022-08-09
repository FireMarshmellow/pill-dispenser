from utime import sleep
import machine

def Contener(x): #x = pin num
    print("Contener" + str(x))
    Servo = machine.PWM(machine.Pin(int(x)),freq=40)
    Senser = machine.Pin(14, machine.Pin.IN) #D5
    while int(Senser.value()) == 0:
        Servo.duty(100)
        sleep(0.5)
        Servo.duty(40)
        sleep(0.3)
        Servo.duty(30)
        sleep(0.3)
        Servo.duty(25)
        sleep(0.3)
        Servo.duty(20)
        sleep(0.3)
        Servo.duty(15)
        sleep(0.3)