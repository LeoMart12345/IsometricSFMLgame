#include <SFML/Graphics.hpp>
#include "button.hpp"
#include <iostream>

void Button::draw(sf::RenderWindow& window, sf::Texture& texture,
    sf::IntRect intRect, sf::Vector2f pos) {
    const auto [mouseX, mouseY] = sf::Mouse::getPosition(window);

    sf::Sprite buttonSprite(texture, intRect);
    buttonSprite.setPosition(pos);
    buttonSprite.setScale({ 4, 4 });
    
    const auto [spriteX, spriteY] = buttonSprite.getGlobalBounds().position;
    const auto [spriteWidth, spriteHeight] = buttonSprite.getGlobalBounds().size;

    bool inArea = isInArea(window, spriteX, spriteY, spriteWidth, spriteHeight);

	if (inArea) {
		std::cout << "Hovering Over button!" << std::endl;
	}

	else
    if (!inArea) {
        std::cout << "Not over button!" << std::endl;
        isClicked = false;
    }

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        if (inArea) {
            isClicked = true;
            std::cout << "clicked true" << std::endl;
        }
    }
    else if (isClicked) {
        onClick();
        std::cout << "onclick" << std::endl;

        isClicked = false;
    }
    else {
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
    bool x1 = mouseX > x;
    bool x2 = mouseX < x + width;
    bool y1 = mouseY > y;
    bool y2 = mouseY < y + height;

    return x1 && x2 && y1 && y2;
}