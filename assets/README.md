# Assets

Drop the art files here. The game looks them up through
`IsometricSFMLgame/src/core/asset_path.cpp`, so the exact working directory does
not matter: it searches the current directory and the executable's directory
plus up to four parents (that covers Visual Studio's `x64/Debug/` and a CMake
`build/` folder).

**No files here is fine.** Missing art is an expected state, not an error: tiles
get a generated 2:1 diamond in a distinct colour and the player gets a generated
four-frame idle strip, and one summary block on the console lists what was not
found. Nothing throws, and any file you add is used the next time you start the
game.

| File | Used for | Placeholder |
|---|---|---|
| `pxArt.png` | Grass tile | green diamond |
| `pxArt (1).png` | Forest tile | dark green diamond |
| `Block.png` | Water, residential, commercial, industrial and road tiles | blue / sand / purple / grey / slate diamonds |
| `Empty.png` | Empty cell sprite (should be fully transparent) | transparent |
| `background.jpeg` | Title screen background (stretched to the window) | dark blue-grey |
| `BlueBackground.png` | Loaded as `"blue"`, reserved for HUD panels | steel blue |
| `player.png` | Player idle animation (see below) | animated four-frame figure |

## Tiles

Tile sprites should be 2:1 isometric diamonds. `Tile` scales the sprite so its
width equals `config::tileWidth` (64 px) and centres the origin, so larger source
art works as long as the 2:1 ratio is kept.

`Block.png` is wired to five tile types, so supplying it makes those five look
the same. For distinct art, add one file per type and change the matching row of
`kTextures` in `IsometricSFMLgame/src/core/game.cpp`.

## Player idle sheet

`player.png` is a **horizontal strip of square frames**; every frame in the strip
is one frame of the idle loop. This is how a Diablo II animation conversion is
normally exported, so cropping one idle direction to a single row gives a file
the game can use directly:

```
+--------+--------+--------+--------+ ...
| frame0 | frame1 | frame2 | frame3 |      each frame is, say, 160x160
+--------+--------+--------+--------+ ...
```

* Frame size is taken from the image height; the frame count is
  `imageWidth / imageHeight`, so the frames must be square.
* The sprite is anchored at the bottom centre and scaled so one frame is
  `config::playerScreenHeight` (64 px) tall. If the character looks too small or
  too large, change that value.
* Frame duration is `config::playerIdleFrameSeconds` (0.12 s by default). Both
  constants live in `IsometricSFMLgame/src/core/constants.hpp`.
* A single-frame image (a plain square) also loads; the animation then simply
  stays on that frame.

Game assets are not mine.
