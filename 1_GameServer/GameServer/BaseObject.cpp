#include "GameServer.h"
#include "BaseObject.h"

void BaseObject::InitBaseObj(GameServer* gamesv, ClientPos cpos, USHORT rotation) {
	_gsv = gamesv;
	_ClientID = _gsv->GetNewClientID();
	UpdatePos(cpos);
	UpdateRotation(rotation);
	return;
}

void BaseObject::UpdatePos(ClientPos cpos) {
	_CPos = cpos;
	_TPos = ClientPosToTilePos(_CPos);
	_SPos = TilePosToSectorPos(_TPos);
	return;
}

void BaseObject::UpdateRotation(USHORT rotation) {
	_Rotation = rotation;
	return;
}
