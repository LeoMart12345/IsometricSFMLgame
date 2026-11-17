// player.hpp
#pragma once

#include <SFML/Graphics.hpp>

// The player character, standing on the map with a looping idle animation.
//
// The sprite sheet is a horizontal strip of square frames (frame width equals
// frame height); every frame in the strip belongs to the idle animation. That is
// how Diablo II animation conversions are usually exported, so a cropped idle
// row can be dropped in as assets/player.png without further work.
//
// The sheet must outlive the Player: TextureManager owns the texture.
class Player
{
public:
    // `feetPosition` is the map position the character stands on (see
    // Map::gridToScreen()); the sprite is anchored at its bottom centre.
    Player(const sf::Texture& sheet, sf::Vector2f feetPosition);

    void setFeetPosition(sf::Vector2f position);

    // Advances the idle animation. Safe to call with any dt.
    void update(float dt);

    void draw(sf::RenderTarget& target) const;

private:
    void showFrame();

    sf::Sprite sprite_;
    int frameSize_ = 1;  // width and height of one square frame in pixels
    int frameCount_ = 1; // number of frames in the strip
    int frame_ = 0;
    float elapsed_ = 0.f;
};
