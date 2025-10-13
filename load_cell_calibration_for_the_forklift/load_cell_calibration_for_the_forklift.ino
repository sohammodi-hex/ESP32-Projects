#include <HX711.h>

#define DOUT D7   // same as your project
#define CLK  D8

HX711 scale;

void setup() {
  Serial.begin(115200);
  scale.begin(DOUT, CLK);
  Serial.println("HX711 raw reader");
  Serial.println("Ensure support is in place, no extra weight.");
  delay(2000);
}

void loop() {
  if (scale.is_ready()) {
    long raw = scale.read_average(10); // average of 10 samples
    Serial.print("Raw reading: ");
    Serial.println(raw);
  } else {
    Serial.println("HX711 not ready");
  }
  delay(1000);
}
