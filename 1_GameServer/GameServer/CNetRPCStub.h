#pragma once
#include <functional>

using RPCID = WORD;
class CNetServer;

class CNetRPCStub : public SuperRPCStub {
private:
	std::unordered_map<RPCID, std::function<bool(CNetRPCStub&, NID, SerialBuffer*, SerialBufferIO*)>> __RPC__RPCDSPHMap;

private:
    virtual bool __RPC__DispatchRPC(NID nid, SerialBuffer* buf) {
        RPCID rpcid;
        SerialBufferIO bufio(buf);
        bufio >> rpcid;

        auto RPCDSPHIter = __RPC__RPCDSPHMap.find(rpcid);
        if (RPCDSPHIter == __RPC__RPCDSPHMap.end()) { return false; }
        auto RPCDSPHFunc = RPCDSPHIter->second;
        return RPCDSPHFunc(*this, nid, buf, &bufio);
    }
	virtual void __RPC__Init() {
		__RPC__RPCDSPHMap.insert({ 1000, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_SERVER });
		__RPC__RPCDSPHMap.insert({ 1001, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_REQ_LOGIN });
		__RPC__RPCDSPHMap.insert({ 1002, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_LOGIN });
		__RPC__RPCDSPHMap.insert({ 1003, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_REQ_CHARACTER_SELECT });
		__RPC__RPCDSPHMap.insert({ 1004, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_CHARACTER_SELECT });
		__RPC__RPCDSPHMap.insert({ 1005, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_CREATE_MY_CHARACTER });
		__RPC__RPCDSPHMap.insert({ 1006, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER });
		__RPC__RPCDSPHMap.insert({ 1007, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER });
		__RPC__RPCDSPHMap.insert({ 1008, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_REMOVE_OBJECT });
		__RPC__RPCDSPHMap.insert({ 1009, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_REQ_MOVE_CHARACTER });
		__RPC__RPCDSPHMap.insert({ 1010, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_MOVE_CHARACTER });
		__RPC__RPCDSPHMap.insert({ 1011, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_REQ_STOP_CHARACTER });
		__RPC__RPCDSPHMap.insert({ 1012, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_STOP_CHARACTER });
		__RPC__RPCDSPHMap.insert({ 1013, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_MOVE_MONSTER });
		__RPC__RPCDSPHMap.insert({ 1014, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_REQ_ATTACK1 });
		__RPC__RPCDSPHMap.insert({ 1015, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_ATTACK1 });
		__RPC__RPCDSPHMap.insert({ 1016, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_REQ_ATTACK2 });
		__RPC__RPCDSPHMap.insert({ 1017, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_ATTACK2 });
		__RPC__RPCDSPHMap.insert({ 1018, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_MONSTER_ATTACK });
		__RPC__RPCDSPHMap.insert({ 1019, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_DAMAGE });
		__RPC__RPCDSPHMap.insert({ 1020, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_MONSTER_DIE });
		__RPC__RPCDSPHMap.insert({ 1021, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_CREATE_CRISTAL });
		__RPC__RPCDSPHMap.insert({ 1022, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_REQ_PICK });
		__RPC__RPCDSPHMap.insert({ 1023, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_PICK });
		__RPC__RPCDSPHMap.insert({ 1024, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_REQ_SIT });
		__RPC__RPCDSPHMap.insert({ 1025, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_SIT });
		__RPC__RPCDSPHMap.insert({ 1026, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_PICK_CRISTAL });
		__RPC__RPCDSPHMap.insert({ 1027, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_PLAYER_HP });
		__RPC__RPCDSPHMap.insert({ 1028, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_PLAYER_DIE });
		__RPC__RPCDSPHMap.insert({ 1029, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_REQ_PLAYER_RESTART });
		__RPC__RPCDSPHMap.insert({ 1030, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_PLAYER_RESTART });
		__RPC__RPCDSPHMap.insert({ 5000, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_REQ_ECHO });
		__RPC__RPCDSPHMap.insert({ 5001, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_RES_ECHO });
		__RPC__RPCDSPHMap.insert({ 5002, &CNetRPCStub::__RPCDSPH__PACKET_CS_GAME_REQ_HEARTBEAT });
	}
	bool __RPCDSPH__PACKET_CS_GAME_SERVER(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		return PACKET_CS_GAME_SERVER(__RPC__nid);
	}
	bool __RPCDSPH__PACKET_CS_GAME_REQ_LOGIN(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 AccountNo;
		char SessionKey[64];
		int Version;
		*__RPC__pbufio >> AccountNo;
		for(int __RPC__idx_0=0; __RPC__idx_0<64; __RPC__idx_0++) {
			*__RPC__pbufio >> SessionKey[__RPC__idx_0];
		}
		*__RPC__pbufio >> Version;
		return PACKET_CS_GAME_REQ_LOGIN(__RPC__nid, AccountNo, SessionKey, Version);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_LOGIN(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		BYTE Status;
		INT64 AccountNo;
		*__RPC__pbufio >> Status;
		*__RPC__pbufio >> AccountNo;
		return PACKET_CS_GAME_RES_LOGIN(__RPC__nid, Status, AccountNo);
	}
	bool __RPCDSPH__PACKET_CS_GAME_REQ_CHARACTER_SELECT(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		BYTE CharacterType;
		*__RPC__pbufio >> CharacterType;
		return PACKET_CS_GAME_REQ_CHARACTER_SELECT(__RPC__nid, CharacterType);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_CHARACTER_SELECT(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		BYTE Status;
		*__RPC__pbufio >> Status;
		return PACKET_CS_GAME_RES_CHARACTER_SELECT(__RPC__nid, Status);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_CREATE_MY_CHARACTER(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		BYTE CharacterType;
		WCHAR Nickname[20];
		float PosX;
		float PosY;
		USHORT Rotation;
		int Cristal;
		int HP;
		INT64 Exp;
		USHORT Level;
		*__RPC__pbufio >> ClientID;
		*__RPC__pbufio >> CharacterType;
		for(int __RPC__idx_0=0; __RPC__idx_0<20; __RPC__idx_0++) {
			*__RPC__pbufio >> Nickname[__RPC__idx_0];
		}
		*__RPC__pbufio >> PosX;
		*__RPC__pbufio >> PosY;
		*__RPC__pbufio >> Rotation;
		*__RPC__pbufio >> Cristal;
		*__RPC__pbufio >> HP;
		*__RPC__pbufio >> Exp;
		*__RPC__pbufio >> Level;
		return PACKET_CS_GAME_RES_CREATE_MY_CHARACTER(__RPC__nid, ClientID, CharacterType, Nickname, PosX, PosY, Rotation, Cristal, HP, Exp, Level);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		BYTE CharacterType;
		WCHAR Nickname[20];
		float PosX;
		float PosY;
		USHORT Rotation;
		USHORT Level;
		BYTE Respawn;
		BYTE Sit;
		BYTE Die;
		*__RPC__pbufio >> ClientID;
		*__RPC__pbufio >> CharacterType;
		for(int __RPC__idx_0=0; __RPC__idx_0<20; __RPC__idx_0++) {
			*__RPC__pbufio >> Nickname[__RPC__idx_0];
		}
		*__RPC__pbufio >> PosX;
		*__RPC__pbufio >> PosY;
		*__RPC__pbufio >> Rotation;
		*__RPC__pbufio >> Level;
		*__RPC__pbufio >> Respawn;
		*__RPC__pbufio >> Sit;
		*__RPC__pbufio >> Die;
		return PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER(__RPC__nid, ClientID, CharacterType, Nickname, PosX, PosY, Rotation, Level, Respawn, Sit, Die);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		float PosX;
		float PosY;
		USHORT Rotation;
		BYTE Respawn;
		*__RPC__pbufio >> ClientID;
		*__RPC__pbufio >> PosX;
		*__RPC__pbufio >> PosY;
		*__RPC__pbufio >> Rotation;
		*__RPC__pbufio >> Respawn;
		return PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER(__RPC__nid, ClientID, PosX, PosY, Rotation, Respawn);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_REMOVE_OBJECT(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		*__RPC__pbufio >> ClientID;
		return PACKET_CS_GAME_RES_REMOVE_OBJECT(__RPC__nid, ClientID);
	}
	bool __RPCDSPH__PACKET_CS_GAME_REQ_MOVE_CHARACTER(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		float X;
		float Y;
		USHORT Rotation;
		BYTE VKey;
		BYTE HKey;
		*__RPC__pbufio >> ClientID;
		*__RPC__pbufio >> X;
		*__RPC__pbufio >> Y;
		*__RPC__pbufio >> Rotation;
		*__RPC__pbufio >> VKey;
		*__RPC__pbufio >> HKey;
		return PACKET_CS_GAME_REQ_MOVE_CHARACTER(__RPC__nid, ClientID, X, Y, Rotation, VKey, HKey);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_MOVE_CHARACTER(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		float X;
		float Y;
		USHORT Rotation;
		BYTE VKey;
		BYTE HKey;
		*__RPC__pbufio >> ClientID;
		*__RPC__pbufio >> X;
		*__RPC__pbufio >> Y;
		*__RPC__pbufio >> Rotation;
		*__RPC__pbufio >> VKey;
		*__RPC__pbufio >> HKey;
		return PACKET_CS_GAME_RES_MOVE_CHARACTER(__RPC__nid, ClientID, X, Y, Rotation, VKey, HKey);
	}
	bool __RPCDSPH__PACKET_CS_GAME_REQ_STOP_CHARACTER(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		float X;
		float Y;
		USHORT Rotation;
		*__RPC__pbufio >> ClientID;
		*__RPC__pbufio >> X;
		*__RPC__pbufio >> Y;
		*__RPC__pbufio >> Rotation;
		return PACKET_CS_GAME_REQ_STOP_CHARACTER(__RPC__nid, ClientID, X, Y, Rotation);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_STOP_CHARACTER(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		float X;
		float Y;
		USHORT Rotation;
		*__RPC__pbufio >> ClientID;
		*__RPC__pbufio >> X;
		*__RPC__pbufio >> Y;
		*__RPC__pbufio >> Rotation;
		return PACKET_CS_GAME_RES_STOP_CHARACTER(__RPC__nid, ClientID, X, Y, Rotation);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_MOVE_MONSTER(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		float X;
		float Y;
		USHORT Rotation;
		*__RPC__pbufio >> ClientID;
		*__RPC__pbufio >> X;
		*__RPC__pbufio >> Y;
		*__RPC__pbufio >> Rotation;
		return PACKET_CS_GAME_RES_MOVE_MONSTER(__RPC__nid, ClientID, X, Y, Rotation);
	}
	bool __RPCDSPH__PACKET_CS_GAME_REQ_ATTACK1(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		*__RPC__pbufio >> ClientID;
		return PACKET_CS_GAME_REQ_ATTACK1(__RPC__nid, ClientID);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_ATTACK1(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		*__RPC__pbufio >> ClientID;
		return PACKET_CS_GAME_RES_ATTACK1(__RPC__nid, ClientID);
	}
	bool __RPCDSPH__PACKET_CS_GAME_REQ_ATTACK2(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		*__RPC__pbufio >> ClientID;
		return PACKET_CS_GAME_REQ_ATTACK2(__RPC__nid, ClientID);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_ATTACK2(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		*__RPC__pbufio >> ClientID;
		return PACKET_CS_GAME_RES_ATTACK2(__RPC__nid, ClientID);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_MONSTER_ATTACK(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		*__RPC__pbufio >> ClientID;
		return PACKET_CS_GAME_RES_MONSTER_ATTACK(__RPC__nid, ClientID);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_DAMAGE(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 AttackClientID;
		INT64 TargetClientID;
		int Damage;
		*__RPC__pbufio >> AttackClientID;
		*__RPC__pbufio >> TargetClientID;
		*__RPC__pbufio >> Damage;
		return PACKET_CS_GAME_RES_DAMAGE(__RPC__nid, AttackClientID, TargetClientID, Damage);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_MONSTER_DIE(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 MonsterClientID;
		*__RPC__pbufio >> MonsterClientID;
		return PACKET_CS_GAME_RES_MONSTER_DIE(__RPC__nid, MonsterClientID);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_CREATE_CRISTAL(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 CristalClientID;
		BYTE byCristalType;
		float fPosX;
		float fPosY;
		*__RPC__pbufio >> CristalClientID;
		*__RPC__pbufio >> byCristalType;
		*__RPC__pbufio >> fPosX;
		*__RPC__pbufio >> fPosY;
		return PACKET_CS_GAME_RES_CREATE_CRISTAL(__RPC__nid, CristalClientID, byCristalType, fPosX, fPosY);
	}
	bool __RPCDSPH__PACKET_CS_GAME_REQ_PICK(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		*__RPC__pbufio >> ClientID;
		return PACKET_CS_GAME_REQ_PICK(__RPC__nid, ClientID);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_PICK(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		*__RPC__pbufio >> ClientID;
		return PACKET_CS_GAME_RES_PICK(__RPC__nid, ClientID);
	}
	bool __RPCDSPH__PACKET_CS_GAME_REQ_SIT(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		*__RPC__pbufio >> ClientID;
		return PACKET_CS_GAME_REQ_SIT(__RPC__nid, ClientID);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_SIT(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		*__RPC__pbufio >> ClientID;
		return PACKET_CS_GAME_RES_SIT(__RPC__nid, ClientID);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_PICK_CRISTAL(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		INT64 CristalClientID;
		int AmountCristal;
		*__RPC__pbufio >> ClientID;
		*__RPC__pbufio >> CristalClientID;
		*__RPC__pbufio >> AmountCristal;
		return PACKET_CS_GAME_RES_PICK_CRISTAL(__RPC__nid, ClientID, CristalClientID, AmountCristal);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_PLAYER_HP(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT HP;
		*__RPC__pbufio >> HP;
		return PACKET_CS_GAME_RES_PLAYER_HP(__RPC__nid, HP);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_PLAYER_DIE(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 ClientID;
		int MinusCristal;
		*__RPC__pbufio >> ClientID;
		*__RPC__pbufio >> MinusCristal;
		return PACKET_CS_GAME_RES_PLAYER_DIE(__RPC__nid, ClientID, MinusCristal);
	}
	bool __RPCDSPH__PACKET_CS_GAME_REQ_PLAYER_RESTART(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		return PACKET_CS_GAME_REQ_PLAYER_RESTART(__RPC__nid);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_PLAYER_RESTART(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		return PACKET_CS_GAME_RES_PLAYER_RESTART(__RPC__nid);
	}
	bool __RPCDSPH__PACKET_CS_GAME_REQ_ECHO(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 AccountoNo;
		LONGLONG SendTick;
		*__RPC__pbufio >> AccountoNo;
		*__RPC__pbufio >> SendTick;
		return PACKET_CS_GAME_REQ_ECHO(__RPC__nid, AccountoNo, SendTick);
	}
	bool __RPCDSPH__PACKET_CS_GAME_RES_ECHO(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		INT64 AccountoNo;
		LONGLONG SendTick;
		*__RPC__pbufio >> AccountoNo;
		*__RPC__pbufio >> SendTick;
		return PACKET_CS_GAME_RES_ECHO(__RPC__nid, AccountoNo, SendTick);
	}
	bool __RPCDSPH__PACKET_CS_GAME_REQ_HEARTBEAT(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) { 
		return PACKET_CS_GAME_REQ_HEARTBEAT(__RPC__nid);
	}
protected:
	virtual bool PACKET_CS_GAME_SERVER(NID __RPC__nid) { return false; }
	virtual bool PACKET_CS_GAME_REQ_LOGIN(NID __RPC__nid, INT64 AccountNo, char (&SessionKey)[64], int Version) { return false; }
	virtual bool PACKET_CS_GAME_RES_LOGIN(NID __RPC__nid, BYTE Status, INT64 AccountNo) { return false; }
	virtual bool PACKET_CS_GAME_REQ_CHARACTER_SELECT(NID __RPC__nid, BYTE CharacterType) { return false; }
	virtual bool PACKET_CS_GAME_RES_CHARACTER_SELECT(NID __RPC__nid, BYTE Status) { return false; }
	virtual bool PACKET_CS_GAME_RES_CREATE_MY_CHARACTER(NID __RPC__nid, INT64 ClientID, BYTE CharacterType, WCHAR (&Nickname)[20], float PosX, float PosY, USHORT Rotation, int Cristal, int HP, INT64 Exp, USHORT Level) { return false; }
	virtual bool PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER(NID __RPC__nid, INT64 ClientID, BYTE CharacterType, WCHAR (&Nickname)[20], float PosX, float PosY, USHORT Rotation, USHORT Level, BYTE Respawn, BYTE Sit, BYTE Die) { return false; }
	virtual bool PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER(NID __RPC__nid, INT64 ClientID, float PosX, float PosY, USHORT Rotation, BYTE Respawn) { return false; }
	virtual bool PACKET_CS_GAME_RES_REMOVE_OBJECT(NID __RPC__nid, INT64 ClientID) { return false; }
	virtual bool PACKET_CS_GAME_REQ_MOVE_CHARACTER(NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation, BYTE VKey, BYTE HKey) { return false; }
	virtual bool PACKET_CS_GAME_RES_MOVE_CHARACTER(NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation, BYTE VKey, BYTE HKey) { return false; }
	virtual bool PACKET_CS_GAME_REQ_STOP_CHARACTER(NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation) { return false; }
	virtual bool PACKET_CS_GAME_RES_STOP_CHARACTER(NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation) { return false; }
	virtual bool PACKET_CS_GAME_RES_MOVE_MONSTER(NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation) { return false; }
	virtual bool PACKET_CS_GAME_REQ_ATTACK1(NID __RPC__nid, INT64 ClientID) { return false; }
	virtual bool PACKET_CS_GAME_RES_ATTACK1(NID __RPC__nid, INT64 ClientID) { return false; }
	virtual bool PACKET_CS_GAME_REQ_ATTACK2(NID __RPC__nid, INT64 ClientID) { return false; }
	virtual bool PACKET_CS_GAME_RES_ATTACK2(NID __RPC__nid, INT64 ClientID) { return false; }
	virtual bool PACKET_CS_GAME_RES_MONSTER_ATTACK(NID __RPC__nid, INT64 ClientID) { return false; }
	virtual bool PACKET_CS_GAME_RES_DAMAGE(NID __RPC__nid, INT64 AttackClientID, INT64 TargetClientID, int Damage) { return false; }
	virtual bool PACKET_CS_GAME_RES_MONSTER_DIE(NID __RPC__nid, INT64 MonsterClientID) { return false; }
	virtual bool PACKET_CS_GAME_RES_CREATE_CRISTAL(NID __RPC__nid, INT64 CristalClientID, BYTE byCristalType, float fPosX, float fPosY) { return false; }
	virtual bool PACKET_CS_GAME_REQ_PICK(NID __RPC__nid, INT64 ClientID) { return false; }
	virtual bool PACKET_CS_GAME_RES_PICK(NID __RPC__nid, INT64 ClientID) { return false; }
	virtual bool PACKET_CS_GAME_REQ_SIT(NID __RPC__nid, INT64 ClientID) { return false; }
	virtual bool PACKET_CS_GAME_RES_SIT(NID __RPC__nid, INT64 ClientID) { return false; }
	virtual bool PACKET_CS_GAME_RES_PICK_CRISTAL(NID __RPC__nid, INT64 ClientID, INT64 CristalClientID, int AmountCristal) { return false; }
	virtual bool PACKET_CS_GAME_RES_PLAYER_HP(NID __RPC__nid, INT HP) { return false; }
	virtual bool PACKET_CS_GAME_RES_PLAYER_DIE(NID __RPC__nid, INT64 ClientID, int MinusCristal) { return false; }
	virtual bool PACKET_CS_GAME_REQ_PLAYER_RESTART(NID __RPC__nid) { return false; }
	virtual bool PACKET_CS_GAME_RES_PLAYER_RESTART(NID __RPC__nid) { return false; }
	virtual bool PACKET_CS_GAME_REQ_ECHO(NID __RPC__nid, INT64 AccountoNo, LONGLONG SendTick) { return false; }
	virtual bool PACKET_CS_GAME_RES_ECHO(NID __RPC__nid, INT64 AccountoNo, LONGLONG SendTick) { return false; }
	virtual bool PACKET_CS_GAME_REQ_HEARTBEAT(NID __RPC__nid) { return false; }
};

#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_SERVER NID __RPC__nid
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_LOGIN NID __RPC__nid, INT64 AccountNo, char (&SessionKey)[64], int Version
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_LOGIN NID __RPC__nid, BYTE Status, INT64 AccountNo
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_CHARACTER_SELECT NID __RPC__nid, BYTE CharacterType
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_CHARACTER_SELECT NID __RPC__nid, BYTE Status
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_CREATE_MY_CHARACTER NID __RPC__nid, INT64 ClientID, BYTE CharacterType, WCHAR (&Nickname)[20], float PosX, float PosY, USHORT Rotation, int Cristal, int HP, INT64 Exp, USHORT Level
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER NID __RPC__nid, INT64 ClientID, BYTE CharacterType, WCHAR (&Nickname)[20], float PosX, float PosY, USHORT Rotation, USHORT Level, BYTE Respawn, BYTE Sit, BYTE Die
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER NID __RPC__nid, INT64 ClientID, float PosX, float PosY, USHORT Rotation, BYTE Respawn
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_REMOVE_OBJECT NID __RPC__nid, INT64 ClientID
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_MOVE_CHARACTER NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation, BYTE VKey, BYTE HKey
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_MOVE_CHARACTER NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation, BYTE VKey, BYTE HKey
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_STOP_CHARACTER NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_STOP_CHARACTER NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_MOVE_MONSTER NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK1 NID __RPC__nid, INT64 ClientID
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_ATTACK1 NID __RPC__nid, INT64 ClientID
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK2 NID __RPC__nid, INT64 ClientID
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_ATTACK2 NID __RPC__nid, INT64 ClientID
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_MONSTER_ATTACK NID __RPC__nid, INT64 ClientID
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_DAMAGE NID __RPC__nid, INT64 AttackClientID, INT64 TargetClientID, int Damage
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_MONSTER_DIE NID __RPC__nid, INT64 MonsterClientID
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_CREATE_CRISTAL NID __RPC__nid, INT64 CristalClientID, BYTE byCristalType, float fPosX, float fPosY
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_PICK NID __RPC__nid, INT64 ClientID
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_PICK NID __RPC__nid, INT64 ClientID
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_SIT NID __RPC__nid, INT64 ClientID
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_SIT NID __RPC__nid, INT64 ClientID
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_PICK_CRISTAL NID __RPC__nid, INT64 ClientID, INT64 CristalClientID, int AmountCristal
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_PLAYER_HP NID __RPC__nid, INT HP
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_PLAYER_DIE NID __RPC__nid, INT64 ClientID, int MinusCristal
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_PLAYER_RESTART NID __RPC__nid
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_PLAYER_RESTART NID __RPC__nid
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ECHO NID __RPC__nid, INT64 AccountoNo, LONGLONG SendTick
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_RES_ECHO NID __RPC__nid, INT64 AccountoNo, LONGLONG SendTick
#define dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_HEARTBEAT NID __RPC__nid

#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_SERVER __RPC__nid
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_LOGIN __RPC__nid, AccountNo, SessionKey, Version
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_LOGIN __RPC__nid, Status, AccountNo
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_CHARACTER_SELECT __RPC__nid, CharacterType
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_CHARACTER_SELECT __RPC__nid, Status
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_CREATE_MY_CHARACTER __RPC__nid, ClientID, CharacterType, Nickname, PosX, PosY, Rotation, Cristal, HP, Exp, Level
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER __RPC__nid, ClientID, CharacterType, Nickname, PosX, PosY, Rotation, Level, Respawn, Sit, Die
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER __RPC__nid, ClientID, PosX, PosY, Rotation, Respawn
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_REMOVE_OBJECT __RPC__nid, ClientID
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_MOVE_CHARACTER __RPC__nid, ClientID, X, Y, Rotation, VKey, HKey
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_MOVE_CHARACTER __RPC__nid, ClientID, X, Y, Rotation, VKey, HKey
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_STOP_CHARACTER __RPC__nid, ClientID, X, Y, Rotation
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_STOP_CHARACTER __RPC__nid, ClientID, X, Y, Rotation
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_MOVE_MONSTER __RPC__nid, ClientID, X, Y, Rotation
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK1 __RPC__nid, ClientID
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_ATTACK1 __RPC__nid, ClientID
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK2 __RPC__nid, ClientID
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_ATTACK2 __RPC__nid, ClientID
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_MONSTER_ATTACK __RPC__nid, ClientID
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_DAMAGE __RPC__nid, AttackClientID, TargetClientID, Damage
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_MONSTER_DIE __RPC__nid, MonsterClientID
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_CREATE_CRISTAL __RPC__nid, CristalClientID, byCristalType, fPosX, fPosY
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_PICK __RPC__nid, ClientID
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_PICK __RPC__nid, ClientID
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_SIT __RPC__nid, ClientID
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_SIT __RPC__nid, ClientID
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_PICK_CRISTAL __RPC__nid, ClientID, CristalClientID, AmountCristal
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_PLAYER_HP __RPC__nid, HP
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_PLAYER_DIE __RPC__nid, ClientID, MinusCristal
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_PLAYER_RESTART __RPC__nid
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_PLAYER_RESTART __RPC__nid
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_ECHO __RPC__nid, AccountoNo, SendTick
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_RES_ECHO __RPC__nid, AccountoNo, SendTick
#define dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_HEARTBEAT __RPC__nid
