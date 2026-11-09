// animation_handler.cpp
#include "animation/animation_handler.hpp"

AnimationHandler::AnimationHandler(const sf::IntRect& frameSize) : frameSize_(frameSize)
{
    bounds_.size = frameSize.size;
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
    if (duration <= 0.f)
        return;

    // Compare the frame index before and after advancing the timer, so a large
    // dt skips frames instead of stalling on the first one.
    const auto previousFrame = static_cast<unsigned int>(elapsed_ / duration);
    elapsed_ += dt;
    const auto frameIndex = static_cast<unsigned int>(elapsed_ / duration);

    if (frameIndex != previousFrame)
    {
        const unsigned int frame = frameIndex % animation.getLength();
        bounds_.position = {frameSize_.position.x + frameSize_.size.x * static_cast<int>(frame),
                            frameSize_.position.y + frameSize_.size.y * currentAnim_};
    }

    // Restart the cycle after the last frame, snapping back to frame 0.
    if (elapsed_ >= duration * static_cast<float>(animation.getLength()))
    {
        elapsed_ = 0.f;
        bounds_.position = {frameSize_.position.x, frameSize_.position.y + frameSize_.size.y * currentAnim_};
    }
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
    bounds_.position = {frameSize_.position.x, frameSize_.position.y + frameSize_.size.y * newAnim};
    bounds_.size = frameSize_.size;
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
    bounds_.size = frameSize.size;
}

int AnimationHandler::getCurrentAnim() const
{
    return currentAnim_;
}
