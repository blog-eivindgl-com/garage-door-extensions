#include <vector>
#include "SevenSegmentDisplay.h"

const int ButtonPin = 26;
const int DayLedPin = 12;
const int WeekLedPin = 14;
const int MonthLedPin = 27;
std::vector<int> indicatorLedPins = { DayLedPin, WeekLedPin, MonthLedPin };
int currentSelectorValue = 0;
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
    cycleDisplayDigit();
    cycleSelector();
    buttonStateChanged = false;
  }
}

void cycleDisplayDigit() {
  currentDisplayDigit++;

  if (currentDisplayDigit > 9999) {
    currentDisplayDigit = 0;
  }

  Serial.printf("Updating display to digit %d\n", currentDisplayDigit);
}

void cycleSelector() {
  // Query for the counter value of the selected period
  switch (currentSelectorValue) {
    case 0:  // Day
      queryCounter("week");
      break;
    case 1:  // Week
      queryCounter("month");
      break;
    default:  // Month (or something unknown)
      queryCounter("day");
      break;
  }
}

void queryCounter(const char* selectorQuery) {
  // TODO: Send MQTT message to query the selected periods counter

  // TODO: Move code below to the callback when the MQTT response message has arrived, 
  // and the display has updated. 
  // Set the period LED indicator for the value displayed
  digitalWrite(DayLedPin, LOW);
  digitalWrite(WeekLedPin, LOW);
  digitalWrite(MonthLedPin, LOW);

  if (selectorQuery == "day") {
    currentSelectorValue = 0;
    digitalWrite(DayLedPin, HIGH);
  } else if (selectorQuery == "week") {
    currentSelectorValue = 1;
    digitalWrite(WeekLedPin, HIGH);
  } else if (selectorQuery == "month") {
    currentSelectorValue = 2;
    digitalWrite(MonthLedPin, HIGH);
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

  // Setup button pin for interrupt
  Serial.println("Assigning button pin...");
  pinMode(ButtonPin, INPUT_PULLUP);
  attachInterrupt(ButtonPin, handleButtonInterrupt, CHANGE);

  Serial.println("Query counter for current day");
  queryCounter("day");
}

void loop() {
  static unsigned long lastUpdateTime = 0;
  static unsigned long lastCheckedButtonState = 0;

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
