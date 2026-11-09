// animation.hpp
#pragma once

#include <algorithm>

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

    // Number of frames in the range (always at least 1).
    [[nodiscard]] unsigned int getLength() const { return endFrame_ - startFrame_ + 1; }

private:
    unsigned int startFrame_;
    unsigned int endFrame_;
    float duration_;
};
