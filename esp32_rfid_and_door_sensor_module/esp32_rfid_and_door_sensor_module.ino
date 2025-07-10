#include <WiFi.h>
#include <PubSubClient.h>
#include <atomic>
#include "time.h"
#include "esp_sntp.h"
#include "parameters.h"

const int MagnetSensorPin = 15;
char doorSensorState[7] = "closed";
std::atomic<bool> doorSensorStateChanged = false;
unsigned long doorSensorChangedTime = 0;

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

void setup() {
  Serial.begin(115200);
  
  // Setup magnet sensor pin for interrupt
  Serial.println("Assigning magnet sensor pin...");
  pinMode(MagnetSensorPin, INPUT_PULLUP);

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
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMqttBroker();
  }

  mqttClient.loop(); // Process incoming MQTT messages

  checkDoorSensorState();

  delay(100);
}
