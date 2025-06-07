#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

// Motor Pins
int in1 = D5;
int in2 = D6;
int in3 = D7;
int in4 = D8;

// Relay Pins (updated)
const int relay1Pin = D3; // Changed from D0 to D3 (GPIO0) ✅
const int relay2Pin = D1; // GPIO5 is safe
const int wifiLedPin = D2; // WiFi indicator LED

String command;
int SPEED = 122;
int speed_Coeff = 3;

ESP8266WebServer server(80);

unsigned long previousMillis = 0;

String sta_ssid = "";
String sta_password = "";

void setup() {
  Serial.begin(115200);
  Serial.println("*WiFi Robot Remote Control Mode - L298N + Relay*");

  // Relay setup
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay1Pin, HIGH);  // relay OFF (active LOW)
  digitalWrite(relay2Pin, HIGH);

  pinMode(wifiLedPin, OUTPUT);
  digitalWrite(wifiLedPin, HIGH);

  // Motor setup
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);

  // WiFi connection
  String chip_id = String(ESP.getChipId(), HEX);
  chip_id = "WiFi_RC_Car-" + chip_id.substring(chip_id.length() - 4);
  String hostname(chip_id);

  Serial.println("Hostname: " + hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
  Serial.print("Connecting to: "); Serial.println(sta_ssid);

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;
  while (WiFi.status() != WL_CONNECTED && currentMillis - previousMillis <= 10000) {
    delay(500);
    Serial.print(".");
    currentMillis = millis();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n*WiFi-STA-Mode*");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    digitalWrite(wifiLedPin, LOW);
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hostname.c_str());
    Serial.println("\n*WiFi-AP-Mode*");
    Serial.print("AP IP address: "); Serial.println(WiFi.softAPIP());
    digitalWrite(wifiLedPin, HIGH);
  }

  server.on("/", HTTP_handleRoot);
  server.onNotFound(HTTP_handleRoot);
  server.begin();

  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  command = server.arg("State");

  if (command == "F") Forward();
  else if (command == "B") Backward();
  else if (command == "R") TurnRight();
  else if (command == "L") TurnLeft();
  else if (command == "G") ForwardLeft();
  else if (command == "H") BackwardLeft();
  else if (command == "I") ForwardRight();
  else if (command == "J") BackwardRight();
  else if (command == "S") Stop();
  else if (command == "R1_ON") Relay1_On();
  else if (command == "R1_OFF") Relay1_Off();
  else if (command == "R2_ON") Relay2_On();
  else if (command == "R2_OFF") Relay2_Off();
  else if (command == "0") SPEED = 60;
  else if (command == "1") SPEED = 70;
  else if (command == "2") SPEED = 81;
  else if (command == "3") SPEED = 95;
  else if (command == "4") SPEED = 105;
  else if (command == "5") SPEED = 122;
  else if (command == "6") SPEED = 150;
  else if (command == "7") SPEED = 196;
  else if (command == "8") SPEED = 272;
  else if (command == "9") SPEED = 400;
  else if (command == "q") SPEED = 1023;
}

void HTTP_handleRoot() {
  server.send(200, "text/html", "");
  if (server.hasArg("State")) {
    Serial.println(server.arg("State"));
  }
}

void Forward() {
  analogWrite(in1, SPEED); digitalWrite(in2, LOW);
  analogWrite(in3, SPEED); digitalWrite(in4, LOW);
}

void Backward() {
  digitalWrite(in1, LOW); analogWrite(in2, SPEED);
  digitalWrite(in3, LOW); analogWrite(in4, SPEED);
}

void TurnRight() {
  digitalWrite(in1, LOW); analogWrite(in2, SPEED);
  analogWrite(in3, SPEED); digitalWrite(in4, LOW);
}

void TurnLeft() {
  analogWrite(in1, SPEED); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); analogWrite(in4, SPEED);
}

void ForwardLeft() {
  analogWrite(in1, SPEED); digitalWrite(in2, LOW);
  analogWrite(in3, SPEED / speed_Coeff); digitalWrite(in4, LOW);
}

void BackwardLeft() {
  digitalWrite(in1, LOW); analogWrite(in2, SPEED);
  digitalWrite(in3, LOW); analogWrite(in4, SPEED / speed_Coeff);
}

void ForwardRight() {
  analogWrite(in1, SPEED / speed_Coeff); digitalWrite(in2, LOW);
  analogWrite(in3, SPEED); digitalWrite(in4, LOW);
}

void BackwardRight() {
  digitalWrite(in1, LOW); analogWrite(in2, SPEED / speed_Coeff);
  digitalWrite(in3, LOW); analogWrite(in4, SPEED);
}

void Stop() {
  digitalWrite(in1, LOW); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, LOW);
}

void Relay1_On() {
  Serial.println("Relay 1 ON");
  digitalWrite(relay1Pin, LOW); // Active LOW
}

void Relay1_Off() {
  Serial.println("Relay 1 OFF");
  digitalWrite(relay1Pin, HIGH);
}

void Relay2_On() {
  Serial.println("Relay 2 ON");
  digitalWrite(relay2Pin, LOW);
}

void Relay2_Off() {
  Serial.println("Relay 2 OFF");
  digitalWrite(relay2Pin, HIGH);
}
