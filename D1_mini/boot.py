import network, re
import time
import config
import servo_driver

#   __  __      _ _               ______ _
#  |  \/  |    | | |             |  ____(_)
#  | \  / | ___| | | _____      _| |__   _ _ __ ___
#  | |\/| |/ _ \ | |/ _ \ \ /\ / /  __| | | '__/ _ \
#  | |  | |  __/ | | (_) \ V  V /| |    | | | |  __/
#  |_|  |_|\___|_|_|\___/ \_/\_/ |_|    |_|_|  \___|
#  https://www.mellowfire.com/

from machine import RTC
import ntptime


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
    rtc = RTC()
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
            servo_driver.Contener(config.servo1pin)
    for i in config.contaner2:
        if i == time:
            print("contaner_2")
            servo_driver.Contener(config.servo2pin)
    for i in config.contaner3:
        if i == time:
            print("contaner_3")
            servo_driver.Contener(config.servo3pin)
    for i in config.contaner4:
        if i == time:
            print("contaner_4")
            servo_driver.Contener(config.servo4pin)


# This test each servo individually,
# note: the server will not stop unless the vibration sensor is triggered.
def test_run():
    servo_driver.Contener(config.servo1pin)
    servo_driver.Contener(config.servo2pin)
    servo_driver.Contener(config.servo3pin)
    servo_driver.Contener(config.servo4pin)


# This loop will check the list against the time every minute,
# note: if it's set to less than a minute it will go through
# the list twice with the same time dispensing more pills then required.
while True:
    # test_run()
    print(Whats_the_time())
    check_time(Whats_the_time())
    time.sleep(60)
