#include "GameServer.h"
#include "MonsterObject.h"
#include "PlayerObject.h"

constexpr DWORD PLAYER_TIMEOUT = 40000;

void PlayerObject::Init(GameServer* gamesv, NID sessID, const PlayerData* pdata) {
	//init member vars
	InitBaseObj(gamesv, pdata->CPos, pdata->Rotation);

	_Nid = sessID;
	_HP = pdata->HP;
	_Crystal = pdata->Crystal;
	_Exp = pdata->Exp;
	_Level = pdata->Level;
	wcscpy_s(_Nickname, pdata->Nickname);
	_AccountNo = pdata->AccountNo;
	_Chartype = pdata->Chartype;
	_isDead = 0;
	_isSit = 0;
	_prevHP = 0;
	_SitSec = 0;
	UpdateLastRecvTime();

	return;
}

void PlayerObject::UpdateLastRecvTime() {
	_LastRecvTime = timeGetTime();
	return;
}

void PlayerObject::Update() {
	if (timeGetTime() - _LastRecvTime > PLAYER_TIMEOUT) {
		_gsv->GetNetServer().Disconnect(_Nid);
		return;
	}
	if (_gsv->isOnTickBound() && isSit()) {
		_HP += RECOVERY_HP;
		_HP = min(_HP, INITIAL_HP);
		_SitSec++;
		_gsv->GetRPCProxy().PACKET_CS_GAME_RES_PLAYER_HP(_Nid, _HP);
	}
	return;
}

bool PlayerObject::TakeDamage(MonsterObject* mobj, int damage) {
	if (isDead()) { return false; }

	_isSit = false;

	//give damage to player
	_HP -= damage;
	if (_HP <= 0) {				//If player's HP runs out
		_isDead = true;			//Mark as Dead player
		_HP = INITIAL_HP;		//reset HP
		_Crystal -= 1000;		//decrease crystal
		_gsv->GetAsyncGameDB().Write_Die(_AccountNo, _CPos, _Rotation, _Crystal, _HP);

		//broadcast player's dead to around players
		auto aroundsector = GetAroundSectorPos(_SPos, _gsv->GetProps().ActiveSectorRange);
		auto aroundplayer = _gsv->GetPlayerObjListFromSPosVec(&aroundsector);
		for (auto p : aroundplayer) {
			_gsv->GetRPCProxy().PACKET_CS_GAME_RES_PLAYER_DIE(p->_Nid,
				_ClientID, 1000);
		}

		return true;			//alert that this player is dead
	}
	_gsv->GetRPCProxy().PACKET_CS_GAME_RES_PLAYER_HP(_Nid, _HP);
	return false;
}

void PlayerObject::SetSit(bool on) {
	if (on) {
		if (!_isSit) {
			_isSit = true;
			_prevHP = _HP;
			_SitSec = 0;
			_gsv->GetAsyncGameDB().Write_Heal(_AccountNo, _prevHP, _HP, _SitSec);
		}
	}
	else {
		if (_isSit) {
			_isSit = false;
			_gsv->GetAsyncGameDB().Write_Heal(_AccountNo, _prevHP, _HP, _SitSec);
		}
	}
}
