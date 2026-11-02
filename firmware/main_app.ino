#include <Arduino.h>
#include <SPI.h>

#include "pins.h"
#include "config.h"
#include "game_logic.h"
#include "inputs.h"
#include "motion.h"
#include "audio_player.h"
#include "display.h"

enum class PlayPhase : uint8_t {
    WAITING_FOR_COMMAND_AUDIO,
    WAITING_FOR_ACTION,
    RESULT_PAUSE
};

// Game state
static GameState state = IDLE;
static PlayPhase phase = PlayPhase::WAITING_FOR_COMMAND_AUDIO;

static Command activeCommand;

static int livesRemaining = 3;
static float activeGhostAngle = 0.0f;

// Timing
static unsigned long commandStartedMs = 0;
static unsigned long resultStartedMs = 0;
static const unsigned long RESULT_PAUSE_MS = 450;

// Function declarations
static void nextCommand();
static void correctAnswer();
static void loseLife();
static void finishGame(bool won);
static void startRound();

// Select the next command
static void nextCommand() {
    inputs_clear_events();

    motion_end_scan();

    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);

    activeCommand = game_pick_command();

    // Set audio speed based on the current difficulty.
    audio_set_speed_level(game_speed_level());

    // Play the command. The audio module pauses the theme automatically.
    audio_play_command(activeCommand);

    phase = PlayPhase::WAITING_FOR_COMMAND_AUDIO;
}

static void correctAnswer() {
    if (activeCommand == SCAN) {
        motion_end_scan();
    }

    game_record_correct();

    bool won = game_complete_command();

    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);

    // Check whether 100 commands have been completed.
    if (won) {
        finishGame(true);
        return;
    }

    phase = PlayPhase::RESULT_PAUSE;
    resultStartedMs = millis();
}


// Incorrect response or timeout
static void loseLife() {
    if (activeCommand == SCAN) {
        motion_end_scan();
    }

    if (livesRemaining > 0) {
        livesRemaining--;
    }

    // Incorrect command still counts as completed
    bool won = game_complete_command();

    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);

    if (livesRemaining == 0) {
        finishGame(false);
        return;
    }

    if (won) {
        finishGame(true);
        return;
    }

    phase = PlayPhase::RESULT_PAUSE;
    resultStartedMs = millis();
}

// End the game
static void finishGame(bool won) {
    motion_end_scan();

    state = GAME_OVER;

    if (won) {
        display_show_you_win(game_score());
        audio_play_win();
    }
    else {
        display_show_game_over(game_score());
        audio_play_lose();
    }
}

// Start a new round
static void startRound() {
    inputs_clear_events();

    audio_stop_all();

    game_reset();
    livesRemaining = 3;

    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);

    motion_end_scan();

    state = PLAYING;

    // Start the background theme
    audio_start_theme();

    nextCommand();
}


void setup() {
    randomSeed(micros());

    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);

    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);

    // Deselect both SPI devices before initializing the bus
    pinMode(OLED_CS, OUTPUT);
    digitalWrite(OLED_CS, HIGH);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);

    audio_player_init();

    display_init();

    inputs_init();
    motion_init();

    if (audio_player_has_error()) {
        display_show_error("AUDIO ERROR", audio_player_error_message());

        while (true) {
            delay(100);
        }
    }

    if (!motion_is_ready()) {
        display_show_error("MPU6050 ERROR", "CHECK SENSOR");

        while (true) {
            delay(100);
        }
    }

    state = IDLE;

    display_show_title();
}


void loop() {
    audio_player_update();

    if (audio_player_has_error()) {
        motion_end_scan();
        display_show_error("AUDIO ERROR", audio_player_error_message());

        while (true) {
            delay(100);
        }
    }

    start_button_update();
    purge_switch_update();
    trap_switch_update();

    bool startPressed = start_button_press();

    motion_update();

    if (state == PLAYING) {

        bool showGhost = (activeCommand == SCAN) && (phase == PlayPhase::WAITING_FOR_ACTION);

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
            // Wait for the command audio to finish before starting the timer
            if (phase == PlayPhase::WAITING_FOR_COMMAND_AUDIO) {
                if (audio_command_finished()) {
                    inputs_clear_events();

                    commandStartedMs = millis();
                    phase = PlayPhase::WAITING_FOR_ACTION;

                    if (activeCommand == SCAN) {
                        activeGhostAngle = game_pick_ghost_angle_deg();
                        motion_begin_scan();
                    }
                }

                break;
            }

            // Pause after each response
            if (phase == PlayPhase::RESULT_PAUSE) {

                if (millis() - resultStartedMs >= RESULT_PAUSE_MS) {
                    nextCommand();
                }

                break;
            }

            // Response timeout
            if (millis() - commandStartedMs >= (unsigned long)response_time()) {
                loseLife();
                break;
            }

            if (activeCommand == SCAN) {
                bool purgeChanged = purge_switch_toggle();
                bool trapChanged = trap_switch_toggle();

                bool purgePressed = purgeChanged && digitalRead(PURGE_BUTTON) == LOW;

                bool trapPressed = trapChanged && digitalRead(TRAP_SWITCH) == LOW;

                if (purgePressed || trapPressed) {
                    loseLife();
                    break;
                }

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