#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <PulseSensorPlayground.h>

// ===== PINS =====
#define DHTPIN D5
#define DHTTYPE DHT11
#define PULSE_PIN A0
#define BUZZER_PIN D7

// ===== CONSTANTS =====
#define TEMP_THRESHOLD 35.0       // Temp for buzzer
#define PULSE_THRESHOLD 500       // Adjust for your sensor
#define DHT_INTERVAL 2000         // 2 seconds

// ===== OBJECTS =====
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
PulseSensorPlayground pulseSensor;

// ===== VARIABLES =====
float temperature = 0.0;
float humidity = 0.0;
int bpm = 0;

// Timing
unsigned long lastDHT = 0;

// Pulse smoothing
const int MAX_BEATS = 5;
int bpmHistory[MAX_BEATS] = {0};
int bpmIndex = 0;

void setup() {
  Serial.begin(115200);

  // LCD setup
  Wire.begin(D2, D1);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Initializing...");

  // DHT
  dht.begin();

  // Pulse Sensor
  pulseSensor.analogInput(PULSE_PIN);
  pulseSensor.setThreshold(PULSE_THRESHOLD);
  pulseSensor.blinkOnPulse(false); // disable LED interrupts
  pulseSensor.begin();

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Health Monitor");
  delay(1000);
  lcd.clear();
}

void loop() {
  unsigned long now = millis();

  // --- Pulse Sensor Reading ---
  pulseSensor.sawNewSample();
  if(pulseSensor.sawStartOfBeat()){
    int currentBPM = pulseSensor.getBeatsPerMinute();

    // Only consider realistic human BPM
    if(currentBPM > 30 && currentBPM < 180){
      bpmHistory[bpmIndex] = currentBPM;
      bpmIndex = (bpmIndex + 1) % MAX_BEATS;

      // Calculate average BPM
      int sum = 0;
      for(int i=0; i<MAX_BEATS; i++) sum += bpmHistory[i];
      bpm = sum / MAX_BEATS;
    }

    Serial.print("BPM: "); Serial.println(bpm);
  }

  // --- DHT Sensor Reading every 2s ---
  if(now - lastDHT >= DHT_INTERVAL){
    lastDHT = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if(!isnan(t)) temperature = t;
    if(!isnan(h)) humidity = h;

    // Buzzer logic
    if(temperature > TEMP_THRESHOLD){
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }

    // LCD Display
    lcd.setCursor(0,0);
    lcd.print("Temp:");
    lcd.print(temperature,1);
    lcd.print("C ");

    lcd.setCursor(0,1);
    lcd.print("Hum:");
    lcd.print(humidity,0);
    lcd.print("% BPM:");
    lcd.print(bpm);
  }

  delay(2); // small non-blocking delay
}
