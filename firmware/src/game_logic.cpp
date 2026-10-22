#include <Arduino.h>
#include "game_logic.h"

static uint32_t responseWinMs = START_RESPONSE_MS;
static int score = 0;

void game_reset() {
    responseWinMs = START_RESPONSE_MS;
    score = 0;
}

void step_response_time() {
    score++;

    if (responseWindowMs - MIN_RESPONSE_MS
            <= RESPONSE_STEP_MS) {
            responseWindowMs = MIN_RESPONSE_MS;
    } else {
        responseWindowMs -= RESPONSE_STEP_MS;
    }
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

float game_pick_ghost_angle_deg() {
    return random(0, 360);
}