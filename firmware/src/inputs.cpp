#include <Arduino.h>

#include "inputs.h"
#include "pins.h"
#include "config.h"

// Pending input events.
static bool startPressed = false;
static bool purgeToggled = false;
static bool trapToggled = false;

// Last accepted electrical states.
static bool startState = LOW;
static bool purgeState = LOW;
static bool trapState = LOW;


void inputs_init()
{
    pinMode(START_BUTTON, INPUT_PULLUP);
    pinMode(PURGE_BUTTON, INPUT_PULLUP);
    pinMode(TRAP_SWITCH, INPUT_PULLUP);

    // Record the initial states so startup doesn't trigger events.
    startState = digitalRead(START_BUTTON);
    purgeState = digitalRead(PURGE_BUTTON);
    trapState = digitalRead(TRAP_SWITCH);

    inputs_clear_events();
}


void start_button_update()
{
    bool reading = digitalRead(START_BUTTON);

    // Only debounce when the pin differs from its accepted state.
    if (reading != startState) {
        delay(DEBOUNCE_MS);
        reading = digitalRead(START_BUTTON);

        if (reading != startState) {
            startState = reading;

            // INPUT_PULLUP: LOW means pressed.
            if (startState == LOW) {
                startPressed = true;
            }
        }
    }
}


void purge_switch_update()
{
    bool reading = digitalRead(PURGE_BUTTON);

    if (reading != purgeState) {
        delay(DEBOUNCE_MS);
        reading = digitalRead(PURGE_BUTTON);

        if (reading != purgeState) {
            purgeState = reading;
            purgeToggled = true;
        }
    }
}


void trap_switch_update()
{
    bool reading = digitalRead(TRAP_SWITCH);

    if (reading != trapState) {
        delay(DEBOUNCE_MS);
        reading = digitalRead(TRAP_SWITCH);

        if (reading != trapState) {
            trapState = reading;
            trapToggled = true;
        }
    }
}


bool start_button_press()
{
    if (startPressed) {
        startPressed = false;
        return true;
    }

    return false;
}


bool purge_switch_toggle()
{
    if (purgeToggled) {
        purgeToggled = false;
        return true;
    }

    return false;
}


bool trap_switch_toggle()
{
    if (trapToggled) {
        trapToggled = false;
        return true;
    }

    return false;
}


void inputs_clear_events()
{
    startPressed = false;
    purgeToggled = false;
    trapToggled = false;
}