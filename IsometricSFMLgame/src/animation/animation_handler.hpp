// animation_handler.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <vector>

#include "animation/animation.hpp"

// Steps through the frames of a sprite sheet.
//
// The owner stays in charge of the sprite: after update(), assign
// getBounds() to it with sf::Sprite::setTextureRect(). The handler only
// computes which part of the sheet should be visible.
class AnimationHandler
{
public:
    AnimationHandler() = default;

    // `frameSize` is the size of a single frame *and* the origin of the sheet
    // inside the texture (usually {0, 0, frameWidth, frameHeight}).
    explicit AnimationHandler(const sf::IntRect& frameSize);

    void addAnim(const Animation& animation);

    void update(float dt);

    // Switches to `animId` (index into the added animations). Out-of-range ids
    // and re-selecting the current animation are ignored.
    void changeAnim(std::size_t animId);

    [[nodiscard]] const sf::IntRect& getBounds() const;
    [[nodiscard]] const sf::IntRect& getFrameSize() const;
    void setFrameSize(const sf::IntRect& frameSize);

    // Index of the active animation, or -1 when none was selected yet.
    [[nodiscard]] int getCurrentAnim() const;

private:
    std::vector<Animation> animations_;
    sf::IntRect bounds_{};    // Visible part of the sheet
    sf::IntRect frameSize_{}; // Pixel size/origin of one frame
    float elapsed_ = 0.f;
    int currentAnim_ = -1;
};
