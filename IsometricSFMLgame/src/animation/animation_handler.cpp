// animation_handler.cpp
#include "animation/animation_handler.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace
{
int clampToInt(std::int64_t value)
{
    return static_cast<int>(
        std::clamp<std::int64_t>(value, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
}
} // namespace

AnimationHandler::AnimationHandler(const sf::IntRect& frameSize) : frameSize_(frameSize)
{
    syncBounds();
}

void AnimationHandler::addAnim(const Animation& animation)
{
    animations_.push_back(animation);
}

void AnimationHandler::update(float dt)
{
    if (currentAnim_ < 0 || static_cast<std::size_t>(currentAnim_) >= animations_.size())
        return;

    const Animation& animation = animations_[static_cast<std::size_t>(currentAnim_)];
    const float duration = animation.getDuration();

    // Non-finite values would leave elapsed_ NaN and make the float -> unsigned
    // conversion below undefined.
    if (!(duration > 0.f) || !std::isfinite(duration) || !(dt > 0.f) || !std::isfinite(dt))
        return;

    const float cycle = duration * static_cast<float>(animation.getLength());
    if (!(cycle > 0.f) || !std::isfinite(cycle))
        return;

    elapsed_ += dt;

    // Wrap instead of restarting at zero: if a frame took 1.5 cycles, the
    // animation is half way through, not back at its first frame.
    if (!std::isfinite(elapsed_))
    {
        elapsed_ = 0.f;
    }
    else
    {
        elapsed_ = std::fmod(elapsed_, cycle);
        if (!(elapsed_ >= 0.f))
            elapsed_ = 0.f;
    }

    syncBounds();
}

void AnimationHandler::changeAnim(std::size_t animId)
{
    if (animId >= animations_.size())
        return;

    const int newAnim = static_cast<int>(animId);
    if (newAnim == currentAnim_)
        return;

    currentAnim_ = newAnim;
    elapsed_ = 0.f;
    syncBounds();
}

const sf::IntRect& AnimationHandler::getBounds() const
{
    return bounds_;
}

const sf::IntRect& AnimationHandler::getFrameSize() const
{
    return frameSize_;
}

void AnimationHandler::setFrameSize(const sf::IntRect& frameSize)
{
    frameSize_ = frameSize;
    syncBounds();
}

int AnimationHandler::getCurrentAnim() const
{
    return currentAnim_;
}

void AnimationHandler::syncBounds()
{
    bounds_.size = frameSize_.size;

    if (currentAnim_ < 0 || static_cast<std::size_t>(currentAnim_) >= animations_.size())
    {
        bounds_.position = frameSize_.position;
        return;
    }

    const Animation& animation = animations_[static_cast<std::size_t>(currentAnim_)];
    const float duration = animation.getDuration();

    std::uint64_t frame = 0;
    if (duration > 0.f && std::isfinite(duration) && elapsed_ > 0.f && std::isfinite(elapsed_))
    {
        frame = static_cast<std::uint64_t>(elapsed_ / duration);
        frame = std::min(frame, animation.getLength() - 1);
    }

    // The animation's start frame is the first column of its range; ignoring it
    // used to play every animation from column zero.
    const std::uint64_t absoluteFrame = static_cast<std::uint64_t>(animation.getStartFrame()) + frame;

    // Visible rectangles use int coordinates. Saturating before the int64
    // multiplication also keeps an intentionally absurd frame range safe.
    const auto visibleFrame = std::min<std::uint64_t>(absoluteFrame, std::numeric_limits<int>::max());
    const std::int64_t x = static_cast<std::int64_t>(frameSize_.position.x) +
                           static_cast<std::int64_t>(frameSize_.size.x) *
                               static_cast<std::int64_t>(visibleFrame);
    const std::int64_t y = static_cast<std::int64_t>(frameSize_.position.y) +
                           static_cast<std::int64_t>(frameSize_.size.y) * currentAnim_;

    bounds_.position = {clampToInt(x), clampToInt(y)};
}
