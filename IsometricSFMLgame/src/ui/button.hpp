// button.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include <functional>

// Minimal image button.
//
// Hit testing happens in handleEvent() (never while drawing), so clicking the
// button fires the callback exactly once. Event positions are window pixels,
// which means the window has to use the identity/GUI view while the button is
// being used.
class Button
{
public:
    // `frame` is the part of `texture` shown by the button; `position` is the
    // top-left corner in window pixels.
    Button(const sf::Texture& texture, const sf::IntRect& frame, sf::Vector2f position, float scale = 1.f);

    void handleEvent(const sf::Event& event);
    void draw(sf::RenderTarget& target) const;

    void setOnClick(std::function<void()> callback);

    [[nodiscard]] bool isPressed() const;

private:
    sf::Sprite sprite_;
    std::function<void()> onClick_;
    bool pressed_ = false;
};
