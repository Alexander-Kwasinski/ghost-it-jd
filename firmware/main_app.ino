#include <Arduino.h>

#include "pins.h"
#include "config.h"
#include "game_logic.h"
#include "inputs.h"
#include "motion.h"
#include "pins.h"
#include "audio_player.h"
#include "display.h"

enum class PlayPhase : uint8_t {
    WAITING_FOR_ACTION,
    CORRECT_PAUSE
};

GameState state = GameState::IDLE;
PlayPhase phase = PlayPhase::WAITING_FOR_INPUT;

static Command activeCommand = SCAN;

static int livesRemaining = 3;
static float activeGhostAngle = 0.0f;

static unsigned long commandStartedMs = 0;
static unsigned long resultStartedMs = 0;

static const unsigned long RESULT_PAUSE_MS = 450;


void nextCommand()
{
    inputs_clear_events();

    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);

    motion_end_scan();

    activeCommand = game_pick_command();

    commandStartedMs = millis();
    phase = PlayPhase::WAITING_FOR_ACTION;

    if (activeCommand == SCAN) {
        activeGhostAngle = game_pick_ghost_angle_deg();
        motion_begin_scan();
    }
}

void correctAnswer()
{
    game_record_correct();

    display_set_green_led(true);
    audio_play_correct();

    if (activeCommand == SCAN) {
        motion_end_scan();
    }

    phase = PlayPhase::CORRECT_SOUND;
}

void startRound() {
    inputs_clear_events();
    game_reset();

    display_set_green_led(false);
    display_set_red_led(false);
    display_show_score(0);

    state = GameState::PLAYING;
    phase = PlayPhase::START_SOUND;
}

void endGame() {
    motion_end_scan();

    state = GAME_OVER;
    phase = PlayPhase::WAITING_FOR_INPUT;
}

void setup() {
    Serial.begin(115200);

    randomSeed(micros());

    inputs_init();
    motion_init();

}


void loop() {
    start_button_update();
    purge_switch_update();
    trap_switch_update();

    motion_update();

    bool startPressed = start_button_press();

    switch (state) {

        case GameState::IDLE:
            // screen start screen
            // start button led turns
            if (startPressed) {
                startRound();
            }
            break;

        case GameState::PLAYING:

            // Check whether the response window expired.
            if (millis() - commandStartedMs >=
                    (unsigned long)response_time()) {

                endGame();
                break;
            }

            if (activeCommand == SCAN) {
                float activeGhostAngle = game_pick_ghost_angle_deg();

                motion_begin_scan();

                while (!motion_target_found(activeGhostAngle)) {
                    motion_update();
                    float direction = motion_scan_direction(activeGhostAngle);
                }
                motion_end_scan();
                correctAnswer();
            }

        case GameState::GAME_OVER:
            if(start_button_press()) {
                startRound();
            }

    }


}