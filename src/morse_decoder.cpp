#include "morse_decoder.h"

#include <algorithm>
#include <array>

namespace morse {
namespace {

struct MorseEntry {
    const char* pattern;
    char character;
};

constexpr std::array<MorseEntry, 36> kAlphabet = {{
    {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'},
    {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'},
    {"-.-", 'K'}, {".-..", 'L'}, {"--", 'M'}, {"-.", 'N'}, {"---", 'O'},
    {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
    {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'}, {"-.--", 'Y'},
    {"--..", 'Z'}, {"-----", '0'}, {".----", '1'}, {"..---", '2'},
    {"...--", '3'}, {"....-", '4'}, {".....", '5'}, {"-....", '6'},
    {"--...", '7'}, {"---..", '8'}, {"----.", '9'},
}};

constexpr std::array<MorseEntry, 10> kCutNumbers = {{
    {"-", '0'}, {".-", '1'}, {"..-", '2'}, {"...-", '3'},
    {"....-", '4'}, {".", '5'}, {"-....", '6'}, {"-...", '7'},
    {"-..", '8'}, {"-.", '9'},
}};

template <typename T>
T clampValue(T value, T minimum, T maximum) {
    return std::max(minimum, std::min(value, maximum));
}

}  // namespace

Decoder::Decoder(DecoderConfig config) : config_(config), unitMs_(config.initialUnitMs) {
    config_.minimumUnitMs = std::max<std::uint32_t>(1, config_.minimumUnitMs);
    config_.maximumUnitMs = std::max(config_.minimumUnitMs, config_.maximumUnitMs);
    unitMs_ = clampValue(unitMs_, config_.minimumUnitMs, config_.maximumUnitMs);
    config_.adaptation = clampValue(config_.adaptation, 0.0f, 1.0f);
}

void Decoder::keyDown(std::uint32_t atMs) {
    if (keyIsDown_) {
        return;
    }
    keyIsDown_ = true;
    keyDownAtMs_ = atMs;
}

DecodeEvent Decoder::keyUp(std::uint32_t atMs) {
    if (!keyIsDown_) {
        return {};
    }
    keyIsDown_ = false;
    const std::uint32_t durationMs = atMs - keyDownAtMs_;
    lastReleaseAtMs_ = atMs;
    haveRelease_ = true;
    wordGapEmitted_ = false;

    if (durationMs < config_.minimumPulseMs) {
        return {};
    }
    if (durationMs > unitMs_ * config_.maximumPulseUnits) {
        const std::string rejected = pattern_;
        clear();
        return {EventType::Invalid, '\0', rejected, false};
    }

    const bool isDash = durationMs >= unitMs_ * 2U;
    pattern_ += isDash ? '-' : '.';
    adaptUnit(isDash ? std::max<std::uint32_t>(1, durationMs / 3U) : durationMs);

    if (pattern_.size() > 5) {
        const std::string rejected = pattern_;
        clear();
        return {EventType::Invalid, '\0', rejected, false};
    }
    return {};
}

DecodeEvent Decoder::update(std::uint32_t nowMs) {
    if (keyIsDown_ || !haveRelease_) {
        return {};
    }
    const std::uint32_t silenceMs = nowMs - lastReleaseAtMs_;
    if (!pattern_.empty() && silenceMs >= unitMs_ * 3U) {
        return commit(false);
    }
    if (pattern_.empty() && !wordGapEmitted_ && silenceMs >= unitMs_ * 7U) {
        wordGapEmitted_ = true;
        return {EventType::WordGap, '\0', {}, false};
    }
    return {};
}

DecodeEvent Decoder::forceCommit(std::uint32_t) {
    if (keyIsDown_ || pattern_.empty()) {
        return {};
    }
    return commit(true);
}

void Decoder::clear() {
    pattern_.clear();
    keyIsDown_ = false;
}

char Decoder::decodePattern(const std::string& pattern) {
    for (const auto& entry : kAlphabet) {
        if (pattern == entry.pattern) {
            return entry.character;
        }
    }
    return '\0';
}

char Decoder::decodeCutNumber(const std::string& pattern) {
    // Amateur-radio cut numbers shorten common numerals by reusing letter
    // patterns. Simplified-number mode prefers this compact interpretation.
    for (const auto& entry : kCutNumbers) {
        if (pattern == entry.pattern) {
            return entry.character;
        }
    }
    return '\0';
}

std::string Decoder::encodePattern(char character, DecodeMode mode) {
    if (mode == DecodeMode::SimplifiedNumbers) {
        for (const auto& entry : kCutNumbers) {
            if (character == entry.character) {
                return entry.pattern;
            }
        }
    }
    for (const auto& entry : kAlphabet) {
        if (character == entry.character) {
            return entry.pattern;
        }
    }
    return {};
}

char Decoder::decodeForMode(const std::string& pattern) const {
    const char alphabetCharacter = decodePattern(pattern);
    if (alphabetCharacter == '\0') {
        return '\0';
    }
    if (mode_ == DecodeMode::SimplifiedNumbers) {
        const char cutNumber = decodeCutNumber(pattern);
        if (cutNumber != '\0') {
            return cutNumber;
        }
    }
    return alphabetCharacter;
}

char Decoder::decodePending() const {
    return decodeForMode(pattern_);
}

std::string Decoder::previewPattern(std::uint32_t nowMs) const {
    std::string preview = pattern_;
    if (keyIsDown_ && nowMs - keyDownAtMs_ >= unitMs_ * 2U) {
        preview += '-';
    }
    return preview;
}

char Decoder::decodePreview(std::uint32_t nowMs) const {
    return decodeForMode(previewPattern(nowMs));
}

DecodeEvent Decoder::commit(bool forced) {
    const std::string completed = pattern_;
    pattern_.clear();
    const char decoded = decodeForMode(completed);
    if (decoded == '\0') {
        return {EventType::Invalid, '\0', completed, forced};
    }
    return {EventType::Character, decoded, completed, forced};
}

void Decoder::adaptUnit(std::uint32_t sampleMs) {
    sampleMs = clampValue(sampleMs, config_.minimumUnitMs, config_.maximumUnitMs);
    const float next = static_cast<float>(unitMs_) * (1.0f - config_.adaptation) +
                       static_cast<float>(sampleMs) * config_.adaptation;
    unitMs_ = clampValue(static_cast<std::uint32_t>(next + 0.5f),
                         config_.minimumUnitMs, config_.maximumUnitMs);
}

}  // namespace morse
