#pragma once

#include "config.h"

void game_reset();

void game_record_correct();
bool game_complete_command();

int game_score();
int response_time();

Command game_pick_command();

float game_pick_ghost_angle_deg();