#include "SPI.h"
#include "RF24.h"
#include "nRF24L01.h"
#include <XBOXONE.h>
#include <usbhub.h>

#define INTERVAL_MS_TRANSMISSION 10

#define CE_PIN 6   // CE pin for RF24
#define CSN_PIN 7  // CSN pin for RF24

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

USB Usb;
XBOXONE Xbox(&Usb);

// USB initialization and state management
uint8_t usbstate;
uint8_t laststate = 0;
USB_DEVICE_DESCRIPTOR buf;

// Data structure to hold the Xbox controller inputs
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
bool startButtonState = false;
bool radioInitialized = false;

void setup() {
  Serial.begin(115200);

#if !defined(__MIPSEL__)
  while (!Serial)
    ;  // Wait for serial port to connect
#endif

  if (Usb.Init() == -1) {
    Serial.println("USB Host initialization failed");
    while (1)
      ;  // Halt if USB fails
  } else Serial.println("USB Host initialization success");
}

void initializeRadio() {
  radio.begin();
  if (!radio.isChipConnected()) {
    Serial.println("Radio not connected. Please check your wiring.");
    while (1)
      ;  // Stop the program if the radio is not connected
  }

  // Configure the radio settings
  radio.setAutoAck(false);          // (true|false)
  radio.setDataRate(RF24_250KBPS);  // (RF24_250KBPS|RF24_1MBPS|RF24_2MBPS)
  radio.setPALevel(RF24_PA_HIGH);   // (RF24_PA_MIN|RF24_PA_LOW|RF24_PA_HIGH|RF24_PA_MAX)
  radio.setPayloadSize(sizeof(ControllerData));
  radio.openWritingPipe(address);
  radio.stopListening();
  radioInitialized = true;  // Mark radio as initialized
  Serial.println("Radio initialized");
}

void loop() {
  Usb.Task();
  usbstate = Usb.getUsbTaskState();

  if (usbstate != laststate) {
    laststate = usbstate;
    switch (usbstate) {
      case USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE:
        Serial.println("Waiting for device...");
        break;
      case USB_ATTACHED_SUBSTATE_RESET_DEVICE:
        Serial.println("Device connected. Resetting...");
        break;
      case USB_ATTACHED_SUBSTATE_WAIT_SOF:
        Serial.println("Reset complete. Waiting for the first SOF...");
        break;
      case USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE:
        Serial.println("SOF generation started. Enumerating device...");
        break;
      case USB_STATE_ADDRESSING:
        Serial.println("Setting device address...");
        break;
      case USB_STATE_RUNNING:
        Serial.println("Getting device descriptor...");
        uint8_t rcode = Usb.getDevDescr(1, 0, sizeof(USB_DEVICE_DESCRIPTOR), (uint8_t*)&buf);
        if (rcode) {
          Serial.print("Error reading device descriptor. Error code: ");
          Serial.println(rcode, HEX);
        } else {
          Serial.print("Vendor ID: ");
          Serial.println(buf.idVendor, HEX);
          Serial.print("Product ID: ");
          Serial.println(buf.idProduct, HEX);
        }
        break;
      case USB_STATE_ERROR:
        Serial.println("USB state machine reached error state");
        break;
      default:
        break;
    }
  }

  if (Xbox.XboxOneConnected) {
    // Initialize the radio only if the controller is connected and not yet initialized
    if (!radioInitialized) {
      initializeRadio();
    }

    ControllerData.rbValue = Xbox.getButtonClick(RB);  // Right bumper value
    ControllerData.lbValue = Xbox.getButtonClick(LB);  // Left bumper value

    ControllerData.dpadUp = Xbox.getButtonPress(UP);
    ControllerData.dpadDown = Xbox.getButtonPress(DOWN);
    ControllerData.A = Xbox.getButtonClick(A);
    ControllerData.B = Xbox.getButtonClick(B);
    ControllerData.X = Xbox.getButtonClick(X);
    ControllerData.Y = Xbox.getButtonClick(Y);

    if (Xbox.getButtonClick(START)) {
      startButtonState = !startButtonState;
    }
    ControllerData.startButton = startButtonState;

    ControllerData.leftX = Xbox.getAnalogHat(LeftHatX);
    ControllerData.leftY = Xbox.getAnalogHat(LeftHatY);
    ControllerData.rightX = Xbox.getAnalogHat(RightHatX);
    ControllerData.rightY = Xbox.getAnalogHat(RightHatY);

    int leftX = (ControllerData.leftX > 7500 || ControllerData.leftX < -7500) ? ControllerData.leftX : 0;
    int leftY = (ControllerData.leftY > 7500 || ControllerData.leftY < -7500) ? ControllerData.leftY : 0;
    int rightX = (ControllerData.rightX > 7500 || ControllerData.rightX < -7500) ? ControllerData.rightX : 0;
    int rightY = (ControllerData.rightY > 7500 || ControllerData.rightY < -7500) ? ControllerData.rightY : 0;

    Serial.print("LeftX: ");
    Serial.print(leftX);
    Serial.print(", LeftY: ");
    Serial.print(leftY);
    Serial.print(", RightX: ");
    Serial.print(rightX);
    Serial.print(", RightY: ");
    Serial.print(rightY);
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

    // Send the controller data over radio
    radio.write(&ControllerData, sizeof(ControllerData));
    delay(INTERVAL_MS_TRANSMISSION);
  }
}