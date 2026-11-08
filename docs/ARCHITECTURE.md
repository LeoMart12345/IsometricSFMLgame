# IsometricSFMLgame — Architecture

This document describes how the game is structured, how the main loop drives
state transitions, and how isometric coordinates are converted to screen space.

## High-level overview

The game uses a **state stack** pattern. `Game` owns the window, the texture
manager, the tile atlas, and a stack of `GameState` objects. Each frame the
top state handles input, updates, and draws itself.

```mermaid
classDiagram
    class Game {
        +RenderWindow window
        +InputHandler inputHandler
        +TextureManager& texmgr
        +map~string, unique_ptr~Tile~~ tileAtlas
        +stack~GameState*~ states
        +pushState(GameState*)
        +popState()
        +changeState(GameState*)
        +peekState() GameState*
        +gameLoop()
    }

    class GameState {
        <<abstract>>
        +Game* game
        +draw(float dt)*
        +update(float dt)*
        +handleInput()*
    }

    class GameStateStart {
        +draw(float dt)
        +update(float dt)
        +handleInput()
        +loadgame()
    }

    class GameStateEditor {
        -Map map
        -View gameView
        -View guiView
        -TileType currentTileType
        -int currentZLevel
        +draw(float dt)
        +update(float dt)
        +handleInput()
    }

    class Map {
        -vector~vector~vector~shared_ptr~Tile~~~~ tiles
        -float tileWidth
        -float tileHeight
        -TextureManager& texmgr
        -shared_ptr~Tile~ hoverTile
        +getScreenPos(x, y, z) Vector2f
        +screenToGrid(sx, sy, z) Vector3i
        +placeTile(x, y, z, type)
        +draw(RenderWindow&)
        +drawOutline(RenderWindow&)
        +updateHover(pos, type)
        +drawHover(RenderWindow&)
    }

    class Tile {
        +Sprite sprite
        +TileType tileType
        +draw(RenderWindow&, float)
        +update()
    }

    class TextureManager {
        -map~string, Texture~ textures
        +loadTexture(name, filename)
        +getRef(name) Texture&
    }

    class InputHandler {
        +update()
        +handleEvent(event)
        +isMousePressed(button) bool
        +isMouseJustPressed(button) bool
        +isMouseReleased(button) bool
        +isMouseDragging(button, minDistance) bool
        +isKeyPressed(key) bool
        +isKeyJustPressed(key) bool
        +isKeyReleased(key) bool
        +getMouseWorldPos(view) Vector2f
        +getMouseScreenPos() Vector2i
    }

    Game "1" o-- "*" GameState : owns stack
    GameState <|-- GameStateStart
    GameState <|-- GameStateEditor
    GameStateEditor *-- Map
    Map *-- Tile
    Game --> TextureManager
    Game --> InputHandler
    Map --> TextureManager
```

## Main loop

```mermaid
sequenceDiagram
    participant Main
    participant Game
    participant State as GameState (top)
    participant Window

    Main->>Game: gameLoop()
    loop while window.isOpen()
        Game->>Game: dt = clock.restart()
        Game->>Game: inputHandler.update()
        Game->>State: handleInput()
        Game->>State: update(dt)
        Game->>Window: clear(Black)
        Game->>State: draw(dt)
        Game->>Window: display()
    end
```

## State transitions

```mermaid
stateDiagram-v2
    [*] --> Start
    Start --> Editor : Space pressed
    Start --> [*] : Escape / window closed
    Editor --> [*] : window closed
```

## Isometric coordinate transform

Grid coordinates `(x, y, z)` map to screen coordinates using a 2:1 isometric
projection. `tileWidth = 64`, `tileHeight = 32`.

```mermaid
flowchart LR
    subgraph Grid["Grid space (x, y, z)"]
        G["(x, y, z)"]
    end

    subgraph Screen["Screen space (px, py)"]
        S["px = (x - y) * tileWidth / 2<br/>py = (x + y) * tileHeight / 2 - z * tileHeight / 2"]
    end

    G -->|getScreenPos| S

    subgraph Inverse["Inverse (screenToGrid)"]
        I["halfW = tileWidth / 2, halfH = tileHeight / 2<br/>adjY = sy + z * halfH<br/>gridX = (sx / halfW + adjY / halfH) / 2<br/>gridY = (adjY / halfH - sx / halfW) / 2"]
    end

    S -->|screenToGrid| I
```

## Tile placement flow

```mermaid
sequenceDiagram
    participant User
    participant Editor as GameStateEditor
    participant Map
    participant Tile

    User->>Editor: Mouse move
    Editor->>Map: screenToGrid(mouse)
    Editor->>Map: updateHover(gridPos, currentTileType)
    Map->>Tile: create hover sprite (50% alpha)

    User->>Editor: Left click
    Editor->>Map: screenToGrid(click)
    Editor->>Map: placeTile(x, y, z, currentTileType)
    Map->>Tile: setTexture + setType

    User->>Editor: Right click
    Editor->>Map: placeTile(x, y, z, VOID)
```

## Controls

| Key / Button | Action |
|---|---|
| `Space` (Start) | Enter editor |
| `Escape` (Start) | Quit |
| `H` | Raise Z-level |
| `G` | Lower Z-level |
| `1`–`7` | Select tile type (Grass, Forest, Water, Residential, Commercial, Industrial, Road) |
| Left click | Place selected tile |
| Right click | Clear tile (VOID) |
