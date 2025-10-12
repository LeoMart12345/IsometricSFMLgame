// game_state_editor.cpp
#include "game_state_editor.hpp"
#include "game.hpp"
#include <iostream>
#include <SFML/Graphics.hpp>
#include "map.hpp"

GameStateEditor::GameStateEditor(Game* game) :
    GameState(game),
    map(10, 10, 3, game->texmgr), // Small 10x10 test map
    currentTileType(TileType::GRASS),
    currentZLevel(0)
{
    // 1. Keep the window size detection
    sf::Vector2f pos = static_cast<sf::Vector2f>(this->game->window.getSize());

    // 2. Set up GUI view (unchanged)
    this->guiView.setSize(pos);
    guiView.setCenter(pos * 0.5f);

    // 3. Modified game view setup:
    this->gameView.setSize(pos);

    // Calculate center position for a 10x10 isometric map
    // For 64x32 tiles:
    float mapCenterX = (10 - 10) * 64 / 2;  // = 0 (from getScreenPos math)
    float mapCenterY = (10 + 10) * 32 / 4;  // = 160 (center of diamond shape)

    // Center view on map's natural center point
    gameView.setCenter({mapCenterX, mapCenterY });

    // Optional: Zoom out a bit to see more of the map
    gameView.zoom(0.8f);
}

void GameStateEditor::draw(const float dt) {
    if (!this->game) return;

    this->game->window.clear(sf::Color::Black);
    this->game->window.setView(gameView);
    //dont want a background for now
    //this->game->window.draw(this->game->background);

    // TEST: Stack tiles at the SAME X,Y coordinates
    const int testX = 30;
    const int testY = 10;

    // Draw all tiles
    map.draw(this->game->window);
    //draw the hovering tile
    map.drawHover(this->game->window); // Add this after regular draw
    //draw the outline of the buildable area
    map.drawOutline(this->game->window);

    this->game->window.setView(guiView);
}
void GameStateEditor::update(const float dt) {
    // Update logic can be added here
}

void GameStateEditor::handleInput() {
    if (!this->game) return;


    sf::Vector2f mousePos = this->game->window.mapPixelToCoords(
        sf::Mouse::getPosition(this->game->window),
        gameView );

    sf::Vector3i gridPos = map.screenToGrid(mousePos.x, mousePos.y, currentZLevel);
    map.updateHover(gridPos, currentTileType);


    while (std::optional<sf::Event> event = this->game->window.pollEvent()) {
        if (!event) continue;

        if (event->is<sf::Event::Closed>()) {
            this->game->window.close();
        }
        else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f worldPos = this->game->window.mapPixelToCoords(
                mousePressed->position,
                gameView
            );

            sf::Vector3i gridPos = map.screenToGrid(worldPos.x, worldPos.y, currentZLevel);

            if (mousePressed->button == sf::Mouse::Button::Left) {
                // Use tile atlas for tile placement
                std::string tileName;
                switch (currentTileType) {
                case TileType::GRASS: tileName = "grass"; break;
                case TileType::FOREST: tileName = "forest"; break;
                case TileType::WATER: tileName = "water"; break;
                case TileType::RESIDENTIAL: tileName = "residential"; break;
                case TileType::COMMERCIAL: tileName = "commercial"; break;
                case TileType::INDUSTRIAL: tileName = "industrial"; break;
                case TileType::ROAD: tileName = "road"; break;
                default: tileName = "grass"; break;
                }

                if (game->tileAtlas.find(tileName) != game->tileAtlas.end()) {
                    map.placeTile(gridPos.x, gridPos.y, gridPos.z, currentTileType);
                }
            }
            else if (mousePressed->button == sf::Mouse::Button::Right) {
                map.placeTile(gridPos.x, gridPos.y, gridPos.z, TileType::VOID);
            }
        }
        else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            switch (keyPressed->scancode) {
            case sf::Keyboard::Scancode::H:
                if (currentZLevel < MAX_Z_LEVEL) {
                    currentZLevel++;
                    std::cout << "Z-Level increased to: " << currentZLevel
                        << " (MAX: " << MAX_Z_LEVEL << ")" << std::endl;
                }
                else {
                    std::cout << "Warning: Already at maximum Z-Level: " << MAX_Z_LEVEL << std::endl;
                }
                break;
            case sf::Keyboard::Scancode::G:
                if (currentZLevel > 0) {
                    currentZLevel--;
                    std::cout << "Z-Level decreased to: " << currentZLevel << std::endl;
                }
                else {
                    std::cout << "Warning: Already at ground level (Z=0)" << std::endl;
                }
                break;
                // Add hotkeys for tile types
            case sf::Keyboard::Scancode::Num1:
                currentTileType = TileType::GRASS;
                break;
            case sf::Keyboard::Scancode::Num2:
                currentTileType = TileType::FOREST;
                break;
				// Add more hotkeys as for more tile types// Can change this to inventory system when have it
            }
        }
    }
}
