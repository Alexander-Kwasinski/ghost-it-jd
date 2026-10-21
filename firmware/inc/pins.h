#pragma once

// I2C
constexpr int SDA = 21;
constexpr int SCL = 22;

// SD card (SPI)
constexpr int SD_CS = 5;

// I2S amp
constexpr int I2S_BCLK = 26;
constexpr int I2S_LRC  = 25;
constexpr int I2S_DIN  = 27;

// Inputs
constexpr int TRAP_SWITCH  = 32;
constexpr int PURGE_BUTTON = 33;
constexpr int START_BUTTON = 14;

// Outputs
constexpr int LED_GREEN = 16;
constexpr int LED_RED   = 17;