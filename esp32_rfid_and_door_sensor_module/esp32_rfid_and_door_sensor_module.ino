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

// Adapted from: https://github.com/espressif/arduino-esp32/blob/master/libraries/LittleFS/examples/LITTLEFS_test/LITTLEFS_test.ino
// Project details: https://RandomNerdTutorials.com/esp32-write-data-littlefs-arduino/
#include <Arduino.h>
#include "FS.h"
#include <LittleFS.h>
#define FORMAT_LITTLEFS_IF_FAILED true

#include <WiFi.h>
#include <PubSubClient.h>
#include <atomic>
#include <vector>
#include "time.h"
#include "esp_sntp.h"
#include "parameters.h"

const int BeepPin = 13;
const int BlueLedPin = 26;
const int GreenLedPin = 27;
const int OpenDoorPin = 25;
const int MagnetSensorPin = 15;

char doorSensorState[7] = "closed";
std::atomic<bool> doorSensorStateChanged = false;
unsigned long doorSensorChangedTime = 0;
std::vector<String> validRfidValues = { };

// Learn more about using SPI/I2C or check the pin assigment for your board: https://github.com/OSSLibraries/Arduino_MFRC522v2#pin-layout
MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin}; // Create SPI driver
MFRC522 mfrc522{driver};         // Create MFRC522 instance

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);  // MQTT

void checkDoorSensorState() {
  if (digitalRead(MagnetSensorPin) == LOW) {
    // Capture change if previous state was open, and that state was captured at least 1s ago
    if (strcmp(doorSensorState, "open") == 0 && (millis() - doorSensorChangedTime) >= 1000) {
      strncpy(doorSensorState, "closed", 7);
      doorSensorStateChanged = true;
      doorSensorChangedTime = millis();
    }
  } else {
    // Capture change if previous state was closed, and that state was captured at least 1s ago
    if (strcmp(doorSensorState, "closed") == 0 && (millis() - doorSensorChangedTime) >= 1000) {
      strncpy(doorSensorState, "open", 7);
      doorSensorStateChanged = true;
      doorSensorChangedTime = millis();
    }
  }

  // Report new state if it has actually changed
  if (doorSensorStateChanged && strcmp(doorSensorState, "closed") == 0) {
    Serial.println("Door is closed");
    // Message for ESP32 display module
    mqttClient.publish("garageDoor/doorStateChanged", "closed");
    // Message for Home Assitant
    mqttClient.publish("door/sensor", "CLOSED");
    doorSensorStateChanged = false;
  } else if (doorSensorStateChanged && strcmp(doorSensorState, "open") == 0) {
    Serial.println("Door is opening");
    // Message for ESP32 display module
    mqttClient.publish("garageDoor/doorStateChanged", "opening");
    // Message for Home Assitant
    mqttClient.publish("door/sensor", "OPEN");
    doorSensorStateChanged = false;
  }
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// Callback function (gets called when time adjusts via NTP)
void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void incomingMqttMessage(char *topic, uint8_t *message, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  Serial.print("Message: ");

  String value = "";

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    value += (char)message[i];
  }

  Serial.println();

  if (strcmp(topic, "garageDoor/queryDeviceStatus") == 0) {
    // Send current state of garage door sensor
    if (digitalRead(MagnetSensorPin) == HIGH) {
      strncpy(doorSensorState, "open", 7);
      doorSensorStateChanged = true;
    } else {
      strncpy(doorSensorState, "closed", 7);
      doorSensorStateChanged = true;
    }
  }
}

void publishDiscoveryMqttMessage() {
  String discoveryMessage = "{"
    "\"name\": \"Garage Door\","
    "\"unique_id\": \"garage_door_sensor\","
    "\"state_topic\": \"door/sensor\","
    "\"payload_on\": \"OPEN\","
    "\"payload_off\": \"CLOSED\","
    "\"device_class\": \"door\","
    "\"icon\": \"mdi:garage\""
  "}";

  Serial.println("Publishing Home Assistant Binary Sensor Discovery Message");
  mqttClient.publish("homeassistant/binary_sensor/garage_door_sensor/config", discoveryMessage.c_str(), true); // true means the message is retained when HA is restarted
}

void reconnectMqttBroker() {
  while (!mqttClient.connected()) {
    Serial.printf("Connecting to %s...\n", mqttServer);

    if (mqttClient.connect("GarageDoorSensor", mqtt_user, mqtt_password)) {
      Serial.printf("connected to %s\n", mqttServer);
    } else {
      Serial.print("Failed, rc=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }

  subscribeToMqttTopics();
}

void subscribeToMqttTopics() {
  if (mqttClient.subscribe("garageDoor/queryDeviceStatus")) {
    Serial.println("Subscribed to topic: garageDoor/queryDeviceStatus");
  } else {
    Serial.println("Failed to subscribe to topic!");
  }
}

bool ValidateRfid(String rfid) {
  if (rfid.length() == 0) {
    return false;
  }

  Serial.printf("Validating %s. ", rfid);
  for (String validRfid : validRfidValues) {
    Serial.printf("Comparing it to %s. ", validRfid);
    if (rfid == validRfid) {
      Serial.printf("%s is valid.\n", rfid);
      return true;
    }
  }

  Serial.printf("%s is NOT valid.\n", rfid);
  return false;
}

void UpdateFileOfValidRfidValues(String mqttMessage) {
  File file = LittleFS.open("/rfid-values.txt", "w");
  file.print(mqttMessage);
  file.close();
  Serial.println("Updated /rfid-values.txt");
}

void ReadFileOfValidRfidValues() {
  File file = LittleFS.open("/rfid-values.txt", "r");

  if (!file) {
    Serial.println("Failed to open rfid-values.txt for reading");
    return;
  }

  validRfidValues.clear();
  String rfid = "";
  Serial.println("File Content:");
  while (file.available()) {
    // Read each byte and append to the rfid string until line shift char(10), 
    // then store all RFID values in memory as a vector until all file content is read
    char c = file.read();

    if (c == '\n') {
      // We've read an RFID value, add it to the vector and clear rfid string to read the next one
      validRfidValues.push_back(rfid);
      Serial.printf(" Read an RFID: %s\n", rfid);
      rfid = "";
    } else {
      rfid += c;
      Serial.write(c);
    }    
  }
  file.close();
}

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
  Serial.begin(115200);
  
  // Setup magnet sensor pin for interrupt
  Serial.println("Assigning magnet sensor pin...");
  pinMode(MagnetSensorPin, INPUT_PULLUP);

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

  // Initialize LittleFS to read and store valid RFID values
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // Provide some test data in /rfid-values.txt if file doesn't already exist
  File file = LittleFS.open("/rfid-values.txt", "r");
  if (!file) {
    file.close();
    Serial.println("No /rfid-values.txt file exists, so create one with dummy data.");
    // Dummy data to test list of valid RFID values
    String dummyMqttMessage = "b338ec13\nc30e262a\n";  // Replace these values with your own RFID Cards IDs
    UpdateFileOfValidRfidValues(dummyMqttMessage);
  } else {
    file.close();
    Serial.println("Using /rfid-values.txt to validate RFID cards");
  }

  // Read file of valid RFID cards into memory
  ReadFileOfValidRfidValues();


  // First step is to configure WiFi STA and connect in order to get the current time and date.
  Serial.printf("Connecting to %s ", wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);

  /**
   * NTP server address could be acquired via DHCP,
   *
   * NOTE: This call should be made BEFORE esp32 acquires IP address via DHCP,
   * otherwise SNTP option 42 would be rejected by default.
   * NOTE: configTime() function call if made AFTER DHCP-client run
   * will OVERRIDE acquired NTP server address
   */
  esp_sntp_servermode_dhcp(1);  // (optional)

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  // set notification call-back function
  sntp_set_time_sync_notification_cb(timeavailable);

  /**
   * This will set configured ntp servers and constant TimeZone/daylightOffset
   * should be OK if your time zone does not need to adjust daylightOffset twice a year,
   * in such a case time adjustment won't be handled automagically.
   */
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  /**
   * A more convenient approach to handle TimeZones with daylightOffset
   * would be to specify a environment variable with TimeZone definition including daylight adjustmnet rules.
   * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
   */
  //configTzTime(time_zone, ntpServer1, ntpServer2);

  // Connect to MQTT broker
  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(incomingMqttMessage);

  if (!mqttClient.connected()) {
    reconnectMqttBroker();
  }

  // Register this device as a door sensor in Home Assistant
  publishDiscoveryMqttMessage();

  // Run sequence of LEDs and beep to verify that components are OK
  RunStartupSequence();
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMqttBroker();
  }

  mqttClient.loop(); // Process incoming MQTT messages

  checkDoorSensorState();

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
if (ValidateRfid(uidString)) {
      AccessGranted();
      Serial.printf("Opened door for RFID %s\n", uidString);
    } else {
      Serial.printf("WARNING: %s is NOT valid for this door\n", uidString);
    }
  }

  delay(100);
}
