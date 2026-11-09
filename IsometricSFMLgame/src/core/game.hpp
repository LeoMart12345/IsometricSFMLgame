// game.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <vector>

#include "core/input_handler.hpp"
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
    explicit Game(TextureManager& textureManager);
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Pushes a new state on top of the stack (the previous one keeps running
    // underneath until it is popped).
    void pushState(std::unique_ptr<GameState> state);

    // Replaces the topmost state. The swap happens at the end of the current
    // frame, so a state can safely request its own replacement from inside
    // handleInput()/update() without being destroyed while still executing.
    void changeState(std::unique_ptr<GameState> state);

    // Pops the topmost state (also deferred to the end of the frame).
    void popState();

    void gameLoop();

    [[nodiscard]] sf::RenderWindow& window();
    [[nodiscard]] InputHandler& input();
    [[nodiscard]] TextureManager& textures();
    [[nodiscard]] const TileAtlas& tileAtlas() const;

private:
    [[nodiscard]] GameState* peekState();
    void loadTextures();
    void loadTiles();
    void applyPendingTransition();

    sf::RenderWindow window_;
    InputHandler input_;
    TextureManager& textures_;
    TileAtlas tileAtlas_;

    std::vector<std::unique_ptr<GameState>> states_;
    std::unique_ptr<GameState> pendingState_;
    bool pendingPop_ = false;
};
