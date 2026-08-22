#pragma once

#include <cstdint>

namespace morse {

enum class TrainingAnswer {
    Correct,
    Retry,
    Reveal,
};

enum class TrainingMode {
    Normal,
    SimplifiedNumbers,
};

class Trainer {
public:
    static constexpr std::uint8_t kMaximumAttempts = 3;

    void begin(std::uint32_t seed, TrainingMode mode = TrainingMode::Normal);
    void setMode(TrainingMode mode);
    void next();
    TrainingAnswer submit(char answer);

    char target() const { return target_; }
    std::uint8_t attempts() const { return attempts_; }
    TrainingMode mode() const { return mode_; }

private:
    std::uint32_t state_ = 1;
    char target_ = 'A';
    std::uint8_t attempts_ = 0;
    TrainingMode mode_ = TrainingMode::Normal;
};

}  // namespace morse
