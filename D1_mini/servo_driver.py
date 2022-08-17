from time import sleep
import machine
import config

# this function will get the required gpio pins from the config file and used them to run the Servos with predetermined position and time intervals.
# in case of Change remember Servos need time to move from position A to position B.
def Contener(x):
    sleep(2)  # time needed for vibration sensor to turn off
    print("Contener" + str(x))
    Servo = machine.PWM(machine.Pin(int(x)), freq=40)
    Senser = machine.Pin(config.senserpin, machine.Pin.IN)
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
        sleep(0.5)
