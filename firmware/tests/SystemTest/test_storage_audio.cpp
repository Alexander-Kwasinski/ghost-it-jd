#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputInternalDAC.h>

#include "../../inc/pins.h"
#include "test_storage_audio.h"

#if !defined(ESP32) || !defined(SOC_DAC_SUPPORTED) || (SOC_DAC_SUPPORTED == 0)
#error "This test expects a classic ESP32 target with its internal DAC (ESP32-WROOM-32)."
#endif

namespace {

// AudioOutputInternalDAC normally follows the MP3's stereo channel count,
// which activates GPIO25 AND GPIO26 on a classic ESP32. GPIO26 is OLED_DC in
// this project's pins.h. Force one output channel so only DAC1/GPIO25 is used.
class AudioOutputGPIO25Only : public AudioOutputInternalDAC {
public:
    AudioOutputGPIO25Only() : AudioOutputInternalDAC(512) {}

    bool SetChannels(int requestedChannels) override {
        (void)requestedChannels;
        return AudioOutputInternalDAC::SetChannels(1);
    }

    // Combine left and right into mono instead of silently discarding the
    // right channel from stereo MP3 files.
    bool ConsumeSample(int16_t samples[2]) override {
        const int32_t mixed = (static_cast<int32_t>(samples[0]) +
                               static_cast<int32_t>(samples[1])) / 2;
        int16_t monoSamples[2] = {
            static_cast<int16_t>(mixed),
            static_cast<int16_t>(mixed)
        };
        return AudioOutputInternalDAC::ConsumeSample(monoSamples);
    }
};

constexpr uint32_t SD_SPI_HZ = 4000000;
constexpr unsigned long SD_BACKGROUND_READ_INTERVAL_MS = 1000;

const char *TRACK_BASE_NAMES[] = {
    "PurgeIt",
    "ScanIt",
    "Theme",
    "TrapIt",
    "YouLose",
    "YouWin"
};

constexpr size_t TRACK_COUNT = sizeof(TRACK_BASE_NAMES) / sizeof(TRACK_BASE_NAMES[0]);

char trackPaths[TRACK_COUNT][48] = {};
bool trackExists[TRACK_COUNT] = {};

bool sdReady = false;
bool audioStartAttempted = false;
unsigned long totalSdBytesRead = 0;
unsigned long lastBackgroundReadMs = 0;

File backgroundReadFile;
uint8_t backgroundReadBuffer[512];

AudioOutputGPIO25Only *audioOutput = nullptr;
AudioFileSourceSD *audioSource = nullptr;
AudioGeneratorMP3 *mp3 = nullptr;

int nextTrackIndex = 0;
int activeTrackIndex = -1;

bool resolveTrackPath(const char *baseName, char *result, size_t resultSize)
{
    char candidate[48];
    const char *folders[] = {"/audio/", "/"};
    const char *extensions[] = {".mp3", ""};

    for (const char *folder : folders) {
        for (const char *extension : extensions) {
            snprintf(candidate, sizeof(candidate), "%s%s%s", folder, baseName, extension);
            if (SD.exists(candidate)) {
                snprintf(result, resultSize, "%s", candidate);
                return true;
            }
        }
    }

    return false;
}

void stopCurrentTrack()
{
    if (mp3 != nullptr) {
        if (mp3->isRunning()) {
            mp3->stop();
        }
        delete mp3;
        mp3 = nullptr;
    }

    if (audioSource != nullptr) {
        delete audioSource;
        audioSource = nullptr;
    }

    activeTrackIndex = -1;
}

bool startNextAvailableTrack()
{
    audioStartAttempted = true;
    stopCurrentTrack();

    for (size_t attempt = 0; attempt < TRACK_COUNT; ++attempt) {
        const int candidateIndex = nextTrackIndex;
        nextTrackIndex = (nextTrackIndex + 1) % TRACK_COUNT;

        if (!trackExists[candidateIndex]) {
            continue;
        }

        audioSource = new AudioFileSourceSD(trackPaths[candidateIndex]);
        if (audioSource == nullptr || !audioSource->isOpen()) {
            Serial.printf("AUDIO: could not open %s\n", trackPaths[candidateIndex]);
            if (audioSource != nullptr) {
                delete audioSource;
                audioSource = nullptr;
            }
            continue;
        }

        mp3 = new AudioGeneratorMP3();
        if (mp3 == nullptr || !mp3->begin(audioSource, audioOutput)) {
            Serial.printf("AUDIO: MP3 decoder could not start %s\n", trackPaths[candidateIndex]);
            if (mp3 != nullptr) {
                delete mp3;
                mp3 = nullptr;
            }
            delete audioSource;
            audioSource = nullptr;
            continue;
        }

        activeTrackIndex = candidateIndex;
        Serial.printf("AUDIO: now playing %s\n", trackPaths[candidateIndex]);
        return true;
    }

    Serial.println(F("AUDIO: no playable tracks found"));
    return false;
}

void doBackgroundSdRead()
{
    if (!backgroundReadFile) {
        return;
    }

    const unsigned long now = millis();
    if (now - lastBackgroundReadMs < SD_BACKGROUND_READ_INTERVAL_MS) {
        return;
    }
    lastBackgroundReadMs = now;

    const int bytesRead = backgroundReadFile.read(backgroundReadBuffer, sizeof(backgroundReadBuffer));
    if (bytesRead > 0) {
        totalSdBytesRead += static_cast<unsigned long>(bytesRead);
    } else {
        // Loop the same file so the SD interface continues to be exercised.
        backgroundReadFile.seek(0);
    }
}

} // namespace

bool testStorageAudioBegin()
{
    Serial.println(F("SD: initializing shared SPI microSD interface..."));
    sdReady = SD.begin(SD_CS, SPI, SD_SPI_HZ);

    if (!sdReady) {
        Serial.println(F("SD: initialization failed. Check card, format, power, and CS wiring."));
        return false;
    }

    Serial.println(F("SD: card initialized."));

    int firstAvailable = -1;
    for (size_t i = 0; i < TRACK_COUNT; ++i) {
        trackExists[i] = resolveTrackPath(TRACK_BASE_NAMES[i], trackPaths[i], sizeof(trackPaths[i]));

        if (trackExists[i]) {
            Serial.printf("SD AUDIO FILE: FOUND  %s\n", trackPaths[i]);
            if (firstAvailable < 0) {
                firstAvailable = static_cast<int>(i);
            }
        } else {
            Serial.printf("SD AUDIO FILE: MISSING /audio/%s.mp3 (or extensionless equivalent)\n",
                          TRACK_BASE_NAMES[i]);
        }
    }

    // Open a second handle to an audio file and read small blocks periodically.
    // This adds SD activity while the MP3 decoder is streaming another handle.
    if (firstAvailable >= 0) {
        backgroundReadFile = SD.open(trackPaths[firstAvailable], FILE_READ);
        if (backgroundReadFile) {
            Serial.printf("SD: background read test using %s\n", trackPaths[firstAvailable]);
        }
    }

    // Create the DAC object before OLED initialization. The OLED is initialized
    // afterward so its GPIO26 D/C pin is configured after the DAC driver setup.
    audioOutput = new AudioOutputGPIO25Only();
    if (audioOutput != nullptr) {
        audioOutput->SetChannels(1);
        audioOutput->SetGain(0.65f); // moderately loud for a useful system-load test
    }

    return sdReady;
}

void testStorageAudioUpdate()
{
    if (!sdReady) {
        return;
    }

    doBackgroundSdRead();

    if (audioOutput == nullptr) {
        return;
    }

    if (mp3 != nullptr && mp3->isRunning()) {
        if (!mp3->loop()) {
            Serial.println(F("AUDIO: track finished"));
            stopCurrentTrack();
            startNextAvailableTrack();
        }
        return;
    }

    if (!audioStartAttempted) {
        startNextAvailableTrack();
    }
}

bool testStorageReady()
{
    return sdReady;
}

bool testAudioPlaying()
{
    return (mp3 != nullptr) && mp3->isRunning();
}

const char *testAudioCurrentTrack()
{
    if (activeTrackIndex < 0) {
        return "none";
    }
    return trackPaths[activeTrackIndex];
}

unsigned long testStorageBytesRead()
{
    return totalSdBytesRead;
}
