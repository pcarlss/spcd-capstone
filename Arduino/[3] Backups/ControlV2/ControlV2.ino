#include "SPI.h"
#include "RF24.h"
#include "nRF24L01.h"
#include <XBOXONE.h>
#include <usbhub.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define INTERVAL_MS_TRANSMISSION 10
#define LCD_UPDATE_INTERVAL 300

#define CE_PIN 6   // CE pin for RF24
#define CSN_PIN 7  // CSN pin for RF24

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

USB Usb;
XBOXONE Xbox(&Usb);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust address if needed

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

unsigned long lastLcdUpdate = 0;
bool freezeDisplay = false;
int scrollPosition = 0;  // Position of the scrolling message

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

  lcd.init();
  lcd.backlight();
  lcd.setCursor(2, 0);
  lcd.print("[SPCD Start]");
  lcd.setCursor(1, 1);
  lcd.print("Capstone  2024");
  delay(8000);
}

void initializeRadio() {
  radio.begin();
  if (!radio.isChipConnected()) {
    Serial.println("Radio not connected. Please check your wiring.");
    lcd.clear();
    lcd.print("Radio Error");
    delay(2000);
    while (1)
      ;  // Stop the program if the radio is not connected
  } else if (radio.isChipConnected()) {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Radio Success");
    delay(3000);
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

void scrollString(String message) {
  // Clear the display and print the message at the start
  lcd.clear();
  lcd.print(message);

  // Calculate the length of the message
  int messageLength = message.length();

  // Scroll the message if it's longer than 16 characters
  if (messageLength > 16) {
    // Loop through the message to create a scrolling effect
    for (int position = 0; position <= messageLength - 16; position++) {
      lcd.setCursor(0, 0);                                    // Move cursor to the beginning
      lcd.print(message.substring(position, position + 16));  // Print the substring
      delay(120);                                             // Introduce a small delay to control the scrolling speed
    }

    // Wait until the message is completely off the screen
    for (int position = messageLength - 16; position < messageLength; position++) {
      lcd.setCursor(0, 0);                                    // Move cursor to the beginning
      lcd.print("                ");                          // Clear the line for scrolling effect
      lcd.setCursor(0, 0);                                    // Move cursor to the beginning
      lcd.print(message.substring(position, position + 16));  // Print the substring
      delay(120);                                             // Introduce a small delay to control the scrolling speed
    }

    // Clear the display to remove any remaining characters
    lcd.clear();
  } else {
    // If the message fits, print it without scrolling
    lcd.clear();
    lcd.print(message);
  }
}

void loop() {
  Usb.Task();
  usbstate = Usb.getUsbTaskState();

  if (usbstate != laststate) {
    laststate = usbstate;
    switch (usbstate) {
      case USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE:
        Serial.println("Waiting for device...");
        scrollString("Waiting for device...");
        lcd.setCursor(2, 0);
        lcd.print("Insert USB");
        break;
      case USB_ATTACHED_SUBSTATE_RESET_DEVICE:
        Serial.println("Device connected. Resetting...");
        // scrollString("Device connected.");
        break;
      case USB_ATTACHED_SUBSTATE_WAIT_SOF:
        Serial.println("Reset complete. Waiting for the first SOF...");
        // scrollString("Reset complete. Waiting for SOF...");
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("USB Connected");
        delay(1000);
        break;
      case USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE:
        Serial.println("SOF generation started. Enumerating device...");
        // scrollString("SOF generation started...");
        break;
      case USB_STATE_ADDRESSING:
        Serial.println("Setting device address...");
        // scrollString("Setting device address...");
        break;
      case USB_STATE_RUNNING:
        Serial.println("Getting device descriptor...");
        // scrollString("Getting device descriptor...");
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

// Check if 500 ms have passed since the last update
if (millis() - lastLcdUpdate >= LCD_UPDATE_INTERVAL) {
    lastLcdUpdate = millis();  // Update the last update time

    // Check if all values are 0
    if (leftX == 0 && leftY == 0 && rightX == 0 && rightY == 0) {
        // If the display hasn't been frozen yet, update it to show 0s
        if (!freezeDisplay) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("LX 0");
            lcd.setCursor(0, 1);
            lcd.print("LY 0");
            lcd.setCursor(8, 0);
            lcd.print("RX 0");
            lcd.setCursor(8, 1);
            lcd.print("RY 0");
            freezeDisplay = true;  // Freeze the display after showing 0s
        }
    } else {
        // If at least one value is non-zero, update the display
        freezeDisplay = false;  // Reset freeze flag to allow updates
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("LX");  
        lcd.print(leftX >= 0 ? "+" : "");  // Print "+" for positive values
        lcd.print(leftX);
        lcd.setCursor(0, 1);
        lcd.print("LY");
        lcd.print(leftY >= 0 ? "+" : "");  // Print "+" for positive values
        lcd.print(leftY);

        lcd.setCursor(8, 0);
        lcd.print("RX");
        lcd.print(rightX >= 0 ? "+" : "");  // Print "+" for positive values
        lcd.print(rightX);
        lcd.setCursor(8, 1);
        lcd.print("RY");
        lcd.print(rightY >= 0 ? "+" : "");  // Print "+" for positive values
        lcd.print(rightY);
    }
}


    // Send the controller data over radio
    radio.write(&ControllerData, sizeof(ControllerData));
    delay(INTERVAL_MS_TRANSMISSION);
  }
}
