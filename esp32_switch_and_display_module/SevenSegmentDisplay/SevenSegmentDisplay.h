#ifndef SEVENSEGMENTDISPLAY_H
#define SEVENSEGMENTDISPLAY_H

#include <Arduino.h>
#include <vector>

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
    SevenSegmentDisplay(const SegmentPins& pins);
    void begin();
    void updateDisplay(int digit);
    void clearDisplay();

private:
    std::vector<int> segmentPins;
    std::vector<std::vector<int>> digitMappings;
    void turnOffAllSegments();
};

#endif