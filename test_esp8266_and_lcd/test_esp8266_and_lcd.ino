#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); // change to 0x3F if needed

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);      // SDA, SCL explicit for ESP8266
  lcd.init();              // initialize
  lcd.backlight();         // turn on backlight
  lcd.setCursor(0,0);
  lcd.print("Hello, LCD!");
  lcd.setCursor(0,1);
  lcd.print("ESP8266 I2C OK");
}
void loop(){}
