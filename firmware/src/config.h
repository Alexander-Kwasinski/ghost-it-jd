#pragma once

// The commands the game can issue, and what the player can do
enum Command {
  SCAN,
  TRAP,
  PURGE
};

// The three game states
enum GameState {
  IDLE,
  PLAYING,
  GAME_OVER
};

// Response timer.
constexpr unsigned long START_RESPONSE_MS = 3000;
constexpr unsigned long MIN_RESPONSE_MS = 100;
constexpr unsigned long RESPONSE_STEP_MS = 323;

// Game progression.
constexpr int COMMANDS_PER_SPEEDUP = 10;
constexpr int MAX_COMMANDS = 100;

// Input and motion.
constexpr unsigned long DEBOUNCE_MS = 50;
constexpr float SCAN_THRESHOLD_DPS = 200.0f;