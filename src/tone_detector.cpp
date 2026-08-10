#include "tone_detector.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace morse {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr std::array<float, 5> kFrequencies = {600.0f, 700.0f, 800.0f, 900.0f, 1000.0f};

}  // namespace

ToneDetector::ToneDetector(ToneDetectorConfig config) : config_(config) {
    reset();
}

void ToneDetector::reset() {
    calibrationSamples_ = 0;
    noisePower_ = 1.0f;
    level_ = 0.0f;
    threshold_ = 1.0f;
    aboveCount_ = 0;
    belowCount_ = 0;
    calibrated_ = false;
    tonePresent_ = false;
}

bool ToneDetector::process(const std::int16_t* samples, std::size_t count) {
    if (samples == nullptr || count == 0) {
        return tonePresent_;
    }

    double sumSquares = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        const double value = samples[i];
        sumSquares += value * value;
    }
    const float rms = static_cast<float>(std::sqrt(sumSquares / static_cast<double>(count)));

    float peakPower = 0.0f;
    float bandPower = 0.0f;
    for (const float frequency : kFrequencies) {
        const float power = goertzel(samples, count, frequency);
        peakPower = std::max(peakPower, power);
        bandPower += power;
    }
    level_ = peakPower;
    const float concentration = bandPower > 0.0f ? peakPower / bandPower : 0.0f;
    const float totalPower = rms * rms;
    const float bandFraction = totalPower > 0.0f ? bandPower / totalPower : 0.0f;

    if (!calibrated_) {
        noisePower_ = calibrationSamples_ == 0
                          ? std::max(1.0f, peakPower)
                          : noisePower_ * 0.92f + std::max(1.0f, peakPower) * 0.08f;
        calibrationSamples_ += static_cast<std::uint32_t>(count);
        const std::uint64_t required =
            static_cast<std::uint64_t>(config_.sampleRate) * config_.calibrationMs / 1000U;
        calibrated_ = calibrationSamples_ >= required;
        threshold_ = noisePower_ * config_.onNoiseMultiplier;
        return false;
    }

    const float multiplier = tonePresent_ ? config_.offNoiseMultiplier
                                          : config_.onNoiseMultiplier;
    threshold_ = std::max(1.0f, noisePower_ * multiplier);
    const bool candidate = rms >= config_.minimumRms && peakPower >= threshold_ &&
                           concentration >= config_.minimumConcentration &&
                           bandFraction >= config_.minimumBandFraction;

    if (candidate) {
        ++aboveCount_;
        belowCount_ = 0;
        if (!tonePresent_ && aboveCount_ >= 2) {
            tonePresent_ = true;
            aboveCount_ = 0;
        }
    } else {
        ++belowCount_;
        aboveCount_ = 0;
        if (tonePresent_ && belowCount_ >= 2) {
            tonePresent_ = false;
            belowCount_ = 0;
        }
        if (!tonePresent_ && rms < config_.minimumRms * 1.5f) {
            noisePower_ = noisePower_ * 0.995f + std::max(1.0f, peakPower) * 0.005f;
        }
    }
    return tonePresent_;
}

float ToneDetector::goertzel(const std::int16_t* samples, std::size_t count,
                             float frequency) const {
    const float omega = 2.0f * kPi * frequency / static_cast<float>(config_.sampleRate);
    const float coefficient = 2.0f * std::cos(omega);
    float previous = 0.0f;
    float previous2 = 0.0f;
    for (std::size_t i = 0; i < count; ++i) {
        const float current = static_cast<float>(samples[i]) + coefficient * previous - previous2;
        previous2 = previous;
        previous = current;
    }
    const float power = previous2 * previous2 + previous * previous -
                        coefficient * previous * previous2;
    const float normalization = static_cast<float>(count) * static_cast<float>(count);
    return std::max(0.0f, power / std::max(1.0f, normalization));
}

}  // namespace morse
