#include "game.hpp"
#include "game_state_start.hpp"
#include "texture_manager.hpp"

int main()
{
    // Create and load textures in the TextureManager
    TextureManager texmgr;
    texmgr.loadTexture("background", "C:/Users/leoma/Desktop/background.jpeg");

    // Pass texmgr to game
    Game game(texmgr);

    game.pushState(new GameStateStart(&game));
    game.gameLoop();

    return 0;
}
