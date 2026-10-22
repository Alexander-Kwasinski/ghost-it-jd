#include <Arduino.h>
#include "game_logic.h"

static uint32_t responseWinMs = START_RESPONSE_MS;
static int score = 0;

void game_reset() {
    responseWinMs = START_RESPONSE_MS;
    score = 0;
}

void step_response_time() {
    
}

int game_score() {
    return score;
}

int response_time() {
    return responseWinMs;
}

Command game_pick_command() {
    return static_cast<Command>(random(0,3))
}