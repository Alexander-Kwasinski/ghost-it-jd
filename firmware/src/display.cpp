#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <math.h>
#include <stdio.h>

#include "display.h"
#include "pins.h"
#include "spi_bus.h"

U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI oled(
    U8G2_R0,
    OLED_CS,
    OLED_DC,
    OLED_RST
);

// The audio task reads the SD card on the same SPI bus.
static void sendFrame() {
    SpiBusLock lock;
    oled.sendBuffer();
}

static const int RADAR_X = 64;
static const int RADAR_Y = 39;
static const int RADAR_RADIUS = 23;
static const int GHOST_RADIUS = 17;

static float sweepAngle = 0.0f;
static unsigned long lastFrameMs = 0;
static unsigned long pingStartedMs = 0;
static float previousSweepAngle = 0.0f;

static bool radarStarted = false;
static bool pingActive = false;


static float normalizeDegrees(float angle) {
    while (angle < 0.0f) angle += 360.0f;
    while (angle >= 360.0f) angle -= 360.0f;

    return angle;
}


static void drawCentered(const char *text, int y) {
    int x = (128 - oled.getStrWidth(text)) / 2;

    if (x < 0) x = 0;

    oled.drawStr(x, y, text);
}


// Draw radar rings and center lines.
static void drawRadarBase() {
    oled.drawCircle(RADAR_X, RADAR_Y, RADAR_RADIUS);
    oled.drawCircle(RADAR_X, RADAR_Y, 15);
    oled.drawCircle(RADAR_X, RADAR_Y, 8);

    oled.drawHLine(
        RADAR_X - RADAR_RADIUS,
        RADAR_Y,
        RADAR_RADIUS * 2 + 1
    );

    oled.drawVLine(
        RADAR_X,
        RADAR_Y - RADAR_RADIUS,
        RADAR_RADIUS * 2 + 1
    );

    oled.drawDisc(RADAR_X, RADAR_Y, 1);
}


// Draw sparse pixels behind the main sweep to imitate a fading trail.
static void drawSweepTrail(float angleDeg) {
    for (int trail = 1; trail <= 4; trail++) {

        float angle =
            (angleDeg - trail * 7.0f) * PI / 180.0f;

        for (int r = 5; r < RADAR_RADIUS; r += 3 + trail) {

            int x = RADAR_X + (int)(sinf(angle) * r);
            int y = RADAR_Y - (int)(cosf(angle) * r);

            oled.drawPixel(x, y);
        }
    }
}

// draw a dotted ring around the ghost.
// pixels outside the radar circle are ignored.
static void drawPingRing(int x, int y, int radius) {
    for (int angle = 0; angle < 360; angle += 15) {

        float radians = angle * PI / 180.0f;

        int px = x + (int)(cosf(radians) * radius);
        int py = y + (int)(sinf(radians) * radius);

        int dx = px - RADAR_X;
        int dy = py - RADAR_Y;

        if (dx * dx + dy * dy <= RADAR_RADIUS * RADAR_RADIUS) {
            oled.drawPixel(px, py);
        }
    }
}


void display_init() {
    oled.begin();
    oled.setContrast(180);

    radarStarted = false;
    pingActive = false;
    sweepAngle = 0.0f;
}

void display_show_title() {
    oled.clearBuffer();

    oled.setFont(u8g2_font_7x14B_tf);
    drawCentered("GHOST-IT!", 25);

    oled.setFont(u8g2_font_6x10_tf);
    drawCentered("PRESS START", 45);

    sendFrame();
}

void display_update_radar(int score, int lives, bool showGhost, float ghostDirectionDeg) {
    unsigned long now = millis();

    if (!radarStarted) {
        radarStarted = true;
        lastFrameMs = now;
        previousSweepAngle = sweepAngle;
    }

    unsigned long elapsed = now - lastFrameMs;

    // Update the OLED approximately 25 times per second.
    if (elapsed < 40) {
        return;
    }

    lastFrameMs = now;

    // Rotate the sweep at 120 degrees per second.
    float sweepAdvance = elapsed * 0.12f;

    previousSweepAngle = sweepAngle;

    sweepAngle = normalizeDegrees(
        sweepAngle + sweepAdvance
    );

    float targetAngle = normalizeDegrees(ghostDirectionDeg);


    // Check whether the sweep passes over the ghost.
    if (showGhost && sweepAdvance > 0.0f) {

        float distanceFromPreviousSweep =
            normalizeDegrees(targetAngle - previousSweepAngle);

        if (distanceFromPreviousSweep <= sweepAdvance + 1.0f) {
            pingStartedMs = now;
            pingActive = true;
        }
    }
    else if (!showGhost) {
        pingActive = false;
    }


    // Prepare the status text.
    char livesText[12];
    char scoreText[16];

    snprintf(livesText, sizeof(livesText), "L:%d/3", lives);
    snprintf(scoreText, sizeof(scoreText), "S:%03d", score);


    oled.clearBuffer();

    // Lives and score stay at the top.
    oled.setFont(u8g2_font_5x7_tf);

    oled.drawStr(1, 8, livesText);
    oled.drawStr(98, 8, scoreText);


    // Draw the radar background.
    drawRadarBase();
    drawSweepTrail(sweepAngle);


    // Draw the main rotating sweep.
    // Zero degrees points toward the top.
    float sweepRadians = sweepAngle * PI / 180.0f;

    int beamX =
        RADAR_X + (int)(sinf(sweepRadians) * RADAR_RADIUS);

    int beamY =
        RADAR_Y - (int)(cosf(sweepRadians) * RADAR_RADIUS);

    oled.drawLine(
        RADAR_X,
        RADAR_Y,
        beamX,
        beamY
    );


    // Draw the ghost only during SCAN IT.
    if (showGhost) {

        float targetRadians = targetAngle * PI / 180.0f;

        int ghostX =
            RADAR_X + (int)(sinf(targetRadians) * GHOST_RADIUS);

        int ghostY =
            RADAR_Y - (int)(cosf(targetRadians) * GHOST_RADIUS);


        // Expanding ping effect after the radar detects the ghost.
        if (pingActive) {

            unsigned long pingAge = now - pingStartedMs;

            if (pingAge < 650) {

                int ringRadius = 2 + (int)(pingAge / 65);

                drawPingRing(ghostX, ghostY, ringRadius);

                if (pingAge > 100 && pingAge < 600) {
                    drawPingRing(
                        ghostX,
                        ghostY,
                        ringRadius / 2 + 2
                    );
                }
            }
            else {
                pingActive = false;
            }
        }


        // Draw the ghost as a bright dot.
        oled.drawCircle(ghostX, ghostY, 4);
        oled.drawDisc(ghostX, ghostY, 2);
    }

    sendFrame();
}

void display_show_game_over(int finalScore)
{
    char scoreText[24];
    snprintf(scoreText, sizeof(scoreText), "FINAL SCORE: %d", finalScore);

    oled.clearBuffer();

    oled.setFont(u8g2_font_7x14B_tf);
    drawCentered("GAME OVER", 20);

    oled.setFont(u8g2_font_6x10_tf);
    drawCentered(scoreText, 36);
    drawCentered("Press start to retry", 54);

    sendFrame();
}

void display_show_you_win(int finalScore)
{
    char scoreText[16];
    snprintf(scoreText, sizeof(scoreText), "%d", finalScore);

    oled.clearBuffer();

    oled.setFont(u8g2_font_7x14B_tf);
    drawCentered("YOU WIN!", 20);

    oled.setFont(u8g2_font_6x10_tf);
    drawCentered("FINAL SCORE", 36);

    oled.setFont(u8g2_font_10x20_tf);
    drawCentered(scoreText, 57);

    sendFrame();
}


void display_show_error(const char *title, const char *message)
{
    oled.clearBuffer();

    oled.setFont(u8g2_font_7x14B_tf);
    drawCentered(title, 24);

    oled.setFont(u8g2_font_6x10_tf);
    drawCentered(message, 45);

    sendFrame();
}