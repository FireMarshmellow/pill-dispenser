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

// ------------------- Settings Dialog Functions -------------------
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