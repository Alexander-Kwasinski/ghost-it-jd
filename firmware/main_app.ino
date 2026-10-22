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
    START_SOUND,
    COMMAND_SOUND,
    WAITING_FOR_ACTION,
    CORRECT_SOUND
};

GameState state = GameState::IDLE;
PlayPhase phase = PlayPhase::START_SOUND;
Command activeCommand = static_cast<Command>(random(0,3));

void nextCommand()
{
    inputs_clear_events();

    activeCommand = game_pick_command();

    display_set_green_led(false);
    display_set_red_led(false);
    display_show_score(game_score());

    audio_play_command(activeCommand);

    phase = PlayPhase::COMMAND_SOUND;
}

void correctAnswer()
{
    game_record_correct();

    display_set_green_led(true);
    audio_play_correct();

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
    
}

setup() {
    Serial.begin(115200);

    randomSeed(micros());

    inputs_init();
    motion_init();

}


loop() {
    motion_update();
    inputs_update();

    switch (state) {

        case GameState::IDLE:


        case GameState::PLAYING:


        case GameState::GAME_OVER:
            if(start_button_press()) {
                startRound();
            }

    }


}