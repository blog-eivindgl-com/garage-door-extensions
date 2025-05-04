#include "SevenSegmentDisplay.h"

SevenSegmentDisplay::SevenSegmentDisplay(const SegmentPins& pins) {
    // Store segment pins in order
    segmentPins = { pins.top, pins.upperRight, pins.lowerRight, pins.decimalPoint, pins.bottom, pins.lowerLeft, pins.upperLeft, pins.center };

    // Define segment mappings for digits 0-9 using named parameters
    digitMappings = {
        {pins.top, pins.upperRight, pins.lowerRight, pins.bottom, pins.lowerLeft, pins.upperLeft},  // Display_0
        {pins.upperRight, pins.lowerRight},  // Display_1
        {pins.top, pins.upperRight, pins.center, pins.lowerLeft, pins.bottom},  // Display_2
        {pins.top, pins.upperRight, pins.center, pins.lowerRight, pins.bottom},  // Display_3
        {pins.upperLeft, pins.center, pins.upperRight, pins.lowerRight},  // Display_4
        {pins.top, pins.upperLeft, pins.center, pins.lowerRight, pins.bottom},  // Display_5
        {pins.upperLeft, pins.lowerLeft, pins.bottom, pins.lowerRight, pins.center},  // Display_6
        {pins.top, pins.upperRight, pins.lowerRight},  // Display_7
        {pins.top, pins.upperRight, pins.lowerRight, pins.bottom, pins.lowerLeft, pins.upperLeft, pins.center},  // Display_8
        {pins.top, pins.upperRight, pins.center, pins.upperLeft, pins.lowerRight}  // Display_9
    };
}

void SevenSegmentDisplay::begin() {
    // Initialize pins as output
    for (int pin : segmentPins) {
        pinMode(pin, OUTPUT);
    }
    turnOffAllSegments();
}

void SevenSegmentDisplay::clearDisplay() {
    turnOffAllSegments();
}

void SevenSegmentDisplay::updateDisplay(int digit) {
    if (digit < 0 || digit > 9) return;

    turnOffAllSegments();

    // Turn on segments corresponding to the digit
    for (int pin: digitMappings[digit]) {
        digitalWrite(pin, LOW);
    }
}

void SevenSegmentDisplay::turnOffAllSegments() {
    for (int pin : segmentPins) {
        digitalWrite(pin, HIGH);
    }
}