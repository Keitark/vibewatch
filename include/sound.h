#pragma once

#include <cstdint>

namespace sound {

constexpr float kKeyToneHz = 880.0f;

void startKeyTone(std::uint8_t volume = 115);
void stopKeyTone();
void playAck(float frequency, std::uint32_t durationMs, std::uint8_t volume = 96);

}  // namespace sound
