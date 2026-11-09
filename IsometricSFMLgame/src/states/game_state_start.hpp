// game_state_start.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include "states/game_state.hpp"

// Title screen: press Enter/Space to open the editor, Escape to quit.
class GameStateStart : public GameState
{
public:
    explicit GameStateStart(Game& game);

    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void draw(float dt) override;

private:
    void startEditor();

    sf::View view_;
    sf::Sprite background_;
};
