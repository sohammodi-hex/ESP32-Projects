#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// -------- LCD Setup --------
LiquidCrystal_I2C lcd(0x27, 16, 2);      // change to 0x3F if your module uses that address

byte degree_symbol[8] = {
  0b00111,
  0b00101,
  0b00111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

// -------- Access Point credentials --------
const char* ap_ssid     = "ESP8266_AirQuality";
const char* ap_password = "12345678";

// -------- Web server --------
ESP8266WebServer server(80);

// -------- Sensors --------
int gas = A0;               // MQ analog output to A0
int soundSensor = D5;       // KY-037/38 DO to D5 (GPIO14)

#define DHTPIN 2            // D4 on NodeMCU (GPIO2)
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// -------- State --------
float temperature = 0;
float humidity    = 0;
int   gasValue    = 0;
int   soundLevel  = 0;
String airQuality = "Fresh Air";
String noiseLevel = "Normal";

// -------- Timing --------
unsigned long previousMillis = 0;
const unsigned long interval = 2000;  // 2s update

// -------- Forward declares --------
void handleRoot();
void handleData();
void readSensors();
void updateLCD();

void setup() {
  Serial.begin(115200);
  delay(100);

  // Pins
  pinMode(soundSensor, INPUT);

  // Sensors
  dht.begin();

  // I2C + LCD (match your working test)
  Wire.begin(D2, D1);       // SDA=D2(GPIO4), SCL=D1(GPIO5)
  Wire.setClock(100000);    // optional stability at 100kHz
  lcd.init();
  lcd.backlight();
  lcd.createChar(1, degree_symbol);

  lcd.setCursor(1, 0);
  lcd.print("Air Quality &");
  lcd.setCursor(2, 1);
  lcd.print("Sound Monitor");
  delay(1500);
  lcd.clear();

  // Wi‑Fi AP
  Serial.println();
  Serial.println("Setting up Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  lcd.setCursor(0, 0);
  lcd.print("WiFi: ");
  lcd.print(ap_ssid);
  lcd.setCursor(0, 1);
  lcd.print("IP: ");
  lcd.print(IP);
  delay(2500);
  lcd.clear();

  // Web routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  unsigned long now = millis();
  if (now - previousMillis >= interval) {
    previousMillis = now;
    readSensors();
    updateLCD();
  }
}

void readSensors() {
  // DHT
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity = h;

  // MQ analog
  gasValue = analogRead(gas);

  // Sound digital
  soundLevel = digitalRead(soundSensor);

  // Qualitative statuses
  airQuality = (gasValue < 600) ? "Fresh Air" : "Bad Air";
  noiseLevel = (soundLevel == HIGH) ? "Noisy" : "Normal";

  // Debug
  Serial.print("T: "); Serial.print(temperature, 1);
  Serial.print(" C  H: "); Serial.print(humidity, 1);
  Serial.print(" %  Gas: "); Serial.print(gasValue);
  Serial.print(" -> "); Serial.print(airQuality);
  Serial.print("  Sound: "); Serial.print(soundLevel);
  Serial.print(" -> "); Serial.println(noiseLevel);
}

void updateLCD() {
  static uint8_t page = 0;

  lcd.clear();

  if (page == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Temperature");
    lcd.setCursor(0, 1);
    lcd.print(temperature, 1);
    lcd.write(1);
    lcd.print("C");
  } else if (page == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Humidity");
    lcd.setCursor(0, 1);
    lcd.print(humidity, 1);
    lcd.print("%");
  } else if (page == 2) {
    lcd.setCursor(0, 0);
    lcd.print("Gas: ");
    lcd.print(gasValue);
    lcd.setCursor(0, 1);
    lcd.print(airQuality);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Sound Level");
    lcd.setCursor(0, 1);
    lcd.print(noiseLevel);
    if (soundLevel == HIGH) lcd.print(" !");
  }

  page = (page + 1) & 0x03;  // 0..3
}

void handleRoot() {
  String html = "<!DOCTYPE html><html lang='en'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Air Quality and Sound Monitor</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 15px; box-shadow: 0 10px 40px rgba(0,0,0,0.2); }";
  html += "h1 { color: #333; text-align: center; margin-bottom: 30px; font-size: 26px; }";
  html += ".card { background: #f8f9fa; padding: 18px; margin: 12px 0; border-radius: 10px; border-left: 4px solid #667eea; }";
  html += ".card.alert { border-left-color: #dc3545; background: #fff5f5; }";
  html += ".card.warning { border-left-color: #ffc107; background: #fffbf0; }";
  html += ".label { font-size: 14px; color: #666; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 6px; }";
  html += ".value { font-size: 30px; font-weight: bold; color: #333; }";
  html += ".unit { font-size: 18px; color: #999; margin-left: 5px; }";
  html += ".status { display: inline-block; padding: 6px 14px; border-radius: 18px; font-weight: bold; margin-top: 8px; }";
  html += ".status.good { background: #d4edda; color: #155724; }";
  html += ".status.bad { background: #f8d7da; color: #721c24; }";
  html += ".status.noisy { background: #fff3cd; color: #856404; }";
  html += ".status.normal { background: #d1ecf1; color: #0c5460; }";
  html += ".refresh { text-align: center; margin-top: 14px; color: #666; font-size: 14px; }";
  html += "@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }";
  html += ".pulse { animation: pulse 2s infinite; }";
  html += ".icon { font-size: 22px; margin-right: 8px; vertical-align: middle; }";
  html += "</style>";
  html += "<script>";
  html += "function updateData() {";
  html += "  fetch('/data').then(r=>r.json()).then(data => {";
  html += "    document.getElementById('temp').innerHTML = data.temperature.toFixed(1) + '<span class=\"unit\">°C</span>';";
  html += "    document.getElementById('humidity').innerHTML = data.humidity.toFixed(1) + '<span class=\"unit\">%</span>';";
  html += "    document.getElementById('gas').innerHTML = data.gasValue;";
  html += "    var statusElem = document.getElementById('status');";
  html += "    statusElem.innerHTML = data.airQuality;";
  html += "    statusElem.className = 'status ' + (data.gasValue < 600 ? 'good' : 'bad');";
  html += "    var gasCard = document.getElementById('gasCard');";
  html += "    if (data.gasValue >= 600) { gasCard.classList.add('alert'); } else { gasCard.classList.remove('alert'); }";
  html += "    var soundStatus = document.getElementById('soundStatus');";
  html += "    soundStatus.innerHTML = data.noiseLevel;";
  html += "    soundStatus.className = 'status ' + (data.noiseLevel === 'Noisy' ? 'noisy' : 'normal');";
  html += "    var soundCard = document.getElementById('soundCard');";
  html += "    if (data.noiseLevel === 'Noisy') { soundCard.classList.add('warning'); } else { soundCard.classList.remove('warning'); }";
  html += "  }).catch(e=>console.log(e));";
  html += "}";
  html += "setInterval(updateData, 2000);";
  html += "window.onload = updateData;";
  html += "</script>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>🌡️ Air Quality & Sound Monitoring</h1>";

  html += "<div class='card'>";
  html += "<div class='label'><span class='icon'>🌡️</span>Temperature</div>";
  html += "<div class='value' id='temp'>" + String(temperature, 1) + "<span class='unit'>°C</span></div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<div class='label'><span class='icon'>💧</span>Humidity</div>";
  html += "<div class='value' id='humidity'>" + String(humidity, 1) + "<span class='unit'>%</span></div>";
  html += "</div>";

  html += "<div class='card' id='gasCard'>";
  html += "<div class='label'><span class='icon'>💨</span>Gas Level</div>";
  html += "<div class='value' id='gas'>" + String(gasValue) + "</div>";
  html += "<div class='status " + String(gasValue < 600 ? "good" : "bad") + "' id='status'>" + airQuality + "</div>";
  html += "</div>";

  html += "<div class='card' id='soundCard'>";
  html += "<div class='label'><span class='icon'>🔊</span>Sound Level</div>";
  html += "<div class='status " + String(noiseLevel == "Noisy" ? "noisy" : "normal") + "' id='soundStatus'>" + noiseLevel + "</div>";
  html += "</div>";

  html += "<div class='refresh pulse'>⟳ Auto-refreshing every 2 seconds</div>";
  html += "</div>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"gasValue\":" + String(gasValue) + ",";
  json += "\"airQuality\":\"" + airQuality + "\",";
  json += "\"soundLevel\":" + String(soundLevel) + ",";
  json += "\"noiseLevel\":\"" + noiseLevel + "\"";
  json += "}";
  server.send(200, "application/json", json);
}
