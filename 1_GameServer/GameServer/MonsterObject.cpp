#include "GameServer.h"
#include "MonsterObject.h"

void MonsterObject::Init(GameServer* gamesv, TilePos center, int range) {
	//get random start pos & init base
	ClientPos rand_startpos = TilePosToClientPos(GetRandomTilePosInRange(center, range));
	InitBaseObj(gamesv, rand_startpos, rand() % 360);

	//init monster's default var
	_Center = center;
	_Range = range;

	//init monster's common var
	_HP = MON_HP;
	_isDead = false;

	//init monster's AI var
	AI_Init();

	//join to game
	//insert to globalMap
	_gsv->InsertObjectToMap(this);
	//insert to tile
	_gsv->InsertObjectToTile(this, _TPos);

	//Will not broadcast monster's join - there is no player in GameServer init sequence.
	return;
}

void MonsterObject::Update() {
	//Run AI
	switch (_AIMode) {
	case MonsterObject::AIMODE::DEAD:
		AI_DEAD();
		break;
	case MonsterObject::AIMODE::ROAM:
		AI_ROAM();
		break;
	case MonsterObject::AIMODE::ATTACK:
		AI_ATTACK();
		break;
	case MonsterObject::AIMODE::RETURN:
		AI_RETURN();
		break;
	default:
		break;
	}
	return;
}

void MonsterObject::Move(ClientPos dest, USHORT rotation) {
	//backup prev Pos info and update
	TilePos prevTPos = _TPos;
	SectorPos prevSPos = _SPos;
	USHORT prevRotation = _Rotation;
	UpdatePos(dest);
	UpdateRotation(rotation);
	TilePos newTPos = _TPos;
	SectorPos newSPos = _SPos;
	USHORT newRotation = _Rotation;

	//update tile and sector if needed
	if (prevTPos != newTPos) {
		//get new-around player list to send move msg
		auto newAroundSPosVec = GetAroundSectorPos(newSPos, _gsv->GetProps().ActiveSectorRange);
		auto AroundPlayerList = _gsv->GetPlayerObjListFromSPosVec(&newAroundSPosVec);

		//move tile
		_gsv->EraseObjectFromTile(this, prevTPos);
		_gsv->InsertObjectToTile(this, newTPos);

		//if sector has been changed
		if (prevSPos != newSPos) {
			//get prev sector's around sectors
			auto prevAroundSPosVec = GetAroundSectorPos(prevSPos, _gsv->GetProps().ActiveSectorRange);

			//get new and del sector
			std::vector<SectorPos> delSPosVec;
			std::vector<SectorPos> newSPosVec;
			GetNewAndDelSectorPos(&prevAroundSPosVec, &newAroundSPosVec, &delSPosVec, &newSPosVec);

			//get new and del player list
			auto DelPlayerList = _gsv->GetPlayerObjListFromSPosVec(&delSPosVec);
			auto NewPlayerList = _gsv->GetPlayerObjListFromSPosVec(&newSPosVec);

			//send deletion of monster to del-players
			for (auto& i : DelPlayerList) {
				_gsv->GetRPCProxy().PACKET_CS_GAME_RES_REMOVE_OBJECT(i->_Nid, _ClientID);
			}
			//send creation of monster to new-players
			for (auto& i : NewPlayerList) {
				_gsv->GetRPCProxy().PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER(i->_Nid,
					_ClientID, _CPos.x, _CPos.y, _Rotation, 0);
			}
		}

		//send move msg
		for (auto& i : AroundPlayerList) {
			_gsv->GetRPCProxy().PACKET_CS_GAME_RES_MOVE_MONSTER(i->_Nid,
				_ClientID, _CPos.x, _CPos.y, _Rotation);
		}
	}

	//send msg when only rotation is changed
	if (prevTPos == newTPos && prevRotation != newRotation) {
		auto newAroundSPosVec = GetAroundSectorPos(newSPos, _gsv->GetProps().ActiveSectorRange);
		auto AroundPlayerList = _gsv->GetPlayerObjListFromSPosVec(&newAroundSPosVec);

		for (auto& i : AroundPlayerList) {
			_gsv->GetRPCProxy().PACKET_CS_GAME_RES_MOVE_MONSTER(i->_Nid,
				_ClientID, _CPos.x, _CPos.y, _Rotation);
		}
	}

	return;
}

bool MonsterObject::TakeDamage(PlayerObject* pobj, int damage) {
	if (isDead()) { return false; }

	//Make AI to attack player
	_LastAttackedPlayer = pobj;
	_AIMode = AIMODE::ATTACK;
	_LastPlayerSeen.Reset();

	//give damage to monster
	_HP -= damage;
	if (_HP <= 0) {									//If monster's HP runs out
		_gsv->EraseObjectFromTile(this, _TPos);		//Remove from tile
		_isDead = true;								//Mark as Dead monster
		_TimeToRevive.Reset();						//Start revive count
		_AIMode = AIMODE::DEAD;						//Make AI waiting for revive
		return true;								//alert that this monster is dead
	}

	return false;									//alert that this monster is Still Alive				
}

void MonsterObject::AI_Init() {
	_AIMode = AIMODE::ROAM;
	_LastAttackedPlayer = nullptr;
	_TimeToRevive.Reset();
	_LastPlayerSeen.Reset();
	_LastMoved.Reset();
	_TargetedTPos = GetRandomTilePosInRange(_Center, _Range);
	return;
}

void MonsterObject::AI_DEAD() {
	//if time to revive
	if (_TimeToRevive.CheckElapsed(10000)) {
		//get random start pos & set pos
		ClientPos rand_startpos = TilePosToClientPos(GetRandomTilePosInRange(_Center, _Range));
		UpdatePos(rand_startpos);
		UpdateRotation(rand() % 360);

		//init monster's common var
		_HP = MON_HP;
		_isDead = false;

		//init monster's AI var
		AI_Init();

		//insert to tile
		_gsv->InsertObjectToTile(this, _TPos);

		//broadcast monster's respawn
		auto aroundsector = GetAroundSectorPos(_SPos, _gsv->GetProps().ActiveSectorRange);
		auto aroundplayer = _gsv->GetPlayerObjListFromSPosVec(&aroundsector);
		for (auto p : aroundplayer) {
			_gsv->GetRPCProxy().PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER(p->_Nid,
				_ClientID, _CPos.x, _CPos.y, _Rotation, 1);
		}
	}
	return;
}

void MonsterObject::AI_ROAM() {
	//check is time to move
	if (!_LastMoved.CheckElapsed(1000)) { return; }

	//if monster arrived at targeted tile, re-set target tile
	if (_TPos == _TargetedTPos || GetTileDistance(_TPos, _TargetedTPos) < 1.5f) {
		_TargetedTPos = GetRandomTilePosInRange(_Center, _Range);
	}

	//calculate move
	USHORT nextRotation = GetTileRotation(_TPos, _TargetedTPos);
	ClientPos nextCPos = GetMovedClientPos(_CPos, nextRotation, 0.5f);

	//do move
	Move(nextCPos, nextRotation);

	return;
}

void MonsterObject::AI_ATTACK() {
	//check is time to move
	if (!_LastMoved.CheckElapsed(1000)) { return; }

	//check player is dead
	if (_LastAttackedPlayer->isDead()) {
		_LastAttackedPlayer = nullptr;
		_AIMode = AIMODE::RETURN;
		return;
	}

	//check player is in view
	USHORT rotation = 0;
	if (_TPos == _LastAttackedPlayer->_TPos) {
		rotation = GetClientRotation(_CPos, _LastAttackedPlayer->_CPos);
	}
	else {
		rotation = GetTileRotation(_TPos, _LastAttackedPlayer->_TPos);
	}
	float distance = GetTileDistance(_TPos, _LastAttackedPlayer->_TPos);
	bool isInView = distance <= 5.0f;

	//check last seen timer & quit attack mode when player is out of view range too long
	if (!isInView && _LastPlayerSeen.CheckElapsed(5000)) {
		_LastAttackedPlayer = nullptr;
		_AIMode = AIMODE::RETURN;
		return;
	}
	//reset last seen timer if player is in view range
	if (isInView) { _LastPlayerSeen.Reset(); }

	//Attack player when player is in attack range
	if (distance <= 1.0f) {
		Move(_CPos, rotation);
		bool isPlayerDead = _LastAttackedPlayer->TakeDamage(this, MON_DAMAGE);
		if (isPlayerDead) {
			_LastAttackedPlayer = nullptr;
			_AIMode = AIMODE::RETURN;
			return;
		}
	}
	//chase player when player is out of attack range
	else {
		ClientPos movedCPos = GetMovedClientPos(_CPos, rotation, 0.5f);
		Move(movedCPos, rotation);
	}

	return;
}

void MonsterObject::AI_RETURN() {
	_TargetedTPos = GetRandomTilePosInRange(_Center, _Range);
	_AIMode = AIMODE::ROAM;
	return;
}
