#pragma once

// ---------- I2C bus (OLED + gyroscope/accelerometer) ----------
constexpr int I2C_SDA = 21;
constexpr int I2C_SCL = 22;
 
// ---------- microSD card (SPI / VSPI default pins) ----------
constexpr int SD_SCK  = 18;
constexpr int SD_MISO = 19;
constexpr int SD_MOSI = 23;
constexpr int SD_CS   = 5;
constexpr int SD_CD = 6
 
// ---------- Audio out to PAM8403 amplifier (analog) ----------
// GPIO 25 is a built-in DAC pin on the classic ESP32.
constexpr int AUDIO_DAC = 25;
 
// ---------- DFPlayer Mini (alternate audio option) ----------
// Only used if the DFPlayer replaces the SD card + PAM8403.
constexpr int DFPLAYER_RX = 16;   // ESP32 RX2 <- DFPlayer TX
constexpr int DFPLAYER_TX = 17;   // ESP32 TX2 -> DFPlayer RX
 
// ---------- Inputs (internal pull-up, switch wired to GND) ----------
constexpr int TRAP_SWITCH  = 32;  // momentary toggle switch
constexpr int PURGE_BUTTON = 33;  // big red pushbutton
constexpr int START_BUTTON = 14;  // big green pushbutton
 
// ---------- Status LEDs (through current-limiting resistors) ----------
constexpr int LED_GREEN = 13;     // correct response (left antenna)
constexpr int LED_RED   = 4;      // wrong action / timeout (right antenna)
 
// ---------- Button lamps (optional, drive through a transistor) ----------
constexpr int PURGE_LAMP = 26;
constexpr int START_LAMP = 27;
 
// ---------- Battery voltage sense (optional, via resistor divider) ----------
// GPIO 35 is input-only, which is fine for an analog read.
constexpr int BATTERY_SENSE = 35;