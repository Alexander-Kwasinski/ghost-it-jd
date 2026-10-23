#pragma once

// The commands the game can issue, and what the player can do
enum Command {
  CMD_NONE,
  CMD_SCAN,
  CMD_TRAP,
  CMD_PURGE
};

// The three game states
enum GameState {
  IDLE,
  PLAYING,
  GAME_OVER
};

// Tunable numbers
constexpr unsigned long START_RESPONSE_MS = 3000;
constexpr unsigned long RESPONSE_STEP_MS  = 150;
constexpr unsigned long MIN_RESPONSE_MS   = 800;
constexpr unsigned long DEBOUNCE_MS     = 50;
constexpr float SCAN_THRESHOLD_DPS      = 200.0;  // gyro rate that counts as a sweep