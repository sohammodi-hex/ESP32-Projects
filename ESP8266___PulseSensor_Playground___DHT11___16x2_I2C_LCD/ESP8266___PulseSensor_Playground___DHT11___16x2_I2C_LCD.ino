// ESP8266 + PulseSensor Playground + DHT11 + 16x2 I2C LCD
#define USE_ARDUINO_INTERRUPTS false
#include <PulseSensorPlayground.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ----- PulseSensor -----
const int PULSE_INPUT = A0;  
const int THRESHOLD = 550;  
const unsigned long SAMPLE_US = 2000;  // 500 Hz
PulseSensorPlayground pulseSensor;

// ----- DHT11 -----
#define DHTPIN 2          
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
unsigned long lastDHT = 0;
const unsigned long DHT_PERIOD_MS = 2000;

// ----- LCD 16x2 -----
LiquidCrystal_I2C lcd(0x27, 16, 2); // Change 0x27 if your LCD I2C address is different

// ----- Timing -----
unsigned long lastSample = 0;

void setup() {
  Serial.begin(115200);

  // Pulse Sensor setup
  pulseSensor.analogInput(PULSE_INPUT);
  pulseSensor.setThreshold(THRESHOLD);
  pulseSensor.begin();

  // DHT setup
  dht.begin();

  // LCD setup
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Pulse + DHT11");

  Serial.println("Starting Pulse + DHT11 + LCD...");
}

void loop() {
  unsigned long now = micros();
  if (now - lastSample >= SAMPLE_US) {
    lastSample += SAMPLE_US;
    pulseSensor.sawNewSample();

    if (pulseSensor.sawStartOfBeat()) {
      int bpm = pulseSensor.getBeatsPerMinute();
      Serial.print("BPM: "); Serial.println(bpm);

      // Update LCD first row with BPM
      lcd.setCursor(0,0);
      lcd.print("BPM:");
      lcd.setCursor(4,0);
      lcd.print("    ");          // clear old digits
      lcd.setCursor(4,0);
      lcd.print(bpm);
    }
  }

  unsigned long nowMs = millis();
  if (nowMs - lastDHT >= DHT_PERIOD_MS) {
    lastDHT = nowMs;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println("DHT11 read failed");
      lcd.setCursor(0,1);
      lcd.print("DHT error      "); // pad to clear row
    } else {
      Serial.print("Temp(C): "); Serial.print(t,1);
      Serial.print("  Hum(%): "); Serial.println(h,1);

      // Update LCD second row with Temp & Hum
      lcd.setCursor(0,1);
      lcd.print("T:");
      lcd.print(t,1);
      lcd.print("C H:");
      lcd.print(h,1);
      lcd.print("%");
    }
  }
}
