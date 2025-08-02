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

    letterMappings = {
        {segPins.top, segPins.upperLeft, segPins.center, segPins.lowerRight, segPins.bottom},  // Display_S
        {segPins.upperLeft, segPins.center, segPins.lowerLeft, segPins.bottom},  // Display_t
        {segPins.lowerLeft, segPins.center, segPins.lowerRight, segPins.bottom},  // Display_o
        {segPins.top, segPins.upperLeft, segPins.center, segPins.lowerLeft, segPins.upperRight},  // Display_P
        {segPins.top, segPins.upperLeft, segPins.center, segPins.lowerLeft},  // Display_F
        {segPins.top, segPins.upperLeft, segPins.center, segPins.lowerLeft, segPins.bottom},  // Display_E
        {segPins.upperLeft, segPins.lowerLeft},  // Display_I
        {segPins.upperLeft, segPins.lowerLeft, segPins.bottom},  // Display_L
        {segPins.center}  // Diplay -
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
    if (number < -2 || number > 9999) return;

    // Negative number have special meaning to simplify displaying two specific words
    if (number == -1) {
        // -1 should display text STOP when emergency stop button is held
        displayText("StoP");
    } else if (number == -2) {
        // -2 should display text FEIL (Norwegian for error) when door is open for very long and the emergency stop button is not held
        displayText("FEIL");
    } else {
        // display numbers 0-9999 using common logic for numbers
        displayNumber(number);
    }    
}

void SevenSegmentDisplay::displayText(char* text) {
    if (strcmp(text, "StoP") == 0) {
        // Multiplexing: Cycle through each letter quickly
        digitalWrite(digitPins[0], LOW);  // Turn off fourth character
        
        // Display an S
        digitalWrite(digitPins[3], HIGH);  // Turn on first character
        turnOnLetterSegments('S');
        delayMicroseconds(5000);  // Small delay for persistence

        // Display a t
        digitalWrite(digitPins[3], LOW);  // Turn off first character
        digitalWrite(digitPins[2], HIGH);  // Turn on second character
        turnOnLetterSegments('t');
        delayMicroseconds(5000);  // Small delay for persistence

        // Display an o
        digitalWrite(digitPins[2], LOW);  // Turn off second character
        digitalWrite(digitPins[1], HIGH);  // Turn on third character
        turnOnLetterSegments('o');
        delayMicroseconds(5000);  // Small delay for persistence

        // Display a P
        digitalWrite(digitPins[1], LOW);  // Turn off third character
        digitalWrite(digitPins[0], HIGH);  // Turn on fourth character
        turnOnLetterSegments('P');
        delayMicroseconds(5000);  // Small delay for persistence
    } else if (strcmp(text, "FEIL") == 0) {
        // Multiplexing: Cycle through each letter quickly
        digitalWrite(digitPins[0], LOW);  // Turn off fourth character
        
        // Display an F
        digitalWrite(digitPins[3], HIGH);  // Turn on first character
        turnOnLetterSegments('F');
        delayMicroseconds(5000);  // Small delay for persistence

        // Display an E
        digitalWrite(digitPins[3], LOW);  // Turn off first character
        digitalWrite(digitPins[2], HIGH);  // Turn on second character
        turnOnLetterSegments('E');
        delayMicroseconds(5000);  // Small delay for persistence

        // Display an I
        digitalWrite(digitPins[2], LOW);  // Turn off second character
        digitalWrite(digitPins[1], HIGH);  // Turn on third character
        turnOnLetterSegments('I');
        delayMicroseconds(5000);  // Small delay for persistence

        // Display an L
        digitalWrite(digitPins[1], LOW);  // Turn off third character
        digitalWrite(digitPins[0], HIGH);  // Turn on fourth character
        turnOnLetterSegments('L');
        delayMicroseconds(5000);  // Small delay for persistence
    } else {
        // Unsupported word, write ---- on the display
        // Multiplexing: Cycle through each letter quickly
        digitalWrite(digitPins[0], LOW);  // Turn off fourth character
        
        // Display character -
        digitalWrite(digitPins[3], HIGH);  // Turn on first character
        turnOnLetterSegments('-');
        delayMicroseconds(5000);  // Small delay for persistence

        // Display character -
        digitalWrite(digitPins[3], LOW);  // Turn off first character
        digitalWrite(digitPins[2], HIGH);  // Turn on second character
        turnOnLetterSegments('-');
        delayMicroseconds(5000);  // Small delay for persistence

        // Display character -
        digitalWrite(digitPins[2], LOW);  // Turn off second character
        digitalWrite(digitPins[1], HIGH);  // Turn on third character
        turnOnLetterSegments('-');
        delayMicroseconds(5000);  // Small delay for persistence

        // Display character -
        digitalWrite(digitPins[1], LOW);  // Turn off third character
        digitalWrite(digitPins[0], HIGH);  // Turn on fourth character
        turnOnLetterSegments('-');
        delayMicroseconds(5000);  // Small delay for persistence
    }
}

void SevenSegmentDisplay::displayNumber(int number) {
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

void SevenSegmentDisplay::turnOnLetterSegments(char letter)
{
    // Turn off all segment before tuning on the ones that should be on
    for (int pin: segmentPins) {
        digitalWrite(pin, HIGH);
    }

    // Turn on segments corresponding to the letter
    std::vector<int> letterMapping;
    if (letter == 'S') {
        letterMapping = letterMappings[0];
    } else if (letter == 't') {
        letterMapping = letterMappings[1];
    } else if (letter == 'o') {
        letterMapping = letterMappings[2];
    } else if (letter == 'P') {
        letterMapping = letterMappings[3];
    } else if (letter == 'F') {
        letterMapping = letterMappings[4];
    } else if (letter == 'E') {
        letterMapping = letterMappings[5];
    } else if (letter == 'I') {
        letterMapping = letterMappings[6];
    } else if (letter == 'L') {
        letterMapping = letterMappings[7];
    } else {
        // Letter is not supported, display character - instead an empty display
        letterMapping = letterMappings[8];
    }

    for (int pin: letterMapping) {
        digitalWrite(pin, LOW);
    }
}

void SevenSegmentDisplay::turnOffAllSegments() {
    for (int digitPin : digitPins) {
        digitalWrite(digitPin, LOW); // Turn off digits (common anode)
    }
}