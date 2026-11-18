#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputInternalDAC.h"

#include "pins.h"
#include "spi_bus.h"
#include "audio_player.h"

// mp3->loop() does not return until the clip ends, so decoding runs on its
// own task. The public functions below take audioMutex to change the track.
static SemaphoreHandle_t audioMutex = nullptr;

// Makes the decoder give control back so the mutex can be taken quickly.
static volatile bool abortPlayback = false;

// The ESP32 has one DAC output connected to GPIO25
// Mix the left and right MP3 channels into mono
class GameAudioOutput : public AudioOutputInternalDAC {
private:
    float speedMultiplier = 1.0f;

public:
    void setSpeedMultiplier(float speed) {
        speedMultiplier = speed;

        if (speedMultiplier < 1.0f) {
            speedMultiplier = 1.0f;
        }

        if (speedMultiplier > 1.45f) {
            speedMultiplier = 1.45f;
        }
    }


    // Increase the sample rate to speed up playback
    bool SetRate(int hz) override {
        int adjustedRate = (int)(hz * speedMultiplier);

        return AudioOutputInternalDAC::SetRate(adjustedRate);
    }


    // Force mono output
    bool SetChannels(int channels) override {
        return AudioOutputInternalDAC::SetChannels(1);
    }

    // Mix left and right audio samples
    bool ConsumeSample(int16_t samples[2]) override {
        if (abortPlayback) {
            return false;
        }

        int32_t mixed =
            ((int32_t)samples[0] + (int32_t)samples[1]) / 2;

        int16_t monoSamples[2] = {
            (int16_t)mixed,
            (int16_t)mixed
        };

        return AudioOutputInternalDAC::ConsumeSample(monoSamples);
    }
};

// The OLED shares the SPI bus with the SD card, so SD reads take the bus lock.
class LockedSdSource : public AudioFileSourceSD {
public:
    bool open(const char *filename) override {
        SpiBusLock lock;
        return AudioFileSourceSD::open(filename);
    }

    uint32_t read(void *data, uint32_t length) override {
        SpiBusLock lock;
        return AudioFileSourceSD::read(data, length);
    }

    bool seek(int32_t position, int direction) override {
        SpiBusLock lock;
        return AudioFileSourceSD::seek(position, direction);
    }

    bool close() override {
        SpiBusLock lock;
        return AudioFileSourceSD::close();
    }
};


// Takes over the audio state until the end of the scope.
class AudioLock {
public:
    AudioLock() {
        abortPlayback = true;
        xSemaphoreTakeRecursive(audioMutex, portMAX_DELAY);
        abortPlayback = false;
    }

    ~AudioLock() {
        xSemaphoreGiveRecursive(audioMutex);
    }
};


enum AudioTrack {
    TRACK_NONE,
    TRACK_THEME,
    TRACK_COMMAND,
    TRACK_RESULT
};


static GameAudioOutput *audioOutput = nullptr;

static AudioFileSourceSD *audioFile = nullptr;
static AudioGeneratorMP3 *mp3 = nullptr;

static volatile AudioTrack currentTrack = TRACK_NONE;

static bool sdReady = false;

static bool themeEnabled = false;
static volatile bool commandFinished = false;

static uint32_t themeResumePosition = 0;

static int currentSpeedLevel = 0;

static volatile bool audioError = false;
static const char *audioErrorText = "AUDIO ERROR";

static void setAudioError(const char *message) {
    audioError = true;
    audioErrorText = message;

    themeEnabled = false;
    commandFinished = false;
}

bool audio_player_has_error() {
    return audioError;
}


const char *audio_player_error_message() {
    return audioErrorText;
}

bool audio_command_finished() {
    if (commandFinished) {
        commandFinished = false;
        return true;
    }

    return false;
}

static void stopCurrentTrack() {
    if (mp3 != nullptr) {

        mp3->stop();

        delete mp3;
        mp3 = nullptr;
    }

    if (audioFile != nullptr) {
        if (audioFile->isOpen()) {
            audioFile->close();
        }

        delete audioFile;
        audioFile = nullptr;
    }

    currentTrack = TRACK_NONE;
}

static bool startTrack(const char *filename, AudioTrack track, uint32_t resumePosition, float speedMultiplier) {
    if (!sdReady || audioOutput == nullptr || audioError) {
        return false;
    }

    // Open the MP3 file
    audioFile = new LockedSdSource();

    if (audioFile == nullptr) {
        setAudioError("AUDIO MEMORY FAILED");
        return false;
    }

    if (!audioFile->open(filename)) {

        delete audioFile;
        audioFile = nullptr;

        setAudioError("MP3 FILE OPEN FAILED");
        return false;
    }

    // Resume the theme near its previous position
    if (resumePosition > 0 &&
        resumePosition < audioFile->getSize()) {

        // If seeking fails, the file remains at its beginning
        audioFile->seek((int32_t)resumePosition, SEEK_SET);
    }

    // Create the MP3 decoder
    mp3 = new AudioGeneratorMP3();

    if (mp3 == nullptr) {

        audioFile->close();

        delete audioFile;
        audioFile = nullptr;

        setAudioError("AUDIO MEMORY FAILED");
        return false;
    }

    // Set playback speed before starting the decoder
    audioOutput->setSpeedMultiplier(speedMultiplier);

    // Start decoding
    if (!mp3->begin(audioFile, audioOutput)) {

        mp3->stop();

        delete mp3;
        mp3 = nullptr;

        if (audioFile->isOpen()) {
            audioFile->close();
        }

        delete audioFile;
        audioFile = nullptr;

        setAudioError("MP3 DECODER FAILED");
        return false;
    }

    currentTrack = track;

    return true;
}

static const char *missingAudioFile() {
    if (!SD.exists("/PurgeIt.mp3")) {
        return "MISSING PurgeIt.mp3";
    }
    if (!SD.exists("/ScanIt.mp3")) {
        return "MISSING ScanIt.mp3";
    }
    if (!SD.exists("/TrapIt.mp3")) {
        return "MISSING TrapIt.mp3";
    }
    if (!SD.exists("/YouLose.mp3")) {
        return "MISSING YouLose.mp3";
    }
    if (!SD.exists("/YouWin.mp3")) {
        return "MISSING YouWin.mp3";
    }
    if (!SD.exists("/Theme.mp3")) {
        return "MISSING Theme.mp3";
    }

    return nullptr;
}


static void audio_player_update();

static void audioTask(void *unused) {
    while (true) {
        xSemaphoreTakeRecursive(audioMutex, portMAX_DELAY);
        audio_player_update();
        xSemaphoreGiveRecursive(audioMutex);

        vTaskDelay(1);
    }
}


bool audio_player_init()
{
    spi_bus_init();

    audioError = false;
    audioErrorText = "AUDIO ERROR";

    themeEnabled = false;
    commandFinished = false;
    sdReady = false;

    // Configure card detection
    // pinMode(SD_CD, INPUT_PULLUP);
    // delay(20);

    // // Check whether a card is physically inserted
    // if (digitalRead(SD_CD) == HIGH) {
    //     setAudioError("NO SD CARD INSERTED");
    //     return false;
    // }

    // Initialize the SD card using the shared SPI bus
    if (!SD.begin(SD_CS, SPI, 10000000)) {
        setAudioError("SD CARD INIT FAILED");
        return false;
    }

    sdReady = true;

    // Verify all required audio files
    const char *missingFile = missingAudioFile();

    if (missingFile != nullptr) {
        setAudioError(missingFile);
        return false;
    }

    // Initialize audio output
    audioOutput = new GameAudioOutput();

    if (audioOutput == nullptr) {
        setAudioError("AUDIO MEMORY FAILED");
        return false;
    }

    audioMutex = xSemaphoreCreateRecursiveMutex();

    if (audioMutex == nullptr) {
        setAudioError("AUDIO MEMORY FAILED");
        return false;
    }

    xTaskCreatePinnedToCore(audioTask, "audio", 12288, nullptr, 2, nullptr, 0);

    return true;
}

// Save and resume theme position
static void saveThemePosition() {
    if (currentTrack != TRACK_THEME || audioFile == nullptr || !audioFile->isOpen()) {
        return;
    }

    uint32_t position = audioFile->getPos();

    // The MP3 decoder reads ahead while decoding
    // Rewind by approximately two input buffers
    uint32_t rewindBytes =
        AudioGeneratorMP3::preAllocBuffSize() * 2U;

    if (position > rewindBytes) {
        themeResumePosition = position - rewindBytes;
    }
    else {
        themeResumePosition = 0;
    }
}

static void resumeTheme() {
    if (!themeEnabled || audioError) {
        return;
    }

    if (!startTrack("/Theme.mp3", TRACK_THEME, themeResumePosition, 1.0f)) {
        themeEnabled = false;
    }
}

void audio_start_theme() {
    AudioLock lock;

    audio_stop_all();

    if (!sdReady || audioError) {
        return;
    }

    themeEnabled = true;
    themeResumePosition = 0;

    if (!startTrack("/Theme.mp3", TRACK_THEME, 0, 1.0f)) {
        themeEnabled = false;
    }
}

void audio_stop_all() {
    AudioLock lock;

    themeEnabled = false;
    commandFinished = false;
    themeResumePosition = 0;

    stopCurrentTrack();
}

void audio_set_speed_level(int level) {
    if (level < 0) {
        level = 0;
    }

    if (level > 9) {
        level = 9;
    }

    currentSpeedLevel = level;
}

void audio_play_command(Command command) {
    AudioLock lock;

    const char *filename = nullptr;

    commandFinished = false;

    switch (command) {
        case SCAN:
            filename = "/ScanIt.mp3";
            break;

        case TRAP:
            filename = "/TrapIt.mp3";
            break;

        case PURGE:
            filename = "/PurgeIt.mp3";
            break;
    }

    if (filename == nullptr) {
        setAudioError("INVALID COMMAND");
        return;
    }

    if (!sdReady || audioError) {
        return;
    }

    if (currentTrack == TRACK_THEME) {
        saveThemePosition();
    }

    stopCurrentTrack();

    // Add 5% playback speed for each difficulty level
    float commandSpeed = 1.0f + (0.05f * currentSpeedLevel);


    if (!startTrack(filename, TRACK_COMMAND, 0, commandSpeed)) {
        if (!audioError) {
            setAudioError("COMMAND AUDIO FAILED");
        }
    }
}

static void playResult(const char *filename)
{
    AudioLock lock;

    themeEnabled = false;
    commandFinished = false;

    stopCurrentTrack();

    startTrack(filename, TRACK_RESULT, 0, 1.0f);
}


void audio_play_win() {
    playResult("/YouWin.mp3");
}


void audio_play_lose() {
    playResult("/YouLose.mp3");
}

// Runs on the audio task with audioMutex held.
static void audio_player_update() {
    if (mp3 == nullptr) {
        return;
    }

    // Keep decoding and sending audio samples
    if (mp3->loop()) {
        return;
    }

    // The current track has finished
    AudioTrack finishedTrack = currentTrack;

    stopCurrentTrack();

    // Loop the theme when its 9-second file finishes
    if (finishedTrack == TRACK_THEME) {
        themeResumePosition = 0;

        if (themeEnabled) {
            startTrack("/Theme.mp3", TRACK_THEME, 0, 1.0f);
        }
    }

    else if (finishedTrack == TRACK_COMMAND) {
        commandFinished = true;

        if (themeEnabled) {
            resumeTheme();
        }
    }
}


// True while a command or result clip is playing
bool audio_effect_is_playing() {
    return (currentTrack == TRACK_COMMAND || currentTrack == TRACK_RESULT);
}