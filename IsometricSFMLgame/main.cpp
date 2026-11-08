#include "game.hpp"
#include "game_state_start.hpp"
#include "texture_manager.hpp"

int main()
{
    // Create the TextureManager. Game::loadTextures() populates it.
    TextureManager texmgr;

    // Pass texmgr to game
    Game game(texmgr);

    game.pushState(new GameStateStart(&game));
    game.gameLoop();

    return 0;
}
