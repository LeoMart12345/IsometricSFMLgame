// game.hpp
#pragma once

#include <stack>
#include <SFML/Graphics.hpp>
#include "texture_manager.hpp"
#include "tile.hpp"
#include <memory> // Add this at top of file for std::make_unique
#include "input_handler.hpp"

class GameState;

class Game
{
public:
    // Instead of passing individual textures, pass the whole TextureManager.
    Game(TextureManager& textureManager);
    ~Game();

    const static int tileSize = 64; 
    const static int height = 32;

    void pushState(GameState* state);
    void popState();
    void changeState(GameState* state);
    GameState* peekState();

    void gameLoop();

    std::stack<GameState*> states;
    sf::RenderWindow window;
    InputHandler inputHandler;

    TextureManager& texmgr;
    std::map<std::string, std::unique_ptr<Tile>> tileAtlas; 
    sf::Sprite background; // This will be constructed with a texture from texmgr.

private:
	void loadTextures();
	void loadTiles();
};

    