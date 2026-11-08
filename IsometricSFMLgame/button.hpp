#pragma once
#include <SFML/Graphics.hpp>
#include <functional>

class Button {
public:
    void draw(sf::RenderWindow& window, sf::Texture& texture, sf::IntRect intRect,
        sf::Vector2f pos);
    std::function<void()> onClick;

private:
    bool isInArea(sf::RenderWindow& window, float x, float y, float width, float height);
    bool isClicked = false;
};
