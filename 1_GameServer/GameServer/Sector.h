#pragma once
#include <unordered_map>
#include "Tile.h"

struct Sector {
	std::list<Tile*> TileList;
};