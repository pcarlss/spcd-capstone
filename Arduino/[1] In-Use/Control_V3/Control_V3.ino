#include "SPI.h"
#include "RF24.h"
#include "nRF24L01.h"
#include <XBOXONE.h>
#include <usbhub.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////// DEFINITIONS AND VARIABLES ///////////////////////////////

#define INTERVAL_MS_TRANSMISSION 10
#define LCD_UPDATE_INTERVAL 300
#define CE_PIN 6   // CE pin for RF24
#define CSN_PIN 7  // CSN pin for RF24

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

USB Usb;
XBOXONE Xbox(&Usb);
LiquidCrystal_I2C lcd(0x27, 16, 2);

uint8_t usbstate;
uint8_t laststate = 0;
USB_DEVICE_DESCRIPTOR buf;

bool radioInitialized = false;
unsigned long lastLcdUpdate = 0;
bool freezeDisplay = false;
int scrollPosition = 0;

struct ControllerData {
  int throttle;
  int yaw;
  int pitch;
  int roll;
} ControllerData;

/////////////////////////////////////////////////////////////////////////////
/////////////////// VOID SETUP //////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while (!Serial)
    ;
#endif
  if (Usb.Init() == -1) {
    Serial.println("USB Host initialization failed");
    while (1)
      ;
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

  radio.setAutoAck(false);         // (true|false)
  radio.setDataRate(RF24_1MBPS);   // (RF24_250KBPS|RF24_1MBPS|RF24_2MBPS)
  radio.setPALevel(RF24_PA_HIGH);  // (RF24_PA_MIN|RF24_PA_LOW|RF24_PA_HIGH|RF24_PA_MAX)
  radio.setPayloadSize(sizeof(ControllerData));
  radio.openWritingPipe(address);
  radio.setChannel(100);
  radio.stopListening();

  Serial.println("Radio initialized");
  radioInitialized = true;  // Mark radio as initialized
}

/////////////////////////////////////////////////////////////////////////////
/////////////////// SCROLL LCD FUNCTION /////////////////////////////////////

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

/////////////////////////////////////////////////////////////////////////////
/////////////////// VOID LOOP MAIN //////////////////////////////////////////

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

  if (Xbox.XboxOneConnected && !radioInitialized) {
    initializeRadio();
  }

  if (Xbox.XboxOneConnected) {

    // Define the dead zone threshold
    const int DEAD_ZONE = 7500;

    int throttleInput = Xbox.getAnalogHat(LeftHatY);
    int yawInput = Xbox.getAnalogHat(LeftHatX);
    int pitchInput = Xbox.getAnalogHat(RightHatY);
    int rollInput = Xbox.getAnalogHat(RightHatX);

    ControllerData.throttle = (abs(throttleInput) < DEAD_ZONE) ? 1500 : map(throttleInput, -32767, 32767, 1000, 2000);
    ControllerData.yaw = (abs(yawInput) < DEAD_ZONE) ? 1500 : map(yawInput, -32767, 32767, 1000, 2000);
    ControllerData.pitch = (abs(pitchInput) < DEAD_ZONE) ? 1500 : map(pitchInput, -32767, 32767, 1000, 2000);
    ControllerData.roll = (abs(rollInput) < DEAD_ZONE) ? 1500 : map(rollInput, -32767, 32767, 1000, 2000);

    Serial.print("Throttle: ");
    Serial.print(ControllerData.throttle);
    Serial.print(", Yaw: ");
    Serial.print(ControllerData.yaw);
    Serial.print(", Pitch: ");
    Serial.print(ControllerData.pitch);
    Serial.print(", Roll: ");
    Serial.println(ControllerData.roll);

    radio.write(&ControllerData, sizeof(ControllerData));  //send data
    delay(INTERVAL_MS_TRANSMISSION);
  }
}