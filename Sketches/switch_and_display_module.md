# ESP32 WROOM 32D - switch and display module

Switch and display module is almost complete on breadboards.

![Tinkercad sketch](switch_and_display_module.png)

## ESP32 WROOM 32D board is visualized as four 8-pin headers in the sketch.
 - D32 is the green wire going through a 10kΩ resistor to pin 4 on a [VO617A Optocoupler](https://mou.sr/45uYfCM). The other side of the optocoupler is connected on pin 1 to the +27V signal from the emergency stop button of the original Windsor control unit through a 2.2kΩ resistor. Pin 2 to GND on the emergency stop button.
 - D33 is the yellow wire going through a 220Ω resistor to pin 1 on a [VO617A Optocoupler](https://mou.sr/45uYfCM). The other side of the optocoupler is connected on pin 4 to the +27V signal to the Open Door switch of the original Windsor control unit without any resistor. Pin 3 to GND on the Open Door switch.
 - D25 is used to receive a signal from the RFID and door sensor module to open the door. This is the brown wire going out of the sketch into D25 of the other ESP WROOM 32D board.
 - D26 is the green wire going to the push button used to select which value to display (day/week/month)
 - D27 is the yellow wire going through a 1kΩ resistor to the yellow LED representing month view
 - D14 is the yellow wire going through a 1kΩ resistor to the yellow LED representing week view
 - D12 is the yellow wire going through a 1kΩ resistor to the yellow LED representing day view

 ## 4 digits 7 segments display
 The 4 digits display is visualized as a single digit in this Tinkercad sketch.
 
 All wires connected to the 4 digits 7 segments display are going through 220Ω resistors, one for each pin. They are all connected to the ESP32 module according to the following pin mapping:
  |ESP32 pin|Wire color|Display pin|
 |--|--|--|
 |D13|white|12|
 |D23|green|11|
 |D22|blue|10|
 |D21|purple|9|
 |D19|gray|8|
 |D18|white|7|
 |D5|white|6|
 |D17|green|5|
 |D16|blue|4|
 |D4|purple|3|
 |D2|gray|2|
 |D15|white|1|
