#include <vector>

const int ButtonPin = 15;
volatile bool buttonState = false;
volatile bool buttonStateChanged = false;

const int TopSegment = 19;  // Top segment
const int UpperRightSegment = 21;  // Upper Right segment
const int LowerRightSegment = 16;  // Lower Right segment
const int DecimalPointSegment = 17;  // Decimal point segment
const int BottomSegment = 4;  // Bottom segment
const int LowerLeftSegment = 2;  // Lower Left segment
const int UpperLeftSegment = 18;  // Upper Left segment
const int CenterSegment = 5;  // Center segment

const std::vector<int> AllLedSegmentPins = { TopSegment, UpperRightSegment, LowerRightSegment, DecimalPointSegment, BottomSegment, LowerLeftSegment, UpperLeftSegment, CenterSegment };
const std::vector<int> Display_0 = { TopSegment, UpperRightSegment, LowerRightSegment, BottomSegment, LowerLeftSegment, UpperLeftSegment };
const std::vector<int> Display_1 = { UpperRightSegment, LowerRightSegment };
const std::vector<int> Display_2 = { TopSegment, UpperRightSegment, CenterSegment, LowerLeftSegment, BottomSegment };
const std::vector<int> Display_3 = { TopSegment, UpperRightSegment, CenterSegment, LowerRightSegment, BottomSegment };
const std::vector<int> Display_4 = { UpperLeftSegment, CenterSegment, UpperRightSegment, LowerRightSegment };
const std::vector<int> Display_5 = { TopSegment, UpperLeftSegment, CenterSegment, LowerRightSegment, BottomSegment };
const std::vector<int> Display_6 = { UpperLeftSegment, LowerLeftSegment, BottomSegment, LowerRightSegment, CenterSegment };
const std::vector<int> Display_7 = { TopSegment, UpperRightSegment, LowerRightSegment };
const std::vector<int> Display_8 = { TopSegment, UpperRightSegment, LowerRightSegment, BottomSegment, LowerLeftSegment, UpperLeftSegment, CenterSegment };
const std::vector<int> Display_9 = { TopSegment, UpperRightSegment, CenterSegment, UpperLeftSegment, LowerRightSegment };
const std::vector<int> DisplayNothing;
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

void updateDisplay() {
  std::vector<int> currentSegments = DisplayNothing;

  switch (currentDisplayDigit) {
    case 0:
      currentSegments = Display_0;
      break;
    case 1:
      currentSegments = Display_1;
      break;
    case 2:
      currentSegments = Display_2;
      break;
    case 3:
      currentSegments = Display_3;
      break;
    case 4:
      currentSegments = Display_4;
      break;
    case 5:
      currentSegments = Display_5;
      break;
    case 6:
      currentSegments = Display_6;
      break;
    case 7:
      currentSegments = Display_7;
      break;
    case 8:
      currentSegments = Display_8;
      break;
    case 9:
      currentSegments = Display_9;
      break;
    default:
      currentSegments = DisplayNothing;
      break;
  }

  if (currentDisplayDigit == previousDisplayDigit) {
    // Display hasn't changed
    return;
  }

  // Turn all segments off
  for (int i = 0; i < AllLedSegmentPins.size(); i++ ) { 
    digitalWrite(AllLedSegmentPins[i], HIGH);
  }

  // Turn on current segments
  if (!currentSegments.empty()) {
    for (int i = 0; i < currentSegments.size(); i++) {
      digitalWrite(currentSegments[i], LOW);
    }
  }

  previousDisplayDigit = currentDisplayDigit;
}

void setup() {
  Serial.begin(115200);

  // Setup all LED segment pins for output
  Serial.println("Assigning LED segment pins...");
  for (int i = 0; i < AllLedSegmentPins.size(); i++) {
    pinMode(AllLedSegmentPins[i], OUTPUT);
  }

  // Setup button pin for interrupt
  Serial.println("Assigning button pin...");
  attachInterrupt(ButtonPin, handleButtonInterrupt, CHANGE);

  Serial.println("Ready to display");
}

void loop() {
  checkButtonState();
  updateDisplay();

  delay(100);
}
