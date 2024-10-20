#include "SPI.h"
#include "RF24.h"
#include "nRF24L01.h"
#include <Servo.h>  // Add the Servo library

#define CE_PIN 9
#define CSN_PIN 10
#define SERVO_PIN 8
#define INTERVAL_MS_SIGNAL_LOST 1000
#define INTERVAL_MS_SIGNAL_RETRY 250

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

Servo myServo;
bool state = false;

struct ControllerData {
  int leftX;
  int leftY;
  int rightX;
  int rightY;
  int rbValue;       // Right bumper value
  int lbValue;       // Left bumper value
  bool dpadUp;       // D-pad up button
  bool dpadDown;     // D-pad down button
  bool A;            // A button
  bool B;            // B button
  bool X;            // X button
  bool Y;            // Y button
  bool startButton;  // Start button
};

ControllerData ControllerData;
unsigned long lastSignalMillis = 0;

void setup() {
  Serial.begin(115200);
  radio.begin();

  // Append ACK packet from the receiving radio back to the transmitting radio
  radio.setAutoAck(false);  // (true|false)

  // Set the transmission data rate
  radio.setDataRate(RF24_250KBPS);  // (RF24_250KBPS|RF24_1MBPS|RF24_2MBPS)

  // Greater level = more consumption = longer distance
  radio.setPALevel(RF24_PA_MIN);  // (RF24_PA_MIN|RF24_PA_LOW|RF24_PA_HIGH|RF24_PA_MAX)

  // Set the payload size according to the ControllerData structure
  radio.setPayloadSize(sizeof(ControllerData));

  // Act as receiver
  radio.openReadingPipe(1, address);  // Changed to pipe 1
  radio.startListening();

  // Attach the servo to its pin
  myServo.attach(SERVO_PIN);
  myServo.write(90);  // Initialize the servo to the middle position (90 degrees)
}

void loop() {
  unsigned long currentMillis = millis();
  if (radio.available() > 0) {
    radio.read(&ControllerData, sizeof(ControllerData));

    ControllerData.leftX = (ControllerData.leftX > 7500 || ControllerData.leftX < -7500) ? ControllerData.leftX : 0;
    ControllerData.leftY = (ControllerData.leftY > 7500 || ControllerData.leftY < -7500) ? ControllerData.leftY : 0;
    ControllerData.rightX = (ControllerData.rightX > 7500 || ControllerData.rightX < -7500) ? ControllerData.rightX : 0;
    ControllerData.rightY = (ControllerData.rightY > 7500 || ControllerData.rightY < -7500) ? ControllerData.rightY : 0;

    if (ControllerData.rightY == 0) {
      if (ControllerData.startButton == 1) myServo.write(180);
      else if (ControllerData.startButton == 0) myServo.write(0);
    } else {
      // Control the servo with right joystick Y-axis
      int servoPos = map(ControllerData.rightY, -32768, 32767, 0, 180);  // Map joystick range to servo angle
      myServo.write(servoPos);
    }

    // Print the received data
    Serial.print("LeftX: ");
    Serial.print(ControllerData.leftX);
    Serial.print(", LeftY: ");
    Serial.print(ControllerData.leftY);
    Serial.print(", RightX: ");
    Serial.print(ControllerData.rightX);
    Serial.print(", RightY: ");
    Serial.print(ControllerData.rightY);
    Serial.print(", RB: ");
    Serial.print(ControllerData.rbValue);
    Serial.print(", LB: ");
    Serial.print(ControllerData.lbValue);
    Serial.print(", DPad Up: ");
    Serial.print(ControllerData.dpadUp);
    Serial.print(", DPad Down: ");
    Serial.print(ControllerData.dpadDown);
    Serial.print(", A: ");
    Serial.print(ControllerData.A);
    Serial.print(", B: ");
    Serial.print(ControllerData.B);
    Serial.print(", X: ");
    Serial.print(ControllerData.X);
    Serial.print(", Y: ");
    Serial.print(ControllerData.Y);
    Serial.print(", Start: ");
    Serial.println(ControllerData.startButton);


    lastSignalMillis = currentMillis;
  }
  if (currentMillis - lastSignalMillis > INTERVAL_MS_SIGNAL_LOST) {
    lostConnection();
  }
}

void lostConnection() {
  Serial.println("We have lost connection, preventing unwanted behavior");
  delay(INTERVAL_MS_SIGNAL_RETRY);
}
