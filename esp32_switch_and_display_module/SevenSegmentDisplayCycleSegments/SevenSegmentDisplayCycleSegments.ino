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
const std::vector<int> DisplayNothing;
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

  Serial.printf("Updating display to digit %s\n", segmentDescription);
}

void updateDisplay() {
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

  if (currentSegment == previousSegment) {
    // Display hasn't changed
    return;
  }

  // turn all segments off
  for (int i = 0; i < AllLedSegmentPins.size(); i++ ) { 
    digitalWrite(AllLedSegmentPins[i], HIGH);
  }

  digitalWrite(segment, LOW);
  previousSegment = currentSegment;
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
