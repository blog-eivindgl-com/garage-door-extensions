/*
  RFID reader module - Raspberry Pi Pico H
  ------------------------------------------
  Reads card UIDs from an RC522 module over short SPI wiring, and forwards
  each UID as a plain text line over hardware UART to an indoor ESP32
  controller, which is responsible for validation and WiFi/MQTT.

  Design decision (see GitHub issue #35): this module intentionally does
  NOT validate RFID cards. It only reads UIDs and forwards them. Validation
  logic lives on the indoor ESP32 controller so it can't be tampered with
  by someone with physical access to this outdoor unit.

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

  Wiring - Pico to ESP32 (UART, cross-connected):
    Pico GP0 (UART0 TX, physical pin 1) -> ESP32 RX  (e.g. GPIO16)
    Pico GP1 (UART0 RX, physical pin 2) <- ESP32 TX  (e.g. GPIO17)
    Pico GND                            <-> ESP32 GND (common ground - required)

  UART protocol (Pico -> ESP32), one line per scanned card:
    RFID:<lowercase hex uid>\n
  e.g.
    RFID:b338ec13
*/

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// ---- RC522 SPI pins ----
// SPI0 defaults on the Pico: SCK=GP18, MOSI(TX)=GP19, MISO(RX)=GP16.
// Only the chip-select pin needs to be told to the driver explicitly.
constexpr uint8_t RFID_CS_PIN = 17;

// ---- UART to the ESP32 controller ----
// Serial1 on the Pico maps to hardware UART0 (GP0=TX, GP1=RX) by default.
// Serial (USB) stays free for debug logging over the USB cable.
constexpr unsigned long UART_TO_ESP32_BAUD = 115200;

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

void setup() {
  Serial.begin(115200);               // USB serial, for debugging via USB cable
  Serial1.begin(UART_TO_ESP32_BAUD);  // Hardware UART to the ESP32 controller

  // driver.init() (called from PCD_Init()) handles SPI.begin() and the CS
  // pin itself, so nothing else needs to be set up here.
  mfrc522.PCD_Init();

  Serial.println("RFID module starting...");
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  Serial.println("Ready. Waiting for cards...");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
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
