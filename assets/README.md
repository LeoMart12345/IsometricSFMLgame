# Assets

Drop the art files here. The game looks them up through
`IsometricSFMLgame/src/core/asset_path.cpp`, so the exact working directory does
not matter: it searches the current directory and the executable's directory
plus up to four parents (that covers Visual Studio's `x64/Debug/` and a CMake
`build/` folder).

**No files here is fine.** Missing art is an expected state, not an error: for
every missing file the game draws a generated solid-colour diamond placeholder
and prints one summary block at start-up listing what it could not find. Nothing
throws, nothing crashes, and any file you add is used the next time you start the
game.

```
[assets] 10 of 10 textures have no art file; drawing generated placeholders for them:
           'grass' from 'assets/pxArt.png'
           'forest' from 'assets/pxArt (1).png'
           ...
[assets] drop those files into assets/ and restart (see assets/README.md)
```

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

### One image, several tile types

`Block.png` is wired to five tile types, so once you supply it those five all
look the same. While the art is missing each type keeps its own placeholder
colour, which is what makes the editor readable. If you want distinct art per
type, add a file per type and point the matching row of `kTextures` in
`IsometricSFMLgame/src/core/game.cpp` at it:

```cpp
{"water", "assets/Block.png", sf::Color(70, 130, 220, 255)},
// becomes
{"water", "assets/water.png", sf::Color(70, 130, 220, 255)},
```

The third field is only used when the file is missing, so you can leave the
colour alone.

Game assets are not mine.
