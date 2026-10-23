#pragma once
#include "config.h"

void game_reset();
void step_game_time();

int game_score();
int response_time();

Command game_pick_command();

float game_pick_ghost_angle_deg();