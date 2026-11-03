
#include <Arduino.h>
#include "../../inc/pins.h"
#include "test_inputs.h"

namespace {

constexpr unsigned long INPUT_DEBOUNCE_MS = 35;

struct DebouncedInput {
    uint8_t pin;
    bool useInternalPulldown;
    bool lastRawActive;
    bool stableActive;
    bool pressEvent;
    unsigned long rawChangedAt;
};

// GPIO34 and GPIO35 require external pull-down resistors.
// GPIO16 can use its internal pull-down.
DebouncedInput trapInput  = {TRAP_SWITCH, false, false, false, false, 0};
DebouncedInput purgeInput = {PURGE_BUTTON, false, false, false, false, 0};
DebouncedInput startInput = {START_BUTTON, true, false, false, false, 0};

bool readRawActive(const DebouncedInput &input)
{
    // All controls are active-HIGH.
    return digitalRead(input.pin) == HIGH;
}

void beginInput(DebouncedInput &input)
{
    pinMode(input.pin, input.useInternalPulldown ? INPUT_PULLDOWN : INPUT);

    input.lastRawActive = readRawActive(input);
    input.stableActive = input.lastRawActive;
    input.pressEvent = false;
    input.rawChangedAt = millis();
}

void updateInput(DebouncedInput &input)
{
    const unsigned long now = millis();
    const bool rawActive = readRawActive(input);

    if (rawActive != input.lastRawActive) {
        input.lastRawActive = rawActive;
        input.rawChangedAt = now;
    }

    if ((now - input.rawChangedAt) >= INPUT_DEBOUNCE_MS &&
        input.stableActive != input.lastRawActive) {

        input.stableActive = input.lastRawActive;

        if (input.stableActive) {
            input.pressEvent = true;
        }
    }
}

bool takePressEvent(DebouncedInput &input)
{
    const bool event = input.pressEvent;
    input.pressEvent = false;
    return event;
}

}

void testInputsBegin()
{
    beginInput(trapInput);
    beginInput(purgeInput);
    beginInput(startInput);
}

void testInputsUpdate(InputSnapshot &snapshot)
{
    updateInput(trapInput);
    updateInput(purgeInput);
    updateInput(startInput);

    snapshot.trapActive = trapInput.stableActive;
    snapshot.purgeActive = purgeInput.stableActive;
    snapshot.startActive = startInput.stableActive;

    snapshot.trapPressed = takePressEvent(trapInput);
    snapshot.purgePressed = takePressEvent(purgeInput);
    snapshot.startPressed = takePressEvent(startInput);
}
