#include <XBOXONE.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <usbhub.h>  // Include USBHost Shield library for better USB handling

// USB Host Objects
USB Usb;
XBOXONE Xbox(&Usb);

// NRF24L01 radio object (pins for CE and CSN may vary, adjust if needed)
RF24 radio(7, 8);  // CE pin 7, CSN pin 8

// Define the radio pipe address (same on both receiver and transmitter)
const byte address[6] = "00001";  // Address for communication

// Data structure to hold the Xbox controller inputs
struct ControllerData {
  int leftX;
  int leftY;
  int rightX;
  int rightY;
  int rbValue;
  int lbValue;
  bool dpadUp;
  bool dpadDown;
  bool A;
  bool B;
  bool X;
  bool Y;
  bool startButton;
};

ControllerData controllerData;

// USB initialization and state management
uint8_t usbstate;
uint8_t laststate = 0;
USB_DEVICE_DESCRIPTOR buf;  // To hold the device descriptor

void setup() {
  // Begin Serial Communication
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while (!Serial);  // Wait for serial port to connect
#endif

  // Initialize USB and check for errors
  if (Usb.Init() == -1) {
    Serial.println("USB Host initialization failed");
    while (1)
      ;  // Halt if USB fails
  }

  // // Initialize NRF24L01 radio
  // radio.begin();
  // radio.openReadingPipe(0, address); // Open reading pipe
  // radio.startListening();  // Start listening for incoming radio data

  Serial.println("Setup complete. Waiting for controller...");
}

void loop() {
  // Process USB tasks and check the state
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
          // Print device descriptor details
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

  unsigned long lastJoystickPrintTime = 0;
  unsigned long lastButtonPrintTime = 0;

  if (Xbox.XboxOneConnected) {
    controllerData.leftX = Xbox.getAnalogHat(LeftHatX);
    controllerData.leftY = Xbox.getAnalogHat(LeftHatY);
    controllerData.rightX = Xbox.getAnalogHat(RightHatX);
    controllerData.rightY = Xbox.getAnalogHat(RightHatY);

    controllerData.rbValue = Xbox.getButtonClick(RB);  // Right bumper value
    controllerData.lbValue = Xbox.getButtonClick(LB);  // Left bumper value

    controllerData.dpadUp = Xbox.getButtonPress(UP);
    controllerData.dpadDown = Xbox.getButtonPress(DOWN);
    controllerData.A = Xbox.getButtonClick(A);
    controllerData.B = Xbox.getButtonClick(B);
    controllerData.X = Xbox.getButtonClick(X);
    controllerData.Y = Xbox.getButtonClick(Y);
    controllerData.startButton = Xbox.getButtonClick(START);

    // Delay for joystick printing (50 ms)
    if (millis() - lastJoystickPrintTime >= 50) {
      bool printNewLine = false;  // Flag to check if we need a new line

      if (controllerData.leftX > 7500 || controllerData.leftX < -7500 || controllerData.leftY > 7500 || controllerData.leftY < -7500) {
        Serial.print(F("LeftHatX: "));
        Serial.print(controllerData.leftX > 0 ? "+" : "");  // Add + for positive values
        Serial.print(controllerData.leftX);
        Serial.print(F("     LeftHatY: "));
        Serial.print(controllerData.leftY > 0 ? "+" : "");  // Add + for positive values
        Serial.print(controllerData.leftY);
        printNewLine = true;  // Set flag to true to print a newline later
      }

      if (controllerData.rightX > 7500 || controllerData.rightX < -7500 || controllerData.rightY > 7500 || controllerData.rightY < -7500) {
        if (printNewLine) {
          Serial.println();  // Print a newline if we've printed left hat data
        }
        Serial.print(F("RightHatX: "));
        Serial.print(controllerData.rightX > 0 ? "+" : "");  // Add + for positive values
        Serial.print(controllerData.rightX);
        Serial.print(F("     RightHatY: "));
        Serial.print(controllerData.rightY > 0 ? "+" : "");
        Serial.println(controllerData.rightY);  // Add + for positive values
      } else if (printNewLine) {
        Serial.println();  // Print a newline if we printed left hat data
      }

      lastJoystickPrintTime = millis();  // Reset joystick print time
    }



    // Delay for button printing (10 ms)
    if (millis() - lastButtonPrintTime >= 50) {
      if (controllerData.dpadUp) Serial.println(F("Up"));
      if (controllerData.dpadDown) Serial.println(F("Down"));
      if (controllerData.rbValue) Serial.println(F("RB"));
      if (controllerData.lbValue) Serial.println(F("LB"));
      if (controllerData.startButton) Serial.println(F("Start"));
      if (controllerData.A) Serial.println(F("A"));
      if (controllerData.B) Serial.println(F("B"));
      if (controllerData.X) Serial.println(F("X"));
      if (controllerData.Y) Serial.println(F("Y"));

      lastButtonPrintTime = millis();  // Reset button print time
    }

    // Send data via NRF24L01 (if needed)
    if (radio.available()) {
      radio.read(&controllerData, sizeof(controllerData));  // Read incoming data
    }
  }
}
