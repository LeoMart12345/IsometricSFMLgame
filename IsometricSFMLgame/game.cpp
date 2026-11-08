// game.cpp
#include "game.hpp"
#include "game_state.hpp"
#include "game_state_start.hpp"
#include "game_state_editor.hpp"
#include "tile.hpp"
#include <optional>


Game::Game(TextureManager& textureManager)
	: texmgr(textureManager),
	inputHandler(window)  // Initialize InputHandler with window
{
	this->window.create(sf::VideoMode({ 1920, 1080 }), "SFML 3 Platformer with Rectangle Character");
	this->window.setFramerateLimit(120);
	this->loadTextures();
	this->loadTiles();

	// Construct the background sprite *after* textures are loaded so the
	// sprite holds a valid reference to the texture.
	this->background = sf::Sprite(texmgr.getRef("background"));
}

void Game::pushState(GameState* state)
{
	this->states.push(state);
}

void Game::popState()
{
	if (this->states.empty()) return;
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

	while (this->window.isOpen())
	{
		const float dt = clock.restart().asSeconds();

		inputHandler.update();

		GameState* state = peekState();
		if (state == nullptr) {
			// No state to render; still pump events so the window can close.
			while (std::optional<sf::Event> event = this->window.pollEvent()) {
				if (event->is<sf::Event::Closed>()) {
					this->window.close();
				}
			}
			continue;
		}

		state->handleInput();
		state->update(dt);
		this->window.clear(sf::Color::Black);
		state->draw(dt);
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
	// Paths are relative to the working directory (the project root when
	// launched from Visual Studio). Copy the assets into ./assets/.
	texmgr.loadTexture("grass",       "assets/pxArt.png");
	texmgr.loadTexture("forest",      "assets/pxArt (1).png");
	texmgr.loadTexture("water",       "assets/Block.png");
	texmgr.loadTexture("residential", "assets/Block.png");
	texmgr.loadTexture("commercial",  "assets/Block.png");
	texmgr.loadTexture("industrial",  "assets/Block.png");
	texmgr.loadTexture("road",        "assets/Block.png");
	texmgr.loadTexture("empty",       "assets/Empty.png");

	texmgr.loadTexture("background",  "assets/background.jpeg");

	// UI textures
	texmgr.loadTexture("blue",        "assets/BlueBackground.png");
}

Game::~Game()
{
	while (!this->states.empty()) popState();
}
