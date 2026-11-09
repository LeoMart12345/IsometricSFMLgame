// input_handler.cpp
#include "core/input_handler.hpp"

#include <cstdlib>

InputHandler::InputHandler(sf::RenderWindow& window) : window_(window)
{
}

void InputHandler::update()
{
    for (unsigned int i = 0; i < sf::Mouse::ButtonCount; ++i)
    {
        const auto button = static_cast<sf::Mouse::Button>(i);
        const bool pressed = sf::Mouse::isButtonPressed(button);

        // Only record a start position for a fresh press, and never overwrite
        // the pixel exact position captured by handleEvent().
        if (pressed && !mouseStates_[i] && dragStartPositions_.find(button) == dragStartPositions_.end())
            dragStartPositions_[button] = getMouseScreenPos();

        if (!pressed)
            dragStartPositions_.erase(button);

        prevMouseStates_[i] = mouseStates_[i];
        mouseStates_[i] = pressed;
    }

    for (unsigned int i = 0; i < sf::Keyboard::KeyCount; ++i)
    {
        prevKeyStates_[i] = keyStates_[i];
        keyStates_[i] = sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(i));
    }
}

void InputHandler::handleEvent(const sf::Event& event)
{
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        dragStartPositions_[mousePressed->button] = mousePressed->position;
    }
    else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        dragStartPositions_.erase(mouseReleased->button);
    }
}

bool InputHandler::isMousePressed(sf::Mouse::Button button) const
{
    return isValidMouseButton(button) && mouseStates_[static_cast<unsigned int>(button)];
}

bool InputHandler::isMouseJustPressed(sf::Mouse::Button button) const
{
    if (!isValidMouseButton(button))
        return false;

    const unsigned int index = static_cast<unsigned int>(button);
    return mouseStates_[index] && !prevMouseStates_[index];
}

bool InputHandler::isMouseReleased(sf::Mouse::Button button) const
{
    if (!isValidMouseButton(button))
        return false;

    const unsigned int index = static_cast<unsigned int>(button);
    return !mouseStates_[index] && prevMouseStates_[index];
}

bool InputHandler::isMouseDragging(sf::Mouse::Button button, float minDistance) const
{
    if (!isMousePressed(button))
        return false;

    const auto start = dragStartPositions_.find(button);
    if (start == dragStartPositions_.end())
        return false;

    const sf::Vector2i delta = getMouseScreenPos() - start->second;
    return static_cast<float>(std::abs(delta.x)) >= minDistance ||
           static_cast<float>(std::abs(delta.y)) >= minDistance;
}

sf::Vector2i InputHandler::getMouseScreenPos() const
{
    return sf::Mouse::getPosition(window_);
}

sf::Vector2f InputHandler::getMouseWorldPos(const sf::View& view) const
{
    return window_.mapPixelToCoords(getMouseScreenPos(), view);
}

bool InputHandler::isKeyPressed(sf::Keyboard::Key key) const
{
    return isValidKey(key) && keyStates_[static_cast<unsigned int>(key)];
}

bool InputHandler::isKeyJustPressed(sf::Keyboard::Key key) const
{
    if (!isValidKey(key))
        return false;

    const unsigned int index = static_cast<unsigned int>(key);
    return keyStates_[index] && !prevKeyStates_[index];
}

bool InputHandler::isKeyReleased(sf::Keyboard::Key key) const
{
    if (!isValidKey(key))
        return false;

    const unsigned int index = static_cast<unsigned int>(key);
    return !keyStates_[index] && prevKeyStates_[index];
}

bool InputHandler::isValidMouseButton(sf::Mouse::Button button)
{
    return static_cast<unsigned int>(button) < sf::Mouse::ButtonCount;
}

bool InputHandler::isValidKey(sf::Keyboard::Key key)
{
    // Key::Unknown is -1 and must be rejected before the unsigned conversion.
    return key != sf::Keyboard::Key::Unknown && static_cast<unsigned int>(key) < sf::Keyboard::KeyCount;
}
