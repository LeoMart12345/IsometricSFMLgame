// game_state_editor.cpp
#include "states/game_state_editor.hpp"

#include <memory>

#include "core/constants.hpp"
#include "core/game.hpp"
#include "states/game_state_start.hpp"

GameStateEditor::GameStateEditor(Game& game)
    : GameState(game),
      map_(static_cast<int>(config::mapWidth), static_cast<int>(config::mapHeight),
           static_cast<int>(config::mapLayers), game.tileAtlas()),
      currentZLevel_(0)
{
    const sf::Vector2u windowSize = game.window().getSize();
    const sf::Vector2f size{static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)};

    // GUI view: pixel perfect, used for HUD elements.
    guiView_.setSize(size);
    guiView_.setCenter(size / 2.f);

    // World view: same size, but centred on the map and zoomed out a little.
    gameView_.setSize(size);
    gameView_.setCenter(map_.getCenter());
    gameView_.zoom(config::editorViewZoom);
}

void GameStateEditor::handleInput(const sf::Event& event)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
        handleKey(key->code);
    else if (const auto* mouse = event.getIf<sf::Event::MouseButtonPressed>())
        placeTileAtMouse(*mouse);
}

void GameStateEditor::handleKey(sf::Keyboard::Key key)
{
    switch (key)
    {
    case sf::Keyboard::Key::Escape:
        game_.changeState(std::make_unique<GameStateStart>(game_));
        return;
    case sf::Keyboard::Key::H:
        if (currentZLevel_ < config::maxZLevel)
            ++currentZLevel_;
        break;
    case sf::Keyboard::Key::G:
        if (currentZLevel_ > 0)
            --currentZLevel_;
        break;
    case sf::Keyboard::Key::Num0:
        selectTile(TileType::VOID);
        break;
    case sf::Keyboard::Key::Num1:
        selectTile(TileType::GRASS);
        break;
    case sf::Keyboard::Key::Num2:
        selectTile(TileType::FOREST);
        break;
    case sf::Keyboard::Key::Num3:
        selectTile(TileType::WATER);
        break;
    case sf::Keyboard::Key::Num4:
        selectTile(TileType::RESIDENTIAL);
        break;
    case sf::Keyboard::Key::Num5:
        selectTile(TileType::COMMERCIAL);
        break;
    case sf::Keyboard::Key::Num6:
        selectTile(TileType::INDUSTRIAL);
        break;
    case sf::Keyboard::Key::Num7:
        selectTile(TileType::ROAD);
        break;
    default:
        break;
    }
}

void GameStateEditor::placeTileAtMouse(const sf::Event::MouseButtonPressed& mouse)
{
    if (mouse.button != sf::Mouse::Button::Left && mouse.button != sf::Mouse::Button::Right)
        return;

    // Convert with the world view, otherwise the click would be mapped with
    // the GUI view's (unzoomed) transform and land on the wrong tile.
    const sf::Vector2f worldPos = game_.window().mapPixelToCoords(mouse.position, gameView_);
    const sf::Vector3i gridPos = map_.screenToGrid(worldPos, currentZLevel_);

    if (!map_.isInBounds(gridPos.x, gridPos.y, gridPos.z))
        return;

    const TileType type = (mouse.button == sf::Mouse::Button::Right) ? TileType::VOID : currentTileType_;
    map_.placeTile(gridPos.x, gridPos.y, gridPos.z, type);
}

void GameStateEditor::updateHover()
{
    const sf::Vector2f mouseWorld = game_.input().getMouseWorldPos(gameView_);
    const sf::Vector3i gridPos = map_.screenToGrid(mouseWorld, currentZLevel_);
    map_.updateHover(gridPos, currentTileType_);
}

void GameStateEditor::update(float /*dt*/)
{
    updateHover();
}

void GameStateEditor::draw(float /*dt*/)
{
    sf::RenderWindow& window = game_.window();

    window.setView(gameView_);
    map_.draw(window);
    map_.drawOutline(window, currentZLevel_);
    map_.drawHover(window);

    // Future HUD elements go here: everything drawn after this point uses the
    // pixel perfect GUI view instead of the zoomed world view.
    window.setView(guiView_);
}

void GameStateEditor::selectTile(TileType type)
{
    currentTileType_ = type;

    // Refresh immediately so the preview matches the new selection even if the
    // mouse does not move before the next update().
    updateHover();
}
