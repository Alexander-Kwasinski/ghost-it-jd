#pragma once

// ---------- I2C bus (OLED + gyroscope/accelerometer) ----------
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;
 
// ---------- microSD card (SPI / VSPI default pins) ----------
constexpr int PIN_SD_SCK  = 18;
constexpr int PIN_SD_MISO = 19;
constexpr int PIN_SD_MOSI = 23;
constexpr int PIN_SD_CS   = 5;
 
// ---------- Audio out to PAM8403 amplifier (analog) ----------
// GPIO 25 is a built-in DAC pin on the classic ESP32.
constexpr int PIN_AUDIO_DAC = 25;
 
// ---------- DFPlayer Mini (alternate audio option) ----------
// Only used if the DFPlayer replaces the SD card + PAM8403.
constexpr int PIN_DFPLAYER_RX = 16;   // ESP32 RX2 <- DFPlayer TX
constexpr int PIN_DFPLAYER_TX = 17;   // ESP32 TX2 -> DFPlayer RX
 
// ---------- Inputs (internal pull-up, switch wired to GND) ----------
constexpr int PIN_TRAP_SWITCH  = 32;  // momentary toggle switch
constexpr int PIN_PURGE_BUTTON = 33;  // big red pushbutton
constexpr int PIN_START_BUTTON = 14;  // big green pushbutton
 
// ---------- Status LEDs (through current-limiting resistors) ----------
constexpr int PIN_LED_GREEN = 13;     // correct response (left antenna)
constexpr int PIN_LED_RED   = 4;      // wrong action / timeout (right antenna)
 
// ---------- Button lamps (optional, drive through a transistor) ----------
constexpr int PIN_PURGE_LAMP = 26;
constexpr int PIN_START_LAMP = 27;
 
// ---------- Battery voltage sense (optional, via resistor divider) ----------
// GPIO 35 is input-only, which is fine for an analog read.
constexpr int PIN_BATTERY_SENSE = 35;