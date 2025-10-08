#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "car";
const char* password = "car12345";

// Motor1 & Motor2 (DriverA)
const int in1 = D5;
const int in2 = D6;
const int in3 = D7;
const int in4 = D8;

// Motor3 & Motor4 (DriverB)
const int in5 = D1;
const int in6 = D2;
const int in7 = D3;
const int in8 = D4;

// Motor states
int motor1Speed = 400, motor2Speed = 400, motor3Speed = 400, motor4Speed = 400;
bool motor1Running = false, motor2Running = false, motor3Running = false, motor4Running = false;

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);

  // Motor pins
  int pins[] = {in1,in2,in3,in4,in5,in6,in7,in8};
  for(int i=0;i<8;i++) pinMode(pins[i], OUTPUT);

  // Stop all motors initially
  for(int i=0;i<8;i++) digitalWrite(pins[i], LOW);

  // Increase PWM frequency for smoother response
  analogWriteFreq(2000); // 2kHz PWM

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  // Web routes
  server.on("/", handleRoot);

  // Motor1
  server.on("/start1", [](){ motor1Running = true; server.send(200,"text/plain","Motor1 Started"); });
  server.on("/stop1", [](){ motor1Running = false; server.send(200,"text/plain","Motor1 Stopped"); });
  server.on("/speed1", [](){ if(server.hasArg("value")) motor1Speed=server.arg("value").toInt(); server.send(200,"text/plain","Motor1 Speed Updated"); });

  // Motor2
  server.on("/start2", [](){ motor2Running = true; server.send(200,"text/plain","Motor2 Started"); });
  server.on("/stop2", [](){ motor2Running = false; server.send(200,"text/plain","Motor2 Stopped"); });
  server.on("/speed2", [](){ if(server.hasArg("value")) motor2Speed=server.arg("value").toInt(); server.send(200,"text/plain","Motor2 Speed Updated"); });

  // Motor3
  server.on("/start3", [](){ motor3Running = true; server.send(200,"text/plain","Motor3 Started"); });
  server.on("/stop3", [](){ motor3Running = false; server.send(200,"text/plain","Motor3 Stopped"); });
  server.on("/speed3", [](){ if(server.hasArg("value")) motor3Speed=server.arg("value").toInt(); server.send(200,"text/plain","Motor3 Speed Updated"); });

  // Motor4
  server.on("/start4", [](){ motor4Running = true; server.send(200,"text/plain","Motor4 Started"); });
  server.on("/stop4", [](){ motor4Running = false; server.send(200,"text/plain","Motor4 Stopped"); });
  server.on("/speed4", [](){ if(server.hasArg("value")) motor4Speed=server.arg("value").toInt(); server.send(200,"text/plain","Motor4 Speed Updated"); });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  // Apply PWM continuously for instant response
  analogWrite(in1, motor1Running ? motor1Speed : 0); digitalWrite(in2, motor1Running ? LOW : LOW);
  analogWrite(in3, motor2Running ? motor2Speed : 0); digitalWrite(in4, motor2Running ? LOW : LOW);
  analogWrite(in5, motor3Running ? motor3Speed : 0); digitalWrite(in6, motor3Running ? LOW : LOW);
  analogWrite(in7, motor4Running ? motor4Speed : 0); digitalWrite(in8, motor4Running ? LOW : LOW);
}

// HTML Page
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>ESP8266 4-Motor Control</title>
    <style>
      body { font-family: Arial; text-align: center; margin-top: 30px; }
      h2 { color: #333; }
      button { width: 120px; height: 40px; font-size: 16px; margin: 5px; border-radius: 8px; cursor: pointer; border: none; }
      .start { background: #4CAF50; color: white; } .stop { background: #f44336; color: white; }
      input[type=range] { width: 80%; margin-top: 10px; }
      .motor { margin-bottom: 30px; }
    </style>
  </head>
  <body>
)rawliteral";

  for(int m=1;m<=4;m++){
    html += "<div class='motor'><h2>Motor"+String(m)+" Control</h2>";
    html += "<button class='start' onclick=\"fetch('/start"+String(m)+"')\">Start</button>";
    html += "<button class='stop' onclick=\"fetch('/stop"+String(m)+"')\">Stop</button><br>";
    html += "<label>Speed"+String(m)+": <span id='speed"+String(m)+"Val'>400</span></label><br>";
    html += "<input type='range' min='0' max='1023' value='400' oninput='updateSpeed"+String(m)+"(this.value)'>";
    html += "</div>";
  }

  html += "<script>\n";
  for(int m=1;m<=4;m++){
    html += "let timeout"+String(m)+";\n";
    html += "function updateSpeed"+String(m)+"(val){\n";
    html += "document.getElementById('speed"+String(m)+"Val').innerText=val;\n";
    html += "clearTimeout(timeout"+String(m)+");\n";
    html += "timeout"+String(m)+"=setTimeout(()=>{fetch('/speed"+String(m)+"?value='+val);},30);\n";
    html += "}\n";
  }
  html += "</script>\n</body></html>";

  server.send(200,"text/html",html);
}
