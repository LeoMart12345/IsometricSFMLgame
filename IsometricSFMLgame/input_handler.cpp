#include "Input_Handler.hpp"

InputHandler::InputHandler(sf::RenderWindow& window)
    : window(window),
    prevMouseStates{},
    prevKeyStates{} {
}

void InputHandler::update() {
    // Update mouse button states (SFML 3 enum style)
    for (size_t i = 0; i < sf::Mouse::ButtonCount; ++i) {
        auto button = static_cast<sf::Mouse::Button>(i);
        prevMouseStates[i] = sf::Mouse::isButtonPressed(button);
    }

    // Update key states (SFML 3 enum style)
    for (size_t i = 0; i < sf::Keyboard::KeyCount; ++i) {
        auto key = static_cast<sf::Keyboard::Key>(i);
        prevKeyStates[i] = sf::Keyboard::isKeyPressed(key);
    }
}

void InputHandler::handleEvent(const sf::Event& event) {
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        dragStartPositions[mousePressed->button] = mousePressed->position;
    }
}

// Mouse functions (SFML 3)
bool InputHandler::isMousePressed(sf::Mouse::Button button) const {
    return sf::Mouse::isButtonPressed(button);
}

bool InputHandler::isMouseJustPressed(sf::Mouse::Button button) const {
    return !prevMouseStates[static_cast<size_t>(button)] && isMousePressed(button);
}

bool InputHandler::isMouseReleased(sf::Mouse::Button button) const {
    return prevMouseStates[static_cast<size_t>(button)] && !isMousePressed(button);
}
//error! maybe!
bool InputHandler::isMouseDragging(sf::Mouse::Button button, float minDistance) const {
    if (!isMousePressed(button)) return false;

    auto it = dragStartPositions.find(button);
    if (it == dragStartPositions.end()) return false;

    auto currentPos = getMouseScreenPos();
    float dx = static_cast<float>(currentPos.x) - static_cast<float>(it->second.x);
    float dy = static_cast<float>(currentPos.y) - static_cast<float>(it->second.y);
    return (dx * dx + dy * dy) >= (minDistance * minDistance);
}

sf::Vector2f InputHandler::getMouseWorldPos(const sf::View& view) const {
    return window.mapPixelToCoords(getMouseScreenPos(), view);
}

sf::Vector2i InputHandler::getMouseScreenPos() const {
    return sf::Mouse::getPosition(window);
}

// Keyboard functions (SFML 3)
bool InputHandler::isKeyPressed(sf::Keyboard::Key key) const {
    return sf::Keyboard::isKeyPressed(key);
}

bool InputHandler::isKeyJustPressed(sf::Keyboard::Key key) const {
    return !prevKeyStates[static_cast<size_t>(key)] && isKeyPressed(key);
}

bool InputHandler::isKeyReleased(sf::Keyboard::Key key) const {
    return prevKeyStates[static_cast<size_t>(key)] && !isKeyPressed(key);
}
