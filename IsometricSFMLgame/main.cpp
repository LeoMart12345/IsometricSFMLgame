#include "game.hpp"
#include "game_state_start.hpp"
#include "texture_manager.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>

int main()
{
    // Create and load textures in the TextureManager
    TextureManager texmgr;
    texmgr.loadTexture("background", "C:/Users/leoma/Desktop/background.jpeg");

    //pass texmgr to game
    Game game(texmgr);

    game.pushState(new GameStateStart(&game));
    game.gameLoop();

    return 0;
}
