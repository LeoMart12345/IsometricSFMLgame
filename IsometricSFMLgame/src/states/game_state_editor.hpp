// game_state_editor.hpp
#pragma once

#include <SFML/Graphics.hpp>

#include "map/map.hpp"
#include "map/tile_type.hpp"
#include "states/game_state.hpp"

// Map editor: left click places the selected tile, right click clears a cell,
// G/H move between z levels and 0-7 pick the tile type.
class GameStateEditor : public GameState
{
public:
    explicit GameStateEditor(Game& game);

    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void draw(float dt) override;

private:
    void handleKey(sf::Keyboard::Key key);
    void placeTileAtMouse(const sf::Event::MouseButtonPressed& mouse);
    void updateHover();
    void selectTile(TileType type);

    Map map_;
    sf::View gameView_;
    sf::View guiView_;
    TileType currentTileType_ = TileType::GRASS;
    int currentZLevel_ = 0;
};
