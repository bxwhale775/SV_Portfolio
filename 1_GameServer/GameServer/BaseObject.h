#pragma once

class GameServer;

enum class OBJTYPE {
	NULLTYPE,
	PLAYER,
	MONSTER,
	CRYSTAL
};

class BaseObject {
public:
	const OBJTYPE		_ObjType = OBJTYPE::NULLTYPE;
	GameServer*			_gsv = nullptr;
	INT64				_ClientID = 0;
						
	ClientPos			_CPos;
	TilePos				_TPos;
	SectorPos			_SPos;
	USHORT				_Rotation = 0;

public:
	BaseObject() = default;
	BaseObject(OBJTYPE objtype) : _ObjType(objtype) {};

	void InitBaseObj(GameServer* gamesv, ClientPos cpos, USHORT rotation);

	void UpdatePos(ClientPos cpos);
	void UpdateRotation(USHORT rotation);

	virtual void Update() { return; };
};

