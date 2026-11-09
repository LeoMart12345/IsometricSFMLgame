// game_state.hpp
#pragma once

#include <SFML/Window/Event.hpp>

class Game;

// Base class for every screen of the game (menu, editor, ...).
//
// States never poll events themselves: Game forwards the events it polls, which
// makes sure every state sees exactly the same events in the same order.
class GameState
{
public:
    explicit GameState(Game& game) : game_(game) {}
    virtual ~GameState() = default;

    GameState(const GameState&) = delete;
    GameState& operator=(const GameState&) = delete;

    virtual void handleInput(const sf::Event& event) = 0;
    virtual void update(float dt) = 0;
    virtual void draw(float dt) = 0;

protected:
    Game& game_;
};
