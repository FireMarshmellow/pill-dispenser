#   __  __      _ _               ______ _
#  |  \/  |    | | |             |  ____(_)
#  | \  / | ___| | | _____      _| |__   _ _ __ ___
#  | |\/| |/ _ \ | |/ _ \ \ /\ / /  __| | | '__/ _ \
#  | |  | |  __/ | | (_) \ V  V /| |    | | | |  __/
#  |_|  |_|\___|_|_|\___/ \_/\_/ |_|    |_|_|  \___|
#  https://www.mellowfire.com/

# Change this if the time I'm out put is wrong.
timezone = +1

# please enter at what time you'd like which container to release pills.
# this works on a 24-hour clock if you require two of the same pill please enter the time twice.
# note if it is a single digit time do not put a 0 in front of it, for example 9:05 a.m. correct:(9:5) wrong:(09:05).
# if you wanted to using the container just leave the list empty.
contaner1 = ["7:0"]
contaner2 = ["7:0"]
contaner3 = []
contaner4 = []

# fill in the GPIO pins numbers for the servos and vibration sensor.
senserpin = 14  # D5 = GPIO 14

servo1pin = 5  # D1 = GPIO 5
servo2pin = 4  # D2 = GPIO 4
servo3pin = 12  # D6 = GPIO 12
servo4pin = 15  # D8 = GPIO 15
