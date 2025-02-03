/**************************************************************************
 * Required libraries:
 *   - WiFi.h, WebServer.h: For network connectivity and HTTP server.
 *   - FS.h, LittleFS.h: For file system operations.
 *   - Wire.h: For I2C communication (used with RTC).
 *   - uRTCLib.h: For RTC (DS1307) control.
 *   - ArduinoJson.h: For parsing JSON configuration files.
 *   - time.h: For working with system time and NTP.
 **************************************************************************/

#include <WiFi.h>
#include "FS.h"
#include "LittleFS.h"
#include <WebServer.h>
#include <Wire.h>
#include <uRTCLib.h>
#include <ArduinoJson.h>
#include <time.h>
#include <vector>  // For using std::vector as a task queue

// ==========================================================================
//                          CONFIGURATION & GLOBALS
// ==========================================================================

// ----------------------
// WiFi Credentials, Motor & Sensor Definitions
// ----------------------
const char* ssid     = "CommunityFibre10Gb_9617A";
const char* password = "bnqfuutaqu";

const int MOTOR_PINS[] = {7, 21, 22, 23};  // Motor control output pins
const int ANALOG_PIN   = 4;                // Sensor input pin for motor feedback

WebServer server(80);  // HTTP server on port 80

// ----------------------
// NTP & RTC Settings
// ----------------------
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
uRTCLib rtc;  // RTC object

const int NUM_MOTORS   = 4;

// Per-container motor parameters (default settings)
int thresholdValues[NUM_MOTORS] = {400, 400, 400, 400};  // Sensor threshold values
int motorPwmValues[NUM_MOTORS]  = {80, 80, 80, 80};        // PWM speed values

// Motor control flags and variables
bool motorRunning  = false;     // Flag indicating if a motor is active
int  currentMotor  = -1;        // Index of currently active motor (-1 if none)
bool stopControl   = false;     // Flag to stop motor control loop
unsigned long previousMillis = 0;
const long motorInterval = 50;  // Interval (ms) between sensor readings

// Delay (in ms) between a dispensing cycle stopping and starting the next one.
const int interCycleDelay = 500;

// ----------------------
// Schedule Checking Variables
// ----------------------
unsigned long lastScheduleCheck = 0;
const unsigned long scheduleCheckInterval = 60000; // Check schedules every 1 minute

// ----------------------
// Global Variables for Scheduled Dispensing
// ----------------------
int scheduledContainer          = 0;  // Container number (1-indexed)
int scheduledMotorSpeed         = 0;  // PWM speed for scheduled dispensing
int scheduledTriggerThreshold   = 0;  // Sensor threshold for scheduled dispensing
int scheduledPillCountRemaining = 0;  // Number of pills remaining to dispense
bool scheduleActive             = false;  // Flag indicating an active scheduled dispensing

// ----------------------
// Dispensing Task Queue
// ----------------------
// Define a structure for a dispensing task.
struct DispenseTask {
  int container;
  int motorSpeed;
  int triggerThreshold;
  int pillCount;
};

// Use a vector to queue multiple dispensing tasks.
std::vector<DispenseTask> dispenseQueue;

// ==========================================================================
//                           UTILITY FUNCTIONS
// ==========================================================================

/**
 * @brief Activates a specific motor by index and sets its PWM value.
 *
 * This function sets the specified motor to the PWM value defined in
 * motorPwmValues and ensures all other motors are turned off.
 *
 * @param motorIndex Index of the motor to activate.
 */
void setMotorActive(int motorIndex) {
  for (int i = 0; i < NUM_MOTORS; i++) {
    pinMode(MOTOR_PINS[i], OUTPUT);
    if (i == motorIndex) {
      // Activate selected motor with its configured PWM value
      analogWrite(MOTOR_PINS[i], motorPwmValues[motorIndex]);
      Serial.printf("Motor %d (GPIO %d) set to PWM: %d\n",
                    motorIndex, MOTOR_PINS[motorIndex], motorPwmValues[motorIndex]);
    } else {
      // Ensure other motors are off
      analogWrite(MOTOR_PINS[i], 0);
    }
  }
}

/**
 * @brief Returns the current weekday name (e.g., "Monday").
 *
 * Uses system time to determine the current day.
 *
 * @return String representing the current day.
 */
String getCurrentDayName() {
  time_t now;
  time(&now);
  struct tm* timeinfo = localtime(&now);
  int wday = timeinfo->tm_wday; // Sunday = 0, Monday = 1, etc.
  switch (wday) {
    case 0: return "Sunday";
    case 1: return "Monday";
    case 2: return "Tuesday";
    case 3: return "Wednesday";
    case 4: return "Thursday";
    case 5: return "Friday";
    case 6: return "Saturday";
    default: return "";
  }
}

// ==========================================================================
//                      FILE SERVING & WEB ENDPOINTS (LittleFS)
// ==========================================================================

/**
 * @brief Serves a file from LittleFS.
 *
 * @param path The file path to serve.
 * @param contentType The MIME type of the file.
 * @return true if the file was successfully served; false otherwise.
 */
bool serveFile(const char* path, const char* contentType) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    Serial.printf("Failed to open %s\n", path);
    return false;
  }
  server.streamFile(file, contentType);
  file.close();
  return true;
}

/**
 * @brief Handles the root ("/") endpoint by serving index.html.
 */
void handleRoot() {
  if (!serveFile("/index.html", "text/html")) {
    // Fallback HTML if index.html is not found
    server.send(200, "text/html", "<html><body><h1>Welcome</h1></body></html>");
  }
}

// ----------------------
// Schedule Endpoints
// ----------------------

/**
 * @brief GET endpoint to retrieve current schedules from schedules.json.
 */
void handleGetSchedules() {
  if (LittleFS.exists("/schedules.json")) {
    File file = LittleFS.open("/schedules.json", "r");
    if (!file) {
      server.send(500, "text/plain", "Failed to open schedules file");
      return;
    }
    server.streamFile(file, "application/json");
    file.close();
  } else {
    // Return an empty JSON array if no schedules exist.
    server.send(200, "application/json", "[]");
  }
}

/**
 * @brief POST endpoint to save schedules to schedules.json.
 */
void handleSaveSchedules() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "No data received");
    return;
  }
  String schedulesData = server.arg("plain");
  File file = LittleFS.open("/schedules.json", "w");
  if (!file) {
    server.send(500, "text/plain", "Failed to open file for writing");
    return;
  }
  file.print(schedulesData);
  file.close();
  server.send(200, "text/plain", "Schedules saved");
}

// ----------------------
// Settings Endpoints
// ----------------------

/**
 * @brief GET endpoint to retrieve settings from settings.json.
 */
void handleGetSettings() {
  if (LittleFS.exists("/settings.json")) {
    File file = LittleFS.open("/settings.json", "r");
    if (!file) {
      server.send(500, "text/plain", "Failed to open settings file");
      return;
    }
    server.streamFile(file, "application/json");
    file.close();
  } else {
    // Return an empty JSON object if no settings exist.
    server.send(200, "application/json", "{}");
  }
}

/**
 * @brief POST endpoint to save settings to settings.json.
 */
void handleSaveSettings() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "No data received");
    return;
  }
  String settingsData = server.arg("plain");
  File file = LittleFS.open("/settings.json", "w");
  if (!file) {
    server.send(500, "text/plain", "Failed to open settings file for writing");
    return;
  }
  file.print(settingsData);
  file.close();
  server.send(200, "text/plain", "Settings saved");
}

// ----------------------
// RTC Endpoints
// ----------------------

/**
 * @brief GET endpoint to set the RTC using NTP time.
 *
 * This endpoint synchronizes system time with NTP servers and then
 * updates the RTC (DS1307) accordingly.
 */
void handleSetRTC() {
  Serial.println("Received request to set RTC time.");

  // Configure NTP servers
  configTime(0, 0, ntpServer1, ntpServer2);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 5000)) {
    Serial.println("Failed to obtain NTP time.");
    server.send(500, "text/plain", "Error: Failed to get NTP time.");
    return;
  }

  // Extract time components
  int second = timeinfo.tm_sec;
  int minute = timeinfo.tm_min;
  int hour   = timeinfo.tm_hour;
  int day    = timeinfo.tm_mday;
  int month  = timeinfo.tm_mon + 1;
  int year   = timeinfo.tm_year + 1900;
  int rtcYear = year - 2000;

  // Set and refresh RTC time
  rtc.set(second, minute, hour, 0, day, month, rtcYear);
  rtc.refresh();

  char buffer[40];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
           year, month, day, hour, minute, second);
  Serial.println("RTC time successfully set to:");
  Serial.println(buffer);
  server.send(200, "text/plain", "RTC time set successfully");
}

/**
 * @brief GET endpoint to retrieve the current RTC time.
 */
void handleGetRTCTime() {
  rtc.refresh();
  int year   = rtc.year() + 2000;
  int month  = rtc.month();
  int day    = rtc.day();
  int hour   = rtc.hour();
  int minute = rtc.minute();
  int second = rtc.second();
  char timeString[30];
  snprintf(timeString, sizeof(timeString), "%04d-%02d-%02d %02d:%02d:%02d",
           year, month, day, hour, minute, second);
  server.send(200, "text/plain", timeString);
}

// ----------------------
// Motor Test Endpoint
// ----------------------

/**
 * @brief GET endpoint to test a motor.
 *
 * Expects the following parameters:
 *   - container: Container number (1-indexed)
 *   - motorSpeed: Desired PWM speed value
 *   - triggerThreshold: Sensor trigger threshold
 */
void handleTestMotor() {
  if (!server.hasArg("container") || 
      !server.hasArg("motorSpeed") || 
      !server.hasArg("triggerThreshold")) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }
  
  // Adjust container number from 1-indexed to 0-indexed
  int container = server.arg("container").toInt() - 1;
  int motorSpeed = server.arg("motorSpeed").toInt();
  int triggerThreshold = server.arg("triggerThreshold").toInt();
  
  if (container < 0 || container >= NUM_MOTORS) {
    server.send(400, "text/plain", "Invalid container (motor index)");
    return;
  }
  
  // Update motor settings immediately
  motorPwmValues[container]  = motorSpeed;
  thresholdValues[container] = triggerThreshold;
  
  // If no motor is currently running, start the test motor
  if (!motorRunning) {
    currentMotor  = container;
    motorRunning  = true;
    stopControl   = false;
    previousMillis = millis();
    setMotorActive(currentMotor);
    Serial.printf("Motor test initiated for container %d: PWM %d, Threshold %d\n",
                  container, motorSpeed, triggerThreshold);
  } else {
    Serial.println("A motor is already running. Ignoring new test command.");
  }
  
  server.send(200, "text/plain", "Motor test initiated");
}

// ==========================================================================
//              SCHEDULE & MULTI-PILL DISPENSING FUNCTIONS
// ==========================================================================

/**
 * @brief Starts the motor for a scheduled dispensing operation.
 *
 * Configures the motor parameters based on the scheduled settings and
 * starts the motor.
 */
void startMotorForSchedule() {
  int index = scheduledContainer - 1;  // Adjust from 1-indexed to 0-indexed
  motorPwmValues[index]  = scheduledMotorSpeed;
  thresholdValues[index] = scheduledTriggerThreshold;
  
  currentMotor = index;
  motorRunning = true;
  stopControl  = false;
  previousMillis = millis();
  setMotorActive(currentMotor);
  
  Serial.printf("Scheduled dispensing: Container %d started (PWM: %d, Threshold: %d). Pills remaining: %d\n",
                scheduledContainer, scheduledMotorSpeed, scheduledTriggerThreshold, scheduledPillCountRemaining);
}

/**
 * @brief Adds a dispensing task to the queue.
 *
 * This function creates a dispensing task from the given parameters and
 * pushes it into the global queue.
 *
 * @param container Container number (1-indexed).
 * @param motorSpeed PWM speed for the motor.
 * @param triggerThreshold Sensor trigger threshold.
 * @param pillCount Number of pills to dispense.
 */
void addDispenseTask(int container, int motorSpeed, int triggerThreshold, int pillCount) {
  DispenseTask task = { container, motorSpeed, triggerThreshold, pillCount };
  dispenseQueue.push_back(task);
  Serial.printf("Added task: Container %d, PWM %d, Threshold %d, Pills %d\n",
                container, motorSpeed, triggerThreshold, pillCount);
}

/**
 * @brief Checks the schedules from schedules.json against the current time.
 *
 * If a schedule matches the current day and time, it reads container
 * settings from settings.json and queues a dispensing operation.
 */
void checkSchedules() {
  // Get current time information
  time_t now;
  time(&now);
  struct tm* timeinfo = localtime(&now);
  int currentHour = timeinfo->tm_hour;
  int currentMinute = timeinfo->tm_min;
  
  char currentTimeStr[6];
  sprintf(currentTimeStr, "%02d:%02d", currentHour, currentMinute);
  String dayStr = getCurrentDayName();

  // Open and parse schedules.json from LittleFS
  if (!LittleFS.exists("/schedules.json")) {
    Serial.println("No schedules file found");
    return;
  }
  
  File file = LittleFS.open("/schedules.json", "r");
  if (!file) {
    Serial.println("Failed to open schedules file");
    return;
  }
  
  // Adjust capacity as needed based on your JSON size
  const size_t capacity = 2048;
  DynamicJsonDocument schedulesDoc(capacity);
  DeserializationError error = deserializeJson(schedulesDoc, file);
  file.close();
  if (error) {
    Serial.println("Failed to parse schedules JSON");
    return;
  }
  
  JsonArray schedules = schedulesDoc.as<JsonArray>();
  for (JsonObject sch : schedules) {
    // Check if current day is included in the schedule's "days" array.
    bool dayMatch = false;
    JsonArray days = sch["days"].as<JsonArray>();
    for (JsonVariant v : days) {
      if (v.as<String>() == dayStr) {
        dayMatch = true;
        break;
      }
    }
    if (!dayMatch)
      continue;
    
    // Check if current time matches one of the scheduled times.
    bool timeMatch = false;
    JsonArray times = sch["times"].as<JsonArray>();
    for (JsonVariant v : times) {
      if (String(v.as<String>()) == String(currentTimeStr)) {
        timeMatch = true;
        break;
      }
    }
    if (!timeMatch)
      continue;
    
    // Matching schedule found - retrieve container and pill count
    int container = sch["container"];      // Container number (1-indexed)
    int pillCount = sch["pillCount"];        // Number of pills to dispense
    
    // Retrieve container settings from settings.json
    if (!LittleFS.exists("/settings.json")) {
      Serial.println("No settings file found");
      continue;
    }
    File sFile = LittleFS.open("/settings.json", "r");
    if (!sFile) continue;
    DynamicJsonDocument settingsDoc(1024);
    DeserializationError sError = deserializeJson(settingsDoc, sFile);
    sFile.close();
    if (sError) {
      Serial.println("Failed to parse settings JSON");
      continue;
    }
    // Use container number as the key (assumed stored as a string)
    char containerKey[3];
    sprintf(containerKey, "%d", container);
    JsonObject containerSettings = settingsDoc[containerKey].as<JsonObject>();
    if (containerSettings.isNull()) {
      Serial.printf("No settings found for container %d\n", container);
      continue;
    }
    int motorSpeed = containerSettings["motorSpeed"];
    int triggerThreshold = containerSettings["triggerThreshold"];
    
    Serial.printf("Schedule match: Container %d at %s on %s. Dispensing %d pill(s).\n", 
                  container, currentTimeStr, dayStr.c_str(), pillCount);
    
    // Instead of running immediately, add this task to the queue.
    addDispenseTask(container, motorSpeed, triggerThreshold, pillCount);
    
    // Note: Removed the break to allow all matching schedules to be queued.
  }
}

// ==========================================================================
//                              SETUP FUNCTION
// ==========================================================================

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000);
  
  // ----------------------
  // Initialize I2C and RTC
  // ----------------------
  Wire.begin();
  rtc.set_rtc_address(0x68);
  rtc.set_model(URTCLIB_MODEL_DS1307);
  if (rtc.lostPower()) {
    Serial.println("RTC lost power! Resetting...");
    rtc.lostPowerClear();
  } else {
    Serial.println("RTC power OK.");
  }
  if (rtc.enableBattery()) {
    Serial.println("Battery activated.");
  } else {
    Serial.println("Battery activation failed!");
  }

  // ----------------------
  // Connect to WiFi
  // ----------------------
  Serial.printf("Connecting to WiFi network: %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // ----------------------
  // Initialize LittleFS
  // ----------------------
  if (!LittleFS.begin()) {
    Serial.println("Error mounting LittleFS");
  }
  
  // ----------------------
  // Initialize Motor Pins
  // ----------------------
  for (int i = 0; i < NUM_MOTORS; i++) {
    pinMode(MOTOR_PINS[i], OUTPUT);
    analogWrite(MOTOR_PINS[i], 0);
  }
  
  // ----------------------
  // Register HTTP Endpoints
  // ----------------------
  server.on("/", handleRoot);
  server.on("/index.html", handleRoot);
  server.on("/getSchedules", HTTP_GET, handleGetSchedules);
  server.on("/saveSchedules", HTTP_POST, handleSaveSchedules);
  server.on("/getSettings", HTTP_GET, handleGetSettings);
  server.on("/saveSettings", HTTP_POST, handleSaveSettings);
  server.on("/setRTC", HTTP_GET, handleSetRTC);
  server.on("/getRTCTime", HTTP_GET, handleGetRTCTime);
  server.on("/testMotor", HTTP_GET, handleTestMotor);
  
  server.begin();
  Serial.println("HTTP server started.");
  
  // ----------------------
  // Set system time from NTP so that localtime() works.
  // ----------------------
  configTime(0, 0, ntpServer1, ntpServer2);
}

// ==========================================================================
//                              MAIN LOOP
// ==========================================================================

void loop() {
  // Handle incoming HTTP client requests.
  server.handleClient();

  // ----------------------
  // Motor Control Loop
  // ----------------------
  // If a motor is running and control has not been stopped:
  if (motorRunning && !stopControl) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= motorInterval) {
      previousMillis = currentMillis;
      int sensorValue = analogRead(ANALOG_PIN);
      Serial.printf("Motor %d (GPIO %d) - Sensor Value: %d\n",
                    currentMotor, MOTOR_PINS[currentMotor], sensorValue);
      
      // If sensor value exceeds the threshold, stop the motor.
      if (sensorValue > thresholdValues[currentMotor]) {
        analogWrite(MOTOR_PINS[currentMotor], 0);  // Stop motor
        motorRunning = false;
        stopControl  = true;
        Serial.printf("Threshold exceeded, motor %d OFF (GPIO %d).\n",
                      currentMotor, MOTOR_PINS[currentMotor]);
        currentMotor = -1;
        
        // Add an extra delay between the motor stopping and starting the next cycle.
        delay(interCycleDelay);
        
        // For scheduled dispensing: if more pills remain, start next cycle.
        if (scheduleActive && scheduledPillCountRemaining > 1) {
          scheduledPillCountRemaining--;  // One pill dispensed
          delay(1000);  // Optional blocking delay between pills (can adjust as needed)
          startMotorForSchedule();
        } else {
          // Clear scheduled state when finished.
          scheduleActive = false;
        }
      }
    }
  }
  
  // ----------------------
  // Check and Start Next Task from the Queue if Motor is Free
  // ----------------------
  if (!motorRunning && !dispenseQueue.empty() && !scheduleActive) {
    // Pop the next task from the queue
    DispenseTask task = dispenseQueue.front();
    dispenseQueue.erase(dispenseQueue.begin());
    
    // Update global scheduled parameters from the task.
    scheduledContainer = task.container;
    scheduledMotorSpeed = task.motorSpeed;
    scheduledTriggerThreshold = task.triggerThreshold;
    scheduledPillCountRemaining = task.pillCount;
    scheduleActive = true;
    
    startMotorForSchedule();
  }
  
  // ----------------------
  // Schedule Checking
  // ----------------------
  // Check schedules at defined intervals.
  if (millis() - lastScheduleCheck >= scheduleCheckInterval) {
    lastScheduleCheck = millis();
    checkSchedules();
  }
}
