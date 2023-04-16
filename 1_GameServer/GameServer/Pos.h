#pragma once

constexpr int TILE_X_MAX = 400;
constexpr int TILE_Y_MAX = 200;

constexpr float TILE_SCALE = 2.0f;
constexpr float CLPOS_X_MAX = TILE_X_MAX / TILE_SCALE;
constexpr float CLPOS_Y_MAX = TILE_Y_MAX / TILE_SCALE;

constexpr int SECTOR_SIZE = 4;
constexpr int SECTOR_X_MAX = TILE_X_MAX / SECTOR_SIZE;
constexpr int SECTOR_Y_MAX = TILE_Y_MAX / SECTOR_SIZE;

using TileID = unsigned long long;
using SectorID = unsigned long long;


struct ClientPos {
	float x = 0.0f;
	float y = 0.0f;
};

struct TilePos {
	int x = 0;
	int y = 0;

	TileID GetID() const noexcept {
		return x + ((TileID)y * TILE_X_MAX);
	}
	bool operator==(const TilePos& other) const noexcept {
		return (x == other.x) && (y == other.y);
	}
	bool operator!=(const TilePos& other) const noexcept {
		return !(*this == other);
	}
	bool operator<(const TilePos& other) const noexcept {
		return this->GetID() < other.GetID();
	}
	bool operator<=(const TilePos& other) const noexcept {
		return this->GetID() <= other.GetID();
	}
	bool operator>(const TilePos& other) const noexcept {
		return this->GetID() > other.GetID();
	}
	bool operator>=(const TilePos& other) const noexcept {
		return this->GetID() >= other.GetID();
	}
};

struct SectorPos {
	int x = 0;
	int y = 0;

	SectorID GetID() const noexcept {
		return x + ((SectorID)y * SECTOR_X_MAX);
	}
	bool operator==(const SectorPos& other) const noexcept {
		return (x == other.x) && (y == other.y);
	}
	bool operator!=(const SectorPos& other) const noexcept {
		return !(*this == other);
	}
	bool operator<(const SectorPos& other) const noexcept {
		return this->GetID() < other.GetID();
	}
	bool operator<=(const SectorPos& other) const noexcept {
		return this->GetID() <= other.GetID();
	}
	bool operator>(const SectorPos& other) const noexcept {
		return this->GetID() > other.GetID();
	}
	bool operator>=(const SectorPos& other) const noexcept {
		return this->GetID() >= other.GetID();
	}
};


template <>
struct std::hash<TilePos> {
	std::size_t operator()(const TilePos& o) const noexcept {
		return o.GetID();
	}
};

template <>
struct std::hash<SectorPos> {
	std::size_t operator()(const SectorPos& o) const noexcept {
		return o.GetID();
	}
};


constexpr TilePos ClientPosToTilePos(const ClientPos cpos) {
	return { 
		(int)(cpos.x * TILE_SCALE),
		(int)(cpos.y * TILE_SCALE)
	};
}

constexpr ClientPos TilePosToClientPos(const TilePos tpos) {
	return {
		(float)tpos.x / TILE_SCALE,
		(float)tpos.y / TILE_SCALE
	};
}


constexpr SectorPos TilePosToSectorPos(const TilePos tpos) {
	return {
		tpos.x / SECTOR_SIZE,
		tpos.y / SECTOR_SIZE
	};
}

constexpr TilePos SectorPosToTilePos(const SectorPos spos) {
	return {
		spos.x * SECTOR_SIZE,
		spos.y * SECTOR_SIZE
	};
}


constexpr bool isClientPosValid(const ClientPos cpos) {
	if ((cpos.x < 0.0f || CLPOS_X_MAX <= cpos.x) ||
		(cpos.y < 0.0f || CLPOS_Y_MAX <= cpos.y)) {
		return false;
	}
	return true;
}

constexpr bool isTilePosValid(const TilePos tpos) {
	if ((tpos.x < 0 || TILE_X_MAX <= tpos.x) ||
		(tpos.y < 0 || TILE_Y_MAX <= tpos.y)) {
		return false;
	}
	return true;
}

constexpr bool isSectorPosValid(const SectorPos spos) {
	if ((spos.x < 0 || SECTOR_X_MAX <= spos.x) ||
		(spos.y < 0 || SECTOR_Y_MAX <= spos.y)) {
		return false;
	}
	return true;
}

constexpr bool isRotationValid(USHORT rotation) {
	if (360 <= rotation) { return false; }
	return true;
}