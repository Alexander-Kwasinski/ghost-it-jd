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
PlayPhase phase = PlayPhase::WAITING_FOR_ACTION;

static Command activeCommand = SCAN;

static int livesRemaining = 3;
static float activeGhostAngle = 0.0f;

static unsigned long commandStartedMs = 0;
static unsigned long resultStartedMs = 0;

static const unsigned long RESULT_PAUSE_MS = 450;


// Select the next command
static void nextCommand() {
    inputs_clear_events();

    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);

    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);

    // Deselect both SPI devices
    pinMode(OLED_CS, OUTPUT);
    digitalWrite(OLED_CS, HIGH);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);

    activeCommand = game_pick_command();

    commandStartedMs = millis();
    phase = PlayPhase::WAITING_FOR_ACTION;

    if (activeCommand == SCAN) {
        activeGhostAngle = game_pick_ghost_angle_deg();
        motion_begin_scan();
    }
}

static void correctAnswer() {
    if (activeCommand == SCAN) {
        motion_end_scan();
    }

    game_record_correct();

    bool won = game_complete_command();

    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);

    // check whether 100 commands have been completed
    if (won) {
        state = GAME_OVER;
        display_show_you_win(game_score());
        return;
    }

    phase = PlayPhase::RESULT_PAUSE;
    resultStartedMs = millis();
}


// incorrect response or timeout
static void loseLife() {
    if (activeCommand == SCAN) {
        motion_end_scan();
    }

    if (livesRemaining > 0) {
        livesRemaining--;
    }

    // incorrect command still counts as completed
    bool won = game_complete_command();

    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);

    if (livesRemaining == 0) {
        state = GAME_OVER;
        display_show_game_over(game_score());
        return;
    }

    if (won) {
        state = GAME_OVER;
        display_show_you_win(game_score());
        return;
    }

    phase = PlayPhase::RESULT_PAUSE;
    resultStartedMs = millis();
}

static void startRound() {
    inputs_clear_events();

    game_reset();
    livesRemaining = 3;

    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);

    state = PLAYING;

    nextCommand();
}


void setup() {
    Serial.begin(115200);

    randomSeed(micros());

    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);

    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);

    display_init();
    inputs_init();
    motion_init();

    if (!motion_is_ready()) {
        display_show_error("MPU6050 ERROR", "CHECK SENSOR");

        while (true) {
            delay(100);
        }
    }

    state = IDLE;

    display_show_title();
}


void loop()
{
    start_button_update();
    purge_switch_update();
    trap_switch_update();

    bool startPressed = start_button_press();

    motion_update();

    if (state == PLAYING) {

        bool showGhost =
            activeCommand == SCAN &&
            phase == PlayPhase::WAITING_FOR_ACTION;

        float direction = 0.0f;

        if (showGhost) {
            direction = motion_scan_direction(activeGhostAngle);
        }

        display_update_radar(game_score(), livesRemaining, showGhost, direction);
    }

    switch (state) {

        case IDLE:

            if (startPressed) {
                startRound();
            }
            break;

        case PLAYING:

            // pause after each response
            if (phase == PlayPhase::RESULT_PAUSE) {

                if (millis() - resultStartedMs >= RESULT_PAUSE_MS) {
                    nextCommand();
                }

                break;
            }

            // response timeout
            if (millis() - commandStartedMs >= (unsigned long)response_time()) {
                loseLife();
                break;
            }

            if (activeCommand == SCAN) {

                bool purgeChanged = purge_switch_toggle();
                bool trapChanged = trap_switch_toggle();

                bool purgePressed = purgeChanged && digitalRead(PURGE_BUTTON) == LOW;

                bool trapPressed = trapChanged && digitalRead(TRAP_SWITCH) == LOW;

                // Purge or trap is incorrect during a scan.
                if (purgePressed || trapPressed) {
                    loseLife();
                    break;
                }

                // check whether the ghost is within 15 degrees.
                if (motion_target_found(activeGhostAngle)) {
                    correctAnswer();
                }
            }


            // TRAP IT or PURGE IT
            else {

                bool purgeChanged = purge_switch_toggle();
                bool trapChanged = trap_switch_toggle();

                bool purgePressed = purgeChanged && digitalRead(PURGE_BUTTON) == LOW;

                bool trapPressed = trapChanged && digitalRead(TRAP_SWITCH) == LOW;

                if (!purgePressed && !trapPressed) {
                    break;
                }

                bool correct = (activeCommand == PURGE && purgePressed && !trapPressed) || 
                                (activeCommand == TRAP && trapPressed && !purgePressed);

                if (correct) {
                    correctAnswer();
                }
                else {
                    loseLife();
                }
            }

            break;

        case GAME_OVER:

            if (startPressed) {
                startRound();
            }

            break;
    }
}