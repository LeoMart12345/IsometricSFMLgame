// game_state_editor.cpp
#include "game_state_editor.hpp"
#include "game.hpp"
#include "map.hpp"
#include <SFML/Graphics.hpp>

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

    // Center the view on the middle of the 10x10 isometric map.
    // getScreenPos(x, y, 0) = ((x - y) * 32, (x + y) * 16)
    // For a 10x10 map, the visual center is at grid (4.5, 4.5):
    //   x = (4.5 - 4.5) * 32 = 0
    //   y = (4.5 + 4.5) * 16 = 144
    gameView.setCenter({ 0.f, 144.f });

    // Zoom out a bit to see more of the map
    gameView.zoom(0.8f);
}

void GameStateEditor::draw(const float dt) {
    if (!this->game) return;

    this->game->window.clear(sf::Color::Black);
    this->game->window.setView(gameView);

    // Draw all tiles
    map.draw(this->game->window);
    //draw the hovering tile
    map.drawHover(this->game->window); // Add this after regular draw
    //draw the outline of the buildable area
    map.drawOutline(this->game->window);

    this->game->window.setView(guiView);
}
void GameStateEditor::update(const float /*dt*/) {
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

            sf::Vector3i clickGridPos = map.screenToGrid(worldPos.x, worldPos.y, currentZLevel);

            if (mousePressed->button == sf::Mouse::Button::Left) {
                map.placeTile(clickGridPos.x, clickGridPos.y, clickGridPos.z, currentTileType);
            }
            else if (mousePressed->button == sf::Mouse::Button::Right) {
                map.placeTile(clickGridPos.x, clickGridPos.y, clickGridPos.z, TileType::VOID);
            }
        }
        else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            switch (keyPressed->scancode) {
            case sf::Keyboard::Scancode::H:
                if (currentZLevel < MAX_Z_LEVEL) {
                    currentZLevel++;
                }
                break;
            case sf::Keyboard::Scancode::G:
                if (currentZLevel > 0) {
                    currentZLevel--;
                }
                break;
                // Add hotkeys for tile types
            case sf::Keyboard::Scancode::Num1:
                currentTileType = TileType::GRASS;
                break;
            case sf::Keyboard::Scancode::Num2:
                currentTileType = TileType::FOREST;
                break;
            case sf::Keyboard::Scancode::Num3:
                currentTileType = TileType::WATER;
                break;
            case sf::Keyboard::Scancode::Num4:
                currentTileType = TileType::RESIDENTIAL;
                break;
            case sf::Keyboard::Scancode::Num5:
                currentTileType = TileType::COMMERCIAL;
                break;
            case sf::Keyboard::Scancode::Num6:
                currentTileType = TileType::INDUSTRIAL;
                break;
            case sf::Keyboard::Scancode::Num7:
                currentTileType = TileType::ROAD;
                break;
            }
        }
    }
}
