#include <vector>
#include "SevenSegmentDisplay.h"

const int ButtonPin = 15;
volatile bool buttonState = false;
volatile bool buttonStateChanged = false;

const SegmentPins segmentPins = {
  .top = 19,
  .upperRight = 21,
  .lowerRight = 16,
  .decimalPoint = 17,
  .bottom = 4,
  .lowerLeft = 2,
  .upperLeft = 18,
  .center = 5
};
SevenSegmentDisplay display(segmentPins);

int currentDisplayDigit = -1;
int previousDisplayDigit = 0;

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

  if (currentDisplayDigit > 9) {
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
}

void loop() {
  checkButtonState();
  display.updateDisplay(currentDisplayDigit);

  delay(100);
}
