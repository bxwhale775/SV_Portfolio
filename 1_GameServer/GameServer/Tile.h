#pragma once
#include <unordered_map>
#include "BaseObject.h"

struct Tile {
	std::unordered_map<INT64, BaseObject*> ObjMap;
};