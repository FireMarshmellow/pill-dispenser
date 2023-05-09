#   __  __      _ _               ______ _
#  |  \/  |    | | |             |  ____(_)
#  | \  / | ___| | | _____      _| |__   _ _ __ ___
#  | |\/| |/ _ \ | |/ _ \ \ /\ / /  __| | | '__/ _ \
#  | |  | |  __/ | | (_) \ V  V /| |    | | | |  __/
#  |_|  |_|\___|_|_|\___/ \_/\_/ |_|    |_|_|  \___|
#  https://www.mellowfire.com/
import ntptime
import time
import re
import config
import machine

ntptime.settime()
#LED = machine.Pin(15, machine.Pin.OUT)#debug

# gets the date and time from network Time protocol,
# and filters it down to just hours and minutes.
def Whats_the_time():
    time.localtime()
    UTC_OFFSET = config.timezone * 60 * 60  # change the '-4' according to your timezone
    actual_time = time.localtime(time.time() + UTC_OFFSET)
    Time_clean = (
        re.sub("[(){}]", "", str(actual_time[3:5])).replace(",", ":").replace(" ", "")
    )
    return Time_clean


# this goes through the list of times in in config,
# and dispensers the pill at the right time.
def check_time(time):
    for i in config.contaner1:
        if i == time:
            Contener(config.servo1pin)
    for i in config.contaner2:
        if i == time:
            Contener(config.servo2pin)
    for i in config.contaner3:
        if i == time:
            Contener(config.servo3pin)
    for i in config.contaner4:
        if i == time:
            Contener(config.servo4pin)


# This test each servo individually,
# note: the server will not stop unless the vibration sensor is triggered.
def test_run():  # debug
    Contener(config.servo1pin)
    Contener(config.servo2pin)
    Contener(config.servo3pin)
    Contener(config.servo4pin)


# this function will get the required gpio pins from the config file and used them to run the Servos with predetermined position and time intervals.
# in case of Change remember Servos need time to move from position A to B.
# This should only need changing if you are using different servos.
def Contener(x):
    time.sleep(2)  # time needed for vibration sensor to turn off
    #print("Contener" + str(x))  # debug
    Servo = machine.PWM(machine.Pin(int(x)), freq=40)
    Senser = machine.Pin(config.senserpin, machine.Pin.IN)
    while int(Senser.value()) == 0:
        Servo.duty(100)
        time.sleep(0.5)
        Servo.duty(40)
        time.sleep(0.3)
        Servo.duty(30)
        time.sleep(0.3)
        Servo.duty(25)
        time.sleep(0.3)
        Servo.duty(20)
        time.sleep(0.3)
        Servo.duty(15)
        time.sleep(0.5)
        Servo.duty(0)
        
    machine.Pin(int(x), machine.Pin.IN)


# This loop will check the list against the time every minute,
# note: if it's set to less than a minute it will go through
# the list twice with the same time dispensing more pills then required.
while True:
    # test_run()# debug
    #print(Whats_the_time())# debug
    check_time(Whats_the_time())
    #LED.on()# debug
    time.sleep(10)
    #LED.off()# debug
    time.sleep(50)
    # machine.reset()
