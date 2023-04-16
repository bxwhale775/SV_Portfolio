#pragma once

using RPCID = WORD;
class CNetServer;

class CNetRPCProxy : public SuperRPCProxy {
public:
	bool PACKET_CS_GAME_SERVER(NID __RPC__nid) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1000;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_REQ_LOGIN(NID __RPC__nid, INT64 AccountNo, char (&SessionKey)[64], int Version) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1001;
		__RPC__bufio << AccountNo;
		for(int __RPC__idx_0=0; __RPC__idx_0<64; __RPC__idx_0++) {
			__RPC__bufio << SessionKey[__RPC__idx_0];
		}
		__RPC__bufio << Version;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_LOGIN(NID __RPC__nid, BYTE Status, INT64 AccountNo) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1002;
		__RPC__bufio << Status;
		__RPC__bufio << AccountNo;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_REQ_CHARACTER_SELECT(NID __RPC__nid, BYTE CharacterType) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1003;
		__RPC__bufio << CharacterType;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_CHARACTER_SELECT(NID __RPC__nid, BYTE Status) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1004;
		__RPC__bufio << Status;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_CREATE_MY_CHARACTER(NID __RPC__nid, INT64 ClientID, BYTE CharacterType, WCHAR (&Nickname)[20], float PosX, float PosY, USHORT Rotation, int Cristal, int HP, INT64 Exp, USHORT Level) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1005;
		__RPC__bufio << ClientID;
		__RPC__bufio << CharacterType;
		for(int __RPC__idx_0=0; __RPC__idx_0<20; __RPC__idx_0++) {
			__RPC__bufio << Nickname[__RPC__idx_0];
		}
		__RPC__bufio << PosX;
		__RPC__bufio << PosY;
		__RPC__bufio << Rotation;
		__RPC__bufio << Cristal;
		__RPC__bufio << HP;
		__RPC__bufio << Exp;
		__RPC__bufio << Level;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER(NID __RPC__nid, INT64 ClientID, BYTE CharacterType, WCHAR (&Nickname)[20], float PosX, float PosY, USHORT Rotation, USHORT Level, BYTE Respawn, BYTE Sit, BYTE Die) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1006;
		__RPC__bufio << ClientID;
		__RPC__bufio << CharacterType;
		for(int __RPC__idx_0=0; __RPC__idx_0<20; __RPC__idx_0++) {
			__RPC__bufio << Nickname[__RPC__idx_0];
		}
		__RPC__bufio << PosX;
		__RPC__bufio << PosY;
		__RPC__bufio << Rotation;
		__RPC__bufio << Level;
		__RPC__bufio << Respawn;
		__RPC__bufio << Sit;
		__RPC__bufio << Die;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER(NID __RPC__nid, INT64 ClientID, float PosX, float PosY, USHORT Rotation, BYTE Respawn) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1007;
		__RPC__bufio << ClientID;
		__RPC__bufio << PosX;
		__RPC__bufio << PosY;
		__RPC__bufio << Rotation;
		__RPC__bufio << Respawn;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_REMOVE_OBJECT(NID __RPC__nid, INT64 ClientID) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1008;
		__RPC__bufio << ClientID;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_REQ_MOVE_CHARACTER(NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation, BYTE VKey, BYTE HKey) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1009;
		__RPC__bufio << ClientID;
		__RPC__bufio << X;
		__RPC__bufio << Y;
		__RPC__bufio << Rotation;
		__RPC__bufio << VKey;
		__RPC__bufio << HKey;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_MOVE_CHARACTER(NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation, BYTE VKey, BYTE HKey) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1010;
		__RPC__bufio << ClientID;
		__RPC__bufio << X;
		__RPC__bufio << Y;
		__RPC__bufio << Rotation;
		__RPC__bufio << VKey;
		__RPC__bufio << HKey;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_REQ_STOP_CHARACTER(NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1011;
		__RPC__bufio << ClientID;
		__RPC__bufio << X;
		__RPC__bufio << Y;
		__RPC__bufio << Rotation;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_STOP_CHARACTER(NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1012;
		__RPC__bufio << ClientID;
		__RPC__bufio << X;
		__RPC__bufio << Y;
		__RPC__bufio << Rotation;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_MOVE_MONSTER(NID __RPC__nid, INT64 ClientID, float X, float Y, USHORT Rotation) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1013;
		__RPC__bufio << ClientID;
		__RPC__bufio << X;
		__RPC__bufio << Y;
		__RPC__bufio << Rotation;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_REQ_ATTACK1(NID __RPC__nid, INT64 ClientID) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1014;
		__RPC__bufio << ClientID;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_ATTACK1(NID __RPC__nid, INT64 ClientID) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1015;
		__RPC__bufio << ClientID;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_REQ_ATTACK2(NID __RPC__nid, INT64 ClientID) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1016;
		__RPC__bufio << ClientID;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_ATTACK2(NID __RPC__nid, INT64 ClientID) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1017;
		__RPC__bufio << ClientID;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_MONSTER_ATTACK(NID __RPC__nid, INT64 ClientID) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1018;
		__RPC__bufio << ClientID;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_DAMAGE(NID __RPC__nid, INT64 AttackClientID, INT64 TargetClientID, int Damage) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1019;
		__RPC__bufio << AttackClientID;
		__RPC__bufio << TargetClientID;
		__RPC__bufio << Damage;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_MONSTER_DIE(NID __RPC__nid, INT64 MonsterClientID) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1020;
		__RPC__bufio << MonsterClientID;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_CREATE_CRISTAL(NID __RPC__nid, INT64 CristalClientID, BYTE byCristalType, float fPosX, float fPosY) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1021;
		__RPC__bufio << CristalClientID;
		__RPC__bufio << byCristalType;
		__RPC__bufio << fPosX;
		__RPC__bufio << fPosY;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_REQ_PICK(NID __RPC__nid, INT64 ClientID) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1022;
		__RPC__bufio << ClientID;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_PICK(NID __RPC__nid, INT64 ClientID) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1023;
		__RPC__bufio << ClientID;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_REQ_SIT(NID __RPC__nid, INT64 ClientID) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1024;
		__RPC__bufio << ClientID;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_SIT(NID __RPC__nid, INT64 ClientID) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1025;
		__RPC__bufio << ClientID;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_PICK_CRISTAL(NID __RPC__nid, INT64 ClientID, INT64 CristalClientID, int AmountCristal) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1026;
		__RPC__bufio << ClientID;
		__RPC__bufio << CristalClientID;
		__RPC__bufio << AmountCristal;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_PLAYER_HP(NID __RPC__nid, INT HP) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1027;
		__RPC__bufio << HP;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_PLAYER_DIE(NID __RPC__nid, INT64 ClientID, int MinusCristal) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1028;
		__RPC__bufio << ClientID;
		__RPC__bufio << MinusCristal;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_REQ_PLAYER_RESTART(NID __RPC__nid) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1029;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_PLAYER_RESTART(NID __RPC__nid) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)1030;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_REQ_ECHO(NID __RPC__nid, INT64 AccountoNo, LONGLONG SendTick) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)5000;
		__RPC__bufio << AccountoNo;
		__RPC__bufio << SendTick;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_RES_ECHO(NID __RPC__nid, INT64 AccountoNo, LONGLONG SendTick) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)5001;
		__RPC__bufio << AccountoNo;
		__RPC__bufio << SendTick;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
	bool PACKET_CS_GAME_REQ_HEARTBEAT(NID __RPC__nid) {
		bool __RPC__ret;
		SerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();
		SerialBufferIO __RPC__bufio(__RPC__buf);
		__RPC__bufio << (RPCID)5002;
		__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);
		__RPC__sv->FreePacket(__RPC__buf);
		return __RPC__ret;
	}
};
