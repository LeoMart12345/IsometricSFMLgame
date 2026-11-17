// constants.hpp
#pragma once

#include <cstdint>

// Central place for the editor's tunable values.
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

// Editor camera.
inline constexpr float editorViewZoom = 0.8f;

// Alpha applied to the hover preview tile (0-255).
inline constexpr std::uint8_t hoverAlpha = 128;
} // namespace config
