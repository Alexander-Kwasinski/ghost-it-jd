#pragma once

// initialize all buttons and toggle switches
void inputs_init();
//detects and records inputs
void inputs_update();

// used to clear an input after it is used 
// prevents someone from holding a button done for too long and register as multiple game state inputs
bool start_button_press();
bool purge_button_press();
bool trap_switch_toggle();

// 
void inputs_clear_events();