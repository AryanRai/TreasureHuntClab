#include <Wire.h>
#include <AS5600.h>  // Use "AS5600" library by Rob Tillaart or Patrick M.

AS5600 as5600;

void setup() {
  Serial.begin(115200);

  // Start I2C (ESP32 default pins: SDA = 21, SCL = 22)
  Wire.begin();

  as5600.begin();  // Initialize AS5600
  delay(100);
  
  Serial.println("AS5600 Magnet Detection Test");
}

void loop() {
  bool magnetDetected = as5600.detectMagnet();

  if (magnetDetected) {
    Serial.println("🧲 Magnet Detected!");
  } else {
    Serial.println("❌ No Magnet.");
  }

  delay(500);
}
