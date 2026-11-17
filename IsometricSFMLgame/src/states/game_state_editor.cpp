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
      player_(game.textures().getRef("player"),
              map_.gridToScreen(map_.getWidth() / 2, map_.getHeight() / 2, 0))
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
        // Clamp against the map's real layer count instead of a hardcoded
        // constant, so changing mapLayers cannot desynchronise the editor.
        if (currentZLevel_ + 1 < map_.getLayers())
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

    // Right click is the eraser; placeTile() ignores clicks outside the map.
    const TileType type = (mouse.button == sf::Mouse::Button::Right) ? TileType::VOID : currentTileType_;
    const sf::Vector3i cell = gridCellAt(mouse.position);

    map_.placeTile(cell.x, cell.y, cell.z, type);
}

void GameStateEditor::updateHover()
{
    map_.updateHover(gridCellAt(sf::Mouse::getPosition(game_.window())), currentTileType_);
}

sf::Vector3i GameStateEditor::gridCellAt(sf::Vector2i pixelPosition) const
{
    // Convert with the world view, otherwise the click would be mapped with the
    // GUI view's (unzoomed) transform and land on the wrong cell.
    const sf::Vector2f worldPosition = game_.window().mapPixelToCoords(pixelPosition, gameView_);

    return map_.screenToGrid(worldPosition, currentZLevel_);
}

void GameStateEditor::update(float dt)
{
    updateHover();
    player_.update(dt);
}

void GameStateEditor::draw(float /*dt*/)
{
    sf::RenderWindow& window = game_.window();

    window.setView(gameView_);
    map_.draw(window);
    // Preview and grid next: the translucent hover tile must not wash out the
    // grid lines that tell the player which cell is selected.
    map_.drawHover(window);
    map_.drawOutline(window, currentZLevel_);
    // The character is drawn last so the grid lines do not cross his body. He
    // stands on the topmost layer for now, so no depth sorting is needed yet.
    player_.draw(window);

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
