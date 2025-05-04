#include <vector>

const int ButtonPin = 15;
volatile bool buttonState = false;
volatile bool buttonStateChanged = false;

const int TopSegment = 18;  // Top segment
const int UpperRightSegment = 12;  // Upper Right segment
const int LowerRightSegment = 16;  // Lower Right segment
const int DecimalPointSegment = 13;  // Decimal point segment
const int BottomSegment = 4;  // Bottom segment
const int LowerLeftSegment = 2;  // Lower Left segment
const int UpperLeftSegment = 19;  // Upper Left segment
const int CenterSegment = 17;  // Center segment

const int Digit_Ones = 23;  // Set value to the right-most of 4 digits, representing the ones
const int Digit_Tens = 22;  // Set value to the second right-most of 4 digits, representing the tens
const int Digit_Hundreds = 21; // Set value to the third right-most of 4 digits, representing the hundreds
const int Digit_Thousands = 5; // Set value to the fourth right-most of 4 digits, representing the thousands

const std::vector<int> AllLedSegmentPins = { TopSegment, UpperRightSegment, LowerRightSegment, DecimalPointSegment, BottomSegment, LowerLeftSegment, UpperLeftSegment, CenterSegment };
const std::vector<int> DigitPins = { Digit_Ones, Digit_Tens, Digit_Hundreds, Digit_Thousands };
int currentDigit = 0;
int previousDigit = 0;
int currentSegment = 0;
int previousSegment = 0;

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
  currentSegment++;

  if (currentSegment > 7) {
    currentSegment = 0;
    currentDigit++;
  }

  if (currentDigit > 3) {
    currentDigit = 0;  // Start over from the right-most digit again
  }

  char* digitDescription = "";
  switch (currentDigit) {
    case 0:
      digitDescription = "Ones";
      break;
    case 1:
      digitDescription = "Tens";
      break;
    case 2:
      digitDescription = "Hundreds";
      break;
    case 3:
      digitDescription = "Thousands";
      break;
    default:
      Serial.println("Unknown digit");
      break;
  }

  char* segmentDescription = "";
  switch (currentSegment) {
    case 0:
      segmentDescription = "Top";
      break;
    case 1:
      segmentDescription = "Upper Right";
      break;
    case 2:
      segmentDescription = "Lower Right";
      break;
    case 3:
      segmentDescription = "Bottom";
      break;
    case 4:
      segmentDescription = "Lower Left";
      break;
    case 5:
      segmentDescription = "Upper Left";
      break;
    case 6:
      segmentDescription = "Center";
      break;
    case 7:
      segmentDescription = "Decimal Point";
      break;
    default:
      Serial.println("Unknown Segment");
      break;
  }

  Serial.printf("Updating display to display %s segment in digit %s\n", segmentDescription, digitDescription);
}

void updateDisplay() {
  int digit = -1;
  switch (currentDigit) {
    case 0:
      digit = Digit_Ones;
      break;
    case 1:
      digit = Digit_Tens;
      break;
    case 2:
      digit = Digit_Hundreds;
      break;
    case 3:
      digit = Digit_Thousands;
      break;
    default:
      Serial.println("Unknown digit");
      break;
  }

  int segment = -1;
  switch (currentSegment) {
    case 0:
      segment = TopSegment;
      break;
    case 1:
      segment = UpperRightSegment;
      break;
    case 2:
      segment = LowerRightSegment;
      break;
    case 3:
      segment = BottomSegment;
      break;
    case 4:
      segment = LowerLeftSegment;
      break;
    case 5:
      segment = UpperLeftSegment;
      break;
    case 6:
      segment = CenterSegment;
      break;
    case 7:
      segment = DecimalPointSegment;
      break;
    default:
      Serial.println("Unknown Segment");
      break;
  }

  if (currentSegment == previousSegment && currentDigit == previousDigit) {
    // Display hasn't changed
    return;
  }

  // Turn all segments off digit by digit
  for (int i = 0; i < DigitPins.size(); i++) {
    digitalWrite(DigitPins[i], LOW);

    for (int j = 0; j < AllLedSegmentPins.size(); j++) { 
      digitalWrite(AllLedSegmentPins[j], HIGH);
    }
  }

  // Turn on the current segment on the current digit
  Serial.printf("Digit pin that will be set HIGH: %d\n", digit);
  digitalWrite(digit, HIGH);
  digitalWrite(segment, LOW);
  previousDigit = currentDigit;
  previousSegment = currentSegment;
}

void setup() {
  Serial.begin(115200);

  // Setup all LED segment pins for output
  Serial.println("Assigning LED segment pins...");
  for (int i = 0; i < DigitPins.size(); i++) {
    pinMode(DigitPins[i], OUTPUT);
  }
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
