/*
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-mfrc522-rfid-reader-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.  
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
//#include <MFRC522DriverI2C.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

const int BeepPin = 13;
const int BlueLedPin = 26;
const int GreenLedPin = 27;
const int OpenDoorPin = 25;

// Learn more about using SPI/I2C or check the pin assigment for your board: https://github.com/OSSLibraries/Arduino_MFRC522v2#pin-layout
MFRC522DriverPinSimple ss_pin(5);

MFRC522DriverSPI driver{ss_pin}; // Create SPI driver
//MFRC522DriverI2C driver{};     // Create I2C driver
MFRC522 mfrc522{driver};         // Create MFRC522 instance

void FeedbackForGrantedAccess() {
  digitalWrite(BlueLedPin, LOW);
  digitalWrite(GreenLedPin, HIGH);
  digitalWrite(BeepPin, HIGH);   // set the Buzzer on
  digitalWrite(OpenDoorPin, HIGH);  // Send signal to display module to open the door
  Serial.println("Opening door");
  delay(500);                  // wait for 500ms
  digitalWrite(OpenDoorPin, LOW);  // Expect the display module to have opened the door within 500ms, so turn this signal off again
  digitalWrite(BlueLedPin, LOW);
  digitalWrite(GreenLedPin, LOW);
  digitalWrite(BeepPin, LOW);   // set the Buzzer off
  delay(500); 
}

// This function is called on every iteration of the main loop
// unless a card is present and access validation runs.
// Blue LED should be on for 1 second and off for 3 seconds.
void FlashBlueLed() {
  static bool isBlueLedOn = false;
  static unsigned long lastSwitchedBlueLed = 0;

  if (isBlueLedOn && millis() - lastSwitchedBlueLed >= 1000) {
    isBlueLedOn = false;
    digitalWrite(BlueLedPin, LOW);
    lastSwitchedBlueLed = millis();
  } else if (!isBlueLedOn && millis() - lastSwitchedBlueLed > 3000) {
    isBlueLedOn = true;
    digitalWrite(BlueLedPin, HIGH);
    lastSwitchedBlueLed = millis();
  }
}

// Flashes both LEDs and beeps to verify that components are OK
void RunStartupSequence() {
  digitalWrite(BeepPin, HIGH);
  digitalWrite(BlueLedPin, HIGH);
  delay(250);
  digitalWrite(BlueLedPin, LOW);
  digitalWrite(GreenLedPin, HIGH);
  delay(250);
  digitalWrite(GreenLedPin, LOW);
  digitalWrite(BeepPin, LOW);
}

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  while (!Serial);       // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4).
  
  // Setup signal to display module to open the door
  pinMode(OpenDoorPin, OUTPUT);
  digitalWrite(OpenDoorPin, LOW);

  // Setup beeper
  pinMode(BeepPin, OUTPUT);
  digitalWrite(BeepPin, LOW);

  // Setup LEDs
  pinMode(BlueLedPin, OUTPUT);
  digitalWrite(BlueLedPin, LOW);
  pinMode(GreenLedPin, OUTPUT);
  digitalWrite(GreenLedPin, LOW);

  mfrc522.PCD_Init();    // Init MFRC522 board.
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);	// Show details of PCD - MFRC522 Card Reader details.
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  // Run sequence of LEDs and beep to verify that components are OK
  RunStartupSequence();
}

void loop() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent()) {
    // Flash blue LED on an off without affecting RFID reader
    FlashBlueLed();
    return;
  }

  // Select one of the cards.
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("Card UID: ");
  MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
  Serial.println();

  // Save the UID on a String variable
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uidString += "0"; 
    }
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }

  if (uidString != "") {
    FeedbackForGrantedAccess();
    Serial.println(uidString);
  }

  // Wait 2s before accepting a new card
  delay(2000);
}