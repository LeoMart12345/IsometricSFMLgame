// main.cpp
#include <exception>
#include <iostream>
#include <memory>

#include "core/asset_path.hpp"
#include "core/game.hpp"
#include "core/texture_manager.hpp"
#include "states/game_state_start.hpp"

int main(int argc, char** argv)
{
    try
    {
        // Lets asset lookups work no matter which directory the executable is
        // started from (Visual Studio, double click, terminal, ...).
        assets::initialize(argc > 0 ? argv[0] : nullptr);

        TextureManager textureManager;

        Game game(textureManager);
        game.pushState(std::make_unique<GameStateStart>(game));
        game.gameLoop();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
