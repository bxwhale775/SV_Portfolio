#pragma once
struct PlayerData;
class MonsterObject;

class PlayerObject : public BaseObject {
public:
	NID			_Nid = NETSV_INVALID_NID;
	int			_HP = 0;
	int			_Crystal = 0;
	INT64		_Exp = 0;
	USHORT		_Level = 0;
	BYTE		_Chartype = 0;
	BYTE		_isDead = 0;
	BYTE		_isSit = 0;

	WCHAR		_Nickname[20] = { '\0', };
	INT64		_AccountNo = 0;
	DWORD		_LastRecvTime = 0;

	int			_prevHP = 0;
	int			_SitSec = 0;

public:
	PlayerObject() : BaseObject(OBJTYPE::PLAYER) {}
	void Init(GameServer* gamesv, NID sessID, const PlayerData* pdata);

	bool isDead() { return (bool)_isDead; }
	bool isSit() { return (bool)_isSit; }
	void UpdateLastRecvTime();

	virtual void Update();

	bool TakeDamage(MonsterObject* mobj, int damage);
	void SetSit(bool on);
};