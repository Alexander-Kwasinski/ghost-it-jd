#pragma once
#include <Arduino.h>

// Target: classic ESP32-WROOM-32E
// OLED: HiLetgo 2.42" SSD1309, 128x64, SPI
// Audio: microSD -> ESP32 DAC -> PAM8403

// ---------- Shared SPI bus (OLED + microSD) ----------
constexpr uint8_t SPI_SCK  = 18;
constexpr uint8_t SPI_MISO = 19;  // Used by microSD
constexpr uint8_t SPI_MOSI = 23;

// ---------- OLED (SSD1309, SPI) ----------
constexpr uint8_t OLED_CS  = 27;
constexpr uint8_t OLED_DC  = 26;
constexpr uint8_t OLED_RST = 4; // active LOW.


// ---------- microSD (audio files) ----------
constexpr uint8_t SD_CS = 13;

// ---------- I2C bus (motion sensor) ----------
constexpr uint8_t I2C_SDA = 21;
constexpr uint8_t I2C_SCL = 22;

// ---------- Audio output ----------
constexpr uint8_t AUDIO_DAC = 25;  // Classic ESP32 DAC1
// GPIO25 -> PAM8403 analog input

// ---------- Inputs (switch/button wired to GND) ----------
constexpr uint8_t TRAP_SWITCH  = 34;
constexpr uint8_t PURGE_BUTTON = 35;
constexpr uint8_t START_BUTTON = 16;

// ---------- Status LEDs ----------
constexpr uint8_t LED_GREEN = 14;  // Correct response
constexpr uint8_t LED_RED   = 17;  // Wrong action / timeout

// ---------- Button LEDs ----------
constexpr uint8_t LED_START_BUTTON = 32;
constexpr uint8_t LED_PURGE_BUTTON = 33;