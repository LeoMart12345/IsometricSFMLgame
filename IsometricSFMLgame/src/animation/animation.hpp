// animation.hpp
#pragma once

#include <algorithm>
#include <cstdint>

// A contiguous frame range inside a sprite sheet.
class Animation
{
public:
    Animation(unsigned int startFrame, unsigned int endFrame, float duration)
        : startFrame_(startFrame), endFrame_(std::max(startFrame, endFrame)), duration_(duration)
    {
    }

    [[nodiscard]] unsigned int getStartFrame() const { return startFrame_; }
    [[nodiscard]] unsigned int getEndFrame() const { return endFrame_; }

    // Seconds each frame is shown. Non-positive values make update() a no-op.
    [[nodiscard]] float getDuration() const { return duration_; }

    // Number of frames in the range (always at least 1). The result is 64 bit:
    // a range covering [0, UINT_MAX] has 2^32 frames, which does not fit in an
    // unsigned int and used to wrap back to zero (causing a division by zero in
    // the animation cycle).
    [[nodiscard]] constexpr std::uint64_t getLength() const
    {
        return static_cast<std::uint64_t>(endFrame_) - static_cast<std::uint64_t>(startFrame_) + 1;
    }

private:
    unsigned int startFrame_;
    unsigned int endFrame_;
    float duration_;
};
