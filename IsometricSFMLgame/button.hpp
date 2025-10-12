#pragma once
#include <SFML/Graphics.hpp>
#include <functional>

class Button {
public:
    void draw(sf::RenderWindow& window, sf::Texture& texture, sf::IntRect intRect,
        sf::Vector2f pos);
    std::function<void()> onClick;

private:
    bool isInArea(sf::RenderWindow& window, int x, int y, int width, int height);
    bool isClicked = false;
};