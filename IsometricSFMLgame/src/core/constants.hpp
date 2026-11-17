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

// Editor camera.
inline constexpr float editorViewZoom = 0.8f;

// Alpha applied to the hover preview tile (0-255).
inline constexpr std::uint8_t hoverAlpha = 128;

// A frame that took longer than this (window drag, breakpoint, alt-tab) is
// treated as this long, so the simulation never teleports actors.
inline constexpr float maxFrameTime = 0.1f;

// ---------------------------------------------------------------------------
// Play mode (Diablo-style dungeon crawl)
// ---------------------------------------------------------------------------
inline constexpr int dungeonWidth = 48;
inline constexpr int dungeonHeight = 48;
inline constexpr int dungeonDepth = 4; // number of levels before the run ends

inline constexpr float playViewZoom = 0.9f;

// Movement speeds in cells per second (a cell's own move cost also applies).
inline constexpr float playerCellsPerSecond = 5.f;
inline constexpr float monsterCellsPerSecond = 2.5f;

// Seconds between two attacks of the same actor.
inline constexpr float playerAttackDelay = 0.5f;
inline constexpr float monsterAttackDelay = 0.9f;

// Monsters only start chasing the player from this distance away (cells).
inline constexpr int monsterAggroRadius = 9;

// Monsters re-run their pathfinding at most this often (seconds).
inline constexpr float monsterRepathDelay = 0.6f;

// Number of monsters on dungeon level 1, plus this many extra per level.
inline constexpr int baseMonsterCount = 6;
inline constexpr int monstersPerLevel = 4;

// Healing potions the player starts with, and how much one restores.
inline constexpr int startingPotions = 3;
inline constexpr float potionHealFraction = 0.45f;

// A monster always drops at most this many items.
inline constexpr int maxDropsPerMonster = 2;

// Player experience per killed monster: base * level of the monster.
inline constexpr int monsterExperience = 25;

// Inventory limits.
inline constexpr int inventorySlots = 24;
inline constexpr float inventoryMaxWeight = 80.f;

// Camera smoothing factor for following the player (1/s).
inline constexpr float cameraFollowSharpness = 7.f;
} // namespace config
