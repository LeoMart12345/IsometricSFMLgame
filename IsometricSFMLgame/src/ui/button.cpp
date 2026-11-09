// button.cpp
#include "ui/button.hpp"

#include <utility>

Button::Button(const sf::Texture& texture, const sf::IntRect& frame, sf::Vector2f position, float scale)
    : sprite_(texture, frame)
{
    sprite_.setPosition(position);
    sprite_.setScale({scale, scale});
}

void Button::handleEvent(const sf::Event& event)
{
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button == sf::Mouse::Button::Left)
        {
            const auto position = static_cast<sf::Vector2f>(mousePressed->position);
            pressed_ = sprite_.getGlobalBounds().contains(position);
        }
    }
    else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (mouseReleased->button != sf::Mouse::Button::Left || !pressed_)
            return;

        pressed_ = false;

        // Only fire when the button was released on top of it, like a real
        // button (drag off the button to cancel).
        const auto position = static_cast<sf::Vector2f>(mouseReleased->position);
        if (sprite_.getGlobalBounds().contains(position) && onClick_)
            onClick_();
    }
}

void Button::draw(sf::RenderTarget& target) const
{
    target.draw(sprite_);
}

void Button::setOnClick(std::function<void()> callback)
{
    onClick_ = std::move(callback);
}

bool Button::isPressed() const
{
    return pressed_;
}
