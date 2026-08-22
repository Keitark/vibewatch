#include "straight_key_input.h"

namespace morse {

StraightKeyInput::StraightKeyInput(std::uint32_t groveDebounceMs)
    : debounceMs_(groveDebounceMs) {}

void StraightKeyInput::reset(bool buttonPressed, bool groveRawPressed,
                             std::uint32_t atMs) {
    buttonPressed_ = buttonPressed;
    groveCandidatePressed_ = groveRawPressed;
    groveStablePressed_ = groveRawPressed;
    groveCandidateSinceMs_ = atMs;
    mergedPressed_ = buttonPressed_ || groveStablePressed_;
}

KeyEdge StraightKeyInput::update(bool buttonPressed, bool groveRawPressed,
                                 std::uint32_t atMs) {
    buttonPressed_ = buttonPressed;

    if (groveRawPressed != groveCandidatePressed_) {
        groveCandidatePressed_ = groveRawPressed;
        groveCandidateSinceMs_ = atMs;
    } else if (groveStablePressed_ != groveCandidatePressed_ &&
               atMs - groveCandidateSinceMs_ >= debounceMs_) {
        groveStablePressed_ = groveCandidatePressed_;
    }

    const bool nextPressed = buttonPressed_ || groveStablePressed_;
    if (nextPressed == mergedPressed_) {
        return KeyEdge::None;
    }
    mergedPressed_ = nextPressed;
    return mergedPressed_ ? KeyEdge::Pressed : KeyEdge::Released;
}

}  // namespace morse
