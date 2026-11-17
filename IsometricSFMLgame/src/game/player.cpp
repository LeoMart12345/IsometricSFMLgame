// player.cpp
#include "game/player.hpp"

#include <algorithm>

#include "core/constants.hpp"

Player::Player(const sf::Texture& sheet, sf::Vector2f feetPosition) : sprite_(sheet)
{
    const sf::Vector2u sheetSize = sheet.getSize();

    // Frames are square, so the sheet height is the frame size and the sheet
    // width tells us how many frames the strip contains.
    frameSize_ = std::max(1, static_cast<int>(sheetSize.y));
    frameCount_ = std::max(1, static_cast<int>(sheetSize.x) / frameSize_);

    // Bottom centre origin: the feet stand exactly on the tile centre, and the
    // fixed on-screen height keeps any source resolution at the same scale.
    sprite_.setOrigin({static_cast<float>(frameSize_) / 2.f, static_cast<float>(frameSize_)});
    const float scale = config::playerScreenHeight / static_cast<float>(frameSize_);
    sprite_.setScale({scale, scale});
    sprite_.setPosition(feetPosition);

    applyFrame();
}

void Player::update(float dt)
{
    // A single-frame "animation" has nothing to advance, and a negative or NaN
    // dt (a paused clock) must not move the frame index backwards.
    if (frameCount_ < 2 || !(dt > 0.f))
        return;

    elapsed_ += dt;

    // Advance as many frames as the elapsed time allows, so a frame that
    // arrives late does not slow the animation down.
    const int frames = static_cast<int>(elapsed_ / config::playerIdleFrameSeconds);
    if (frames == 0)
        return;

    elapsed_ -= static_cast<float>(frames) * config::playerIdleFrameSeconds;
    frame_ = (frame_ + frames) % frameCount_;
    applyFrame();
}

void Player::draw(sf::RenderTarget& target) const
{
    target.draw(sprite_);
}

void Player::applyFrame()
{
    // The strip is one row: frame N starts at x = N * frameSize_.
    sprite_.setTextureRect(sf::IntRect({frame_ * frameSize_, 0}, {frameSize_, frameSize_}));
}
