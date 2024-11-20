#include "SPI.h"
#include "RF24.h"
#include "nRF24L01.h"
#include <IBusBM.h>  // Include IBusBM library for IBUS communication
#include <Arduino.h>

#define CE_PIN 9
#define CSN_PIN 10
#define INTERVAL_MS_SIGNAL_LOST 1000
#define INTERVAL_MS_SIGNAL_RETRY 250

RF24 radio(CE_PIN, CSN_PIN);      // Initialize the RF24 radio module
IBusBM ibus;                      // Create an IBusBM object for IBUS communication
const byte address[6] = "00001";  // Address for RF24 communication

unsigned long lastSignalMillis = 0;
bool DEBUG_MODE = true;  // Set to true to enable Serial Monitor debugging, false for iBUS communication

struct ControllerData {
  int throttle;
  int yaw;
  int pitch;
  int roll;
} ControllerData;

void setup() {
  Serial.begin(115200);  // Use hardware Serial at 115200 baud
  if (!DEBUG_MODE) {
    ibus.begin(Serial);  // Initialize the IBUS communication only if not in debug mode
  }

  // Initialize RF24 radio settings
  radio.begin();
  radio.setAutoAck(false);         // (true|false)
  radio.setDataRate(RF24_1MBPS);   // (RF24_250KBPS|RF24_1MBPS|RF24_2MBPS)
  radio.setPALevel(RF24_PA_HIGH);  // (RF24_PA_MIN|RF24_PA_LOW|RF24_PA_HIGH|RF24_PA_MAX)
  radio.setPayloadSize(sizeof(ControllerData));
  radio.openReadingPipe(1, address);  // Changed to pipe 1
  radio.setChannel(100);
  radio.startListening();
}

void loop() {
  delay(250);
  unsigned long currentMillis = millis();
  if (radio.available()) {
    radio.read(&ControllerData, sizeof(ControllerData));

    if (DEBUG_MODE) {
      // Print formatted iBUS data to Serial Monitor for debugging
      printIBUSData(0, ControllerData.throttle);
      printIBUSData(1, ControllerData.yaw);
      printIBUSData(2, ControllerData.pitch);
      printIBUSData(3, ControllerData.roll);
    } else {
      // Send RC data over iBUS
      ibus.sendIBUSData(0, ControllerData.throttle);  // Channel 0 for Throttle
      ibus.sendIBUSData(1, ControllerData.yaw);       // Channel 1 for Yaw
      ibus.sendIBUSData(2, ControllerData.pitch);     // Channel 2 for Pitch
      ibus.sendIBUSData(3, ControllerData.roll);      // Channel 3 for Roll
    }

    lastSignalMillis = currentMillis;
  }

  if (currentMillis - lastSignalMillis > INTERVAL_MS_SIGNAL_LOST) {
    lostConnection();
  }
}

void lostConnection() {
  if (DEBUG_MODE) {
    Serial.println("We have lost connection, preventing unwanted behavior");
  }
  delay(INTERVAL_MS_SIGNAL_RETRY);
}

// Global array to store values for each channel
uint16_t channelValues[14];

// Function to set a channel value
void setChannelValue(uint8_t channel, uint16_t value) {
    if (channel < 14) {
        channelValues[channel] = value;  // Store the value in the array for that channel
    }
}

// Function to format and print all iBUS channel data in a single line
void printAllChannels() {
  // Prepare the iBUS packet for printing channel data
  uint8_t packet[4 + 14 * 2];  // 4 bytes for header and checksum + 14 channels * 2 bytes each

  // Set packet length and command
  packet[0] = 0x20;  // Fixed packet length for iBUS
  packet[1] = 0x40;  // Command for setting servo/motor speed

  // Populate the packet with values from the channelValues array
  for (uint8_t i = 0; i < 14; i++) {
    packet[2 + i * 2] = channelValues[i] & 0xFF;           // Low byte of the channel value
    packet[3 + i * 2] = (channelValues[i] >> 8) & 0xFF;    // High byte of the channel value
  }

  // Calculate checksum
  uint16_t checksum = 0xFFFF;
  for (uint8_t i = 0; i < sizeof(packet) - 2; i++) {
    checksum -= packet[i];
  }
  packet[sizeof(packet) - 2] = checksum & 0xFF;       // Low byte of checksum
  packet[sizeof(packet) - 1] = (checksum >> 8) & 0xFF; // High byte of checksum

  // Print all channel data in a single line
  Serial.print("Channels: ");
  for (uint8_t i = 0; i < 14; i++) {
    Serial.print("C");
    Serial.print(i);
    Serial.print(":");
    
    // Print each channel value as a 4-digit number with leading zeros
    if (channelValues[i] < 1000) Serial.print("0");
    if (channelValues[i] < 100) Serial.print("0");
    if (channelValues[i] < 10) Serial.print("0");
    Serial.print(channelValues[i]);
    
    Serial.print(" ");
  }

  // Print the full packet in hexadecimal format
  Serial.print("Packet: ");
  for (uint8_t i = 0; i < sizeof(packet); i++) {
    if (packet[i] < 16) Serial.print("0");  // Add leading zero for consistent 2-digit hex format
    Serial.print(packet[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

