#include <Arduino.h>
#include <Wire.h>   // i2c communication library
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

#include "motion.h"

Adafruit_MPU6050 mpu;

// gyroscope reads a rotation speed. need these to take derivative
static float playerRotationDeg = 0;
static unsigned long lastUpdateUs;
static bool sensorReady = false;
static bool scanActive = false;

static float normalizeAngle(float angle) {
    while (angle < 0) {
        angle += 360;
    } 
    
    while (angle > 360) {
        angle -= 360;
    }

    return angle
}

void motion_init() {
    Wire.begin();

    if(!mpu.begin()) {
        // needs updated to display some error code on the screen (probably)
        // shows failure initializing the gyroscope
        return;
    }

    mpu.getGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    sensorReady = true;
}

void motion_begin_scan() {
    if(!sensorRead) {
        return;
    }

    playerRotationDeg = 0;

    lastUpdateUs = micros();

    scanActive = true;
}

// Find shortest distance between two angles.
static float angleDifference(float angle1, float angle2)
{
    float difference = fabs(
        normalizeAngle(angle1) - normalizeAngle(angle2)
    );

    if (difference > 180) {
        difference = 360 - difference;
    }

    return difference;
}

void motion_update() {
    if(!sensorRead || !scanActive) {
        return;
    }

    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    unsigned long now = micros();

    float deltaT = (uint32_t)(now - lastUpdateUs) / 1000000.0f;
    lastUpdateUs = now;

    playerRotationDeg += gyro.gyro.z * (180/PI) * deltaT;
    playerRotationDeg = normalizeAngle(playerRotationDeg);
}

bool motion_scan_target_found(float ghostAngleDeg) {
    if(!sensorRead || !scanActive) {
        return;
    }

    float difference = angleDifference(playerRotationDeg, ghostAngleDeg);

    return difference <= 15.0f;
}

void motion_end_scan() {
    scanActive = false;
}