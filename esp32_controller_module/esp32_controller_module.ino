/*
  Indoor controller module - ESP32
  ---------------------------------
  Receives RFID card UIDs over UART from the outdoor Pico+RC522 module
  (see pico_rfid_module.ino), validates them, and:
    - signals the display/switch module to open the door on a valid card
    - replies GRANTED/DENIED to the Pico over the same UART, so the Pico
      can drive its local LEDs/buzzer (they live in the outdoor enclosure
      now, not here)
    - reports door state and RFID activity over MQTT, same as before

  Design (see GitHub issue #35): this is the only WiFi-connected device.
  Card validation intentionally happens here, not on the outdoor Pico,
  so it can't be tampered with by someone with physical access to the
  reader enclosure.

  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-mfrc522-rfid-reader-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  Wiring - Pico to ESP32 (UART, cross-connected):
    Pico GP0 (UART0 TX) -> ESP32 GPIO16 (RX, UART2)
    Pico GP1 (UART0 RX) <- ESP32 GPIO17 (TX, UART2)
    Pico GND            <-> ESP32 GND (common ground - required)

  UART protocol:
    Pico -> ESP32: "RFID:<lowercase hex uid>\n" for every card scanned
    ESP32 -> Pico: "GRANTED\n" or "DENIED\n" after each validation
*/

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

const int StatusLedPin = 2;
unsigned long statusLedOnTime = 0;

const int OpenDoorPin = 25;
const int MagnetSensorPin = 15;

// UART link to the outdoor Pico+RC522 module (UART2)
const int RfidUartRxPin = 16;  // <- Pico GP0 (TX)
const int RfidUartTxPin = 17;  // -> Pico GP1 (RX)
const unsigned long RfidUartBaud = 115200;
String rfidUartBuffer = "";

char doorSensorState[7] = "closed";
std::atomic<bool> doorSensorStateChanged = false;
unsigned long doorSensorChangedTime = 0;
std::vector<String> validRfidValues = { };
String updateValidRfidCardsTopic = "garageDoor/updateValidRfidCards/" + String(doorId);  // Provide doorId as a const *char to identify the door in MQTT messages in parameters.h
unsigned long rfidReadTime = 0;

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
    //mqttClient.publish("garageDoor/doorStateChanged", "closed"); // TODO: Uncomment after testing
    // Message for Home Assitant
    //mqttClient.publish("door/sensor", "CLOSED"); // TODO: Uncomment after testing
    doorSensorStateChanged = false;
  } else if (doorSensorStateChanged && strcmp(doorSensorState, "open") == 0) {
    Serial.println("Door is opening");
    // Message for ESP32 display module
    //mqttClient.publish("garageDoor/doorStateChanged", "opening"); // TODO: Uncomment after testing
    // Message for Home Assitant
    //mqttClient.publish("door/sensor", "OPEN");  // TODO: Uncomment after testing
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
  } else if(strcmp(topic, "garageDoor/queryValidRfidCards") == 0) {
    String mqttContent = String(doorId) + "\n";
    for (String validRfid : validRfidValues) {
      mqttContent += validRfid + "\n";
    }
    mqttClient.publish("garageDoor/validRfid", mqttContent.c_str());
    Serial.println("Published valid RFID Cards to garageDoor/validRfid");
  } else if (strcmp(topic, updateValidRfidCardsTopic.c_str()) == 0) {
    UpdateFileOfValidRfidValues(value);
    ReadFileOfValidRfidValues();
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

void turnOnStatusLed() {
  // Turn the status LED on and record time to automatically turn it off again after a while
  digitalWrite(StatusLedPin, HIGH);
  statusLedOnTime = millis();
}

void turnOffStatusLed() {
  digitalWrite(StatusLedPin, LOW);
}

void ensureWifiConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    WiFi.disconnect();
    WiFi.begin(wifi_ssid, wifi_password);
    unsigned long startAttemptTime = millis();

    // Try for up to 10 seconds
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(250);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi reconnected");
    } else {
      Serial.println("WiFi reconnect failed");
    }
  }
}

void ensureMqttBrokerConnected() {
  // Reconnect if not connected
  if (!mqttClient.connected()) {
    Serial.printf("Connecting to %s...\n", mqttServer);

    // Attempt to connect
    if (mqttClient.connect("TestDevice", mqtt_user, mqtt_password)) {  // TODO: Change name to "GarageDoorController" after testing
      Serial.printf("connected to %s\n", mqttServer);
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

void subscribeToMqttTopics() {
  if (mqttClient.subscribe("garageDoor/queryDeviceStatus")) {
    Serial.println("Subscribed to topic: garageDoor/queryDeviceStatus");
  } else {
    Serial.println("Failed to subscribe to topic!");
  }

  if (mqttClient.subscribe("garageDoor/queryValidRfidCards")) {
    Serial.println("Subscribed to topic: garageDoor/queryValidRfidCards");
  } else {
    Serial.println("Failed to subscribe to topic!");
  }

  if (mqttClient.subscribe(updateValidRfidCardsTopic.c_str())) {
    Serial.printf("Subscribed to topic: %s/\n", updateValidRfidCardsTopic.c_str());
  } else {
    Serial.println("Failed to subscribe to updateValidRfidCards topic!");
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

void AccessGranted(String uid) {
  digitalWrite(OpenDoorPin, HIGH);  // Send signal to display module to open the door
  Serial.println("Opening door");
  delay(500);                       // wait for 500ms
  digitalWrite(OpenDoorPin, LOW);   // Expect the display module to have opened the door within 500ms, so turn this signal off again
  Serial.printf("Opened door for RFID %s\n", uid);
}

// Reads lines coming in over UART from the Pico+RC522 module and acts on
// any "RFID:<uid>" line. Non-blocking: only consumes what's already
// buffered in the UART, one character at a time, across loop() iterations.
void handleRfidUart() {
  while (Serial2.available()) {
    char c = Serial2.read();

    if (c == '\n') {
      rfidUartBuffer.trim();

      if (rfidUartBuffer.startsWith("RFID:")) {
        String uid = rfidUartBuffer.substring(5);
        uid.trim();
        handleRfidUid(uid);
      } else if (rfidUartBuffer.length() > 0) {
        Serial.printf("Ignoring unrecognized line from RFID module: %s\n", rfidUartBuffer.c_str());
      }

      rfidUartBuffer = "";
    } else if (c != '\r') {
      rfidUartBuffer += c;
    }
  }
}

void handleRfidUid(String uid) {
  // Ignore repeat reads within 3s of the previous one, mirroring the
  // original module's behavior when a card was left sitting on the reader.
  if (millis() - rfidReadTime <= 3000) {
    return;
  }
  rfidReadTime = millis();

  Serial.printf("Card UID received: %s\n", uid);

  if (ValidateRfid(uid)) {
    Serial2.println("GRANTED");  // Tells the Pico to show its granted LED/buzzer
    AccessGranted(uid);
  } else {
    Serial2.println("DENIED");   // Tells the Pico to show its denied LED/buzzer
    Serial.printf("WARNING: %s is NOT valid for this door\n", uid);
    mqttClient.publish("garageDoor/invalidRfid", uid.c_str());
    Serial.println("MQTT message about invalid RFID sent.");
  }
}

void setup() {
  Serial.begin(115200);

  // Turn on status LED until setup is successfully executed
  pinMode(StatusLedPin, OUTPUT);
  digitalWrite(StatusLedPin, HIGH);

  // Setup magnet sensor pin for interrupt
  Serial.println("Assigning magnet sensor pin...");
  pinMode(MagnetSensorPin, INPUT_PULLUP);

  // Setup signal to display module to open the door
  pinMode(OpenDoorPin, OUTPUT);
  digitalWrite(OpenDoorPin, LOW);

  // Set up UART link to the outdoor Pico+RC522 module
  Serial.println("Starting UART link to RFID module...");
  Serial2.begin(RfidUartBaud, SERIAL_8N1, RfidUartRxPin, RfidUartTxPin);

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
  mqttClient.setBufferSize(512);  // default 256 bytes is too small for the sensor discovery payloads

  if (!mqttClient.connected()) {
    ensureWifiConnected();
    ensureMqttBrokerConnected();
  }

  if (mqttClient.connected()) {
    // Register this device as a door sensor in Home Assistant
    publishDiscoveryMqttMessage();
    subscribeToMqttTopics();
  } else {
    Serial.println("MQTT not connected - cannot publish discovery message");
  }

  // Turn off status LED to indicate setup completed
  digitalWrite(StatusLedPin, LOW);
}

void loop() {
  if (millis() - statusLedOnTime >= 1000) {
    turnOffStatusLed();
  }

  // Reconnect both WiFi and MQTT if connection is broken
  ensureWifiConnected();
  ensureMqttBrokerConnected();

  mqttClient.loop(); // Process incoming MQTT messages

  checkDoorSensorState();

  // Process any RFID UID lines coming in from the Pico+RC522 module
  handleRfidUart();
}
