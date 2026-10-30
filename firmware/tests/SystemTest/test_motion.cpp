#include <Arduino.h>
#include <Wire.h>
#include "../../inc/pins.h"
#include "test_motion.h"

namespace {

constexpr uint8_t MPU6050_ADDR_0 = 0x68;
constexpr uint8_t MPU6050_ADDR_1 = 0x69;
constexpr uint8_t MPU_REG_SMPLRT_DIV = 0x19;
constexpr uint8_t MPU_REG_CONFIG = 0x1A;
constexpr uint8_t MPU_REG_GYRO_CONFIG = 0x1B;
constexpr uint8_t MPU_REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t MPU_REG_ACCEL_XOUT_H = 0x3B;
constexpr uint8_t MPU_REG_PWR_MGMT_1 = 0x6B;
constexpr uint8_t MPU_REG_WHO_AM_I = 0x75;

constexpr unsigned long MOTION_SAMPLE_MS = 20; // 50 Hz

uint8_t sensorAddress = 0;
bool sensorReady = false;
unsigned long lastSampleMs = 0;

bool writeRegister(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(sensorAddress);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool readRegisters(uint8_t startRegister, uint8_t *buffer, size_t count)
{
    Wire.beginTransmission(sensorAddress);
    Wire.write(startRegister);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    const size_t received = Wire.requestFrom(sensorAddress, count, true);
    if (received != count) {
        while (Wire.available()) {
            Wire.read();
        }
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        buffer[i] = static_cast<uint8_t>(Wire.read());
    }

    return true;
}

int16_t makeInt16(uint8_t highByte, uint8_t lowByte)
{
    return static_cast<int16_t>((static_cast<uint16_t>(highByte) << 8) | lowByte);
}

} // namespace

bool testMotionBegin()
{
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);

    const uint8_t addresses[] = {MPU6050_ADDR_0, MPU6050_ADDR_1};
    uint8_t whoAmI = 0;

    for (uint8_t address : addresses) {
        sensorAddress = address;
        if (readRegisters(MPU_REG_WHO_AM_I, &whoAmI, 1)) {
            // MPU6050 WHO_AM_I normally reads 0x68, regardless of AD0 state.
            if (whoAmI == 0x68) {
                sensorReady = true;
                break;
            }
        }
    }

    if (!sensorReady) {
        Serial.println(F("MPU6050: no response at I2C address 0x68 or 0x69"));
        return false;
    }

    // Wake the sensor and select modest full-scale ranges for raw test readings.
    bool ok = true;
    ok &= writeRegister(MPU_REG_PWR_MGMT_1, 0x00);
    ok &= writeRegister(MPU_REG_SMPLRT_DIV, 0x07);
    ok &= writeRegister(MPU_REG_CONFIG, 0x03);
    ok &= writeRegister(MPU_REG_GYRO_CONFIG, 0x00);  // +/- 250 degrees/second
    ok &= writeRegister(MPU_REG_ACCEL_CONFIG, 0x00); // +/- 2 g

    if (!ok) {
        sensorReady = false;
        Serial.println(F("MPU6050: failed to configure registers"));
        return false;
    }

    Serial.printf("MPU6050: found at 0x%02X, WHO_AM_I=0x%02X\n", sensorAddress, whoAmI);
    return true;
}

void testMotionUpdate(MotionReading &reading)
{
    if (!sensorReady) {
        reading.valid = false;
        return;
    }

    const unsigned long now = millis();
    if (now - lastSampleMs < MOTION_SAMPLE_MS) {
        return;
    }
    lastSampleMs = now;

    uint8_t raw[14];
    if (!readRegisters(MPU_REG_ACCEL_XOUT_H, raw, sizeof(raw))) {
        reading.valid = false;
        return;
    }

    reading.axRaw = makeInt16(raw[0], raw[1]);
    reading.ayRaw = makeInt16(raw[2], raw[3]);
    reading.azRaw = makeInt16(raw[4], raw[5]);
    reading.temperatureRaw = makeInt16(raw[6], raw[7]);
    reading.gxRaw = makeInt16(raw[8], raw[9]);
    reading.gyRaw = makeInt16(raw[10], raw[11]);
    reading.gzRaw = makeInt16(raw[12], raw[13]);

    // The configured ranges are +/-2 g and +/-250 degrees/second.
    reading.axG = reading.axRaw / 16384.0f;
    reading.ayG = reading.ayRaw / 16384.0f;
    reading.azG = reading.azRaw / 16384.0f;
    reading.temperatureC = (reading.temperatureRaw / 340.0f) + 36.53f;
    reading.gxDps = reading.gxRaw / 131.0f;
    reading.gyDps = reading.gyRaw / 131.0f;
    reading.gzDps = reading.gzRaw / 131.0f;

    reading.i2cAddress = sensorAddress;
    reading.valid = true;
}
