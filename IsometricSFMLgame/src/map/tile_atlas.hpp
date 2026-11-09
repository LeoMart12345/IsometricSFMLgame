// tile_atlas.hpp
#pragma once

#include <map>

#include "map/tile.hpp"
#include "map/tile_type.hpp"

// Prototype tile for every placeable type. Map copies from here, so the atlas
// has to outlive every Map that references it (Game owns it, maps are created
// by GameStates below it).
using TileAtlas = std::map<TileType, Tile>;
