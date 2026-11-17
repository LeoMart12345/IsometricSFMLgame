// game.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <vector>

#include "core/texture_manager.hpp"
#include "map/tile_atlas.hpp"

class GameState;

// Owns the window, the resources and the state stack, and runs the main loop.
//
// Resources (textures, tile atlas) live here so that every state can use them
// without loading its own copies, and so that they outlive the states that
// reference them.
class Game
{
public:
    Game();
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Pushes a new state on top of the stack (the states underneath keep their
    // data but are not updated or drawn).
    void pushState(std::unique_ptr<GameState> state);

    // Replaces the topmost state. The swap happens at the end of the current
    // frame, so a state can safely request its own replacement from inside
    // handleInput()/update() without being destroyed while still executing.
    void changeState(std::unique_ptr<GameState> state);

    void gameLoop();

    // Resources owned by the game, borrowed by the states.
    [[nodiscard]] sf::RenderWindow& window() { return window_; }
    [[nodiscard]] TextureManager& textures() { return textures_; }
    [[nodiscard]] const TileAtlas& tileAtlas() const { return tileAtlas_; }

private:
    [[nodiscard]] GameState* peekState();
    void loadTextures();
    void loadTiles();
    void applyPendingTransition();

    sf::RenderWindow window_;
    TextureManager textures_;
    TileAtlas tileAtlas_;

    std::vector<std::unique_ptr<GameState>> states_;
    std::unique_ptr<GameState> pendingState_; // applied at the end of the frame
};

