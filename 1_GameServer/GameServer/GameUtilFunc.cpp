#define _USE_MATH_DEFINES
#include "NetSV-include/CNetServer.h"
#include <Windows.h>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include "ElapsedTimeChecker.h"
#include "Pos.h"
#include "Range.h"
#include "BaseObject.h"
#include "PlayerObject.h"
#include "MonsterObject.h"
#include "CrystalObject.h"
#include "GameUtilfunc.h"

int RandBetween(int low, int high) {
	return rand() % (high - low + 1) + low;
}

int CrystalTypeToAmount(BYTE crystaltype) {
	switch (crystaltype) {
	case 1:
		return CRYSTAL_1;
		break;
	case 2:
		return CRYSTAL_2;
		break;
	case 3:
		return CRYSTAL_3;
		break;
	default:
		return 0;
		break;
	}
}

TilePos GetRandomTilePosInRange(TilePos center, int range) {
	int minX = max(0, center.x - range);
	int maxX = min(center.x + range, TILE_X_MAX - 1);
	int minY = max(0, center.y - range);
	int maxY = min(center.y + range, TILE_Y_MAX - 1);

	int randX = RandBetween(minX, maxX);
	int randY = RandBetween(minY, maxY);

	return { randX, randY };
}

bool isInRnage(TilePos currentpos, TilePos center, int range) {
	int minX = max(0, center.x - range);
	int maxX = min(center.x + range, TILE_X_MAX - 1);
	int minY = max(0, center.y - range);
	int maxY = min(center.y + range, TILE_Y_MAX - 1);

	return ((minX <= currentpos.x && currentpos.x <= maxX) &&
			(minY <= currentpos.y && currentpos.y <= maxY));
}

std::vector<SectorPos> GetAroundSectorPos(SectorPos center, int range) {
	std::vector<SectorPos> ret;
	if (!isSectorPosValid(center)) { return ret; }

	int minX = max(0, center.x - range);
	int maxX = min(center.x + range, SECTOR_X_MAX - 1);
	int minY = max(0, center.y - range);
	int maxY = min(center.y + range, SECTOR_Y_MAX - 1);

	for (int y = minY; y <= maxY; y++) {
		for (int x = minX; x <= maxX; x++) {
			ret.push_back({ x, y });
		}
	}

	return ret;
}

std::vector<TilePos> GetAroundTilePos(TilePos center, int range) {
	std::vector<TilePos> ret;
	if (!isTilePosValid(center)) { return ret; }

	int minX = max(0, center.x - range);
	int maxX = min(center.x + range, TILE_X_MAX - 1);
	int minY = max(0, center.y - range);
	int maxY = min(center.y + range, TILE_Y_MAX - 1);

	for (int y = minY; y <= maxY; y++) {
		for (int x = minX; x <= maxX; x++) {
			ret.push_back({ x, y });
		}
	}

	return ret;
}

bool GetNewAndDelSectorPos(std::vector<SectorPos>* in_prevSPosVec, std::vector<SectorPos>* in_newSPosVec, std::vector<SectorPos>* out_delSPosVec, std::vector<SectorPos>* out_newSPosVec) {
	std::sort(in_prevSPosVec->begin(), in_prevSPosVec->end());
	std::sort(in_newSPosVec->begin(), in_newSPosVec->end());

	std::set_difference(
		in_prevSPosVec->begin(), in_prevSPosVec->end(),
		in_newSPosVec->begin(), in_newSPosVec->end(),
		std::back_inserter(*out_delSPosVec)
	);

	std::set_difference(
		in_newSPosVec->begin(), in_newSPosVec->end(),
		in_prevSPosVec->begin(), in_prevSPosVec->end(),
		std::back_inserter(*out_newSPosVec)
	);

	return true;
}

USHORT ClientRotationToMathDegree(USHORT rotation) {
	short mdeg = -(rotation - 90);
	if (mdeg < 0) { mdeg = (mdeg + 360) % 360; }
	return mdeg;
}

USHORT MathDegreeToClientRotation(USHORT degree) {
	short crot = 90 - degree;
	if (crot < 0) { crot = (crot + 360) % 360; }
	return crot;
}

float GetTileDistance(TilePos startpos, TilePos endpos) {
	return std::sqrtf(std::powf((float)(endpos.x - startpos.x), 2) + std::powf((float)(endpos.y - startpos.y), 2));
}

USHORT GetTileRotation(TilePos center, TilePos dest) {
	constexpr double RadToDeg = 180.0 / M_PI;

	double rad = atan2(dest.y - center.y, dest.x - center.x);
	double deg = rad * RadToDeg;
	deg = round(deg);
	int ideg = (int)deg;
	if (ideg < 0) { ideg = (ideg + 360) % 360; }
	return MathDegreeToClientRotation(ideg);
}

USHORT GetClientRotation(ClientPos center, ClientPos dest) {
	constexpr float RadToDeg = 180.0f / (float)M_PI;

	float rad = atan2(dest.y - center.y, dest.x - center.x);
	float deg = rad * RadToDeg;
	deg = round(deg);
	int ideg = (int)deg;
	if (ideg < 0) { ideg = (ideg + 360) % 360; }
	return MathDegreeToClientRotation(ideg);
}

std::vector<TilePos> GetTilePosInCircleSector(TilePos center, USHORT rotation, USHORT angle, int radius) {
	std::vector<TilePos> tposvec;

	short startangle = rotation - angle;
	short endangle = rotation + angle;
	if (startangle < 0) { startangle += 360; }
	if (360 <= endangle) { endangle -= 360; }
	bool normalrange = startangle <= endangle;

	//get around tiles in square
	tposvec = GetAroundTilePos(center, radius);

	//get tile in radius -> and in circle sector
	for (auto tpos_iter = tposvec.begin(); tpos_iter != tposvec.end(); ) {
		//erase tiles that not in radius
		if (GetTileDistance(center, *tpos_iter) > radius) {
			tpos_iter = tposvec.erase(tpos_iter);
			continue;
		}
		//erase tiles that not in sector
		USHORT tileangle = GetTileRotation(center, *tpos_iter);
		if (normalrange) {
			if (!((startangle <= tileangle) && (tileangle <= endangle))) {
				if (center == *tpos_iter) { ++tpos_iter; continue; }
				tpos_iter = tposvec.erase(tpos_iter);
				continue;
			}
		}
		else {
			if (!((startangle <= tileangle) || (tileangle <= endangle))) {
				if (center == *tpos_iter) { ++tpos_iter; continue; }
				tpos_iter = tposvec.erase(tpos_iter);
				continue;
			}
		}
		++tpos_iter;
	}

	return tposvec;
}

ClientPos GetMovedClientPos(ClientPos startpos, USHORT rotation, float distance) {
	constexpr double DegToRad = M_PI / 180.0;

	USHORT mathdeg = ClientRotationToMathDegree(rotation);
	double mathrad = mathdeg * DegToRad;
	float x = (float)(distance * cos(mathrad));
	float y = (float)(distance * sin(mathrad));

	return { startpos.x + x, startpos.y + y };
}
