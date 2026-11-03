#pragma once

void display_init();

void display_show_title();

void display_update_radar(int score, int lives, bool showGhost, float ghostDirectionDeg);

void display_show_game_over(int finalScore);

void display_show_error(const char *title, const char *message);

void display_show_you_win(int finalScore);