#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include "../../inc/pins.h"
#include "test_display.h"

namespace {

U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI oled(
    U8G2_R0,
    OLED_CS,
    OLED_DC,
    OLED_RST
);

unsigned long lastDisplayUpdateMs = 0;

} // namespace

bool testDisplayBegin()
{
    oled.begin();
    oled.setContrast(180);
    oled.setFont(u8g2_font_5x7_tf);
    oled.clearBuffer();
    oled.drawStr(0, 12, "GHOST-IT SYSTEM TEST");
    oled.drawStr(0, 27, "OLED INITIALIZED");
    oled.drawStr(0, 42, "Starting peripherals...");
    oled.sendBuffer();
    return true;
}

void testDisplayUpdate(const MotionReading &motion,
                       const InputSnapshot &inputs,
                       bool motionInitialized,
                       bool displayInitialized,
                       bool sdReady,
                       bool audioPlaying,
                       const char *trackName,
                       unsigned long sdBytesRead)
{
    if (!displayInitialized) {
        return;
    }

    const unsigned long now = millis();
    if (now - lastDisplayUpdateMs < 250) {
        return;
    }
    lastDisplayUpdateMs = now;

    char line[32];

    oled.clearBuffer();
    oled.setFont(u8g2_font_5x7_tf);

    oled.drawStr(0, 8, "GHOST-IT LOAD TEST");

    snprintf(line, sizeof(line), "MPU:%s  SD:%s",
             (motionInitialized && motion.valid) ? "OK" : "ERR",
             sdReady ? "OK" : "ERR");
    oled.drawStr(0, 19, line);

    snprintf(line, sizeof(line), "AUDIO:%s", audioPlaying ? "PLAYING" : "STOPPED");
    oled.drawStr(0, 30, line);

    snprintf(line, sizeof(line), "T:%d  P:%d  S:%d",
             inputs.trapActive,
             inputs.purgeActive,
             inputs.startActive);
    oled.drawStr(0, 41, line);

    if (motion.valid) {
        snprintf(line, sizeof(line), "Gx:%5.0f Gy:%5.0f",
                 motion.gxDps, motion.gyDps);
    } else {
        snprintf(line, sizeof(line), "GYRO: no sensor data");
    }
    oled.drawStr(0, 52, line);

    snprintf(line, sizeof(line), "SD read: %lu KB", sdBytesRead / 1024UL);
    oled.drawStr(0, 63, line);

    // The full track path is printed on Serial because it is longer than one OLED line.
    (void)trackName;

    oled.sendBuffer();
}
