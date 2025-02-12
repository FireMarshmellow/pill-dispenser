# DIY Pill Dispenser

This is a 3D Printable DIY pill dispenser, and the Models and Schematics are available for purchase [purchase on my Patreon](https://www.patreon.com/Mellow_labs/shop/pill-dispenser-files-104871?utm_medium=clipboard_copy&utm_source=copyLink&utm_campaign=productshare_creator&utm_content=join_link)

## Project video
[![Video Title](https://img.youtube.com/vi/1kCoDDYpgkE/0.jpg)](https://www.youtube.com/watch?v=1kCoDDYpgkE)

## Bill of Materials

- **Piezo Disc Sensor**
- **4 x  N20 Gear Reducer Motor 6V**
- **Real Time Clock RTC DS1307 I2C Module**
- **DRV8833 2 Channel DC Motor Driver Module**
- **Beetle ESP32 C6**
- **Resistors(1x 1M ohm)(1x 10K ohm)**
- **8 x self tapping screws (9mm)**
- **8 x M3 screws and nuts (12mm)**

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

## Prerequisites

- A computer running Windows, macOS, or Linux.
- A working internet connection.
- An ESP32 board (Beetle ESP32 C6 recommended for this project).
- Basic familiarity with installing software and following step-by-step instructions.
- Your own electronic components as listed in the Bill of Materials.

---

## Step 1: Installing the Arduino IDE

1. **Download the Arduino IDE:**  
   Visit the [Arduino Software page](https://www.arduino.cc/en/software) and choose the version for your operating system.

2. **Install the Arduino IDE:**  
   - **Windows:** Run the installer and follow the on-screen prompts.  
   - **macOS:** Open the downloaded `.dmg` file and drag the Arduino IDE into your Applications folder.  
   - **Linux:** Follow your distribution’s instructions (typically unpacking a tarball and running an install script).

3. **Launch the Arduino IDE:**  
   Once installed, open the IDE to confirm that everything works correctly.

---

## Step 2: Downloading the Repository

1. **Visit the GitHub Repository:**  
   Navigate to the project’s GitHub page.

2. **Download the ZIP File:**  
   - Click the green **Code** button and select **Download ZIP**.  
   - Save the file to your computer.

3. **Extract the Files:**  
   Unzip the downloaded file into your Arduino projects folder. The folder should include the main Arduino sketch (e.g., `PillDispenser.ino`) and a `data` folder if needed.

---

## Step 3: Opening and Configuring the Project

1. **Open the Arduino IDE:**  
   Launch the IDE if it isn’t already running.

2. **Open the Project File:**  
   - Go to **File > Open** and locate the `.ino` file inside your extracted folder.

3. **Select Your Board and Port:**  
   - In **Tools > Board**, choose your ESP32 board (for example, Beetle ESP32 C6).  
   - Then, in **Tools > Port**, select the COM port corresponding to your board.

---

## Step 4: Updating WiFi Credentials and Uploading

1. **Update the WiFi Settings:**  
   Open the `.ino` file and find the section at the top where WiFi credentials are defined. Update the `ssid` and `password` variables with your network’s details:
   ```cpp
   const char* ssid = "your_SSID";
   const char* password = "your_PASSWORD";
   ```

2. **Upload the Sketch:**  
   Click the **Upload** button in the Arduino IDE. Once the code compiles, the sketch will be uploaded to your board.

3. **Upload the Data Folder (if applicable):**  
   If your project requires additional files (e.g., for LittleFS), use the Arduino IDE’s data upload tool to transfer the contents of the `data` folder.

---

## Troubleshooting

- **IDE Issues:**  
  Make sure you’re using the latest Arduino IDE version and have updated your board definitions.

- **COM Port Connectivity:**  
  Verify that the proper drivers are installed for your ESP32 board. A computer restart might help if connectivity issues persist.

- **Library Errors:**  
  Ensure all required libraries are installed via the Library Manager in the Arduino IDE.

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