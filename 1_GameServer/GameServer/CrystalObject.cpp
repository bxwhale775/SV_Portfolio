#include "GameServer.h"
#include "CrystalObject.h"

void CrystalObject::Init(GameServer* gamesv, ClientPos cpos, BYTE CrystalType) {
	InitBaseObj(gamesv, cpos, 0);
	_CrystalType = CrystalType;
	_TTL.Reset();
	return;
}

void CrystalObject::Update() {
	if (_TTL.CheckElapsed(30000)) {
		//get around player list to send remove crystal object
		auto AroundSPosVec = GetAroundSectorPos(_SPos, _gsv->GetProps().ActiveSectorRange);
		auto AroundPlayerList = _gsv->GetPlayerObjListFromSPosVec(&AroundSPosVec);

		//send remove crystal res to around player
		for (auto& p : AroundPlayerList) {
			_gsv->GetRPCProxy().PACKET_CS_GAME_RES_REMOVE_OBJECT(p->_Nid, _ClientID);
		}
		//remove crystal
		_gsv->EraseObjectFromTile(this, _TPos);
		_gsv->EraseObjectFromMap(this);
		_gsv->GetCrystalObjPool()->Free(this);
	}
	return;
}
