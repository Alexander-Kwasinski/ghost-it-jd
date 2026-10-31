
#include <Arduino.h>
#include <Wire.h>
#include "../../inc/pins.h"
#include "test_motion.h"

namespace {

// MPU6050 I2C addresses
constexpr uint8_t MPU6050_ADDR_0 = 0x68;
constexpr uint8_t MPU6050_ADDR_1 = 0x69;

// MPU6050 registers
constexpr uint8_t MPU_REG_SMPLRT_DIV   = 0x19;
constexpr uint8_t MPU_REG_CONFIG       = 0x1A;
constexpr uint8_t MPU_REG_GYRO_CONFIG  = 0x1B;
constexpr uint8_t MPU_REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t MPU_REG_ACCEL_XOUT_H = 0x3B;
constexpr uint8_t MPU_REG_PWR_MGMT_1   = 0x6B;
constexpr uint8_t MPU_REG_WHO_AM_I     = 0x75;

// Sampling configuration
constexpr unsigned long MOTION_SAMPLE_MS = 20; // 50 Hz

uint8_t sensorAddress = 0;

bool sensorReady = false;

unsigned long lastSampleMs = 0;
unsigned long lastErrorPrintMs = 0;

uint8_t lastI2CError = 0;
size_t lastBytesReceived = 0;


// --------------------------------------------------
// I2C REGISTER WRITE
// --------------------------------------------------

bool writeRegister(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(sensorAddress);

    Wire.write(reg);
    Wire.write(value);

    lastI2CError = Wire.endTransmission();

    return lastI2CError == 0;
}


// --------------------------------------------------
// I2C REGISTER READ
// --------------------------------------------------

bool readRegisters(uint8_t startRegister,
                   uint8_t *buffer,
                   size_t count)
{
    Wire.beginTransmission(sensorAddress);

    Wire.write(startRegister);

    lastI2CError = Wire.endTransmission(false);

    if (lastI2CError != 0) {
        lastBytesReceived = 0;
        return false;
    }

    size_t received = Wire.requestFrom(
        static_cast<uint16_t>(sensorAddress),
        count,
        true
    );

    lastBytesReceived = received;

    if (received != count) {

        while (Wire.available()) {
            Wire.read();
        }

        return false;
    }

    for (size_t i = 0; i < count; i++) {

        if (!Wire.available()) {
            return false;
        }

        buffer[i] = static_cast<uint8_t>(Wire.read());
    }

    return true;
}


// --------------------------------------------------
// READ ONE REGISTER
// --------------------------------------------------

bool readRegister(uint8_t reg, uint8_t &value)
{
    return readRegisters(reg, &value, 1);
}


// --------------------------------------------------
// REGISTER CONFIGURATION WITH VERIFICATION
// --------------------------------------------------

bool configureRegister(const char *name,
                       uint8_t reg,
                       uint8_t value)
{
    if (!writeRegister(reg, value)) {

        Serial.printf(
            "MPU6050: WRITE FAILED: %s, error=%u\n",
            name,
            lastI2CError
        );

        return false;
    }

    delay(2);

    uint8_t readback = 0;

    if (!readRegister(reg, readback)) {

        Serial.printf(
            "MPU6050: READBACK FAILED: %s\n",
            name
        );

        return false;
    }

    if (readback != value) {

        Serial.printf(
            "MPU6050: VALUE MISMATCH: %s, wrote 0x%02X, read 0x%02X\n",
            name,
            value,
            readback
        );

        return false;
    }

    Serial.printf(
        "MPU6050: %s configured: 0x%02X\n",
        name,
        readback
    );

    return true;
}


// --------------------------------------------------
// CONVERT TWO BYTES INTO SIGNED 16-BIT VALUE
// --------------------------------------------------

int16_t makeInt16(uint8_t highByte, uint8_t lowByte)
{
    return static_cast<int16_t>(
        (static_cast<uint16_t>(highByte) << 8) | lowByte
    );
}
}


// --------------------------------------------------
// SENSOR INITIALIZATION
// --------------------------------------------------

bool testMotionBegin()
{
    Serial.println();
    Serial.println(F("========== MPU6050 INITIALIZATION =========="));

    sensorReady = false;
    sensorAddress = 0;

    // Initialize I2C using the pins from pins.h.
    Wire.begin(I2C_SDA, I2C_SCL);

    // Start at a conservative I2C speed.
    Wire.setClock(100000);

    Serial.printf(
        "I2C: SDA=GPIO%u SCL=GPIO%u Clock=100kHz\n",
        I2C_SDA,
        I2C_SCL
    );

    // ----------------------------------------------
    // STEP 1: FIND THE MPU6050
    // ----------------------------------------------

    const uint8_t addresses[] = {
        MPU6050_ADDR_0,
        MPU6050_ADDR_1
    };

    uint8_t whoAmI = 0;

    for (uint8_t address : addresses) {

        sensorAddress = address;

        Serial.printf(
            "Checking MPU6050 at 0x%02X...\n",
            sensorAddress
        );

        if (readRegister(MPU_REG_WHO_AM_I, whoAmI)) {

            Serial.printf(
                "WHO_AM_I = 0x%02X\n",
                whoAmI
            );

            // MPU6050 normally returns 0x68.
            if (whoAmI == 0x68) {

                sensorReady = true;

                Serial.println(F("MPU6050 DETECTED"));

                break;
            }

            Serial.println(F("Unexpected WHO_AM_I value"));
        }
        else {

            Serial.printf(
                "No response. I2C error=%u\n",
                lastI2CError
            );
        }
    }

    if (!sensorReady) {

        Serial.println(
            F("ERROR: MPU6050 NOT FOUND")
        );

        Serial.println(
            F("Check VCC, GND, SDA, SCL and AD0.")
        );

        return false;
    }


    // ----------------------------------------------
    // STEP 2: RESET SENSOR
    // ----------------------------------------------

    Serial.println(F("Resetting MPU6050..."));

    if (!writeRegister(MPU_REG_PWR_MGMT_1, 0x80)) {

        Serial.println(F("ERROR: Sensor reset failed"));

        sensorReady = false;

        return false;
    }

    delay(100);


    // ----------------------------------------------
    // STEP 3: WAKE SENSOR
    // ----------------------------------------------

    Serial.println(F("Waking MPU6050..."));

    if (!configureRegister(
            "POWER MANAGEMENT",
            MPU_REG_PWR_MGMT_1,
            0x00)) {

        sensorReady = false;

        return false;
    }

    delay(100);


    // ----------------------------------------------
    // STEP 4: CONFIGURE SAMPLE RATE
    // ----------------------------------------------

    // Internal rate = 1 kHz.
    // Divider = 7.
    // Output rate = 125 Hz.

    if (!configureRegister(
            "SAMPLE RATE",
            MPU_REG_SMPLRT_DIV,
            0x07)) {

        sensorReady = false;

        return false;
    }


    // ----------------------------------------------
    // STEP 5: CONFIGURE DIGITAL LOW PASS FILTER
    // ----------------------------------------------

    if (!configureRegister(
            "DLPF",
            MPU_REG_CONFIG,
            0x03)) {

        sensorReady = false;

        return false;
    }


    // ----------------------------------------------
    // STEP 6: CONFIGURE GYROSCOPE
    // ----------------------------------------------

    // +/- 250 degrees per second.
    // Sensitivity = 131 LSB per degree/second.

    if (!configureRegister(
            "GYROSCOPE",
            MPU_REG_GYRO_CONFIG,
            0x00)) {

        sensorReady = false;

        return false;
    }


    // ----------------------------------------------
    // STEP 7: CONFIGURE ACCELEROMETER
    // ----------------------------------------------

    // +/- 2 g.
    // Sensitivity = 16384 LSB per g.

    if (!configureRegister(
            "ACCELEROMETER",
            MPU_REG_ACCEL_CONFIG,
            0x00)) {

        sensorReady = false;

        return false;
    }


    // ----------------------------------------------
    // STEP 8: TEST COMPLETE SENSOR DATA READ
    // ----------------------------------------------

    Serial.println(F("Testing 14-byte sensor read..."));

    uint8_t raw[14];

    if (!readRegisters(
            MPU_REG_ACCEL_XOUT_H,
            raw,
            sizeof(raw))) {

        Serial.printf(
            "ERROR: Initial sensor data read failed. "
            "I2C error=%u, bytes received=%u\n",
            lastI2CError,
            static_cast<unsigned>(lastBytesReceived)
        );

        sensorReady = false;

        return false;
    }

    Serial.println(F("14-byte sensor read SUCCESS"));


    // ----------------------------------------------
    // INITIALIZATION COMPLETE
    // ----------------------------------------------

    lastSampleMs = millis() - MOTION_SAMPLE_MS;
    lastErrorPrintMs = 0;

    Serial.println(F("--------------------------------------------"));
    Serial.println(F("MPU6050 READY"));
    Serial.println(F("Accelerometer: +/-2 g"));
    Serial.println(F("Gyroscope: +/-250 degrees/sec"));
    Serial.println(F("Sampling: 50 Hz"));
    Serial.println(F("I2C: 100 kHz"));
    Serial.println(F("--------------------------------------------"));
    Serial.println();

    return true;
}


// --------------------------------------------------
// CONTINUOUS MOTION SAMPLING
// --------------------------------------------------

void testMotionUpdate(MotionReading &reading)
{
    if (!sensorReady) {

        reading.valid = false;

        return;
    }

    const unsigned long now = millis();

    // Maintain a 50 Hz sampling rate.
    if (now - lastSampleMs < MOTION_SAMPLE_MS) {
        return;
    }

    lastSampleMs = now;

    uint8_t raw[14];

    // Read accelerometer, temperature and gyroscope
    // in one continuous I2C transaction.

    if (!readRegisters(
            MPU_REG_ACCEL_XOUT_H,
            raw,
            sizeof(raw))) {

        reading.valid = false;

        // Avoid flooding Serial with errors.
        if (now - lastErrorPrintMs >= 1000) {

            lastErrorPrintMs = now;

            Serial.printf(
                "MPU6050 READ ERROR: I2C error=%u, "
                "bytes received=%u of 14\n",
                lastI2CError,
                static_cast<unsigned>(lastBytesReceived)
            );
        }

        return;
    }


    // ----------------------------------------------
    // EXTRACT RAW ACCELEROMETER VALUES
    // ----------------------------------------------

    reading.axRaw = makeInt16(raw[0], raw[1]);
    reading.ayRaw = makeInt16(raw[2], raw[3]);
    reading.azRaw = makeInt16(raw[4], raw[5]);


    // ----------------------------------------------
    // EXTRACT TEMPERATURE
    // ----------------------------------------------

    reading.temperatureRaw = makeInt16(raw[6], raw[7]);


    // ----------------------------------------------
    // EXTRACT RAW GYROSCOPE VALUES
    // ----------------------------------------------

    reading.gxRaw = makeInt16(raw[8], raw[9]);
    reading.gyRaw = makeInt16(raw[10], raw[11]);
    reading.gzRaw = makeInt16(raw[12], raw[13]);


    // ----------------------------------------------
    // CONVERT ACCELEROMETER TO G
    // ----------------------------------------------

    reading.axG = reading.axRaw / 16384.0f;
    reading.ayG = reading.ayRaw / 16384.0f;
    reading.azG = reading.azRaw / 16384.0f;


    // ----------------------------------------------
    // CONVERT TEMPERATURE TO CELSIUS
    // ----------------------------------------------

    reading.temperatureC =
        (reading.temperatureRaw / 340.0f) + 36.53f;


    // ----------------------------------------------
    // CONVERT GYROSCOPE TO DEGREES PER SECOND
    // ----------------------------------------------

    reading.gxDps = reading.gxRaw / 131.0f;
    reading.gyDps = reading.gyRaw / 131.0f;
    reading.gzDps = reading.gzRaw / 131.0f;


    // ----------------------------------------------
    // UPDATE READING STATUS
    // ----------------------------------------------

    reading.i2cAddress = sensorAddress;
    reading.valid = true;
}
