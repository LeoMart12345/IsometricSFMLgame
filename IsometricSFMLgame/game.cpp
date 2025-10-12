// game.cpp
#include "game.hpp"
#include "game_state.hpp" 
#include "game_state_start.hpp"
#include "game_state_editor.hpp"
#include <iostream>
#include "Tile.hpp"


Game::Game(TextureManager& textureManager)
	: texmgr(textureManager),
	background(texmgr.getRef("background")),
	inputHandler(window)  // Initialize InputHandler with window
{
	this->window.create(sf::VideoMode({ 1920, 1080 }), "SFML 3 Platformer with Rectangle Character");
	this->window.setFramerateLimit(120);
	this->loadTextures();
	this->loadTiles();
}

void Game::pushState(GameState* state)
{
	this->states.push(state);
}

void Game::popState()
{
	delete this->states.top();
	this->states.pop();
}

void Game::changeState(GameState* state)
{
	if (!this->states.empty())
		popState();
	pushState(state);
}

GameState* Game::peekState()
{
	if (this->states.empty()) return nullptr;
	return this->states.top();
}

void Game::gameLoop()
{
	sf::Clock clock;
	float dt;

	while (this->window.isOpen())
	{
		dt = clock.restart().asSeconds();

		inputHandler.update();

		if (peekState() == nullptr) continue;
		peekState()->handleInput();
		peekState()->update(dt);
		this->window.clear(sf::Color::Black);
		peekState()->draw(dt);
		this->window.display();
	}
}
void Game::loadTiles()
{
	// Grass - simple ground tile
	tileAtlas["grass"] = std::make_unique<Tile>(
		tileSize,
		1,
		texmgr.getRef("grass"),
		TileType::GRASS
	);

	// Forest - decorative ground tile
	tileAtlas["forest"] = std::make_unique<Tile>(
		tileSize,
		1,
		texmgr.getRef("forest"),
		TileType::FOREST
	);

	// Water - simple water tile
	tileAtlas["water"] = std::make_unique<Tile>(
		tileSize,
		1,
		texmgr.getRef("water"),
		TileType::WATER
	);

	// Residential - buildable zone (taller)
	tileAtlas["residential"] = std::make_unique<Tile>(
		tileSize,
		2,
		texmgr.getRef("residential"),
		TileType::RESIDENTIAL
	);

	// Commercial - buildable zone (taller)
	tileAtlas["commercial"] = std::make_unique<Tile>(
		tileSize,
		2,
		texmgr.getRef("commercial"),
		TileType::COMMERCIAL
	);

	// Industrial - buildable zone (taller)
	tileAtlas["industrial"] = std::make_unique<Tile>(
		tileSize,
		2,
		texmgr.getRef("industrial"),
		TileType::INDUSTRIAL
	);

	// Road - simple path tile
	tileAtlas["road"] = std::make_unique<Tile>(
		tileSize,
		1,
		texmgr.getRef("road"),
		TileType::ROAD
	);
	// Empty
	tileAtlas["empty"] = std::make_unique<Tile>(
		tileSize,
		1,
		texmgr.getRef("empty"),
		TileType::EMPTY
	);
}
void Game::loadTextures()
{
	texmgr.loadTexture("grass", "C:/Users/leoma/Downloads/pxArt.png");
	texmgr.loadTexture("forest", "C:/Users/leoma/Downloads/pxArt (1).png");
	texmgr.loadTexture("water", "C:/Users/leoma/Downloads/Block.png");
	texmgr.loadTexture("residential", "C:/Users/leoma/Downloads/Block.png");
	texmgr.loadTexture("commercial", "C:/Users/leoma/Downloads/Block.png");
	texmgr.loadTexture("industrial", "C:/Users/leoma/Downloads/Block.png");
	texmgr.loadTexture("road", "C:/Users/leoma/Downloads/Block.png");
	texmgr.loadTexture("empty", "C:/Users/leoma/Downloads/Empty.png");

	texmgr.loadTexture("background", "C:/Users/leoma/Desktop/background.jpeg");

	//UI Textures
	texmgr.loadTexture("blue", "C:/Users/leoma/Downloads/BlueBackground.png");
}

Game::~Game()
{
	while (!this->states.empty()) popState();
}
