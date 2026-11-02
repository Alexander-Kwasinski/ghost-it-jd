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

static bool startReading = HIGH;
static bool purgeReading = HIGH;
static bool trapReading = HIGH;

static unsigned long startLastChangeMs = 0;
static unsigned long purgeLastChangeMs = 0;
static unsigned long trapLastChangeMs = 0;


void inputs_init()
{
    pinMode(START_BUTTON, INPUT_PULLUP);
    pinMode(PURGE_BUTTON, INPUT);
    pinMode(TRAP_SWITCH, INPUT);

    // Record the initial states so startup doesn't trigger events.
    startState = digitalRead(START_BUTTON);
    purgeState = digitalRead(PURGE_BUTTON);
    trapState = digitalRead(TRAP_SWITCH);

    startReading = startState;
    purgeReading = purgeState;
    trapReading = trapState;

    startLastChangeMs = millis();
    purgeLastChangeMs = millis();
    trapLastChangeMs = millis();

    inputs_clear_events();
}


void start_button_update()
{
    bool reading = digitalRead(START_BUTTON);

    // Only debounce when the pin differs from its accepted state.
    if (reading != startReading) {
        startReading = reading;
        startLastChangeMs = millis();
    }

    if (startReading != startState &&
        millis() - startLastChangeMs >= DEBOUNCE_MS) {

        startState = startReading;

        // INPUT_PULLUP: LOW means pressed.
        if (startState == LOW) {
            startPressed = true;
        }
    }
}


void purge_switch_update()
{
    bool reading = digitalRead(PURGE_BUTTON);

    if (reading != purgeReading) {
        purgeReading = reading;
        purgeLastChangeMs = millis();
    }

    if (purgeReading != purgeState &&
        millis() - purgeLastChangeMs >= DEBOUNCE_MS) {

        purgeState = purgeReading;
        purgeToggled = true;
    }
}


void trap_switch_update()
{
    bool reading = digitalRead(TRAP_SWITCH);

    if (reading != trapReading) {
        trapReading = reading;
        trapLastChangeMs = millis();
    }

    if (trapReading != trapState &&
        millis() - trapLastChangeMs >= DEBOUNCE_MS) {

        trapState = trapReading;
        trapToggled = true;
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