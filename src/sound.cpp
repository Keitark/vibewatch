#include "sound.h"

#include <Arduino.h>
#include <M5Unified.h>

namespace sound {
namespace {

// A softened square keeps the telegraph attack crisp without driving the
// StickS3 amplifier at full-scale discontinuities.
constexpr std::uint8_t kCrispWave[] = {
    64, 32, 8, 0, 0, 0, 0, 8, 32, 64, 96, 120, 127, 127, 127, 120, 96, 64,
};

bool g_keyTonePlaying = false;

}  // namespace

void startKeyTone(std::uint8_t volume) {
    if (g_keyTonePlaying || volume == 0) {
        return;
    }
    M5.Speaker.setVolume(volume);
    M5.Speaker.tone(kKeyToneHz, 0x7FFFFFFFU, 0, true,
                    kCrispWave, sizeof(kCrispWave), false);
    g_keyTonePlaying = true;
}

void stopKeyTone() {
    if (!g_keyTonePlaying) {
        return;
    }
    M5.Speaker.stop(0);
    g_keyTonePlaying = false;
}

void playAck(float frequency, std::uint32_t durationMs, std::uint8_t volume) {
    if (volume == 0) {
        return;
    }
    M5.Speaker.setVolume(volume);
    M5.Speaker.tone(frequency, durationMs, 0, true,
                    kCrispWave, sizeof(kCrispWave), false);
}

}  // namespace sound
