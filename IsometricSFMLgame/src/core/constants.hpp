// constants.hpp
#pragma once

#include <cstdint>

// Central place for the game's tunable values. Keeping these in one header
// avoids the old problem of the window, the map and the tiles each having
// their own (and sometimes conflicting) idea of how big a tile is.
namespace config
{
inline constexpr unsigned int windowWidth = 1920;
inline constexpr unsigned int windowHeight = 1080;
inline constexpr unsigned int framerateLimit = 120;
inline constexpr const char* windowTitle = "Isometric SFML Game";

// Isometric footprint of a single tile (2:1 projection).
inline constexpr unsigned int tileWidth = 64;
inline constexpr unsigned int tileHeight = 32;

// Size of the editor map (small test map for now).
inline constexpr unsigned int mapWidth = 10;
inline constexpr unsigned int mapHeight = 10;
inline constexpr unsigned int mapLayers = 3;
inline constexpr int maxZLevel = 2;

// Editor camera.
inline constexpr float editorViewZoom = 0.8f;

// Alpha applied to the hover preview tile (0-255).
inline constexpr std::uint8_t hoverAlpha = 128;
} // namespace config
