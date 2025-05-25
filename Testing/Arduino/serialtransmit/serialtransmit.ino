#include <HardwareSerial.h>

// Use UART2 (Serial2) on ESP32
HardwareSerial mySerial(2);

// Pin definitions for UART2
#define RXD2 16  // GPIO16 for RX
#define TXD2 17  // GPIO17 for TX

// Protocol constants (match STM32)
#define START_MARKER 0x7E
#define EXPECTED_LENGTH 0x0A
#define SENSOR_TYPE_MAG 0x01
#define SENSOR_ID_QMC 0x01
#define PACKET_SIZE 11

void setup() {
  // Initialize USB Serial for debugging
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for Serial to connect
  }

  // Initialize UART2 for receiving STM32 data
  mySerial.begin(115200, SERIAL_8N1, RXD2, TXD2);
  
  Serial.println("UART2 Serial receive test started");
}

void loop() {
  static uint8_t buffer[PACKET_SIZE];
  static int index = 0;
  static bool packetStarted = false;

  // Read available bytes from UART2
  while (mySerial.available()) {
    uint8_t byte = mySerial.read();

    // Look for start marker
    if (byte == START_MARKER && !packetStarted) {
      packetStarted = true;
      index = 0;
      buffer[index++] = byte;
      continue;
    }

    // Collect packet bytes after start marker
    if (packetStarted) {
      buffer[index++] = byte;

      // Check if complete packet received
      if (index == PACKET_SIZE) {
        // Validate packet
        if (buffer[1] == EXPECTED_LENGTH && 
            buffer[2] == SENSOR_TYPE_MAG && 
            buffer[3] == SENSOR_ID_QMC) {
          
          // Calculate checksum
          uint8_t checksum = 0;
          for (int i = 1; i < PACKET_SIZE - 1; i++) {
            checksum += buffer[i];
          }
          
          if (checksum == buffer[PACKET_SIZE - 1]) {
            // Extract X, Y, Z (little-endian int16_t)
            int16_t x = (int16_t)(buffer[5] << 8 | buffer[4]);
            int16_t y = (int16_t)(buffer[7] << 8 | buffer[6]);
            int16_t z = (int16_t)(buffer[9] << 8 | buffer[8]);

            // Print to USB Serial
            Serial.print("Valid packet received: ");
            Serial.print("X=");
            Serial.print(x);
            Serial.print(", Y=");
            Serial.print(y);
            Serial.print(", Z=");
            Serial.println(z);
          } else {
            Serial.println("Checksum error");
          }
        } else {
          Serial.println("Invalid packet format");
        }

        // Reset for next packet
        packetStarted = false;
        index = 0;
      }
    }
  }
}