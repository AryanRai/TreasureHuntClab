#include <ESP32Servo.h>

Servo myServo;

#define SERVO_PIN 18  // Choose any PWM-capable pin (e.g., 18, 19, 21, 22, etc.)

void setup() {
  Serial.begin(115200);
  myServo.attach(SERVO_PIN);
  Serial.println("Enter angle (0 to 180):");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');  // Read input until Enter is pressed
    int angle = input.toInt();  // Convert to integer

    if (angle >= 0 && angle <= 180) {
      myServo.write(angle);
      Serial.print("Moving servo to ");
      Serial.print(angle);
      Serial.println(" degrees.");
    } else {
      Serial.println("Invalid angle. Enter a value between 0 and 180.");
    }
  }
}
