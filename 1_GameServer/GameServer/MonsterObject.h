#pragma once

class MonsterObject : public BaseObject {
private:
	enum class AIMODE {
		DEAD,
		ROAM,
		ATTACK,
		RETURN
	};
public:
	//Default stuff
	TilePos	_Center;
	int _Range = 0;

	//Common stuff
	int	_HP = 0;
	bool _isDead = false;

	//AI stuff
	AIMODE _AIMode = AIMODE::ROAM;
	PlayerObject* _LastAttackedPlayer = nullptr;
	ElapsedTimeChecker	_TimeToRevive;
	ElapsedTimeChecker _LastPlayerSeen;
	ElapsedTimeChecker _LastMoved;
	TilePos _TargetedTPos;

public:
	MonsterObject() : BaseObject(OBJTYPE::MONSTER) {}
	void Init(GameServer* gamesv, TilePos center, int range);

	bool isDead() { return _isDead; }
	virtual void Update();
	void Move(ClientPos dest, USHORT rotation);

	bool TakeDamage(PlayerObject* pobj, int damage);

	void AI_Init();
	void AI_DEAD();
	void AI_ROAM();
	void AI_ATTACK();
	void AI_RETURN();
};
