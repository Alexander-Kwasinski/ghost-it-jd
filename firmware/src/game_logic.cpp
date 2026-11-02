#include <Arduino.h>
#include "game_logic.h"

static unsigned long responseWinMs = START_RESPONSE_MS;

static int score = 0;
static int completedCommands = 0;

void game_reset() {
    responseWinMs = START_RESPONSE_MS;

    score = 0;
    completedCommands = 0;
}

void game_record_correct() {
    score++;
}

// Call once whenever a command finishes.
// Returns true when 100 commands have been completed.
bool game_complete_command() {
    completedCommands++;

    // Increase difficulty every 10 completed commands.
    if (completedCommands % COMMANDS_PER_SPEEDUP == 0) {

        if (responseWinMs <= MIN_RESPONSE_MS + RESPONSE_STEP_MS) {
            responseWinMs = MIN_RESPONSE_MS;
        }
        else {
            responseWinMs -= RESPONSE_STEP_MS;
        }
    }

    return completedCommands >= MAX_COMMANDS;
}

int game_score() {
    return score;
}

int response_time() {
    return responseWinMs;
}

Command game_pick_command() {
    // Select SCAN, TRAP, or PURGE.
    return static_cast<Command>(
        random((long)SCAN, (long)PURGE + 1)
    );
}

float game_pick_ghost_angle_deg() {
    return random(0, 36000) / 100.0f;
}