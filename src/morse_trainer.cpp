#include "morse_trainer.h"

#include <cstddef>

namespace morse {
namespace {

constexpr char kNormalPool[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
constexpr char kSimplifiedPool[] = "0123456789";

}  // namespace

void Trainer::begin(std::uint32_t seed, TrainingMode mode) {
    state_ = seed == 0 ? 1U : seed;
    mode_ = mode;
    target_ = '\0';
    next();
}

void Trainer::setMode(TrainingMode mode) {
    mode_ = mode;
    target_ = '\0';
    next();
}

void Trainer::next() {
    const char* pool =
        mode_ == TrainingMode::SimplifiedNumbers ? kSimplifiedPool : kNormalPool;
    const std::size_t poolSize =
        mode_ == TrainingMode::SimplifiedNumbers ? sizeof(kSimplifiedPool) - 1U
                                                 : sizeof(kNormalPool) - 1U;
    const char previous = target_;
    state_ = state_ * 1664525U + 1013904223U;
    std::size_t index = state_ % poolSize;
    if (pool[index] == previous) {
        index = (index + 1U) % poolSize;
    }
    target_ = pool[index];
    attempts_ = 0;
}

TrainingAnswer Trainer::submit(char answer) {
    if (answer == target_) {
        return TrainingAnswer::Correct;
    }
    ++attempts_;
    return attempts_ >= kMaximumAttempts ? TrainingAnswer::Reveal
                                         : TrainingAnswer::Retry;
}

}  // namespace morse
