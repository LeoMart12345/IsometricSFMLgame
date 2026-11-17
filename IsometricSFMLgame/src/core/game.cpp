// game.cpp
#include "core/game.hpp"

#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>

#include "core/constants.hpp"

namespace
{
constexpr std::pair<sf::Keyboard::Key, TileType> kTileKeys[] = {
    {sf::Keyboard::Key::Num0, TileType::VOID},        {sf::Keyboard::Key::Num1, TileType::GRASS},
    {sf::Keyboard::Key::Num2, TileType::FOREST},      {sf::Keyboard::Key::Num3, TileType::WATER},
    {sf::Keyboard::Key::Num4, TileType::RESIDENTIAL}, {sf::Keyboard::Key::Num5, TileType::COMMERCIAL},
    {sf::Keyboard::Key::Num6, TileType::INDUSTRIAL},  {sf::Keyboard::Key::Num7, TileType::ROAD},
};

// The editor is authored at 1920x1080, which does not fit every desktop.
// Shrink the window (never enlarge it) so it always fits on screen.
sf::VideoMode fittedVideoMode()
{
    const sf::Vector2u desktop = sf::VideoMode::getDesktopMode().size;
    if (desktop.x == 0 || desktop.y == 0)
        return sf::VideoMode({config::windowWidth, config::windowHeight});

    return sf::VideoMode({std::min(config::windowWidth, desktop.x), std::min(config::windowHeight, desktop.y)});
}
} // namespace

Game::Game()
    : window_(fittedVideoMode(), config::windowTitle, sf::Style::Titlebar | sf::Style::Close),
      map_(static_cast<int>(config::mapWidth), static_cast<int>(config::mapHeight),
           static_cast<int>(config::mapLayers))
{
    window_.setFramerateLimit(config::framerateLimit);

    const sf::Vector2f size = static_cast<sf::Vector2f>(window_.getSize());
    view_ = sf::View(map_.getCenter(), size);
    view_.zoom(config::editorViewZoom);
}

void Game::run()
{
    while (window_.isOpen())
    {
        while (const std::optional event = window_.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window_.close();
                break;
            }

            handleEvent(*event);
        }

        if (!window_.isOpen())
            break;

        updateHover();

        window_.clear(sf::Color(30, 30, 40));
        window_.setView(view_);
        map_.draw(window_);
        map_.drawHover(window_);
        map_.drawOutline(window_, currentZLevel_);
        window_.display();
    }
}

void Game::handleEvent(const sf::Event& event)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
        handleKey(key->code);
    else if (const auto* mouse = event.getIf<sf::Event::MouseButtonPressed>())
        placeTileAtMouse(*mouse);
}

void Game::handleKey(sf::Keyboard::Key key)
{
    switch (key)
    {
    case sf::Keyboard::Key::Escape:
        window_.close();
        return;
    case sf::Keyboard::Key::H:
        currentZLevel_ = std::min(currentZLevel_ + 1, map_.getLayers() - 1);
        return;
    case sf::Keyboard::Key::G:
        currentZLevel_ = std::max(currentZLevel_ - 1, 0);
        return;
    default:
        break;
    }

    // 0 selects the eraser, 1-7 pick a tile through the lookup table above.
    const auto binding = std::ranges::find_if(kTileKeys, [key](const auto& entry) { return entry.first == key; });
    if (binding != std::end(kTileKeys))
        currentTileType_ = binding->second;
}

void Game::placeTileAtMouse(const sf::Event::MouseButtonPressed& mouse)
{
    if (mouse.button != sf::Mouse::Button::Left && mouse.button != sf::Mouse::Button::Right)
        return;

    // Right click is the eraser; placeTile() ignores clicks outside the map.
    const TileType type = (mouse.button == sf::Mouse::Button::Right) ? TileType::VOID : currentTileType_;
    const sf::Vector3i cell = gridCellAt(mouse.position);

    map_.placeTile(cell.x, cell.y, cell.z, type);
}

void Game::updateHover()
{
    map_.updateHover(gridCellAt(sf::Mouse::getPosition(window_)), currentTileType_);
}

sf::Vector3i Game::gridCellAt(sf::Vector2i pixelPosition) const
{
    // Convert with the world view, otherwise the click would be mapped with the
    // window's untransformed coordinates and land on the wrong cell.
    const sf::Vector2f worldPosition = window_.mapPixelToCoords(pixelPosition, view_);

    return map_.screenToGrid(worldPosition, currentZLevel_);
}
