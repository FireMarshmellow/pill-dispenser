/**************************************************************************
 * Required Libraries
 **************************************************************************/
#include <WiFi.h>
#include "FS.h"
#include "LittleFS.h"
#include <WebServer.h>
#include <Wire.h>
#include <uRTCLib.h>
#include <ArduinoJson.h>
#include <time.h>
#include <vector>


/**************************************************************************
 * User-Defined Constants
 * Modify these values to match your setup and preferences.
 **************************************************************************/

// --- WiFi Credentials ---
const char* ssid     = "ssid";
const char* password = "password";

// --- Hardware Pins & Motor Settings ---
const int MOTOR_PINS[] = {7, 21, 22, 23};  // Motor control output pins; adjust if your wiring is different
const int ANALOG_PIN   = 4;                // Sensor input pin for motor feedback; adjust if needed
const int NUM_MOTORS   = 4;                // Number of motors/containers

// Default motor parameters (per container)
// Adjust these default PWM speeds and sensor threshold values as needed.
int motorPwmValues[NUM_MOTORS]  = {80, 80, 80, 80};   // PWM speed values for motors
int thresholdValues[NUM_MOTORS] = {400, 400, 400, 400}; // Sensor threshold values

// --- Timing & Scheduling Parameters ---
const long motorInterval = 50;             // Interval (in ms) between sensor readings
const int interCycleDelay = 500;           // Delay (in ms) between dispensing cycles
const unsigned long scheduleCheckInterval = 60000; // Check schedules every 1 minute

// --- NTP & RTC Settings ---
const char* ntpServer1 = "pool.ntp.org";  // Primary NTP server
const char* ntpServer2 = "time.nist.gov";   // Secondary NTP server


/**************************************************************************
 * Function Prototypes
 * (Forward declarations so the compiler knows about these functions before use.)
 **************************************************************************/
void addDispenseTask(int container, int motorSpeed, int triggerThreshold, int pillCount);
void setMotorActive(int motorIndex);
void startMotorForSchedule();
void checkSchedules();
String getCurrentDayName();

// HTTP endpoint function prototypes:
void handleRoot();
void handleGetSchedules();
void handleSaveSchedules();
void handleGetSettings();
void handleSaveSettings();
void handleSetRTC();
void handleGetRTCTime();
void handleTestMotor();
void handleTestSchedule();

/**************************************************************************
 * Global Variables (Non-User-Changeable)
 **************************************************************************/

// --- Web Server & RTC ---
WebServer server(80);  // HTTP server on port 80
uRTCLib rtc;           // RTC object

// --- Motor Control Flags & Variables ---
bool motorRunning  = false;  // Flag indicating if a motor is active
int  currentMotor  = -1;     // Index of the currently active motor (-1 means none)
bool stopControl   = false;  // Flag to stop the motor control loop
unsigned long previousMillis = 0;  // Used for motor interval timing

// --- Scheduled Dispensing Global Variables ---
int scheduledContainer = 0;          // Container number (1-indexed)
int scheduledMotorSpeed = 0;         // PWM speed for scheduled dispensing
int scheduledTriggerThreshold = 0;   // Sensor threshold for scheduled dispensing
int scheduledPillCountRemaining = 0; // Number of pills remaining to dispense
bool scheduleActive = false;         // True when a scheduled dispensing is active
unsigned long lastScheduleCheck = 0; // Timestamp of the last schedule check

// --- Dispensing Task Queue ---
// Structure for a dispensing task.
struct DispenseTask {
  int container;
  int motorSpeed;
  int triggerThreshold;
  int pillCount;
};

// Vector to store dispensing tasks.
std::vector<DispenseTask> dispenseQueue;


/**************************************************************************
 * Embedded HTML Content (Stored in PROGMEM)
 * This is the complete content of your index.html.
 **************************************************************************/
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Mellow_labs Pill Dispenser</title>
  <!-- Google Fonts -->
  <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;500;700&display=swap" rel="stylesheet">
  <style>
    /* Global Styles and Resets */
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    body {
      font-family: 'Roboto', sans-serif;
      margin: 15px;
      background-color: #f8f9fa;
      color: #495057;
      line-height: 1.5;
      transition: background-color 0.3s, color 0.3s;
    }
    /* Dark Mode Overrides for Body */
    body.dark-mode {
      background-color: #121212;
      color: #e0e0e0;
    }
    body.dark-mode h1,
    body.dark-mode h2 {
      color: #e0e0e0;
    }
    /* Container Grid Styles */
    .container-grid {
      display: flex;
      flex-direction: column;
      gap: 20px;
      max-width: 800px;
      margin: 15px auto;
      padding: 0 20px;
    }
    /* Block Container Style */
    .container-block {
      background: #fff;
      border-radius: 10px;
      padding: 20px;
      box-shadow: 0 4px 10px rgba(0, 0, 0, 0.08);
      transition: transform 0.2s ease-in-out, background-color 0.3s;
      border-top: 5px solid;
    }
    .container-block:hover {
      transform: scale(1.01);
    }
    /* Dark Mode Overrides for Container Blocks */
    body.dark-mode .container-block {
      background: #101010;
    }
    /* Block Header Style */
    .container-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 12px;
      padding: 12px 20px;
      border-top-left-radius: 8px;
      border-top-right-radius: 8px;
    }
    /* Base Button Styles */
    button {
      color: white;
      border: none;
      border-radius: 6px;
      padding: 10px 16px;
      font-size: 0.9rem;
      cursor: pointer;
      transition: background-color 0.3s ease;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
    }
    button:focus {
      outline: none;
      box-shadow: 0 0 0 3px rgba(0, 123, 255, 0.5);
    }
    /* Container-Specific Button Colors (for header buttons) */
    #container1-block .container-header button {
      background-color: #005aba;
    }
    #container1-block .container-header button:hover {
      background-color: #0065d2;
    }
    #container2-block .container-header button {
      background-color: #a50010;
    }
    #container2-block .container-header button:hover {
      background-color: #b90012;
    }
    #container3-block .container-header button {
      background-color: #e0a800;
      color: #212529;
    }
    #container3-block .container-header button:hover {
      background-color: #d39e00;
    }
    #container4-block .container-header button {
      background-color: #007a1d;
    }
    #container4-block .container-header button:hover {
      background-color: #1e7e34;
    }
    /* Table Styles */
    table {
      width: 100%;
      border-collapse: collapse;
      margin-top: 10px;
      background-color: #fff;
      border-radius: 8px;
      overflow: hidden;
      box-shadow: 0 1px 6px rgba(0, 0, 0, 0.06);
    }
    body.dark-mode table {
      background-color: #1e1e1e;
    }
    th, td {
      border: none;
      padding: 12px;
      text-align: left;
      font-size: 0.85rem;
      vertical-align: middle;
      border-bottom: 1px solid #f1f3f5;
    }
    body.dark-mode th, body.dark-mode td {
      border-bottom: 1px solid #333;
    }
    th {
      background-color: #e9ecef;
      color: #495057;
      font-weight: 500;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    body.dark-mode th {
      background-color: #333;
      color: #e0e0e0;
    }
    th:first-child, td:first-child {
      padding-left: 15px;
    }
    th:last-child, td:last-child {
      padding-right: 15px;
      text-align: center;
    }
    tr:last-child td {
      border-bottom: none;
    }
    /* Gear Icon Styles */
    .gear-icon {
      cursor: pointer;
      font-size: 1.2rem;
      margin-left: 6px;
      user-select: none;
      color: #6c757d;
      transition: color 0.2s ease;
    }
    .gear-icon:hover {
      color: #495057;
    }
    /* Gear Menu Styles */
    .gear-menu {
      position: fixed;
      background: #fff;
      border: 1px solid #ced4da;
      border-radius: 6px;
      padding: 6px 0;
      display: none;
      z-index: 1002;
      min-width: 100px;
      box-shadow: 0 3px 8px rgba(0, 0, 0, 0.1);
    }
    body.dark-mode .gear-menu {
      background: #2c2c2c;
      border-color: #444;
    }
    .gear-menu button {
      display: block;
      width: 100%;
      text-align: left;
      background: none;
      color: #495057;
      border: none;
      padding: 8px 12px;
      font-size: 0.8rem;
      cursor: pointer;
      transition: background-color 0.2s ease;
    }
    body.dark-mode .gear-menu button {
      color: #e0e0e0;
    }
    .gear-menu button:hover {
      background-color: #f8f9fa;
    }
    body.dark-mode .gear-menu button:hover {
      background-color: #444;
    }
    /* Gear Menu Delete Button (second button) */
    .gear-menu button:nth-child(2) {
      background: #dc3545;
      color: #fff;
    }
    .gear-menu button:nth-child(2):hover {
      background: #c82333;
    }
    /* Overlay */
    #overlay {
      position: fixed;
      top: 0; left: 0;
      width: 100%; height: 100%;
      background: rgba(0, 0, 0, 0.5);
      z-index: 1001;
      display: none;
    }
    /* ================== Creative Schedule Dialog ================== */
    #scheduleDialog {
      position: fixed;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      width: 300px;
      background: rgba(255,255,255,0.98);
      backdrop-filter: blur(8px);
      border-radius: 10px;
      box-shadow: 0 4px 15px rgba(0,0,0,0.2);
      z-index: 1002;
      display: none;
      overflow: hidden;
      transition: background 0.3s, color 0.3s;
    }
    /* Dark Mode Override for Schedule Dialog */
    body.dark-mode #scheduleDialog {
      background: rgba(26,26,26,0.98);
    }
    #scheduleDialog .header {
      background: var(--accent-color, #007bff);
      color: #fff;
      padding: 8px 10px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    #scheduleDialog .header h2 {
      font-size: 1.2rem;
      margin: 0;
    }
    #scheduleDialog .header .close-btn {
      cursor: pointer;
      font-size: 1.2rem;
    }
    #scheduleDialog .content {
      padding: 10px;
      text-align: center;
      transition: color 0.3s;
    }
    /* Everyday Button */
    #scheduleDialog .everyday-container {
      text-align: center;
      margin-bottom: 8px;
    }
    #scheduleDialog .everyday-container button {
      padding: 4px 8px;
      font-size: 0.85rem;
      border: 1px solid #ccc;
      background: transparent;
      border-radius: 5px;
      cursor: pointer;
      color: #333;
      transition: background-color 0.3s, color 0.3s;
    }
    body.dark-mode #scheduleDialog .everyday-container button {
      color: #e0e0e0;
      border-color: #555;
    }
    #scheduleDialog .everyday-container button.active {
      background: var(--accent-color, #007bff);
      color: #fff;
      border-color: var(--accent-color, #007bff);
    }
    /* Days Grid - Updated Styles for Day Buttons */
    #scheduleDialog .days-grid {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 4px;
      margin-bottom: 8px;
    }
    #scheduleDialog .days-grid button {
      padding: 4px 8px;
      font-size: 0.85rem;
      border: 1px solid #ccc;
      background: transparent;
      color: #333;
      border-radius: 5px;
      cursor: pointer;
      transition: background-color 0.3s, color 0.3s;
    }
    body.dark-mode #scheduleDialog .days-grid button {
      background: transparent;
      color: #e0e0e0;
      border-color: #555;
    }
    #scheduleDialog .days-grid button.active,
    body.dark-mode #scheduleDialog .days-grid button.active {
      background: var(--accent-color, #007bff);
      color: #fff;
      border-color: var(--accent-color, #007bff);
    }
    /* ================== Pill Count & Spinner Styles ================== */
    /* Pill Count Label */
    #scheduleDialog .pill-count label {
      color: #333;
      font-weight: bold;
      margin-right: 5px;
    }
    body.dark-mode #scheduleDialog .pill-count label {
      color: #e0e0e0;
    }
    /* Add some margin below the pill count section */
    #scheduleDialog .pill-count {
      margin-bottom: 10px;
    }
    /* Pill Counter Container */
    #scheduleDialog .pill-counter {
      display: inline-flex;
      align-items: center;
      border: 1px solid #ddd;
      border-radius: 4px;
      overflow: hidden;
      box-shadow: 0 1px 3px rgba(0,0,0,0.1);
      margin-left: 5px;
      background: #fff;
      transition: background-color 0.3s, border-color 0.3s;
    }
    body.dark-mode #scheduleDialog .pill-counter {
      border-color: #555;
      box-shadow: 0 1px 3px rgba(255,255,255,0.1);
      background: #1e1e1e;
    }
    /* Pill Counter Buttons */
    #scheduleDialog .pill-counter button {
      background: #fdfdfd;
      border: none;
      width: 40px;
      height: 40px;
      font-size: 20px;
      line-height: 1;
      cursor: pointer;
      color: #333;
      transition: background-color 0.2s ease;
    }
    #scheduleDialog .pill-counter button:hover {
      background: #e9ecef;
    }
    body.dark-mode #scheduleDialog .pill-counter button {
      background: #2c2c2c;
      color: #e0e0e0;
    }
    body.dark-mode #scheduleDialog .pill-counter button:hover {
      background: #444;
    }
    /* Pill Counter Input */
    #scheduleDialog .pill-counter input {
      width: 60px;
      text-align: center;
      border: none;
      outline: none;
      font-size: 1rem;
      color: #333;
      background: #fff;
      transition: background-color 0.3s, color 0.3s;
    }
    body.dark-mode #scheduleDialog .pill-counter input {
      background: #1e1e1e;
      color: #e0e0e0;
    }
    /* Time Inputs */
    #scheduleDialog .time-inputs div {
      margin-bottom: 6px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    #scheduleDialog .time-inputs label {
      font-size: 0.8rem;
      margin-right: 5px;
      color: #333;
    }
    #scheduleDialog .time-inputs input[type="time"] {
      padding: 3px;
      font-size: 0.9rem;
      width: 80px;
      color: #333;
    }
    body.dark-mode #scheduleDialog .time-inputs label,
    body.dark-mode #scheduleDialog .time-inputs input[type="time"] {
      color: #e0e0e0;
    }
    body.dark-mode #scheduleDialog .time-inputs input[type="time"] {
      background: #2c2c2c;
      border: 1px solid #555;
    }
    /* Footer Buttons */
    #scheduleDialog .footer {
      display: flex;
      justify-content: center;
      gap: 5px;
      padding: 8px 10px;
      border-top: 1px solid #eee;
    }
    #scheduleDialog .footer button {
      padding: 5px 10px;
      font-size: 0.9rem;
      border: none;
      border-radius: 5px;
      cursor: pointer;
    }
    #scheduleDialog .footer .save-btn {
      background: var(--accent-color, #007bff);
      color: #fff;
    }
    #scheduleDialog .footer .cancel-btn {
      background: #ccc;
      color: #333;
    }
    /* ================== Settings Dialog (Container Settings) ================== */
    #settingsDialog {
      position: fixed;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      background: #fff;
      width: 90%;
      max-width: 350px;
      border-radius: 10px;
      padding: 25px;
      box-shadow: 0 6px 15px rgba(0, 0, 0, 0.15);
      z-index: 1002;
      display: none;
      border-top: 8px solid;
    }
    body.dark-mode #settingsDialog {
      background: #2c2c2c;
    }
    #settingsDialog h2 {
      margin-bottom: 20px;
      text-align: center;
      font-size: 1.5rem;
    }
    /* Color-specific container styles for Light Mode */
    #container1-block { background-color: #b1dbff; }
    #container2-block { background-color: #fcbbc5; }
    #container3-block { background-color: #f8e9b6; }
    #container4-block { background-color: #b6fcbc; }
    /* Make container header the same as container block background in Light Mode */
    #container1-block .container-header {
      background-color: #f0f8ff;
    }
    #container2-block .container-header {
      background-color: #ffebee;
    }
    #container3-block .container-header {
      background-color: #fff8e1;
    }
    #container4-block .container-header {
      background-color: #e8f5e9;
    }
    /* Dark Mode Overrides for Container-Specific Colors */
    body.dark-mode #container1-block,
    body.dark-mode #container2-block,
    body.dark-mode #container3-block,
    body.dark-mode #container4-block {
      background-color: #1a1a1a !important;
    }
    /* Dark Mode Overrides for Container Headers */
    body.dark-mode #container1-block .container-header,
    body.dark-mode #container2-block .container-header,
    body.dark-mode #container3-block .container-header,
    body.dark-mode #container4-block .container-header {
      background-color: #1a1a1a !important;
    }
    body.dark-mode #container1-block .container-header h2,
    body.dark-mode #container2-block .container-header h2,
    body.dark-mode #container3-block .container-header h2,
    body.dark-mode #container4-block .container-header h2 {
      color: #e0e0e0 !important;
    }
    /* ============= Collapsible Settings Panel ============= */
    .collapsible-header {
      background: #8b00c2; /* Purple */
      color: #fff;
      padding: 10px 15px;
      cursor: pointer;
      border-radius: 10px 10px 0 0;
      max-width: 760px;
      margin: 20px auto 0 auto;
      font-size: 1.1rem;
      text-align: center;
    }
    .collapsible-content {
      background: #fff;
      border-radius: 0 0 10px 10px;
      box-shadow: 0 4px 10px rgba(0,0,0,0.08);
      max-width: 760px;
      margin: 0 auto 20px auto;
      padding: 20px;
      display: none;
    }
    body.dark-mode .collapsible-content {
      background: #1e1e1e;
    }
    /* Form inside collapsible content arranged inline */
    .collapsible-content form {
      display: flex;
      align-items: center;
      gap: 10px;
      flex-wrap: wrap;
    }
    .collapsible-content label {
      font-size: 1rem;
    }
    .collapsible-content select {
      padding: 8px;
      font-size: 1rem;
      width: 250px;
    }
    .collapsible-content button {
      background-color: #8b00c2;
      color: #fff;
      border: none;
      border-radius: 6px;
      padding: 8px 12px;
      font-size: 1rem;
      cursor: pointer;
    }
    .collapsible-content button:hover {
      background-color: #8e44ad;
    }
    h1 {
      text-align: center;
    }
    .dialog-buttons-settings {
      display: flex;
      justify-content: center;
      gap: 10px; /* optional: adds spacing between buttons */
      margin-top: 20px;
    }
  </style>
</head>
<body>
  <h1>Mellow_labs Pill Dispenser</h1>
  <div class="container-grid">
    <!-- Container 1 (Blue) -->
    <div class="container-block" id="container1-block">
      <div class="container-header">
        <h2>Container 1</h2>
        <div>
          <button onclick="openScheduleDialog(1, null)">+ Add Schedule</button>
          <button onclick="openSettingsDialog(1)">Settings</button>
        </div>
      </div>
      <div class="table-container">
        <table id="container1-table">
          <thead>
            <tr>
              <th>Days</th>
              <th>Pills</th>
              <th>Times</th>
              <th style="width:50px;"></th>
            </tr>
          </thead>
          <tbody></tbody>
        </table>
      </div>
    </div>
    <!-- Container 2 (Red) -->
    <div class="container-block" id="container2-block">
      <div class="container-header">
        <h2>Container 2</h2>
        <div>
          <button onclick="openScheduleDialog(2, null)">+ Add Schedule</button>
          <button onclick="openSettingsDialog(2)">Settings</button>
        </div>
      </div>
      <div class="table-container">
        <table id="container2-table">
          <thead>
            <tr>
              <th>Days</th>
              <th>Pills</th>
              <th>Times</th>
              <th style="width:50px;"></th>
            </tr>
          </thead>
          <tbody></tbody>
        </table>
      </div>
    </div>
    <!-- Container 3 (Yellow) -->
    <div class="container-block" id="container3-block">
      <div class="container-header">
        <h2>Container 3</h2>
        <div>
          <button onclick="openScheduleDialog(3, null)">+ Add Schedule</button>
          <button onclick="openSettingsDialog(3)">Settings</button>
        </div>
      </div>
      <div class="table-container">
        <table id="container3-table">
          <thead>
            <tr>
              <th>Days</th>
              <th>Pills</th>
              <th>Times</th>
              <th style="width:50px;"></th>
            </tr>
          </thead>
          <tbody></tbody>
        </table>
      </div>
    </div>
    <!-- Container 4 (Green) -->
    <div class="container-block" id="container4-block">
      <div class="container-header">
        <h2>Container 4</h2>
        <div>
          <button onclick="openScheduleDialog(4, null)">+ Add Schedule</button>
          <button onclick="openSettingsDialog(4)">Settings</button>
        </div>
      </div>
      <div class="table-container">
        <table id="container4-table">
          <thead>
            <tr>
              <th>Days</th>
              <th>Pills</th>
              <th>Times</th>
              <th style="width:50px;"></th>
            </tr>
          </thead>
          <tbody></tbody>
        </table>
      </div>
    </div>
  </div>

  <!-- ========== Collapsible Settings Panel ========== -->
  <div class="collapsible-header" onclick="toggleSettingsPanel()">Settings ▾</div>
  <div class="collapsible-content" id="settingsPanel" style="display: none; justify-content: space-between; align-items: center;">
    <!-- Left Side: RTC Button and Clock -->
    <div style="display: flex; align-items: center;">
      <button id="setRTCButton" onclick="setRTC()">Set RTC</button>
      <span id="clock" style="margin-left: 10px; font-weight: bold; border: 1px solid #ccc; padding: 5px 8px; border-radius: 5px;"></span>
    </div>
    <!-- Middle: Test Schedule Button (runs through the entire schedule) -->
    <div style="margin-top: 10px; text-align: center;">
      <button id="testScheduleButton" onclick="testSchedule()">Test Schedule</button>
    </div>
    <!-- Right Side: Dark Mode Toggle -->
    <div>
      <button type="button" id="toggleDarkButton" onclick="toggleDarkMode()">Dark Mode</button>
    </div>
  </div>

  <!-- Overlay -->
  <div id="overlay"></div>

  <!-- ================== Creative Schedule Dialog ================== -->
  <div id="scheduleDialog">
    <div class="header">
      <h2 id="dialogTitle">Add Schedule</h2>
      <span class="close-btn" onclick="closeScheduleDialog()">×</span>
    </div>
    <div class="content">
      <div class="everyday-container">
        <button id="everydayBtn" onclick="toggleEveryday()">Everyday</button>
      </div>
      <div class="days-grid">
        <button data-day="Monday" onclick="toggleDay(this)">Mon</button>
        <button data-day="Tuesday" onclick="toggleDay(this)">Tue</button>
        <button data-day="Wednesday" onclick="toggleDay(this)">Wed</button>
        <button data-day="Thursday" onclick="toggleDay(this)">Thu</button>
        <button data-day="Friday" onclick="toggleDay(this)">Fri</button>
        <button data-day="Saturday" onclick="toggleDay(this)">Sat</button>
        <button data-day="Sunday" onclick="toggleDay(this)">Sun</button>
      </div>
      <div class="pill-count">
        <label>Pill Count:</label>
        <div class="pill-counter">
          <button class="minus-btn" onclick="decrementPillCount()">−</button>
          <input type="text" id="pillCount" value="1" readonly>
          <button class="plus-btn" onclick="incrementPillCount()">+</button>
        </div>
      </div>
      <div class="time-inputs" id="timeInputs">
        <!-- Time inputs will be generated here -->
      </div>
    </div>
    <div class="footer">
      <button class="save-btn" onclick="saveSchedule()">Save</button>
      <button class="cancel-btn" onclick="closeScheduleDialog()">Cancel</button>
    </div>
  </div>

  <!-- ================== Settings Dialog (Container Settings) ================== -->
  <div id="settingsDialog">
    <h2 id="settingsDialogTitle">Container Settings</h2>
    <div style="text-align: center; margin-bottom: 15px;">
      <button id="testMotorButton" onclick="testMotor()" style="margin-bottom: 10px;">Test motor</button>
    </div>
    <div class="slider-group">
      <label for="motorSpeed">Motor speed PWM: <span id="motorSpeedValue" class="slider-value"></span></label>
      <input type="range" id="motorSpeed" min="0" max="255" value="128" oninput="document.getElementById('motorSpeedValue').textContent = this.value">
    </div>
    <div class="slider-group">
      <label for="triggerThreshold">Trigger threshold: <span id="triggerThresholdValue" class="slider-value"></span></label>
      <input type="range" id="triggerThreshold" min="0" max="3000" value="1500" oninput="document.getElementById('triggerThresholdValue').textContent = this.value">
    </div>
    <div class="dialog-buttons-settings">
      <button id="saveSettingsBtn" onclick="saveSettings()">Save</button>
      <button id="CancelSettingsBtn" onclick="closeSettingsDialog()">Cancel</button>
    </div>
  </div>

  <script>
    // Global variables
    let currentContainer = null;
    let editingScheduleId = null;
    let currentSettingsContainer = null;
    let schedulesData = [];    // will hold schedules loaded from the server
    let settingsData = {};     // will hold settings loaded from the server, including theme

    // Define container colors (matching header colors)
    const containerColors = {
      1: "#007bff", // Blue
      2: "#dc3545", // Red
      3: "#ffc107", // Yellow
      4: "#28a745"  // Green
    };

    // ------------------- Server Communication Functions -------------------
    async function loadSchedules() {
      try {
        const response = await fetch('/getSchedules');
        schedulesData = await response.json();
      } catch (e) {
        console.error("Error loading schedules:", e);
        schedulesData = [];
      }
    }

    async function saveSchedulesToServer() {
      try {
        await fetch('/saveSchedules', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify(schedulesData)
        });
      } catch (e) {
        console.error("Error saving schedules:", e);
      }
    }

    async function loadSettings() {
      try {
        const response = await fetch('/getSettings');
        settingsData = await response.json();
      } catch (e) {
        console.error("Error loading settings:", e);
        settingsData = {};
      }
    }

    async function saveSettingsToServer() {
      try {
        await fetch('/saveSettings', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify(settingsData)
        });
      } catch (e) {
        console.error("Error saving settings:", e);
      }
    }

    // ------------------- Clock Function -------------------
    // One version fetches the RTC time from the server.
    function updateClock() {
      fetch('/getRTCTime')
        .then(response => response.text())
        .then(rtcTime => {
          const clockElement = document.getElementById('clock');
          clockElement.textContent = rtcTime;
        })
        .catch(err => {
          console.error("Error fetching RTC time:", err);
        });
    }

    // ------------------- Collapsible Settings Panel Functions -------------------
    function toggleSettingsPanel() {
      const panel = document.getElementById('settingsPanel');
      const header = document.querySelector('.collapsible-header');
      if (panel.style.display === "flex") {
        panel.style.display = "none";
        header.textContent = "Settings ▾";
      } else {
        panel.style.display = "flex";
        header.textContent = "Settings ▴";
      }
    }

    // ------------------- Dark Mode Toggle -------------------
    function toggleDarkMode() {
      document.body.classList.toggle("dark-mode");
      const toggleButton = document.getElementById('toggleDarkButton');
      if (document.body.classList.contains("dark-mode")) {
        settingsData.theme = "dark";
        toggleButton.textContent = "Light Mode";
      } else {
        settingsData.theme = "light";
        toggleButton.textContent = "Dark Mode";
      }
      saveSettingsToServer();
    }

    // ------------------- Schedule Dialog Functions -------------------
    function openScheduleDialog(containerId, scheduleId) {
      currentContainer = containerId;
      editingScheduleId = scheduleId;
      resetDialog();
      const color = containerColors[containerId] || "#007bff";
      const scheduleDialog = document.getElementById('scheduleDialog');
      scheduleDialog.querySelector('.header').style.background = color;
      scheduleDialog.querySelector('.header h2').style.color = "#fff";
      scheduleDialog.style.setProperty('--accent-color', color);
      
      if (editingScheduleId !== null) {
        const scheduleToEdit = schedulesData.find(s => s.id === editingScheduleId);
        if (scheduleToEdit) {
          document.getElementById('dialogTitle').textContent = 'Edit Schedule';
          const allDays = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
          const everydayBtn = document.getElementById('everydayBtn');
          if (allDays.every(day => scheduleToEdit.days.includes(day))) {
            everydayBtn.classList.add('active');
          }
          document.querySelectorAll('.days-grid button').forEach(btn => {
            if (scheduleToEdit.days.includes(btn.getAttribute('data-day'))) {
              btn.classList.add('active');
            }
          });
          document.getElementById('pillCount').value = scheduleToEdit.pillCount;
          updateTimeFields();
          const timeInputs = document.querySelectorAll('.time-inputs input[type="time"]');
          scheduleToEdit.times.forEach((timeVal, idx) => {
            if (timeInputs[idx]) timeInputs[idx].value = timeVal;
          });
        }
      } else {
        document.getElementById('dialogTitle').textContent = 'Add Schedule';
      }
      document.getElementById('overlay').style.display = 'block';
      scheduleDialog.style.display = 'block';
    }

    function closeScheduleDialog() {
      document.getElementById('overlay').style.display = 'none';
      document.getElementById('scheduleDialog').style.display = 'none';
    }

    function resetDialog() {
      document.getElementById('everydayBtn').classList.remove('active');
      document.querySelectorAll('.days-grid button').forEach(btn => btn.classList.remove('active'));
      document.getElementById('pillCount').value = 1;
      updateTimeFields();
    }

    function toggleDay(btn) {
      btn.classList.toggle('active');
    }

    function toggleEveryday() {
      const btn = document.getElementById('everydayBtn');
      btn.classList.toggle('active');
      if (btn.classList.contains('active')) {
        document.querySelectorAll('.days-grid button').forEach(b => b.classList.remove('active'));
      }
    }

    function updateTimeFields() {
      const pillCount = parseInt(document.getElementById('pillCount').value) || 1;
      const timeInputsDiv = document.getElementById('timeInputs');
      timeInputsDiv.innerHTML = "";
      for (let i = 1; i <= pillCount; i++) {
        const div = document.createElement('div');
        div.innerHTML = `<label>Time #${i}:</label><input type="time" value="08:00">`;
        timeInputsDiv.appendChild(div);
      }
    }

    function incrementPillCount() {
      const pillInput = document.getElementById('pillCount');
      let currentVal = parseInt(pillInput.value) || 1;
      pillInput.value = ++currentVal;
      updateTimeFields();
    }

    function decrementPillCount() {
      const pillInput = document.getElementById('pillCount');
      let currentVal = parseInt(pillInput.value) || 1;
      if (currentVal > 1) {
        currentVal--;
      }
      pillInput.value = currentVal;
      updateTimeFields();
    }

    async function saveSchedule() {
      let days = [];
      const everydayBtn = document.getElementById('everydayBtn');
      if (everydayBtn.classList.contains('active')) {
        days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
      } else {
        document.querySelectorAll('.days-grid button.active').forEach(btn => {
          days.push(btn.getAttribute('data-day'));
        });
      }
      const pillCount = parseInt(document.getElementById('pillCount').value) || 1;
      const times = [];
      document.querySelectorAll('.time-inputs input[type="time"]').forEach(input => {
        if (input.value) times.push(input.value);
      });
      
      if (editingScheduleId === null) {
        const newSchedule = {
          id: Date.now(),
          container: currentContainer,
          days: days,
          pillCount: pillCount,
          times: times
        };
        schedulesData.push(newSchedule);
      } else {
        schedulesData = schedulesData.map(s => {
          if (s.id === editingScheduleId) {
            return {
              ...s,
              container: currentContainer,
              days: days,
              pillCount: pillCount,
              times: times
            };
          }
          return s;
        });
      }
      await saveSchedulesToServer();
      closeScheduleDialog();
      renderAllSchedules();
    }

    async function deleteSchedule(scheduleId) {
      schedulesData = schedulesData.filter(s => s.id !== scheduleId);
      await saveSchedulesToServer();
      renderAllSchedules();
    }

    function renderAllSchedules() {
      for (let c = 1; c <= 4; c++) {
        const tableBody = document.querySelector(`#container${c}-table tbody`);
        if (tableBody) tableBody.innerHTML = "";
      }
      schedulesData.forEach(sch => {
        const tableBody = document.querySelector(`#container${sch.container}-table tbody`);
        if (!tableBody) return;
        const row = document.createElement('tr');
        const daysCell = document.createElement('td');
        daysCell.textContent = sch.days.join(', ');
        const pillCell = document.createElement('td');
        pillCell.textContent = sch.pillCount;
        const timesCell = document.createElement('td');
        timesCell.textContent = sch.times.join(', ');
        const gearCell = document.createElement('td');
        gearCell.style.position = 'relative';
        const gearSpan = document.createElement('span');
        gearSpan.className = 'gear-icon';
        gearSpan.textContent = '⚙';
        gearSpan.onclick = (e) => {
          e.stopPropagation();
          toggleMenu(sch.id, e);
        };
        const menuDiv = document.createElement('div');
        menuDiv.className = 'gear-menu';
        menuDiv.id = `menu-${sch.id}`;
        const editBtn = document.createElement('button');
        editBtn.textContent = 'Edit';
        editBtn.onclick = () => {
          openScheduleDialog(sch.container, sch.id);
          toggleMenu(sch.id);
        };
        const deleteBtn = document.createElement('button');
        deleteBtn.textContent = 'Delete';
        deleteBtn.onclick = () => {
          deleteSchedule(sch.id);
          toggleMenu(sch.id);
        };
        menuDiv.appendChild(editBtn);
        menuDiv.appendChild(deleteBtn);
        gearCell.appendChild(gearSpan);
        row.appendChild(daysCell);
        row.appendChild(pillCell);
        row.appendChild(timesCell);
        row.appendChild(gearCell);
        tableBody.appendChild(row);
        document.body.appendChild(menuDiv);
      });
    }

    function toggleMenu(scheduleId, event) {
      const menu = document.getElementById(`menu-${scheduleId}`);
      if (!menu) return;
      closeAllMenus(scheduleId);
      if (menu.style.display === 'block') {
        menu.style.display = 'none';
      } else {
        menu.style.display = 'block';
        if (event) {
          const gearIcon = event.target;
          const gearRect = gearIcon.getBoundingClientRect();
          menu.style.left = `${gearRect.left + window.scrollX - 15}px`;
          menu.style.top = `${gearRect.bottom + window.scrollY}px`;
        }
      }
    }

    function closeAllMenus(exceptId = null) {
      const menus = document.querySelectorAll('.gear-menu');
      menus.forEach(m => {
        if (m.id !== `menu-${exceptId}`) {
          m.style.display = 'none';
        }
      });
    }

    function closeAllMenusOnOutsideClick(e) {
      if (e.target.closest('.gear-menu') || e.target.closest('.gear-icon')) {
        return;
      }
      closeAllMenus();
    }

    // ------------------- Settings Dialog Functions (Container Settings) -------------------
    function openSettingsDialog(containerId) {
      currentSettingsContainer = containerId;
      let savedSettings = settingsData[containerId] || {};
      const motorSpeedSlider = document.getElementById('motorSpeed');
      const triggerThresholdSlider = document.getElementById('triggerThreshold');
      motorSpeedSlider.value = savedSettings.motorSpeed !== undefined ? savedSettings.motorSpeed : 128;
      triggerThresholdSlider.value = savedSettings.triggerThreshold !== undefined ? savedSettings.triggerThreshold : 1500;
      document.getElementById('motorSpeedValue').textContent = motorSpeedSlider.value;
      document.getElementById('triggerThresholdValue').textContent = triggerThresholdSlider.value;
      
      const color = containerColors[containerId] || "#007bff";
      let settingsDialog = document.getElementById('settingsDialog');
      settingsDialog.style.borderTopColor = color;
      settingsDialog.querySelector('h2').style.color = color;
      document.getElementById('settingsDialogTitle').textContent = `Container ${containerId} Settings`;
      
      document.getElementById('testMotorButton').style.backgroundColor = color;
      document.getElementById('CancelSettingsBtn').style.backgroundColor = color;
      document.getElementById('saveSettingsBtn').style.backgroundColor = color;


      

      
      motorSpeedSlider.style.accentColor = color;
      triggerThresholdSlider.style.accentColor = color;

      document.getElementById('overlay').style.display = 'block';
      settingsDialog.style.display = 'block';
      closeAllMenus();
    }

    function closeSettingsDialog() {
      document.getElementById('overlay').style.display = 'none';
      document.getElementById('settingsDialog').style.display = 'none';
    }

    async function saveSettings() {
      const motorSpeed = parseInt(document.getElementById('motorSpeed').value);
      const triggerThreshold = parseInt(document.getElementById('triggerThreshold').value);
      settingsData[currentSettingsContainer] = { motorSpeed, triggerThreshold };
      await saveSettingsToServer();
      closeSettingsDialog();
    }

    function testMotor() {
      const motorSpeed = document.getElementById('motorSpeed').value;
      const triggerThreshold = document.getElementById('triggerThreshold').value;
      const container = currentSettingsContainer;
      const url = `/testMotor?container=${container}&motorSpeed=${motorSpeed}&triggerThreshold=${triggerThreshold}`;
      fetch(url)
        .then(response => response.text())
        .then(data => {
          console.log(data);
        })
        .catch(err => {
          console.error('Error testing motor:', err);
        });
    }

    // ------------------- New: Test Schedule Function (Collapsible Panel) -------------------
    // This function calls the /testSchedule endpoint, which will run through all schedules,
    // dispensing every pill specified regardless of time.
    function testSchedule() {
      fetch('/testSchedule')
        .then(response => response.text())
        .then(data => {
          console.log("Test Schedule Response:", data);
          alert("Test Schedule initiated: " + data);
        })
        .catch(err => {
          console.error("Error initiating test schedule:", err);
          alert("Error initiating test schedule");
        });
    }

    // ------------------- Page Initialization -------------------
    window.onload = async function() {
      await loadSchedules();
      await loadSettings();

      if (settingsData.theme === "dark") {
        document.body.classList.add("dark-mode");
      } else {
        document.body.classList.remove("dark-mode");
      }
      
      document.getElementById('toggleDarkButton').textContent = 
        document.body.classList.contains("dark-mode") ? "Light Mode" : "Dark Mode";
      
      renderAllSchedules();
      document.addEventListener('click', closeAllMenusOnOutsideClick);
      
      document.getElementById('container1-block').style.borderTopColor = containerColors[1];
      document.getElementById('container2-block').style.borderTopColor = containerColors[2];
      document.getElementById('container3-block').style.borderTopColor = containerColors[3];
      document.getElementById('container4-block').style.borderTopColor = containerColors[4];

      document.querySelector('#container1-block .container-header').style.backgroundColor = containerColors[1];
      document.querySelector('#container2-block .container-header').style.backgroundColor = containerColors[2];
      document.querySelector('#container3-block .container-header').style.backgroundColor = containerColors[3];
      document.querySelector('#container4-block .container-header').style.backgroundColor = containerColors[4];

      document.getElementById('motorSpeedValue').textContent = document.getElementById('motorSpeed').value;
      document.getElementById('triggerThresholdValue').textContent = document.getElementById('triggerThreshold').value;

      updateTimeFields();

      updateClock();
      setInterval(updateClock, 1000);
    };

    // Stub for setRTC function (since it's referenced but not defined)
    function setRTC() {
      console.log("setRTC function called");
    }
  </script>
</body>
</html>
)rawliteral";


/**************************************************************************
 * Function Definitions for Motor & Schedule Control
 **************************************************************************/

/**
 * @brief Activates a specific motor by index and sets its PWM value.
 */
void setMotorActive(int motorIndex) {
  for (int i = 0; i < NUM_MOTORS; i++) {
    pinMode(MOTOR_PINS[i], OUTPUT);
    if (i == motorIndex) {
      analogWrite(MOTOR_PINS[i], motorPwmValues[motorIndex]);
      Serial.printf("Motor %d (GPIO %d) set to PWM: %d\n", motorIndex, MOTOR_PINS[motorIndex], motorPwmValues[motorIndex]);
    } else {
      analogWrite(MOTOR_PINS[i], 0);
    }
  }
}

/**
 * @brief Adds a dispensing task to the queue.
 */
void addDispenseTask(int container, int motorSpeed, int triggerThreshold, int pillCount) {
  DispenseTask task = { container, motorSpeed, triggerThreshold, pillCount };
  dispenseQueue.push_back(task);
  Serial.printf("Added task: Container %d, PWM %d, Threshold %d, Pills %d\n",
                container, motorSpeed, triggerThreshold, pillCount);
}

/**
 * @brief Starts the motor for a scheduled dispensing operation.
 */
void startMotorForSchedule() {
  int index = scheduledContainer - 1;  // Convert 1-indexed container to 0-indexed motor pin
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
 * @brief Checks the schedules from schedules.json against the current time.
 */
void checkSchedules() {
  time_t now;
  time(&now);
  struct tm* timeinfo = localtime(&now);
  int currentHour = timeinfo->tm_hour;
  int currentMinute = timeinfo->tm_min;
  
  char currentTimeStr[6];
  sprintf(currentTimeStr, "%02d:%02d", currentHour, currentMinute);
  String dayStr = getCurrentDayName();
  
  if (!LittleFS.exists("/schedules.json")) {
    Serial.println("No schedules file found");
    return;
  }
  
  File file = LittleFS.open("/schedules.json", "r");
  if (!file) {
    Serial.println("Failed to open schedules file");
    return;
  }
  
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
    bool dayMatch = false;
    JsonArray days = sch["days"].as<JsonArray>();
    for (JsonVariant v : days) {
      if (v.as<String>() == dayStr) { dayMatch = true; break; }
    }
    if (!dayMatch)
      continue;
    bool timeMatch = false;
    JsonArray times = sch["times"].as<JsonArray>();
    for (JsonVariant v : times) {
      if (String(v.as<String>()) == String(currentTimeStr)) { timeMatch = true; break; }
    }
    if (!timeMatch)
      continue;
    int container = sch["container"];
    int pillCount = sch["pillCount"];
    // Retrieve container settings from settings.json (omitted here for brevity)
    // For now, assume defaults:
    int motorSpeed = 80;      // default
    int triggerThreshold = 400; // default
    Serial.printf("Schedule match: Container %d at %s on %s. Dispensing %d pill(s).\n", 
                  container, currentTimeStr, dayStr.c_str(), pillCount);
    addDispenseTask(container, motorSpeed, triggerThreshold, pillCount);
  }
}

/**
 * @brief Returns the current weekday name.
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

/**
 * @brief Serves the embedded index.html from PROGMEM.
 */
void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

/**
 * @brief GET endpoint to retrieve current schedules.
 */
void handleGetSchedules() {
  if (LittleFS.exists("/schedules.json")) {
    File file = LittleFS.open("/schedules.json", "r");
    if (!file) { server.send(500, "text/plain", "Failed to open schedules file"); return; }
    server.streamFile(file, "application/json");
    file.close();
  } else {
    server.send(200, "application/json", "[]");
  }
}

/**
 * @brief POST endpoint to save schedules.
 */
void handleSaveSchedules() {
  if (!server.hasArg("plain")) { server.send(400, "text/plain", "No data received"); return; }
  String schedulesData = server.arg("plain");
  File file = LittleFS.open("/schedules.json", "w");
  if (!file) { server.send(500, "text/plain", "Failed to open file for writing"); return; }
  file.print(schedulesData);
  file.close();
  server.send(200, "text/plain", "Schedules saved");
}

/**
 * @brief GET endpoint to retrieve settings.
 */
void handleGetSettings() {
  if (LittleFS.exists("/settings.json")) {
    File file = LittleFS.open("/settings.json", "r");
    if (!file) { server.send(500, "text/plain", "Failed to open settings file"); return; }
    server.streamFile(file, "application/json");
    file.close();
  } else {
    server.send(200, "application/json", "{}");
  }
}

/**
 * @brief POST endpoint to save settings.
 */
void handleSaveSettings() {
  if (!server.hasArg("plain")) { server.send(400, "text/plain", "No data received"); return; }
  String settingsData = server.arg("plain");
  File file = LittleFS.open("/settings.json", "w");
  if (!file) { server.send(500, "text/plain", "Failed to open settings file for writing"); return; }
  file.print(settingsData);
  file.close();
  server.send(200, "text/plain", "Settings saved");
}

/**
 * @brief GET endpoint to set the RTC using NTP.
 */
void handleSetRTC() {
  Serial.println("Received request to set RTC time.");
  configTime(0, 0, ntpServer1, ntpServer2);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 5000)) {
    Serial.println("Failed to obtain NTP time.");
    server.send(500, "text/plain", "Error: Failed to get NTP time.");
    return;
  }
  int second = timeinfo.tm_sec;
  int minute = timeinfo.tm_min;
  int hour   = timeinfo.tm_hour;
  int day    = timeinfo.tm_mday;
  int month  = timeinfo.tm_mon + 1;
  int year   = timeinfo.tm_year + 1900;
  int rtcYear = year - 2000;
  rtc.set(second, minute, hour, 0, day, month, rtcYear);
  rtc.refresh();
  char buffer[40];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
  Serial.println("RTC time successfully set to:"); Serial.println(buffer);
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
  snprintf(timeString, sizeof(timeString), "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
  server.send(200, "text/plain", timeString);
}

/**
 * @brief GET endpoint to test a motor.
 */
void handleTestMotor() {
  if (!server.hasArg("container") || !server.hasArg("motorSpeed") || !server.hasArg("triggerThreshold")) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }
  int container = server.arg("container").toInt() - 1;
  int motorSpeed = server.arg("motorSpeed").toInt();
  int triggerThreshold = server.arg("triggerThreshold").toInt();
  if (container < 0 || container >= NUM_MOTORS) { server.send(400, "text/plain", "Invalid container (motor index)"); return; }
  motorPwmValues[container]  = motorSpeed;
  thresholdValues[container] = triggerThreshold;
  if (!motorRunning) {
    currentMotor  = container;
    motorRunning  = true;
    stopControl   = false;
    previousMillis = millis();
    setMotorActive(currentMotor);
    Serial.printf("Motor test initiated for container %d: PWM %d, Threshold %d\n", container, motorSpeed, triggerThreshold);
  } else {
    Serial.println("A motor is already running. Ignoring new test command.");
  }
  server.send(200, "text/plain", "Motor test initiated");
}

/**
 * @brief GET endpoint to trigger a test schedule.
 */
void handleTestSchedule() {
  if (!LittleFS.exists("/schedules.json")) {
    server.send(404, "text/plain", "No schedules file found");
    return;
  }
  File file = LittleFS.open("/schedules.json", "r");
  if (!file) { server.send(500, "text/plain", "Failed to open schedules file"); return; }
  const size_t capacity = 2048;
  DynamicJsonDocument schedulesDoc(capacity);
  DeserializationError error = deserializeJson(schedulesDoc, file);
  file.close();
  if (error) {
    Serial.println("Failed to parse schedules JSON");
    server.send(500, "text/plain", "Failed to parse schedules JSON");
    return;
  }
  JsonArray schedules = schedulesDoc.as<JsonArray>();
  for (JsonObject sch : schedules) {
    int container = sch["container"];
    int pillCount = sch["pillCount"];
    // For this test endpoint, use default motor parameters:
    int motorSpeed = 80;
    int triggerThreshold = 400;
    addDispenseTask(container, motorSpeed, triggerThreshold, pillCount);
    Serial.printf("Test schedule: Added task for container %d: PWM %d, Threshold %d, Pills %d\n",
                  container, motorSpeed, triggerThreshold, pillCount);
  }
  server.send(200, "text/plain", "Test schedule initiated: All tasks added");
}

/**************************************************************************
 * Setup & Main Loop
 **************************************************************************/
void setup() {
  Serial.begin(115200);
  delay(1000);

  
  // Initialize I2C and RTC
  Wire.begin();
  rtc.set_rtc_address(0x68);
  rtc.set_model(URTCLIB_MODEL_DS1307);
  if (rtc.lostPower()) { Serial.println("RTC lost power! Resetting..."); rtc.lostPowerClear(); }
  else { Serial.println("RTC power OK."); }
  if (rtc.enableBattery()) { Serial.println("Battery activated."); }
  else { Serial.println("Battery activation failed!"); }
  
  // Connect to WiFi
  Serial.printf("Connecting to WiFi network: %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
  
  // Check if schedules.json exists; if not, create it with an empty JSON array
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed, attempting to format...");
    if (LittleFS.format()) {
      Serial.println("LittleFS formatted successfully.");
      if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed after formatting!");
        // Handle error as needed
      }
    } else {
      Serial.println("LittleFS formatting failed!");
      // Handle error as needed
    }
  }

  
  // Initialize LittleFS
  if (!LittleFS.begin()) { Serial.println("Error mounting LittleFS"); }
  
  // Initialize motor pins
  for (int i = 0; i < NUM_MOTORS; i++) {
    pinMode(MOTOR_PINS[i], OUTPUT);
    analogWrite(MOTOR_PINS[i], 0);
  }
  
  // Register HTTP Endpoints
  server.on("/", handleRoot);
  server.on("/index.html", handleRoot);
  server.on("/getSchedules", HTTP_GET, handleGetSchedules);
  server.on("/saveSchedules", HTTP_POST, handleSaveSchedules);
  server.on("/getSettings", HTTP_GET, handleGetSettings);
  server.on("/saveSettings", HTTP_POST, handleSaveSettings);
  server.on("/setRTC", HTTP_GET, handleSetRTC);
  server.on("/getRTCTime", HTTP_GET, handleGetRTCTime);
  server.on("/testMotor", HTTP_GET, handleTestMotor);
  server.on("/testSchedule", HTTP_GET, handleTestSchedule);
  
  server.begin();
  Serial.println("HTTP server started.");
  
  // Set system time from NTP for localtime() to work
  configTime(0, 0, ntpServer1, ntpServer2);
}

void loop() {
  server.handleClient();
  
  // Motor Control Loop
  if (motorRunning && !stopControl) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= motorInterval) {
      previousMillis = currentMillis;
      int sensorValue = analogRead(ANALOG_PIN);
      Serial.printf("Motor %d (GPIO %d) - Sensor Value: %d\n", currentMotor, MOTOR_PINS[currentMotor], sensorValue);
      if (sensorValue > thresholdValues[currentMotor]) {
        analogWrite(MOTOR_PINS[currentMotor], 0);
        motorRunning = false;
        stopControl = true;
        Serial.printf("Threshold exceeded, motor %d OFF (GPIO %d).\n", currentMotor, MOTOR_PINS[currentMotor]);
        currentMotor = -1;
        delay(interCycleDelay);
        if (scheduleActive && scheduledPillCountRemaining > 1) {
          scheduledPillCountRemaining--;
          delay(1000);
          startMotorForSchedule();
        } else { scheduleActive = false; }
      }
    }
  }
  
  // Process next task from the queue if no motor is running and no schedule is active
  if (!motorRunning && !dispenseQueue.empty() && !scheduleActive) {
    DispenseTask task = dispenseQueue.front();
    dispenseQueue.erase(dispenseQueue.begin());
    scheduledContainer = task.container;
    scheduledMotorSpeed = task.motorSpeed;
    scheduledTriggerThreshold = task.triggerThreshold;
    scheduledPillCountRemaining = task.pillCount;
    scheduleActive = true;
    startMotorForSchedule();
  }
  
  // Check schedules every scheduleCheckInterval
  if (millis() - lastScheduleCheck >= scheduleCheckInterval) {
    lastScheduleCheck = millis();
    checkSchedules();
  }
}
