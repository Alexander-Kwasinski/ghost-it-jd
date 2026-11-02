#pragma once

#include "config.h"

bool audio_player_init();
void audio_player_update();

bool audio_player_has_error();
const char *audio_player_error_message();

void audio_start_theme();
void audio_stop_all();

void audio_play_command(Command command);
bool audio_command_finished();

void audio_play_win();
void audio_play_lose();

void audio_set_speed_level(int level);

bool audio_effect_is_playing();