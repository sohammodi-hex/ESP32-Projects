#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Access Point credentials
const char* ssid = "MotorSync";
const char* password = "12345678";

// Motor1 pins
const int in1 = D1;
const int in2 = D2;

// Motor2 pins
const int in3 = D3;
const int in4 = D4;

// Motor3 pins
const int in5 = D5;
const int in6 = D6;

// Motor4 pins
const int in7 = D7;
const int in8 = D8;

// Motor states
int motor1Speed = 400;
int motor2Speed = 400;
int motor3Speed = 400;
int motor4Speed = 400;
int masterSpeed = 400;

bool motor1Running = false;
bool motor2Running = false;
bool motor3Running = false;
bool motor4Running = false;

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);

  // Motor pins setup
  pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);
  pinMode(in5, OUTPUT); pinMode(in6, OUTPUT);
  pinMode(in7, OUTPUT); pinMode(in8, OUTPUT);

  // Stop all motors initially
  digitalWrite(in1, LOW); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, LOW);
  digitalWrite(in5, LOW); digitalWrite(in6, LOW);
  digitalWrite(in7, LOW); digitalWrite(in8, LOW);

  // Create Wi-Fi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started!");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("Password: "); Serial.println(password);
  Serial.print("IP Address: "); Serial.println(WiFi.softAPIP());

  // Web server routes
  server.on("/", handleRoot);

  // Motor 1 routes
  server.on("/start1", handleStart1);
  server.on("/stop1", handleStop1);
  server.on("/speed1", handleSpeed1);

  // Motor 2 routes
  server.on("/start2", handleStart2);
  server.on("/stop2", handleStop2);
  server.on("/speed2", handleSpeed2);

  // Motor 3 routes
  server.on("/start3", handleStart3);
  server.on("/stop3", handleStop3);
  server.on("/speed3", handleSpeed3);

  // Motor 4 routes
  server.on("/start4", handleStart4);
  server.on("/stop4", handleStop4);
  server.on("/speed4", handleSpeed4);

  // Master Speed route
  server.on("/masterspeed", handleMasterSpeed);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  // Motor1
  if (motor1Running) { analogWrite(in1, motor1Speed); digitalWrite(in2, LOW); }
  else { digitalWrite(in1, LOW); digitalWrite(in2, LOW); }

  // Motor2
  if (motor2Running) { analogWrite(in3, motor2Speed); digitalWrite(in4, LOW); }
  else { digitalWrite(in3, LOW); digitalWrite(in4, LOW); }

  // Motor3
  if (motor3Running) { analogWrite(in5, motor3Speed); digitalWrite(in6, LOW); }
  else { digitalWrite(in5, LOW); digitalWrite(in6, LOW); }

  // Motor4
  if (motor4Running) { analogWrite(in7, motor4Speed); digitalWrite(in8, LOW); }
  else { digitalWrite(in7, LOW); digitalWrite(in8, LOW); }
}

// Web Interface
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>Motor Synchronization Project</title>
    <style>
      body { font-family: Arial; text-align: center; background: #f9f9f9; margin-top: 40px; }
      h1 { color: #2c3e50; }
      h2 { color: #333; margin-top: 40px; }
      button { width: 120px; height: 40px; font-size: 16px; margin: 10px; border-radius: 8px; cursor: pointer; border: none; }
      .start { background: #4CAF50; color: white; }
      .stop { background: #f44336; color: white; }
      input[type=range] { width: 80%; margin-top: 20px; }
    </style>
  </head>
  <body>
    <h1>Motor Synchronization Project</h1>

    <h2>Master Speed Control (All Motors)</h2>
    <label>Master Speed: <span id="masterVal">400</span></label><br>
    <input type="range" min="0" max="1023" value="400" oninput="updateMaster(this.value)">

    <h2>Motor 1</h2>
    <button class="start" onclick="fetch('/start1')">Start</button>
    <button class="stop" onclick="fetch('/stop1')">Stop</button><br>
    <label>Speed1: <span id="speed1Val">400</span></label><br>
    <input type="range" min="0" max="1023" value="400" oninput="updateSpeed1(this.value)">

    <h2>Motor 2</h2>
    <button class="start" onclick="fetch('/start2')">Start</button>
    <button class="stop" onclick="fetch('/stop2')">Stop</button><br>
    <label>Speed2: <span id="speed2Val">400</span></label><br>
    <input type="range" min="0" max="1023" value="400" oninput="updateSpeed2(this.value)">

    <h2>Motor 3</h2>
    <button class="start" onclick="fetch('/start3')">Start</button>
    <button class="stop" onclick="fetch('/stop3')">Stop</button><br>
    <label>Speed3: <span id="speed3Val">400</span></label><br>
    <input type="range" min="0" max="1023" value="400" oninput="updateSpeed3(this.value)">

    <h2>Motor 4</h2>
    <button class="start" onclick="fetch('/start4')">Start</button>
    <button class="stop" onclick="fetch('/stop4')">Stop</button><br>
    <label>Speed4: <span id="speed4Val">400</span></label><br>
    <input type="range" min="0" max="1023" value="400" oninput="updateSpeed4(this.value)">

    <script>
      function updateMaster(val) {
        document.getElementById('masterVal').innerText = val;
        fetch('/masterspeed?value=' + val);
      }
      function updateSpeed1(val){ document.getElementById('speed1Val').innerText = val; fetch('/speed1?value='+val); }
      function updateSpeed2(val){ document.getElementById('speed2Val').innerText = val; fetch('/speed2?value='+val); }
      function updateSpeed3(val){ document.getElementById('speed3Val').innerText = val; fetch('/speed3?value='+val); }
      function updateSpeed4(val){ document.getElementById('speed4Val').innerText = val; fetch('/speed4?value='+val); }
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// --- Motor Handlers ---
void handleStart1() { motor1Running = true; server.send(200, "text/plain", "Motor1 Started"); }
void handleStop1() { motor1Running = false; server.send(200, "text/plain", "Motor1 Stopped"); }
void handleSpeed1() { if (server.hasArg("value")) motor1Speed = server.arg("value").toInt(); server.send(200, "text/plain", "Motor1 Speed Updated"); }

void handleStart2() { motor2Running = true; server.send(200, "text/plain", "Motor2 Started"); }
void handleStop2() { motor2Running = false; server.send(200, "text/plain", "Motor2 Stopped"); }
void handleSpeed2() { if (server.hasArg("value")) motor2Speed = server.arg("value").toInt(); server.send(200, "text/plain", "Motor2 Speed Updated"); }

void handleStart3() { motor3Running = true; server.send(200, "text/plain", "Motor3 Started"); }
void handleStop3() { motor3Running = false; server.send(200, "text/plain", "Motor3 Stopped"); }
void handleSpeed3() { if (server.hasArg("value")) motor3Speed = server.arg("value").toInt(); server.send(200, "text/plain", "Motor3 Speed Updated"); }

void handleStart4() { motor4Running = true; server.send(200, "text/plain", "Motor4 Started"); }
void handleStop4() { motor4Running = false; server.send(200, "text/plain", "Motor4 Stopped"); }
void handleSpeed4() { if (server.hasArg("value")) motor4Speed = server.arg("value").toInt(); server.send(200, "text/plain", "Motor4 Speed Updated"); }

// --- Master Speed Handler ---
void handleMasterSpeed() {
  if (server.hasArg("value")) {
    masterSpeed = server.arg("value").toInt();
    motor1Speed = motor2Speed = motor3Speed = motor4Speed = masterSpeed;
  }
  server.send(200, "text/plain", "Master Speed Updated");
}
