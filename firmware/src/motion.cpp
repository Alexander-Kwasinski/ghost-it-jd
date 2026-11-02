#include <Arduino.h>
#include <Wire.h>   // i2c communication library
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

#include "motion.h"
#include "pins.h"
#include "config.h"

Adafruit_MPU6050 mpu;

// gyroscope reads a rotation speed. need these to take derivative
static float playerRotationDeg = 0;
static unsigned long lastUpdateUs;
static bool sensorReady = false;
static bool scanActive = false;
static float gyroBiasZ = 0.0f;

static float normalizeAngle(float angle) {
    while (angle < 0) {
        angle += 360;
    } 

    while (angle >= 360) {
        angle -= 360;
    }

    return angle;
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

static void calibrateGyroscope() {
    const int calibrationSamples = 200;
    float gyroTotal = 0.0f;

    for (int i = 0; i < calibrationSamples; i++) {
        sensors_event_t accel, gyro, temp;
        mpu.getEvent(&accel, &gyro, &temp);

        gyroTotal += gyro.gyro.z;

        delay(5);
    }

    gyroBiasZ = gyroTotal / calibrationSamples;
}

void motion_init() {
    sensorReady = false;
    scanActive = false;

    Wire.begin(I2C_SDA, I2C_SCL);

    if(!mpu.begin()) {
        // needs updated to display some error code on the screen (probably)
        // shows failure initializing the gyroscope
        return;
    }

    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    delay(100);

    calibrateGyroscope();

    sensorReady = true;
}

void motion_begin_scan() {
    if(!sensorReady) {
        return;
    }

    playerRotationDeg = 0;

    lastUpdateUs = micros();

    scanActive = true;
}

bool motion_is_ready() {
    return sensorReady;
}

void motion_update() {
    if(!sensorReady || !scanActive) {
        return;
    }

    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    unsigned long now = micros();

    float deltaT = (uint32_t)(now - lastUpdateUs) / 1000000.0f;
    lastUpdateUs = now;

    float rotationRateDps = (gyro.gyro.z - gyroBiasZ) * (180.0f / PI);

    if (fabs(rotationRateDps) <= SCAN_THRESHOLD_DPS) {
        playerRotationDeg += rotationRateDps * deltaT;
        playerRotationDeg = normalizeAngle(playerRotationDeg);
    }
}

float motion_scan_direction(float ghostAngleDeg)
{
    float difference = ghostAngleDeg - playerRotationDeg;

    while (difference > 180.0f) {
        difference -= 360.0f;
    }

    while (difference < -180.0f) {
        difference += 360.0f;
    }

    return difference;
}

bool motion_target_found(float ghostAngleDeg) {
    if(!sensorReady || !scanActive) {
        return false;
    }

    float difference = angleDifference(playerRotationDeg, ghostAngleDeg);

    return difference <= 15.0f;
}

void motion_end_scan() {
    scanActive = false;
}