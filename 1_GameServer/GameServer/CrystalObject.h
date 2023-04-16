#pragma once

class CrystalObject : public BaseObject {
public:
	BYTE		_CrystalType = 1;
	ElapsedTimeChecker _TTL;

public:
	CrystalObject() : BaseObject(OBJTYPE::CRYSTAL) {}

	void Init(GameServer* gamesv, ClientPos cpos, BYTE CrystalType);
	virtual void Update();
};
