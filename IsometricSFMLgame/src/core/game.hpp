// game.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include "map/map.hpp"
#include "map/tile_type.hpp"

// Owns the window and the map, and runs the tile editor.
class Game
{
public:
    Game();

    void run();

private:
    void handleEvent(const sf::Event& event);
    void handleKey(sf::Keyboard::Key key);
    void placeTileAtMouse(const sf::Event::MouseButtonPressed& mouse);
    void updateHover();

    // The cell under a pixel in the window, on the current z level.
    [[nodiscard]] sf::Vector3i gridCellAt(sf::Vector2i pixelPosition) const;

    sf::RenderWindow window_;
    Map map_;
    sf::View view_;
    TileType currentTileType_ = TileType::GRASS;
    int currentZLevel_ = 0;
};
