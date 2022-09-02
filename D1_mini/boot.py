import network, re
import time
import config
import ntptime
import machine

#   __  __      _ _               ______ _
#  |  \/  |    | | |             |  ____(_)
#  | \  / | ___| | | _____      _| |__   _ _ __ ___
#  | |\/| |/ _ \ | |/ _ \ \ /\ / /  __| | | '__/ _ \
#  | |  | |  __/ | | (_) \ V  V /| |    | | | |  __/
#  |_|  |_|\___|_|_|\___/ \_/\_/ |_|    |_|_|  \___|
#  https://www.mellowfire.com/
global wlan
wlan = None

# this connect to the Wi-Fi network to get accurate a time.
def connectWifi(ssid, passwd):
    global wlan
    cnt = 0
    print("Start Wifi.")
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    print("Connect Wifi.", end="")
    wlan.disconnect()
    wlan.connect(ssid, passwd)
    while not wlan.isconnected():
        time.sleep(0.5)
        print(".", end="")
    print(" OK")
    print("IP ADDR:", wlan.ifconfig()[0])
    return True


connectWifi(config.ssid, config.password)

# this gets the date and time from network Time protocol,
# and filters it down to just hours and minutes.
def Whats_the_time():
    rtc = machine.RTC()
    ntptime.host = "time1.google.com"
    ntptime.NTP_DELTA = 3155673600 - ((config.timezone) * 3600)
    ntptime.settime()
    Time_clean = (
        re.sub("[(){}]", "", str(rtc.datetime()[4:6]))
        .replace(",", ":")
        .replace(" ", "")
    )
    return Time_clean


# this goes through the list of times in in config,
# and dispensers the pill at the right time.
def check_time(time):
    for i in config.contaner1:
        if i == time:
            print("contaner_1")
            Contener(config.servo1pin)
    for i in config.contaner2:
        if i == time:
            print("contaner_2")
            Contener(config.servo2pin)
    for i in config.contaner3:
        if i == time:
            print("contaner_3")
            Contener(config.servo3pin)
    for i in config.contaner4:
        if i == time:
            print("contaner_4")
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
    # print("Contener" + str(x)) #debug
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


# This loop will check the list against the time every minute,
# note: if it's set to less than a minute it will go through
# the list twice with the same time dispensing more pills then required.
while True:
    # test_run() # debug
    print(Whats_the_time())  # debug
    check_time(Whats_the_time())
    time.sleep(60)
    machine.reset()
