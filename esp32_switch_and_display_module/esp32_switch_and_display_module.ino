#include <vector>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include "esp_sntp.h"
#include "SevenSegmentDisplay.h"
#include "parameters.h"

const int ButtonPin = 26;
const int DayLedPin = 12;
const int WeekLedPin = 14;
const int MonthLedPin = 27;
const int OpenDoorPin = 25;
std::vector<int> indicatorLedPins = { DayLedPin, WeekLedPin, MonthLedPin };
int dayCounter = 0;
int weekCounter = 0;
int monthCounter = 0;
int currentSelectorValue = -1;
volatile bool buttonState = false;
volatile bool buttonStateChanged = false;

const DigitPins digitPins = {
  .onesDigit = 5,
  .tensDigit = 19,
  .hundredsDigit = 21,
  .thousandsDigit = 13
};
const SegmentPins segmentPins = {
  .top = 23,
  .upperRight = 18,
  .lowerRight = 16,
  .decimalPoint = 4,
  .bottom = 2,
  .lowerLeft = 15,
  .upperLeft = 22,
  .center = 17
};
SevenSegmentDisplay display(digitPins, segmentPins);

int currentDisplayDigit = -1;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);  // MQTT

void IRAM_ATTR handleButtonInterrupt() {
  if (digitalRead(ButtonPin) == LOW) {
    buttonState = true;
    buttonStateChanged = true;
  } else {
    buttonState = false;
    buttonStateChanged = true;
  }
}

void checkButtonState() {
  if (buttonStateChanged && buttonState) {
    cycleSelector();
    buttonStateChanged = false;
  }
}

/* This function is used to cycle the display digit for testing the display.
 It increments the currentDisplayDigit and updates the display.
 If the digit exceeds 9999, it wraps around to 0.
 */
/*
void cycleDisplayDigit() {
  currentDisplayDigit++;

  if (currentDisplayDigit > 9999) {
    currentDisplayDigit = 0;
  }

  Serial.printf("Updating display to digit %d\n", currentDisplayDigit);
}*/

void cycleSelector() {
  // Turn off all LEDs before cycling
  digitalWrite(DayLedPin, LOW);
  digitalWrite(WeekLedPin, LOW);
  digitalWrite(MonthLedPin, LOW);

  // Switch to the next selector value and update the display digit accordingly and turn on the corresponding LED
  switch (currentSelectorValue) {
    case 0:  // Day
      currentSelectorValue = 1;  // Move to next selector value, representing week
      currentDisplayDigit = weekCounter;  // Update display to week counter
      digitalWrite(WeekLedPin, HIGH); // Turn on week LED
      break;
    case 1:  // Week
      currentSelectorValue = 2;  // Move to next selector value, representing month
      currentDisplayDigit = monthCounter;  // Update display to month counter
      digitalWrite(MonthLedPin, HIGH); // Turn on month LED
      break;
    default:  // Month (or something unknown)
      currentSelectorValue = 0;  // Move to next selector value, representing day
      currentDisplayDigit = dayCounter;  // Update display to day counter
      digitalWrite(DayLedPin, HIGH); // Turn on day LED
      break;
  }
}

void queryCounter() {
  // Send MQTT message to query the display update message
  mqttClient.publish("garageDoor/queryState", "");
}

void handleOpenDoorMessage() {
  // TODO: This function should be a callback with incoming MQTT message subscribing to open door topic
  digitalWrite(OpenDoorPin, HIGH);
  delay(2000);
  digitalWrite(OpenDoorPin, LOW);
}

void reconnectMqttBroker() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.printf("Connecting to %s...\n", mqttServer);
    
    // Attempt to connect
    if (mqttClient.connect("GarageDoorDisplay", mqtt_user, mqtt_password)) {
      Serial.printf("connected to %s\n", mqttServer);
      // Subscribe to topics
      mqttClient.subscribe("garageDoor/display");
      mqttClient.subscribe("garageDoor/alert");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
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

void cycleLedsStartupProcedure() {
  // Make sure all LEDs are OFF
  Serial.println("Running startup sequnce of LEDs");
  for (int pin : indicatorLedPins) {
    digitalWrite(pin, LOW);
  }

  // Turn each LED on, one by one
  for (int pin : indicatorLedPins) {
    Serial.printf("Testing LED %d...\n", pin);
    digitalWrite(pin, HIGH);
    delay(100);
  }

  // Make sure all LEDs are OFF
  delay(500);
  Serial.println("Turning all LEDs off");
  for (int pin : indicatorLedPins) {
    digitalWrite(pin, LOW);
  }
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

  if (strcmp(topic, "garageDoor/display") == 0) {
    // Extract counters from json payload
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, value);
    
    if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
      return;
    }
    
    // Extract integer values from JSON
    int today = doc["today"] | 0;           // Default to 0 if not found
    int thisWeek = doc["thisWeek"] | 0;
    int thisMonth = doc["thisMonth"] | 0;
    /* We don't need these values for the display, but they could be useful for debugging
    unsigned long lastOpened = doc["lastOpened"] | 0;
    unsigned long lastClosed = doc["lastClosed"] | 0;
    int openDuration = doc["openDurationInSeconds"] | 0;
    */

    // Update the counter variables
    dayCounter = today;
    weekCounter = thisWeek;
    monthCounter = thisMonth;
    
    // Update the display based on current selector
    switch (currentSelectorValue) {
      case 0:  // Day
        currentDisplayDigit = dayCounter;
        break;
      case 1:  // Week
        currentDisplayDigit = weekCounter;
        break;
      case 2:  // Month
        currentDisplayDigit = monthCounter;
        break;
    }
    
    Serial.printf("Updated counters - Today: %d, Week: %d, Month: %d\n", 
                  dayCounter, weekCounter, monthCounter);
    /*Serial.printf("Last opened: %lu, Last closed: %lu, Duration: %d seconds\n", 
                  lastOpened, lastClosed, openDuration);*/
  }
}

void setup() {
  Serial.begin(115200);

  // Setup all LED segment pins for output by calling the SevenSegmentDisplay.begin() function
  display.begin();

  // Setup indicator LED pins for output
  for (int pin : indicatorLedPins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  // Setup open door pin for output
  pinMode(OpenDoorPin, OUTPUT);
  digitalWrite(OpenDoorPin, LOW);

  // Setup button pin for interrupt
  Serial.println("Assigning button pin...");
  pinMode(ButtonPin, INPUT_PULLUP);
  attachInterrupt(ButtonPin, handleButtonInterrupt, CHANGE);

  // Test all LEDs on startup and then let cycleSelector function switch to default today
  cycleLedsStartupProcedure();
  cycleSelector();

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

  Serial.println("Query backend for current state of the garage door counters...");
  queryCounter();
}

void loop() {
  static unsigned long lastUpdateTime = 0;
  static unsigned long lastCheckedButtonState = 0;
  static unsigned long lastSimulationOpenDoor = 0;

  if (!mqttClient.connected()) {
    reconnectMqttBroker();
  }

  mqttClient.loop(); // Process incoming MQTT messages

  // Handle button state without affecting display refresh
  if (millis() - lastCheckedButtonState >= 100) {
    checkButtonState();
    lastCheckedButtonState = millis();  // Reset timing
  }

  // Ensure display refresh happens ~every 5ms
  if (millis() - lastUpdateTime >= 5) {
    display.updateDisplay(currentDisplayDigit);
    lastUpdateTime = millis();  // Reset timing
  }
}
