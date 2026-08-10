#pragma once

#include <cstdint>
#include <string>

namespace morse {

enum class EventType {
    None,
    Character,
    Invalid,
    WordGap,
};

struct DecodeEvent {
    EventType type = EventType::None;
    char character = '\0';
    std::string pattern;
    bool forced = false;
};

struct DecoderConfig {
    std::uint32_t initialUnitMs = 100;
    std::uint32_t minimumUnitMs = 50;
    std::uint32_t maximumUnitMs = 300;
    std::uint32_t minimumPulseMs = 20;
    std::uint32_t maximumPulseUnits = 10;
    float adaptation = 0.20f;
};

class Decoder {
public:
    explicit Decoder(DecoderConfig config = {});

    void keyDown(std::uint32_t atMs);
    DecodeEvent keyUp(std::uint32_t atMs);
    DecodeEvent update(std::uint32_t nowMs);
    DecodeEvent forceCommit(std::uint32_t nowMs);
    void clear();

    bool isKeyDown() const { return keyIsDown_; }
    const std::string& pattern() const { return pattern_; }
    std::uint32_t unitMs() const { return unitMs_; }

    static char decodePattern(const std::string& pattern);
    static char decodeCutNumber(const std::string& pattern);

private:
    DecodeEvent commit(bool forced);
    void adaptUnit(std::uint32_t sampleMs);

    DecoderConfig config_;
    std::string pattern_;
    std::uint32_t unitMs_ = 100;
    std::uint32_t keyDownAtMs_ = 0;
    std::uint32_t lastReleaseAtMs_ = 0;
    bool keyIsDown_ = false;
    bool haveRelease_ = false;
    bool wordGapEmitted_ = true;
};

}  // namespace morse
