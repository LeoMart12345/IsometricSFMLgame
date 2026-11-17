// main.cpp
#include <cstdlib>
#include <exception>
#include <iostream>

#include "core/game.hpp"

namespace
{
// SFML cannot create a window without a display server. Checking first turns
// "Failed to open X11 display" into a clear message and a non-zero exit code.
bool hasDisplayServer()
{
#if defined(__linux__) || defined(__unix__)
    const char* const display = std::getenv("DISPLAY");
    const char* const wayland = std::getenv("WAYLAND_DISPLAY");

    return (display && *display) || (wayland && *wayland);
#else
    return true;
#endif
}
} // namespace

int main()
{
    try
    {
        if (!hasDisplayServer())
        {
            std::cerr << "Fatal error: no display server found (neither DISPLAY nor WAYLAND_DISPLAY is set)\n";
            return 1;
        }

        Game game;
        game.run();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
