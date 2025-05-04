#ifndef SEVENSEGMENTDISPLAY_H
#define SEVENSEGMENTDISPLAY_H

#include <Arduino.h>
#include <vector>

struct DigitPins {
    int onesDigit;
    int tensDigit;
    int hundredsDigit;
    int thousandsDigit;
};

struct SegmentPins {
    int top;
    int upperRight;
    int lowerRight;
    int decimalPoint;
    int bottom;
    int lowerLeft;
    int upperLeft;
    int center;
};

class SevenSegmentDisplay {
public:
    SevenSegmentDisplay(const DigitPins& digPins, const SegmentPins& segPins);
    void begin();
    void updateDisplay(int number);
    void clearDisplay();

private:
    std::vector<int> digitPins;
    std::vector<int> segmentPins;
    std::vector<std::vector<int>> digitMappings;
    void turnOffAllSegments();
    void turnOnDigitSegments(int digit);
    void turnOnDigitSegment(int digitPin, int digitValue);
};

#endif