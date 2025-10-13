#include <AccelStepper.h>

// 28BYJ-48 half-step driver wiring order (verify your board's sequence)
#define IN1 8
#define IN3 9
#define IN2 10
#define IN4 11

// AccelStepper interface type 8 = half-step
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

// Typical 28BYJ-48 ≈4096 half-steps per revolution
const long STEPS_PER_REV = 4096;
const float STEPS_PER_DEG = (float)STEPS_PER_REV / 360.0f;

long target = 0;   // target step position (0 = center)

void setup() {
  Serial.begin(115200);

  stepper.setMaxSpeed(1200);      // tune for supply/mechanics
  stepper.setAcceleration(800);   // smooth start/stop
  stepper.setCurrentPosition(0);  // define current as center
}

void loop() {
  // Run motor toward target
  stepper.moveTo(target);
  stepper.run();

  // Serial parsing
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      if (line.startsWith("X")) {
        int delta_units = line.substring(1).toInt();  // signed small integer
        long delta_steps = (long)(delta_units * (STEPS_PER_DEG * 1.0f));
        target += delta_steps;
        // Clamp to +/- 60 deg to protect wiring
        long max_steps = (long)(60.0f * STEPS_PER_DEG);
        if (target > max_steps) target = max_steps;
        if (target < -max_steps) target = -max_steps;
      } else if (line == "CENTER") {
        target = 0;
      } else if (line == "STOP") {
        // Option: freeze where it is
        target = stepper.currentPosition();
      }
      line = "";
    } else if (isPrintable(c)) {
      line += c;
    }
  }
}
