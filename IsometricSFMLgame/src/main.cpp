// main.cpp
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>

#include "core/asset_path.hpp"
#include "core/game.hpp"
#include "core/texture_manager.hpp"
#include "states/game_state_start.hpp"

namespace
{
// SFML aborts the process (one line of text, then a core dump) when it cannot
// reach a display server, which makes a headless run look like a crash. Check
// first and report it like any other start-up failure. Only the Unix ports need
// this: Windows and macOS always provide a display.
bool hasDisplayServer()
{
#if defined(__linux__) || defined(__unix__)
    const char* const display = std::getenv("DISPLAY");
    const char* const wayland = std::getenv("WAYLAND_DISPLAY");

    return (display != nullptr && display[0] != '\0') || (wayland != nullptr && wayland[0] != '\0');
#else
    return true;
#endif
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        // Lets asset lookups work no matter which directory the executable is
        // started from (Visual Studio, double click, terminal, ...).
        assets::initialize(argc > 0 ? argv[0] : nullptr);

        if (!hasDisplayServer())
        {
            std::cerr << "Fatal error: no display server found (neither DISPLAY nor WAYLAND_DISPLAY is set)\n";
            return 1;
        }

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
