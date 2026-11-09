// game.cpp
#include "core/game.hpp"

#include <SFML/Window/Event.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <utility>

#include "core/asset_path.hpp"
#include "core/constants.hpp"
#include "states/game_state.hpp"

namespace
{
struct TextureDefinition
{
    const char* name;
    const char* file;
    sf::Color placeholderColor;
};

struct TileDefinition
{
    const char* textureName;
    TileType type;
};

constexpr TextureDefinition kTextures[] = {
    {"grass", "assets/pxArt.png", sf::Color(110, 190, 90, 255)},
    {"forest", "assets/pxArt (1).png", sf::Color(40, 110, 60, 255)},
    {"water", "assets/Block.png", sf::Color(70, 130, 220, 255)},
    {"residential", "assets/Block.png", sf::Color(220, 180, 90, 255)},
    {"commercial", "assets/Block.png", sf::Color(160, 90, 220, 255)},
    {"industrial", "assets/Block.png", sf::Color(130, 130, 130, 255)},
    {"road", "assets/Block.png", sf::Color(80, 80, 90, 255)},
    {"empty", "assets/Empty.png", sf::Color(0, 0, 0, 0)},
    {"background", "assets/background.jpeg", sf::Color(20, 26, 40, 255)},
    {"blue", "assets/BlueBackground.png", sf::Color(45, 70, 140, 255)},
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
} // namespace

Game::Game(TextureManager& textureManager)
    : window_(sf::VideoMode({config::windowWidth, config::windowHeight}), config::windowTitle,
              sf::Style::Titlebar | sf::Style::Close),
      input_(window_), textures_(textureManager)
{
    window_.setFramerateLimit(config::framerateLimit);

    loadTextures();
    loadTiles();
}

Game::~Game() = default;

void Game::loadTextures()
{
    for (const TextureDefinition& definition : kTextures)
    {
        const std::filesystem::path path = assets::resolve(definition.file);
        if (!textures_.loadTexture(definition.name, path))
        {
            std::cerr << "[assets] using a placeholder for '" << definition.name << "' (expected '"
                      << definition.file << "')\n";
            textures_.createPlaceholder(definition.name, definition.placeholderColor);
        }
    }
}

void Game::loadTiles()
{
    // try_emplace constructs the Tile in place; the atlas is only filled once,
    // right after the textures exist.
    for (const TileDefinition& definition : kTiles)
        tileAtlas_.try_emplace(definition.type, textures_.getRef(definition.textureName), definition.type);
}

void Game::pushState(std::unique_ptr<GameState> state)
{
    if (state)
        states_.push_back(std::move(state));
}

void Game::changeState(std::unique_ptr<GameState> state)
{
    pendingState_ = std::move(state);
    pendingPop_ = false;
}

void Game::popState()
{
    pendingState_.reset();
    pendingPop_ = true;
}

void Game::gameLoop()
{
    sf::Clock clock;

    while (window_.isOpen())
    {
        const float dt = clock.restart().asSeconds();
        input_.update();

        while (const std::optional event = window_.pollEvent())
        {
            input_.handleEvent(*event);

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
    }
}

GameState* Game::peekState()
{
    return states_.empty() ? nullptr : states_.back().get();
}

void Game::applyPendingTransition()
{
    if (pendingState_)
    {
        if (!states_.empty())
            states_.pop_back();

        states_.push_back(std::move(pendingState_));
    }
    else if (pendingPop_ && !states_.empty())
    {
        states_.pop_back();
    }

    pendingPop_ = false;
}

sf::RenderWindow& Game::window()
{
    return window_;
}

InputHandler& Game::input()
{
    return input_;
}

TextureManager& Game::textures()
{
    return textures_;
}

const TileAtlas& Game::tileAtlas() const
{
    return tileAtlas_;
}
