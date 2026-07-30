/*
  RFID reader module - Raspberry Pi Pico H
  ------------------------------------------
  Reads card UIDs from an RC522 module over short SPI wiring, forwards each
  UID as a plain text line over hardware UART to an indoor ESP32 controller,
  and drives the local LEDs/buzzer based on the ESP32's GRANTED/DENIED reply.

  Design decision (see GitHub issue #35): this module intentionally does
  NOT validate RFID cards. It only reads UIDs and forwards them, and shows
  the result the ESP32 tells it to. Validation logic lives on the indoor
  ESP32 controller so it can't be tampered with by someone with physical
  access to this outdoor unit.

  Board: Raspberry Pi Pico H
  Framework: Arduino, using the earlephilhower "Raspberry Pi Pico/RP2040"
             core (Boards Manager URL:
             https://github.com/earlephilhower/arduino-pico)
  Library: MFRC522v2 by OSSLibraries (Library Manager: "MFRC522v2")

  Wiring - RC522 to Pico:
    RC522 VCC  -> Pico 3V3(OUT)   (physical pin 36) -- 3.3V only, do NOT use 5V/VBUS
    RC522 RST  -> Pico 3V3(OUT)   (tied high; this driver controls only CS, not RST)
    RC522 GND  -> Pico GND        (e.g. physical pin 38)
    RC522 SDA  -> Pico GP17 (CS)  (physical pin 22)
    RC522 SCK  -> Pico GP18       (physical pin 24)
    RC522 MOSI -> Pico GP19       (physical pin 25)
    RC522 MISO -> Pico GP16       (physical pin 21)
    RC522 IRQ  -> not connected (unused, matches the original ESP32 sketch)

  Wiring - LEDs/buzzer (moved here from the old combined ESP32 sketch):
    Blue LED  (idle heartbeat) -> Pico GP2 (physical pin 4), via a ~220-330ohm
                                   series resistor, other leg to GND
    Green LED (access granted) -> Pico GP3 (physical pin 5), via a ~220-330ohm
                                   series resistor, other leg to GND
    Buzzer                     -> Pico GP4 (physical pin 6)
                                   A small piezo buzzer can usually be driven
                                   directly from a GPIO. If yours draws more
                                   than a few mA, drive it through a small
                                   NPN transistor instead of straight from GP4.

  Wiring - Pico to ESP32 (UART, cross-connected):
    Pico GP0 (UART0 TX, physical pin 1) -> ESP32 RX  (e.g. GPIO16)
    Pico GP1 (UART0 RX, physical pin 2) <- ESP32 TX  (e.g. GPIO17)
    Pico GND                            <-> ESP32 GND (common ground - required)

  UART protocol:
    Pico -> ESP32: "RFID:<lowercase hex uid>\n" for every card scanned
    ESP32 -> Pico: "GRANTED\n" or "DENIED\n" after each validation
*/

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// ---- RC522 SPI pins ----
// SPI0 defaults on the Pico: SCK=GP18, MOSI(TX)=GP19, MISO(RX)=GP16.
// Only the chip-select pin needs to be told to the driver explicitly.
constexpr uint8_t RFID_CS_PIN = 17;

// ---- LEDs / buzzer ----
constexpr uint8_t BlueLedPin = 2;   // idle heartbeat, off while showing a result
constexpr uint8_t GreenLedPin = 3;  // on while access is granted
constexpr uint8_t BeepPin = 4;      // buzzer

// ---- UART to the ESP32 controller ----
// Serial1 on the Pico maps to hardware UART0 (GP0=TX, GP1=RX) by default.
// Serial (USB) stays free for debug logging over the USB cable.
constexpr unsigned long UART_TO_ESP32_BAUD = 115200;
String esp32RxBuffer = "";

// Avoid flooding the UART with repeated lines while a card sits on the reader
constexpr unsigned long READ_COOLDOWN_MS = 1000;

MFRC522DriverPinSimple ssPin(RFID_CS_PIN);
MFRC522DriverSPI driver{ssPin};  // default SPISettings: 4 MHz, MSBFIRST, MODE0
MFRC522 mfrc522{driver};

String lastUid = "";
unsigned long lastReadTime = 0;

String uidToHex(const MFRC522::Uid &uid) {
  String hex = "";
  for (byte i = 0; i < uid.size; i++) {
    if (uid.uidByte[i] < 0x10) {
      hex += "0";
    }
    hex += String(uid.uidByte[i], HEX);
  }
  hex.toLowerCase();
  return hex;
}

// Blue LED should be on for 1 second and off for 3 seconds while idle.
// Called on every loop iteration where no card is being processed.
void FlashBlueLed() {
  static bool isBlueLedOn = false;
  static unsigned long lastSwitchedBlueLed = 0;

  if (isBlueLedOn && millis() - lastSwitchedBlueLed >= 1000) {
    isBlueLedOn = false;
    digitalWrite(BlueLedPin, LOW);
    lastSwitchedBlueLed = millis();
  } else if (!isBlueLedOn && millis() - lastSwitchedBlueLed > 3000) {
    isBlueLedOn = true;
    digitalWrite(BlueLedPin, HIGH);
    lastSwitchedBlueLed = millis();
  }
}

// Green LED + single long beep to indicate the ESP32 granted access.
void AccessGranted() {
  digitalWrite(BlueLedPin, LOW);
  digitalWrite(GreenLedPin, HIGH);
  digitalWrite(BeepPin, HIGH);
  delay(500);
  digitalWrite(GreenLedPin, LOW);
  digitalWrite(BeepPin, LOW);
}

// Short double-beep to indicate the ESP32 denied access (no green LED).
void AccessDenied() {
  digitalWrite(BlueLedPin, LOW);
  for (int i = 0; i < 2; i++) {
    digitalWrite(BeepPin, HIGH);
    delay(150);
    digitalWrite(BeepPin, LOW);
    delay(150);
  }
}

// Flashes both LEDs and beeps once at boot to verify the components are OK.
void RunStartupSequence() {
  digitalWrite(BeepPin, HIGH);
  digitalWrite(BlueLedPin, HIGH);
  delay(250);
  digitalWrite(BlueLedPin, LOW);
  digitalWrite(GreenLedPin, HIGH);
  delay(250);
  digitalWrite(GreenLedPin, LOW);
  digitalWrite(BeepPin, LOW);
}

// Reads lines coming in over UART from the ESP32 controller and acts on
// "GRANTED"/"DENIED". Non-blocking: only consumes what's already buffered,
// one character at a time, across loop() iterations.
void handleEsp32Reply() {
  while (Serial1.available()) {
    char c = Serial1.read();

    if (c == '\n') {
      esp32RxBuffer.trim();

      if (esp32RxBuffer == "GRANTED") {
        AccessGranted();
      } else if (esp32RxBuffer == "DENIED") {
        AccessDenied();
      } else if (esp32RxBuffer.length() > 0) {
        Serial.print("Ignoring unrecognized line from controller: ");
        Serial.println(esp32RxBuffer);
      }

      esp32RxBuffer = "";
    } else if (c != '\r') {
      esp32RxBuffer += c;
    }
  }
}

void setup() {
  Serial.begin(115200);               // USB serial, for debugging via USB cable
  Serial.println("Initialize UART with ESP32 controller");
  Serial1.begin(UART_TO_ESP32_BAUD);  // Hardware UART to the ESP32 controller

  pinMode(BlueLedPin, OUTPUT);
  digitalWrite(BlueLedPin, LOW);
  pinMode(GreenLedPin, OUTPUT);
  digitalWrite(GreenLedPin, LOW);
  pinMode(BeepPin, OUTPUT);
  digitalWrite(BeepPin, LOW);

  Serial.println("Initialize RFID-RC522 sensor");
  // driver.init() (called from PCD_Init()) handles SPI.begin() and the CS
  // pin itself, so nothing else needs to be set up here.
  mfrc522.PCD_Init();

  Serial.println("RFID module starting...");
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);

  // Verify LEDs/buzzer are wired correctly before waiting for cards.
  RunStartupSequence();

  Serial.println("Ready. Waiting for cards...");
}

void loop() {
  // Always process any pending reply from the ESP32, regardless of
  // whether a card is currently being read.
  handleEsp32Reply();

  if (!mfrc522.PICC_IsNewCardPresent()) {
    FlashBlueLed();
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  String uid = uidToHex(mfrc522.uid);
  unsigned long now = millis();

  // Forward the UID unless we just sent the same one moments ago
  // (a card left sitting on the reader will otherwise be read repeatedly).
  if (uid != lastUid || (now - lastReadTime) > READ_COOLDOWN_MS) {
    lastUid = uid;
    lastReadTime = now;

    Serial.print("Card read: ");
    Serial.println(uid);

    Serial1.print("RFID:");
    Serial1.println(uid);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
