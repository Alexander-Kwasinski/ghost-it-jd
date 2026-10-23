#pragma once

static bool startPressed, purgePressed, trapToggled = false; 

void inputs_init()  {
    return;
}

bool start_button_press() {
    if(startPressed) {
        startPressed = false;
        return true;
    }
    return false;
}

bool purge_button_press() {
    if(purgePressed) {
        purgePressed = false;
        return true;
    }
    return false;
}

bool trap_switch_toggle() {
    if(trapToggled) {
        trapToggled = false;
        return true;
    }
    return false;
}

void input_update() {
    return;
}

void clear_input_events() {
    return;
}