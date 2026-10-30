#pragma once

struct MotionReading {
    bool valid = false;
    uint8_t i2cAddress = 0;

    int16_t axRaw = 0;
    int16_t ayRaw = 0;
    int16_t azRaw = 0;
    int16_t temperatureRaw = 0;
    int16_t gxRaw = 0;
    int16_t gyRaw = 0;
    int16_t gzRaw = 0;

    float axG = 0.0f;
    float ayG = 0.0f;
    float azG = 0.0f;
    float temperatureC = 0.0f;
    float gxDps = 0.0f;
    float gyDps = 0.0f;
    float gzDps = 0.0f;
};

bool testMotionBegin();
void testMotionUpdate(MotionReading &reading);
