# DIY Pill Dispenser

This is a 3D Printable DIY pill dispenser, and the Models and Schematics are available for purchase [purchase on my Patreon](https://www.patreon.com/Mellow_labs/shop/pill-dispenser-files-104871?utm_medium=clipboard_copy&utm_source=copyLink&utm_campaign=productshare_creator&utm_content=join_link)

## Project video
*(wip! Embedded video here.)*

## Project Description

This project provides a 3D printed pill dispenser that utilizes a vibration sensor, a mini gear motor, a real-time clock, a motor driver, and an ESP32 board to control the dispensing mechanism. A detailed video on my main channel goes over everything you need to know about building this project.

## Bill of Materials

- **Piezo Disc Sensor**
- **Micro Mini Gear Motor DC**
- **Real Time Clock RTC DS1307 I2C Module**
- **DRV8833 2 Channel DC Motor Driver Module**
- **Beetle ESP32 C6**
- **Resistors(1x 1M ohm)(1x 10K ohm)**

---

## Required Libraries

This project requires the following Arduino libraries:

- **WiFi.h, WebServer.h:** For network connectivity and HTTP server.
- **FS.h, LittleFS.h:** For file system operations.
- **Wire.h:** For I2C communication (used with RTC).
- **uRTCLib.h:** For RTC (DS1307) control.
- **ArduinoJson.h:** For parsing JSON configuration files.
- **time.h:** For working with system time and NTP.

Make sure these libraries are installed in your Arduino IDE before compiling the project.

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Step 1: Install the Arduino IDE](#step-1-install-the-arduino-ide)
- [Step 2: Install the ESP32 LittleFS Uploader](#step-2-install-the-esp32-littlefs-uploader)
- [Step 3: Download the Repository ZIP](#step-3-download-the-repository-zip)
- [Step 4: Open and Configure the Project](#step-4-open-and-configure-the-project)
- [Step 5: Upload the Sketch and Data Folder](#step-5-upload-the-sketch-and-data-folder)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## Prerequisites

- A computer running Windows, macOS, or Linux.
- A working internet connection.
- An ESP32 board (Beetle ESP32 C6 recommended for this project).
- Basic familiarity with installing software and following step-by-step instructions.
- Your own electronic components as listed in the Bill of Materials.

---

## Step 1: Install the Arduino IDE

1. **Download the Arduino IDE**  
   Visit the [Arduino Software page](https://www.arduino.cc/en/software) and download the version appropriate for your operating system.

2. **Install the Arduino IDE**  
   - **Windows:** Run the downloaded installer and follow the on-screen instructions.  
   - **macOS:** Open the downloaded `.dmg` file and drag the Arduino IDE into your Applications folder.  
   - **Linux:** Follow your distribution’s installation instructions (usually involving unpacking the tarball and running an installer script).

3. **Launch the Arduino IDE**  
   Open the Arduino IDE to ensure it is installed correctly.

---

## Step 2: Install the ESP32 LittleFS Uploader

1. **Follow the Tutorial**  
   To work with the LittleFS filesystem on the ESP32, follow the detailed steps in this tutorial:  
   [Arduino IDE 2: Install ESP32 LittleFS](https://randomnerdtutorials.com/arduino-ide-2-install-esp32-littlefs/).

2. **Installation Overview**  
   The tutorial will guide you through:  
   - Downloading the LittleFS Uploader tool.
   - Placing the tool in the correct Arduino tools directory.
   - Verifying the installation within the Arduino IDE.

*Ensure that you follow all the instructions carefully, as some steps may vary depending on your operating system or Arduino IDE version.*

---

## Step 3: Download the Repository ZIP

1. **Visit the GitHub Repository Page**  
   Navigate to the GitHub page for this project.

2. **Download the ZIP File**  
   - Click on the green **Code** button.
   - Select **Download ZIP** from the dropdown menu.
   - Save the ZIP file to a location on your computer.

3. **Extract the ZIP File**  
   - Locate the downloaded ZIP file.
   - Extract the contents into your Arduino projects folder.
   - Confirm that the extracted folder contains the Arduino sketch (e.g., `PillDispenser.ino`) and a `data` folder if required by your project.

---

## Step 4: Open and Configure the Project

1. **Launch the Arduino IDE**  
   Open the Arduino IDE if it isn’t already running.

2. **Open the Project**  
   - Click on **File > Open**.
   - Navigate to the extracted project folder and select the `.ino` file.

3. **Select the Correct Board and Port**  
   - Go to **Tools > Board** and choose your ESP32 board (e.g., Beetle ESP32 C6).
   - Then, go to **Tools > Port** and select the COM port corresponding to your board.

---

## Step 5: Upload the Sketch and Data Folder

1. **Upload the Sketch**  
   - Click the **Upload** button in the Arduino IDE.
   - Wait for the code to compile and upload to your board.

2. **Upload the Data Folder Using LittleFS Uploader**  
   - With your board connected, go to **Tools > ESP32 LittleFS Upload** (this option will appear after installing the uploader tool).
   - Click this option to start uploading the contents of your `data` folder to the board's filesystem.
   - Verify that the upload completes successfully. If errors occur, refer to the troubleshooting section.

---

## Troubleshooting

- **Arduino IDE Issues:**  
  Ensure you have the latest version of the Arduino IDE and that your board definitions are updated.

- **LittleFS Uploader Not Appearing:**  
  Double-check that the uploader tool was placed in the correct directory, and consult the [tutorial](https://randomnerdtutorials.com/arduino-ide-2-install-esp32-littlefs/) for additional guidance.

- **COM Port Problems:**  
  Confirm that the drivers for your ESP32 board are installed. Restarting your computer may help resolve connectivity issues.

---

## License

```
Copyright © [2025] [Tomasz Burzy]

All Rights Reserved. The contents of this repository,
including but not limited to design files, code, and documentation,
are proprietary and provided for personal use only. 
No part of this repository may be reproduced, distributed,
or transmitted in any form or by any means without the prior written
permission of the copyright owner.
```
---