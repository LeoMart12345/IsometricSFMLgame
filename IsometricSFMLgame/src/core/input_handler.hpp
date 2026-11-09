// input_handler.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <map>

// Polls keyboard/mouse state and remembers the previous frame so callers can
// ask for edge-triggered input ("just pressed", "released").
class InputHandler
{
public:
    explicit InputHandler(sf::RenderWindow& window);

    // Call once per frame, before polling events.
    void update();

    // Feed every polled event so drag start positions are pixel exact.
    void handleEvent(const sf::Event& event);

    [[nodiscard]] bool isMousePressed(sf::Mouse::Button button) const;
    [[nodiscard]] bool isMouseJustPressed(sf::Mouse::Button button) const;
    [[nodiscard]] bool isMouseReleased(sf::Mouse::Button button) const;

    // True while `button` is held and the cursor has moved at least
    // `minDistance` pixels (in screen space) since it was pressed.
    [[nodiscard]] bool isMouseDragging(sf::Mouse::Button button, float minDistance = 5.f) const;

    [[nodiscard]] sf::Vector2i getMouseScreenPos() const;

    // Mouse position converted into the world coordinates of `view`.
    [[nodiscard]] sf::Vector2f getMouseWorldPos(const sf::View& view) const;

    [[nodiscard]] bool isKeyPressed(sf::Keyboard::Key key) const;
    [[nodiscard]] bool isKeyJustPressed(sf::Keyboard::Key key) const;
    [[nodiscard]] bool isKeyReleased(sf::Keyboard::Key key) const;

private:
    // These helpers keep the enum -> array index conversions in one place.
    // Without the bounds checks, Key::Unknown (-1) would index out of bounds.
    [[nodiscard]] static bool isValidMouseButton(sf::Mouse::Button button);
    [[nodiscard]] static bool isValidKey(sf::Keyboard::Key key);

    sf::RenderWindow& window_;

    std::array<bool, sf::Mouse::ButtonCount> mouseStates_{};
    std::array<bool, sf::Mouse::ButtonCount> prevMouseStates_{};
    std::array<bool, sf::Keyboard::KeyCount> keyStates_{};
    std::array<bool, sf::Keyboard::KeyCount> prevKeyStates_{};

    // Screen position where each button was pressed (used by isMouseDragging).
    std::map<sf::Mouse::Button, sf::Vector2i> dragStartPositions_;
};
