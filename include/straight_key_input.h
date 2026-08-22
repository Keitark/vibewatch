#pragma once

#include <cstdint>

namespace morse {

enum class KeyEdge {
    None,
    Pressed,
    Released,
};

// Debounces the external dry contact, then merges it with the already-debounced
// on-board Button A. The logical key remains down until every input is released.
class StraightKeyInput {
   public:
    explicit StraightKeyInput(std::uint32_t groveDebounceMs = 8);

    void reset(bool buttonPressed, bool groveRawPressed, std::uint32_t atMs);
    KeyEdge update(bool buttonPressed, bool groveRawPressed, std::uint32_t atMs);

    bool pressed() const { return mergedPressed_; }
    bool buttonPressed() const { return buttonPressed_; }
    bool grovePressed() const { return groveStablePressed_; }

   private:
    std::uint32_t debounceMs_;
    std::uint32_t groveCandidateSinceMs_ = 0;
    bool groveCandidatePressed_ = false;
    bool groveStablePressed_ = false;
    bool buttonPressed_ = false;
    bool mergedPressed_ = false;
};

}  // namespace morse
