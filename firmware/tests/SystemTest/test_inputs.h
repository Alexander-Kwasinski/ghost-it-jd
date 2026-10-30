#pragma once

struct InputSnapshot {
    bool trapActive = false;
    bool purgeActive = false;
    bool startActive = false;

    bool trapPressed = false;
    bool purgePressed = false;
    bool startPressed = false;
};

void testInputsBegin();
void testInputsUpdate(InputSnapshot &snapshot);
