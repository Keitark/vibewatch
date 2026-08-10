#pragma once

#include <cstddef>
#include <cstdint>

namespace morse {

struct ToneDetectorConfig {
    std::uint32_t sampleRate = 16000;
    std::uint32_t calibrationMs = 1000;
    float minimumRms = 80.0f;
    float onNoiseMultiplier = 6.0f;
    float offNoiseMultiplier = 3.0f;
    float minimumConcentration = 0.35f;
    float minimumBandFraction = 0.04f;
};

class ToneDetector {
public:
    explicit ToneDetector(ToneDetectorConfig config = {});

    void reset();
    bool process(const std::int16_t* samples, std::size_t count);

    bool tonePresent() const { return tonePresent_; }
    bool calibrated() const { return calibrated_; }
    float level() const { return level_; }
    float threshold() const { return threshold_; }

private:
    float goertzel(const std::int16_t* samples, std::size_t count, float frequency) const;

    ToneDetectorConfig config_;
    std::uint32_t calibrationSamples_ = 0;
    float noisePower_ = 1.0f;
    float level_ = 0.0f;
    float threshold_ = 1.0f;
    int aboveCount_ = 0;
    int belowCount_ = 0;
    bool calibrated_ = false;
    bool tonePresent_ = false;
};

}  // namespace morse
