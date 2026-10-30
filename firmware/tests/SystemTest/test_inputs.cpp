#include <Arduino.h>
#include "../../inc/pins.h"
#include "test_inputs.h"

namespace {

constexpr unsigned long INPUT_DEBOUNCE_MS = 35;

struct DebouncedInput {
    uint8_t pin;
    bool useInternalPullup;
    bool lastRawActive;
    bool stableActive;
    bool pressEvent;
    unsigned long rawChangedAt;
};

DebouncedInput trapInput  = {TRAP_SWITCH,  false, false, false, false, 0};
DebouncedInput purgeInput = {PURGE_BUTTON, false, false, false, false, 0};
DebouncedInput startInput = {START_BUTTON, true,  false, false, false, 0};

void beginInput(DebouncedInput &input)
{
    pinMode(input.pin, input.useInternalPullup ? INPUT_PULLUP : INPUT);

    input.lastRawActive = (digitalRead(input.pin) == LOW);
    input.stableActive = input.lastRawActive;
    input.pressEvent = false;
    input.rawChangedAt = millis();
}

void updateInput(DebouncedInput &input)
{
    const unsigned long now = millis();
    const bool rawActive = (digitalRead(input.pin) == LOW); // all controls are active LOW

    if (rawActive != input.lastRawActive) {
        input.lastRawActive = rawActive;
        input.rawChangedAt = now;
    }

    if ((now - input.rawChangedAt) >= INPUT_DEBOUNCE_MS &&
        input.stableActive != input.lastRawActive) {
        input.stableActive = input.lastRawActive;

        // Generate one event only when the control becomes active.
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

} // namespace

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
