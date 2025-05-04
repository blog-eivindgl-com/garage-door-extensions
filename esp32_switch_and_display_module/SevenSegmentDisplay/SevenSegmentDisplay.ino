#include <vector>
#include "SevenSegmentDisplay.h"

const int ButtonPin = 15;
volatile bool buttonState = false;
volatile bool buttonStateChanged = false;

const DigitPins digitPins = {
  .onesDigit = 23,
  .tensDigit = 22,
  .hundredsDigit = 21,
  .thousandsDigit = 5
};
const SegmentPins segmentPins = {
  .top = 18,
  .upperRight = 12,
  .lowerRight = 16,
  .decimalPoint = 13,
  .bottom = 4,
  .lowerLeft = 2,
  .upperLeft = 19,
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

void setup() {
  Serial.begin(115200);

  // Setup all LED segment pins for output by calling the SevenSegmentDisplay.begin() function
  display.begin();

  // Setup button pin for interrupt
  Serial.println("Assigning button pin...");
  attachInterrupt(ButtonPin, handleButtonInterrupt, CHANGE);

  Serial.println("Ready to display");
  currentDisplayDigit = 9995;
}

void loop() {
  static unsigned long lastUpdateTime = 0;

  checkButtonState();  // Handle button state without affecting display refresh

  // Ensure display refresh happens ~every 5ms
  if (millis() - lastUpdateTime >= 5) {
    display.updateDisplay(currentDisplayDigit);
    lastUpdateTime = millis();  // Reset timing
  }
}
