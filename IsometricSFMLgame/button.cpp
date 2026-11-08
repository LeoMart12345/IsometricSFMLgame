#include <SFML/Graphics.hpp>
#include "button.hpp"

void Button::draw(sf::RenderWindow& window, sf::Texture& texture,
    sf::IntRect intRect, sf::Vector2f pos) {
    const auto [mouseX, mouseY] = sf::Mouse::getPosition(window);

    sf::Sprite buttonSprite(texture, intRect);
    buttonSprite.setPosition(pos);
    buttonSprite.setScale({ 4, 4 });
    
    const auto [spriteX, spriteY] = buttonSprite.getGlobalBounds().position;
    const auto [spriteWidth, spriteHeight] = buttonSprite.getGlobalBounds().size;

    const bool inArea = isInArea(window, spriteX, spriteY, spriteWidth, spriteHeight);

    if (!inArea) {
        isClicked = false;
    }

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        if (inArea) {
            isClicked = true;
        }
    }
    else if (isClicked) {
        if (onClick) onClick();
        isClicked = false;
    }

    if (isClicked) {
        buttonSprite.setTextureRect(sf::IntRect(
            { intRect.position.x + 96, intRect.position.y }, intRect.size));
    }

    window.draw(buttonSprite);
}

bool Button::isInArea(sf::RenderWindow& window, int x, int y, int width,
    int height) {
    const auto [mouseX, mouseY] = sf::Mouse::getPosition(window);
    return mouseX > x && mouseX < x + width &&
           mouseY > y && mouseY < y + height;
}
