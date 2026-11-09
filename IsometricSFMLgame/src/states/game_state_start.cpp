// game_state_start.cpp
#include "states/game_state_start.hpp"

#include <memory>

#include "core/game.hpp"
#include "states/game_state_editor.hpp"

GameStateStart::GameStateStart(Game& game) : GameState(game), background_(game.textures().getRef("background"))
{
    const sf::Vector2u windowSize = game.window().getSize();
    const sf::Vector2f size{static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)};

    view_.setSize(size);
    view_.setCenter(size / 2.f);

    // Stretch the background over the whole window. Using the texture size
    // instead of a hardcoded value keeps this correct for any source image.
    const sf::Vector2u textureSize = background_.getTexture().getSize();
    if (textureSize.x > 0 && textureSize.y > 0)
    {
        background_.setScale(
            {size.x / static_cast<float>(textureSize.x), size.y / static_cast<float>(textureSize.y)});
        background_.setPosition({0.f, 0.f});
    }
}

void GameStateStart::handleInput(const sf::Event& event)
{
    const auto* key = event.getIf<sf::Event::KeyPressed>();
    if (key == nullptr)
        return;

    if (key->code == sf::Keyboard::Key::Escape)
        game_.window().close();
    else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
        startEditor();
}

void GameStateStart::update(float /*dt*/)
{
    // Nothing to animate on the title screen yet.
}

void GameStateStart::draw(float /*dt*/)
{
    game_.window().setView(view_);
    game_.window().draw(background_);
}

void GameStateStart::startEditor()
{
    // changeState() is deferred by Game, so `this` stays valid until the end
    // of the frame even though the state is replaced.
    game_.changeState(std::make_unique<GameStateEditor>(game_));
}
