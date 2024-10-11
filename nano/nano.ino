#include <Servo.h> // Include the Servo library

Servo myServo; // Create a Servo object

const int VRX_PIN = A0; // Joystick horizontal pin
const int SERVO_PIN = 9; // Servo control pin

void setup() {
    myServo.attach(SERVO_PIN); // Attach the servo to pin D9
    Serial.begin(9600); // Start serial communication for debugging
}

void loop() {
    int vrxValue = analogRead(VRX_PIN); // Read the joystick horizontal value

    // Map the joystick value (0-1023) to the servo angle (0-180)
    int servoAngle = map(vrxValue, 0, 1023, 0, 180);
    
    myServo.write(servoAngle); // Move the servo to the mapped angle
    Serial.print("VRX: ");
    Serial.print(vrxValue);
    Serial.print(" | Servo Angle: ");
    Serial.println(servoAngle);

    delay(15); // Small delay to allow the servo to reach the position
}
