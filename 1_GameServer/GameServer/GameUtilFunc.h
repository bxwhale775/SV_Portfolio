#pragma once
int RandBetween(int low, int high);
int CrystalTypeToAmount(BYTE crystaltype);
TilePos GetRandomTilePosInRange(TilePos center, int range);
bool isInRnage(TilePos currentpos, TilePos center, int range);

std::vector<SectorPos> GetAroundSectorPos(SectorPos center, int range);
std::vector<TilePos> GetAroundTilePos(TilePos center, int range);
bool GetNewAndDelSectorPos(
	std::vector<SectorPos>* in_prevSPosVec,
	std::vector<SectorPos>* in_newSPosVec,
	std::vector<SectorPos>* out_delSPosVec,
	std::vector<SectorPos>* out_newSPosVec
);
USHORT ClientRotationToMathDegree(USHORT rotation);
USHORT MathDegreeToClientRotation(USHORT degree);
float GetTileDistance(TilePos startpos, TilePos endpos);
USHORT GetTileRotation(TilePos center, TilePos dest);
USHORT GetClientRotation(ClientPos center, ClientPos dest);
std::vector<TilePos> GetTilePosInCircleSector(TilePos center, USHORT rotation, USHORT angle, int radius);
ClientPos GetMovedClientPos(ClientPos startpos, USHORT rotation, float distance);