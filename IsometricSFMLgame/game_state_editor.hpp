#pragma once
#include <SFML/Graphics.hpp>
#include "game_state.hpp"
#include "map.hpp"
#include "button.hpp"

class GameStateEditor : public GameState
{
private:
    
    Map map;                                    // Map instance
    sf::View gameView, guiView;
    TileType currentTileType = TileType::GRASS; // Default brushmap 
    int currentZLevel = 0;                      // Track current Z-level
	const int MAX_Z_LEVEL = 2;				  // Maximum Z-level

public:
    // Constructor declaration
    GameStateEditor(Game* game);

    // Override pure virtual functions from GameState
    void draw(const float dt) override;
    void update(const float dt) override;
    void handleInput() override;
};