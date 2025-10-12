#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <array>

class InputHandler {
public:
    explicit InputHandler(sf::RenderWindow& window);

    void update();
    void handleEvent(const sf::Event& event);

    // Mouse interface (SFML 3 style)
    bool isMousePressed(sf::Mouse::Button button) const;
    bool isMouseJustPressed(sf::Mouse::Button button) const;
    bool isMouseReleased(sf::Mouse::Button button) const;
    bool isMouseDragging(sf::Mouse::Button button, float minDistance = 5.f) const;
    sf::Vector2f getMouseWorldPos(const sf::View& view) const;
    sf::Vector2i getMouseScreenPos() const;

    // Keyboard interface (SFML 3 style)
    bool isKeyPressed(sf::Keyboard::Key key) const;
    bool isKeyJustPressed(sf::Keyboard::Key key) const;
    bool isKeyReleased(sf::Keyboard::Key key) const;

private:
    sf::RenderWindow& window;
    std::array<bool, sf::Mouse::ButtonCount> prevMouseStates;
    std::unordered_map<sf::Mouse::Button, sf::Vector2i> dragStartPositions;
    std::array<bool, sf::Keyboard::KeyCount> prevKeyStates;
};