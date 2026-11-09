# Assets

Drop the art files here. The game looks them up through
`IsometricSFMLgame/src/core/asset_path.cpp`, so the exact working directory does
not matter: it searches the current directory and the executable's directory
plus up to four parents (that covers Visual Studio's `x64/Debug/` and a CMake
`build/` folder).

Any file that is missing produces a console warning and a generated
solid-colour diamond placeholder, so the game still runs with an empty folder.

| File | Used for | Placeholder colour |
|---|---|---|
| `pxArt.png` | Grass tile | green |
| `pxArt (1).png` | Forest tile | dark green |
| `Block.png` | Water, residential, commercial, industrial and road tiles | blue / sand / purple / grey / slate |
| `Empty.png` | Empty cell sprite (should be fully transparent) | transparent |
| `background.jpeg` | Title screen background (stretched to the window) | dark blue-grey |
| `BlueBackground.png` | Loaded as `"blue"`, reserved for HUD panels | steel blue |

Tile sprites should be 2:1 isometric diamonds. `Tile` scales the sprite so its
width equals `config::tileWidth` (64 px), and the sprite origin is the centre of
the texture, so larger source art works as long as it keeps the 2:1 ratio.

Game assets are not mine.
