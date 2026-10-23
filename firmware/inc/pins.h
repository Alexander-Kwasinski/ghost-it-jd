#pragma once

// ---------- I2C bus (motion sensor) ----------
constexpr int I2C_SDA = A4;
constexpr int I2C_SCL = A5;


// ---------- SPI bus (LCD) ----------
constexpr int SPI_MOSI = 11;
constexpr int SPI_MISO = 12;
constexpr int SPI_SCK  = 13;


// ---------- LCD (ILI9341) ----------
constexpr int LCD_CS  = 10;
constexpr int LCD_DC  = 9;
constexpr int LCD_RST = 8;


// ---------- DFPlayer Mini ----------
constexpr int DFPLAYER_RX = 2;  // Arduino RX <- DFPlayer TX
constexpr int DFPLAYER_TX = 3;  // Arduino TX -> DFPlayer RX


// ---------- Inputs (switch/button wired to GND) ----------
constexpr int TRAP_SWITCH  = 4;
constexpr int PURGE_BUTTON = 5;
constexpr int START_BUTTON = 6;


// ---------- Status LEDs ----------
constexpr int LED_GREEN = A0;  // Correct response
constexpr int LED_RED   = A1;  // Wrong action / timeout