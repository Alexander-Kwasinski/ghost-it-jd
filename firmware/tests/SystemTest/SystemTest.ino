#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "../../inc/pins.h"
#include "test_inputs.h"
#include "test_motion.h"
#include "test_display.h"
#include "test_storage_audio.h"

static InputSnapshot inputs;
static MotionReading motion;

static bool displayReady = false;
static bool motionReady = false;

static unsigned long lastReportMs = 0;

static void initializeOutputs()
{
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_START_BUTTON, OUTPUT);
    pinMode(LED_PURGE_BUTTON, OUTPUT);

    // Keep every indicator on during the load test.
    // The button LED pins must drive transistor/MOSFET stages, not the 12 V lamps directly.
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_START_BUTTON, HIGH);
    digitalWrite(LED_PURGE_BUTTON, HIGH);

    // Both peripherals share the same hardware SPI bus.
    pinMode(OLED_CS, OUTPUT);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(OLED_CS, HIGH);
    digitalWrite(SD_CS, HIGH);
}

static void printStatus()
{
    Serial.printf("[%8lu ms] ", millis());

    Serial.printf("MPU=%s ", (motionReady && motion.valid) ? "OK" : "NO DATA");
    if (motion.valid) {
        Serial.printf("A[g]=(%6.2f,%6.2f,%6.2f) ",
                      motion.axG, motion.ayG, motion.azG);
        Serial.printf("G[dps]=(%7.1f,%7.1f,%7.1f) ",
                      motion.gxDps, motion.gyDps, motion.gzDps);
    }

    Serial.printf("INPUTS: TRAP=%d PURGE=%d START=%d ",
                  inputs.trapActive,
                  inputs.purgeActive,
                  inputs.startActive);

    Serial.printf("OLED=%s SD=%s AUDIO=%s ",
                  displayReady ? "INIT" : "NO",
                  testStorageReady() ? "OK" : "NO",
                  testAudioPlaying() ? "PLAYING" : "STOPPED");

    if (testAudioPlaying()) {
        Serial.printf("TRACK=%s ", testAudioCurrentTrack());
    }

    Serial.printf("SD_READ_TOTAL=%lu bytes\n", testStorageBytesRead());
}

void setup()
{
    Serial.begin(115200);
    delay(250);

    Serial.println();
    Serial.println(F("============================================"));
    Serial.println(F(" GHOST-IT! COMPLETE SYSTEM LOAD TEST"));
    Serial.println(F(" ESP32-WROOM-32E"));
    Serial.println(F("============================================"));

    initializeOutputs();

    // Start the shared SPI bus with the exact pins from inc/pins.h.
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    // Initialize each subsystem independently so one missing module does not
    // prevent the remaining hardware from being exercised.
    testInputsBegin();
    motionReady = testMotionBegin();

    // Initialize SD/audio before the OLED. The ESP32's DAC1 is GPIO25. The
    // audio output class below is forced to one channel so DAC2/GPIO26 remains
    // available for OLED_DC in your existing pin map.
    bool sdReady = testStorageAudioBegin();

    // This comes after audio-output creation because the audio driver's initial
    // DAC shutdown may briefly configure GPIO26. U8g2 then initializes OLED_DC.
    displayReady = testDisplayBegin();

    Serial.printf("INPUT TEST: ready\n");
    Serial.printf("MOTION TEST: %s\n", motionReady ? "MPU6050 FOUND" : "MPU6050 NOT FOUND");
    Serial.printf("SD TEST: %s\n", sdReady ? "CARD READY" : "CARD NOT FOUND");
    Serial.printf("OLED TEST: %s\n", displayReady ? "INITIALIZED" : "INITIALIZATION FAILED");
    Serial.println(F("Load test is running. Use Serial Monitor at 115200 baud."));
    Serial.println(F("Press each control and move the MPU6050 to verify live readings."));
}

void loop()
{
    // Inputs are scanned continuously with independent debounce state.
    testInputsUpdate(inputs);

    // The motion function samples at a fixed rate without blocking the loop.
    testMotionUpdate(motion);

    // This services MP3 decode, audio playback, and periodic background SD reads.
    testStorageAudioUpdate();

    // OLED refresh is rate-limited inside this function.
    testDisplayUpdate(motion,
                      inputs,
                      motionReady,
                      displayReady,
                      testStorageReady(),
                      testAudioPlaying(),
                      testAudioCurrentTrack(),
                      testStorageBytesRead());

    if (inputs.trapPressed) {
        Serial.println(F("EVENT: TRAP switch activated"));
    }
    if (inputs.purgePressed) {
        Serial.println(F("EVENT: PURGE button pressed"));
    }
    if (inputs.startPressed) {
        Serial.println(F("EVENT: START button pressed"));
    }

    if (millis() - lastReportMs >= 1000) {
        lastReportMs = millis();
        printStatus();
    }
}
