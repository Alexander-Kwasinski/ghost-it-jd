#pragma once

void inputs_init();

void start_button_update();
void purge_switch_update();
void trap_switch_update();

bool start_button_press();
bool purge_switch_toggle();
bool trap_switch_toggle();

void inputs_clear_events();