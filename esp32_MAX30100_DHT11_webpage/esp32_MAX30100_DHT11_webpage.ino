#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include "DHT.h"

// ----------------- WiFi Settings -----------------
const char* ssid = "car";       
const char* password = "car12345";  

// ----------------- Server -----------------
WebServer server(80);

// ----------------- MAX30100 -----------------
PulseOximeter pox;
#define REPORTING_PERIOD_MS 1000
uint32_t tsLastReport = 0;
float heartRate = 0;
float spO2 = 0;

// ----------------- DHT11 -----------------
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float temperature = 0;
float humidity = 0;

// ----------------- Beat Callback -----------------
void onBeatDetected() {
  Serial.println("Beat detected!");
}

// ----------------- HTML Page -----------------
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Health Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; background: #0f172a; color: #e5e7eb; }
    .card { display: inline-block; background: #111827; border-radius: 12px; margin: 10px; padding: 16px; width: 200px; }
    h2 { color: #22d3ee; }
  </style>
</head>
<body>
  <h2>ESP32 Health Monitor</h2>
  <div class="card">
    <h3>Heart Rate</h3>
    <p id="hr">--</p>
    <p>bpm</p>
  </div>
  <div class="card">
    <h3>SpO₂</h3>
    <p id="spo2">--</p>
    <p>%</p>
  </div>
  <div class="card">
    <h3>Temperature</h3>
    <p id="temp">--</p>
    <p>°C</p>
  </div>
  <div class="card">
    <h3>Humidity</h3>
    <p id="hum">--</p>
    <p>%</p>
  </div>
  <script>
    async function updateData() {
      try {
        const res = await fetch('/readdata');
        const arr = await res.json();
        document.getElementById('hr').textContent = arr[0];
        document.getElementById('spo2').textContent = arr[1];
        document.getElementById('temp').textContent = arr[2];
        document.getElementById('hum').textContent = arr[3];
      } catch(e){}
    }
    setInterval(updateData, 1000);
    updateData();
  </script>
</body>
</html>
)rawliteral";

// ----------------- Server Handlers -----------------
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleData() {
  String data = "[" + String(heartRate) + "," + String(spO2) + "," + String(temperature) + "," + String(humidity) + "]";
  server.send(200, "application/json", data);
}

// ----------------- Setup -----------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // MAX30100 I2C init
  Wire.begin();
  Serial.print("Initializing MAX30100...");
  if (!pox.begin()) {
    Serial.println("FAILED! Check wiring.");
    for (;;); 
  }
  Serial.println("SUCCESS");
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);

  // Warm-up for stable readings
  Serial.println("Warming up MAX30100...");
  for (int i = 0; i < 200; i++) {
    pox.update();
    delay(5);
  }
  Serial.println("Done.");

  // DHT11 start
  dht.begin();

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/readdata", handleData);
  server.begin();
  Serial.println("Web server started!");
}

// ----------------- Loop -----------------
void loop() {
  pox.update();           // critical for HR/SpO2
  server.handleClient();

  // Read DHT11
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity = h;

  // Serial reporting
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    heartRate = pox.getHeartRate();
    spO2 = pox.getSpO2();
    Serial.printf("HeartRate: %.1f bpm / SpO2: %.1f %% / Temp: %.1f C / Hum: %.1f %%\n",
                  heartRate, spO2, temperature, humidity);
    tsLastReport = millis();
  }
}
