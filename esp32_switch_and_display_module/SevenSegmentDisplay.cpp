#include "SevenSegmentDisplay.h"

SevenSegmentDisplay::SevenSegmentDisplay(const DigitPins& digPins, const SegmentPins& segPins) {
    // Store digit and segment pins in order
    digitPins = { digPins.onesDigit, digPins.tensDigit, digPins.hundredsDigit, digPins.thousandsDigit };
    segmentPins = { segPins.top, segPins.upperRight, segPins.lowerRight, segPins.decimalPoint, segPins.bottom, segPins.lowerLeft, segPins.upperLeft, segPins.center };

    // Define segment mappings for digits 0-9 using named parameters
    digitMappings = {
        {segPins.top, segPins.upperRight, segPins.lowerRight, segPins.bottom, segPins.lowerLeft, segPins.upperLeft},  // Display_0
        {segPins.upperRight, segPins.lowerRight},  // Display_1
        {segPins.top, segPins.upperRight, segPins.center, segPins.lowerLeft, segPins.bottom},  // Display_2
        {segPins.top, segPins.upperRight, segPins.center, segPins.lowerRight, segPins.bottom},  // Display_3
        {segPins.upperLeft, segPins.center, segPins.upperRight, segPins.lowerRight},  // Display_4
        {segPins.top, segPins.upperLeft, segPins.center, segPins.lowerRight, segPins.bottom},  // Display_5
        {segPins.upperLeft, segPins.lowerLeft, segPins.bottom, segPins.lowerRight, segPins.center},  // Display_6
        {segPins.top, segPins.upperRight, segPins.lowerRight},  // Display_7
        {segPins.top, segPins.upperRight, segPins.lowerRight, segPins.bottom, segPins.lowerLeft, segPins.upperLeft, segPins.center},  // Display_8
        {segPins.top, segPins.upperRight, segPins.center, segPins.upperLeft, segPins.lowerRight}  // Display_9
    };
}

void SevenSegmentDisplay::begin() {
    // Initialize pins as output
    for (int pin : digitPins) {
        pinMode(pin, OUTPUT);
    }
    for (int pin : segmentPins) {
        pinMode(pin, OUTPUT);
    }
    turnOffAllSegments();
}

void SevenSegmentDisplay::clearDisplay() {
    turnOffAllSegments();
}

void SevenSegmentDisplay::updateDisplay(int number) {
    if (number < 0 || number > 9999) return;

    // Extract inidividual digits
    int thousands = (number / 1000) % 10;
    int hundreds = (number / 100) % 10;
    int tens = (number / 10) % 10;
    int ones = number % 10;

    // Multiplexing: Cycle through each digit quickly
    digitalWrite(digitPins[0], LOW);  // Turn off ones digit

    if (thousands > 0) {
        // Activate thousands only when that digit is greater than 0
        turnOnDigitSegment(digitPins[3], thousands);
        delayMicroseconds(5000);  // Small delay for persistence
    }

    if (hundreds > 0 || thousands > 0) {
        // Turn off thousands digit
        digitalWrite(digitPins[3], LOW);

        // Activate hundreds only when that digit or thousands is greater than 0
        turnOnDigitSegment(digitPins[2], hundreds);
        delayMicroseconds(5000);  // Small delay for persistence
    }

    if (tens > 0 || hundreds > 0 || thousands > 0) {
        // Turn off hundreds digit
        digitalWrite(digitPins[2], LOW);
        
        // Activate tens only when that digit or hundreds or thousands is greater than 0
        turnOnDigitSegment(digitPins[1], tens);
        delayMicroseconds(5000);  // Small delay for persistence
    }

    // Turn off tens digit
    digitalWrite(digitPins[1], LOW);

    // Activate ones no matter what that digit is
    turnOnDigitSegment(digitPins[0], ones);
    delayMicroseconds(5000);  // Small delay for persistence
}

void SevenSegmentDisplay::turnOnDigitSegment(int digitPin, int digitValue) {
    digitalWrite(digitPin, HIGH);  // Activate digit (since common anode)
    turnOnDigitSegments(digitValue);  // Activate segments for this digit
}

void SevenSegmentDisplay::turnOnDigitSegments(int digit) {
    // Turn off all segment before tuning on the ones that should be on
    for (int pin: segmentPins) {
        digitalWrite(pin, HIGH);
    }

    // Turn on segments corresponding to the digit
    for (int pin: digitMappings[digit]) {
        digitalWrite(pin, LOW);
    }
}

void SevenSegmentDisplay::turnOffAllSegments() {
    for (int digitPin : digitPins) {
        digitalWrite(digitPin, LOW); // Turn off digits (common anode)
    }
}