// game.cpp
#include "core/game.hpp"

#include <SFML/Window/Event.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "core/asset_path.hpp"
#include "core/constants.hpp"
#include "states/game_state.hpp"

namespace
{
// How a texture is faked when its art file is missing.
enum class PlaceholderKind
{
    IsometricTile, // 2:1 diamond, so it lines up with the map grid
    PlayerStrip,   // horizontal strip of square idle frames
};

struct TextureDefinition
{
    const char* name;
    const char* file;
    sf::Color placeholderColor;
    PlaceholderKind placeholder;
};

struct TileDefinition
{
    const char* textureName;
    TileType type;
};

constexpr TextureDefinition kTextures[] = {
    {"grass", "assets/pxArt.png", sf::Color(110, 190, 90, 255), PlaceholderKind::IsometricTile},
    {"forest", "assets/pxArt (1).png", sf::Color(40, 110, 60, 255), PlaceholderKind::IsometricTile},
    {"water", "assets/Block.png", sf::Color(70, 130, 220, 255), PlaceholderKind::IsometricTile},
    {"residential", "assets/Block.png", sf::Color(220, 180, 90, 255), PlaceholderKind::IsometricTile},
    {"commercial", "assets/Block.png", sf::Color(160, 90, 220, 255), PlaceholderKind::IsometricTile},
    {"industrial", "assets/Block.png", sf::Color(130, 130, 130, 255), PlaceholderKind::IsometricTile},
    {"road", "assets/Block.png", sf::Color(80, 80, 90, 255), PlaceholderKind::IsometricTile},
    {"empty", "assets/Empty.png", sf::Color(0, 0, 0, 0), PlaceholderKind::IsometricTile},
    {"background", "assets/background.jpeg", sf::Color(20, 26, 40, 255), PlaceholderKind::IsometricTile},
    {"blue", "assets/BlueBackground.png", sf::Color(45, 70, 140, 255), PlaceholderKind::IsometricTile},
    {"player", "assets/player.png", sf::Color(230, 170, 70, 255), PlaceholderKind::PlayerStrip},
};

constexpr TileDefinition kTiles[] = {
    {"grass", TileType::GRASS},
    {"forest", TileType::FOREST},
    {"water", TileType::WATER},
    {"residential", TileType::RESIDENTIAL},
    {"commercial", TileType::COMMERCIAL},
    {"industrial", TileType::INDUSTRIAL},
    {"road", TileType::ROAD},
    {"empty", TileType::EMPTY},
};

// The window is authored at 1920x1080, which does not fit every desktop (a
// 1366x768 laptop, a scaled HiDPI screen). A window bigger than the screen ends
// up with its title bar or its edges off screen, so shrink the requested mode to
// the desktop instead. Views are sized from window.getSize() everywhere, so the
// rest of the game adapts without further changes.
sf::VideoMode fittedVideoMode()
{
    const sf::Vector2u desktop = sf::VideoMode::getDesktopMode().size;
    if (desktop.x == 0 || desktop.y == 0) // no display information
        return sf::VideoMode({config::windowWidth, config::windowHeight});

    return sf::VideoMode({std::min(config::windowWidth, desktop.x), std::min(config::windowHeight, desktop.y)});
}
} // namespace

Game::Game() : window_(fittedVideoMode(), config::windowTitle, sf::Style::Titlebar | sf::Style::Close)
{
    window_.setFramerateLimit(config::framerateLimit);

    loadTextures();
    loadTiles();
}

Game::~Game() = default;

void Game::loadTextures()
{
    // The repository ships without the art files, so a missing texture is an
    // expected state rather than a failure. Probe the filesystem first: letting
    // sf::Texture::loadFromFile() fail would print a three line error per file,
    // which turned a normal start-up into a wall of red text. Everything that is
    // missing gets a generated placeholder and is reported once, as a group.
    std::vector<std::string> missing;

    for (const auto& [name, file, color, kind] : kTextures)
    {
        const std::filesystem::path path = assets::resolve(file);

        std::error_code error;
        if (std::filesystem::is_regular_file(path, error) && textures_.loadTexture(name, path))
            continue;

        // loadTexture() reports a file that exists but cannot be decoded; a
        // missing file just gets a generated stand-in so the game stays playable.
        missing.emplace_back(std::string{"'"} + name + "' from '" + file + "'");

        if (kind == PlaceholderKind::PlayerStrip)
            textures_.createPlayerPlaceholder(name, color);
        else
            textures_.createPlaceholder(name, color);
    }

    if (missing.empty())
        return;

    std::cerr << "[assets] " << missing.size() << " of " << std::size(kTextures)
              << " textures have no art file; drawing generated placeholders for them:\n";
    for (const std::string& entry : missing)
        std::cerr << "           " << entry << '\n';
    std::cerr << "[assets] drop those files into assets/ and restart (see assets/README.md)\n";
}

void Game::loadTiles()
{
    // The atlas is filled once, right after the textures exist. try_emplace
    // constructs the prototype Tile in place.
    for (const auto& [textureName, type] : kTiles)
        tileAtlas_.try_emplace(type, textures_.getRef(textureName), type);
}

void Game::pushState(std::unique_ptr<GameState> state)
{
    if (state)
        states_.push_back(std::move(state));
}

void Game::changeState(std::unique_ptr<GameState> state)
{
    // Ignore a null request rather than tearing the state stack down for no
    // reason.
    if (!state)
        return;

    pendingState_ = std::move(state);
}

void Game::gameLoop()
{
    sf::Clock clock;

    while (window_.isOpen())
    {
        // Clamping keeps the simulation stable when the process is stalled for a
        // while (dragging the window, hitting a breakpoint, alt-tabbing).
        const float dt = std::min(clock.restart().asSeconds(), config::maxFrameTime);

        while (const std::optional event = window_.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window_.close();
                break;
            }

            if (GameState* state = peekState())
                state->handleInput(*event);
        }

        if (!window_.isOpen())
            break;

        if (GameState* state = peekState())
        {
            state->update(dt);

            window_.clear(sf::Color(30, 30, 40));
            state->draw(dt);
            window_.display();
        }

        // Deferred so a state is never destroyed while it is executing.
        applyPendingTransition();

        // No state left to draw: end the loop instead of spinning on an empty
        // stack.
        if (states_.empty() && !pendingState_)
        {
            window_.close();
            break;
        }
    }
}

GameState* Game::peekState()
{
    return states_.empty() ? nullptr : states_.back().get();
}

void Game::applyPendingTransition()
{
    if (!pendingState_)
        return;

    // changeState() replaces the current state rather than stacking on top of
    // it, which is all the title screen and the editor need.
    if (!states_.empty())
        states_.pop_back();

    states_.push_back(std::move(pendingState_));
}
