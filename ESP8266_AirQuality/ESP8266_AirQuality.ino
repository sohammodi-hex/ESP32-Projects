#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

byte degree_symbol[8] = 
{
  0b00111,
  0b00101,
  0b00111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

// Access Point credentials
const char* ap_ssid = "ESP8266_AirQuality";  // WiFi name created by ESP8266
const char* ap_password = "12345678";         // WiFi password (min 8 characters)

// Create web server on port 80
ESP8266WebServer server(80);

// Sensor pins
int gas = A0;
#define DHTPIN 2  // D4 pin
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);

// Sensor readings
float temperature = 0;
float humidity = 0;
int gasValue = 0;
String airQuality = "Fresh Air";

// Timing
unsigned long previousMillis = 0;
const long interval = 2000;  // Update sensors every 2 seconds

void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Initialize DHT sensor
  dht.begin();
  
  // Initialize LCD
  lcd.init();
  lcd.createChar(1, degree_symbol);
  lcd.setCursor(3, 0);
  lcd.print("Air Quality");
  lcd.setCursor(3, 1);
  lcd.print("Monitoring");
  delay(2000);
  lcd.clear();
  
  // Setup Access Point
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
  delay(3000);
  lcd.clear();
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  unsigned long currentMillis = millis();
  
  // Update sensor readings periodically
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readSensors();
    updateLCD();
  }
}

void readSensors() {
  // Read DHT11
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    temperature = 0;
    humidity = 0;
  }
  
  // Read gas sensor
  gasValue = analogRead(gas);
  
  // Determine air quality
  if (gasValue < 600) {
    airQuality = "Fresh Air";
  } else {
    airQuality = "Bad Air";
  }
  
  // Print to Serial
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Humidity: ");
  Serial.print(humidity);
  Serial.print("%, Gas: ");
  Serial.print(gasValue);
  Serial.print(" - ");
  Serial.println(airQuality);
}

void updateLCD() {
  static int displayState = 0;
  
  lcd.clear();
  
  switch(displayState) {
    case 0:  // Temperature
      lcd.setCursor(0, 0);
      lcd.print("Temperature");
      lcd.setCursor(0, 1);
      lcd.print(temperature, 1);
      lcd.write(1);
      lcd.print("C");
      break;
      
    case 1:  // Humidity
      lcd.setCursor(0, 0);
      lcd.print("Humidity");
      lcd.setCursor(0, 1);
      lcd.print(humidity, 1);
      lcd.print("%");
      break;
      
    case 2:  // Gas value and air quality
      lcd.setCursor(0, 0);
      lcd.print("Gas: ");
      lcd.print(gasValue);
      lcd.setCursor(0, 1);
      lcd.print(airQuality);
      break;
  }
  
  displayState++;
  if (displayState > 2) {
    displayState = 0;
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html lang='en'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Air Quality Monitor</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 15px; box-shadow: 0 10px 40px rgba(0,0,0,0.2); }";
  html += "h1 { color: #333; text-align: center; margin-bottom: 30px; font-size: 28px; }";
  html += ".card { background: #f8f9fa; padding: 20px; margin: 15px 0; border-radius: 10px; border-left: 4px solid #667eea; }";
  html += ".card.alert { border-left-color: #dc3545; background: #fff5f5; }";
  html += ".label { font-size: 14px; color: #666; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 5px; }";
  html += ".value { font-size: 32px; font-weight: bold; color: #333; }";
  html += ".unit { font-size: 18px; color: #999; margin-left: 5px; }";
  html += ".status { display: inline-block; padding: 8px 16px; border-radius: 20px; font-weight: bold; margin-top: 10px; }";
  html += ".status.good { background: #d4edda; color: #155724; }";
  html += ".status.bad { background: #f8d7da; color: #721c24; }";
  html += ".refresh { text-align: center; margin-top: 20px; color: #666; font-size: 14px; }";
  html += "@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }";
  html += ".pulse { animation: pulse 2s infinite; }";
  html += "</style>";
  html += "<script>";
  html += "function updateData() {";
  html += "  fetch('/data').then(response => response.json()).then(data => {";
  html += "    document.getElementById('temp').innerHTML = data.temperature + '<span class=\"unit\">°C</span>';";
  html += "    document.getElementById('humidity').innerHTML = data.humidity + '<span class=\"unit\">%</span>';";
  html += "    document.getElementById('gas').innerHTML = data.gasValue;";
  html += "    var statusElem = document.getElementById('status');";
  html += "    statusElem.innerHTML = data.airQuality;";
  html += "    statusElem.className = 'status ' + (data.gasValue < 600 ? 'good' : 'bad');";
  html += "    var card = document.getElementById('gasCard');";
  html += "    if(data.gasValue >= 600) { card.classList.add('alert'); } else { card.classList.remove('alert'); }";
  html += "  });";
  html += "}";
  html += "setInterval(updateData, 2000);";
  html += "window.onload = updateData;";
  html += "</script>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>🌡️ Air Quality Monitoring</h1>";
  
  html += "<div class='card'>";
  html += "<div class='label'>Temperature</div>";
  html += "<div class='value' id='temp'>" + String(temperature, 1) + "<span class='unit'>°C</span></div>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<div class='label'>Humidity</div>";
  html += "<div class='value' id='humidity'>" + String(humidity, 1) + "<span class='unit'>%</span></div>";
  html += "</div>";
  
  html += "<div class='card' id='gasCard'>";
  html += "<div class='label'>Gas Level</div>";
  html += "<div class='value' id='gas'>" + String(gasValue) + "</div>";
  html += "<div class='status " + String(gasValue < 600 ? "good" : "bad") + "' id='status'>" + airQuality + "</div>";
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
  json += "\"airQuality\":\"" + airQuality + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}
