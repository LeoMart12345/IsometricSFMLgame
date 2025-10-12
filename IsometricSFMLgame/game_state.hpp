#pragma once

#include "game.hpp" // Include the full definition of Game

class GameState
{
public:
    Game* game;

    GameState(Game* game) : game(game) {} // Constructor definition

    virtual void draw(const float dt) = 0;
    virtual void update(const float dt) = 0;
    virtual void handleInput() = 0;
};
