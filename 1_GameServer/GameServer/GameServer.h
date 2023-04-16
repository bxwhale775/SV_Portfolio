#pragma once
#include <vector>
#include <list>
#include "NetSV-include/CNetServer.h"
#include "Pos.h"
#include "DBConnMgr_MT.h"
#include "AsyncDBUpdater.h"
#include "ElapsedTimeChecker.h"
#include "BaseObject.h"
#include "PlayerObject.h"
#include "MonsterObject.h"
#include "CrystalObject.h"
#include "Tile.h"
#include "Sector.h"
#include "Range.h"
#include "GameUtilFunc.h"
#include "CNetRPCProxy.h"
#include "CNetRPCStub.h"
#include "PDHCounter.h"
#include "MonitorClientMgr.h"

#pragma comment(lib, "NetSV-include/lib/NetServerLib.lib")


struct GameSVProps : public CNetServer_Properties {
	const char* LoginDB_HostName = "tcp://127.0.0.1:3306/accountdb";
	const char* LoginDB_UserName = "root";
	const char* LoginDB_Password = "12345678";

	const char* GameDB_HostName = "tcp://127.0.0.1:3306/gamedb";
	const char* GameDB_UserName = "root";
	const char* GameDB_Password = "12345678";

	const char* Redis_IP = "127.0.0.1";
	size_t Redis_Port = 6379;

	const WCHAR* MonitorSV_IP = L"127.0.0.1";
	u_short MonitorSV_Port = 12300;

	const WCHAR* GameSVLogger = L"GodDammbugSV-log.txt";

	int Monster1Num = 100;
	int Monster2Num = 100;
	int Monster3Num = 100;
	int Monster4Num = 100;
	int Monster5Num = 100;
	int Monster6Num = 100;
	int Monster7Num = 100;
	int ActiveSectorRange = 1;
};
struct PlayerData {
	WCHAR Nickname[20] = { '\0', };
	INT64 AccountNo = 0;
	BYTE Chartype = 0;
	ClientPos CPos;
	USHORT Rotation = 0;
	int Crystal = 0;
	int HP = 0;
	INT64 Exp = 0;
	USHORT Level = 0;
};

class GameServer {
private:
	class NetCallback : public Interface_CNetServer_Callback {
		virtual bool OnAccept(const WCHAR* ip, const u_short port) {
			return true;
		}
		virtual bool OnJoin(const NID sessID) {
			_sv->MoveContents(sessID, 1, nullptr);
			return true;
		}
	};
	class Con_Auth : public NetContents {
	public:
		GameServer* _gsv;
	protected:
		virtual bool OnContentsThreadStart(DWORD ThrID) {
			_gsv->ConAuth_OnThrStart(ThrID);
			return true;
		}
		virtual bool OnContentsThreadEnd(DWORD ThrID) {
			_gsv->ConAuth_OnThrEnd(ThrID);
			return true;
		}
		virtual bool OnContentsJoin(NID sessID, CID departCID, const PVOID data) {
			_gsv->ConAuth_OnJoin(sessID, departCID, data);
			return true;
		}
		virtual bool OnContentsLeave(NID sessID, CID destCID) {
			_gsv->ConAuth_OnLeave(sessID, destCID);
			return true;
		}
		virtual bool OnContentsRecv(NID sessID, SerialBuffer* databuf) {
			printf("May Wrong Recv at Contents Auth\n");
			return true;
		}
		virtual bool OnContentsUpdate() {

			return true;
		}
		virtual bool OnContentsDisconnect(NID sessID) {
			_gsv->ConAuth_OnDisconnect(sessID);
			return true;
		}
	public:
		UINT GetTickCount() { return _cdb->TickCount; }
		UINT GetTPS() { return _cdb->TPS; }
	};
	class Con_Game : public NetContents {
	public:
		GameServer* _gsv;
	protected:
		virtual bool OnContentsThreadStart(DWORD ThrID) {
			_gsv->ConGame_OnThrStart(ThrID);
			return true;
		}
		virtual bool OnContentsThreadEnd(DWORD ThrID) {
			_gsv->ConGame_OnThrEnd(ThrID);
			return true;
		}
		virtual bool OnContentsJoin(NID sessID, CID departCID, const PVOID data) {
			_gsv->ConGame_OnJoin(sessID, departCID, data);
			return true;
		}
		virtual bool OnContentsLeave(NID sessID, CID destCID) {
			_gsv->ConGame_OnLeave(sessID, destCID);
			return true;
		}
		virtual bool OnContentsRecv(NID sessID, SerialBuffer* databuf) {
			printf("May Wrong Recv at Contents Game\n");
			return true;
		}
		virtual bool OnContentsUpdate() {
			_gsv->ConGame_OnUpdate();
			return true;
		}
		virtual bool OnContentsDisconnect(NID sessID) {
			_gsv->ConGame_OnDisconnect(sessID);
			return true;
		}
	public:
		UINT GetTickCount() { return _cdb->TickCount; }
		UINT GetTPS() { return _cdb->TPS; }
	};
	class Con_Auth_Stub : public CNetRPCStub {
	public:
		GameServer* _gsv;
	protected:
		virtual bool PACKET_CS_GAME_REQ_LOGIN(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_LOGIN) {
			_gsv->ConAuth_REQ_LOGIN(dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_LOGIN);
			return true;
		}
		virtual bool PACKET_CS_GAME_REQ_CHARACTER_SELECT(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_CHARACTER_SELECT) {
			_gsv->ConAuth_REQ_CHARACTER_SELECT(dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_CHARACTER_SELECT);
			return true;
		}
	};
	class Con_Game_Stub : public CNetRPCStub {
	public:
		GameServer* _gsv;
	protected:
		virtual bool PACKET_CS_GAME_REQ_MOVE_CHARACTER(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_MOVE_CHARACTER) {
			_gsv->ConGame_REQ_MOVE_CHARACTER(dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_MOVE_CHARACTER);
			return true;
		}
		virtual bool PACKET_CS_GAME_REQ_STOP_CHARACTER(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_STOP_CHARACTER) {
			_gsv->ConGame_REQ_STOP_CHARACTER(dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_STOP_CHARACTER);
			return true;
		}
		virtual bool PACKET_CS_GAME_REQ_ATTACK1(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK1) {
			_gsv->ConGame_REQ_ATTACK1(dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK1);
			return true;
		}
		virtual bool PACKET_CS_GAME_REQ_ATTACK2(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK2) {
			_gsv->ConGame_REQ_ATTACK2(dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK2);
			return true;
		}
		virtual bool PACKET_CS_GAME_REQ_PICK(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_PICK) {
			_gsv->ConGame_REQ_PICK(dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_PICK);
			return true;
		}
		virtual bool PACKET_CS_GAME_REQ_SIT(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_SIT) {
			_gsv->ConGame_REQ_SIT(dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_SIT);
			return true;
		}
		virtual bool PACKET_CS_GAME_REQ_PLAYER_RESTART(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_PLAYER_RESTART) {
			_gsv->ConGame_REQ_PLAYER_RESTART(dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_PLAYER_RESTART);
			return true;
		}
		virtual bool PACKET_CS_GAME_REQ_ECHO(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ECHO) {
			_gsv->ConGame_REQ_ECHO(dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_ECHO);
			return true;
		}
		virtual bool PACKET_CS_GAME_REQ_HEARTBEAT(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_HEARTBEAT) {
			_gsv->ConGame_REQ_HEARTBEAT(dfRPC_STUB_ARG_CNetRPCStub_PACKET_CS_GAME_REQ_HEARTBEAT);
			return true;
		}
	};
private:
	CNetServer _nsv;
	UINT64 _LastGenClientID = 0;

	Con_Auth _CON_Auth;
	Con_Game _CON_Game;

	CNetRPCProxy _Proxy;
	Con_Auth_Stub _StubAuth;
	Con_Game_Stub _StubGame;
	NetCallback _ncbk;
	GameSVProps _props;
	bool _isInited = false;
	bool _isStarted = false;

	DBConnMgr _SyncedGameDBConn;
	DBConnMgr _SyncedLoginDBConn;
	DBConnMgr _AsyncGameDBConn;
	AsyncDBUpdater _AsyncGameDB;
	cpp_redis::client* _pRedisClient = nullptr;

	MemoryPoolTLS<PlayerObject>* _PlayerObjPool = nullptr;
	MemoryPoolTLS<CrystalObject>* _CrystalObjPool = nullptr;
	MemoryPoolTLS<PlayerData>* _PlayerDataPool = nullptr;

	Tile** _Tile2DArr = nullptr;
	Sector** _Sector2DArr = nullptr;
	std::unordered_map<NID, PlayerData*> _NotLogInPlayerMap;
	std::unordered_map<INT64, BaseObject*> _ObjectMap;
	std::unordered_map<NID, PlayerObject*> _PlayerObjMap;
	std::list<std::pair<bool, BaseObject*>> _NewDelPendingList;
	bool _isOnUpdate = false;

	Logger* _glog = nullptr;
	MonitorClientMgr _mtrmgr;
	PDHCounter _pdh;

public:
	// Logic & math
	UINT64 GetNewClientID();
	CNetRPCProxy& GetRPCProxy();
	CNetServer& GetNetServer();
	AsyncDBUpdater& GetAsyncGameDB();
	GameSVProps& GetProps();
	MemoryPoolTLS<CrystalObject>* GetCrystalObjPool();

	bool isOnTickBound(double scale = 1);
	void FlushMapModifyFromUpdate();
	void InsertObjectToMap(BaseObject* bobj);
	void EraseObjectFromMap(BaseObject* bobj);
	void InsertObjectToTile(BaseObject* bobj, TilePos tpos);
	void EraseObjectFromTile(BaseObject* bobj, TilePos tpos);

	BaseObject* GetBaseObjByClientID(INT64 ClientID);
	PlayerObject* GetPlayerObjByNID(NID nid);
	Tile* GetTileByPos(TilePos tpos);
	Sector* GetSectorByPos(SectorPos spos);

	std::list<PlayerObject*> GetPlayerObjListFromSPosVec(const std::vector<SectorPos>* SPosVec, bool excludeDead = false);
	std::list<MonsterObject*> GetMonsterObjListFromSPosVec(const std::vector<SectorPos>* SPosVec, bool excludeDead = false);
	std::list<CrystalObject*> GetCrystalObjListFromSPosVec(const std::vector<SectorPos>* SPosVec);

	void JoinPlayerToGame(PlayerObject* player, bool revive = false);
	void RemovePlayerFromGame(PlayerObject* player);

	// Server Ctrl
	bool Init(GameSVProps props);
	bool Start();
	bool Stop();
	void PrintLog();
	~GameServer();

	//Contents Auth interface & stubs
	bool ConAuth_OnThrStart(DWORD ThrID);
	bool ConAuth_OnThrEnd(DWORD ThrID);
	bool ConAuth_OnJoin(NID sessID, CID departCID, const PVOID data);
	bool ConAuth_OnLeave(NID sessID, CID destCID);
	bool ConAuth_OnDisconnect(NID sessID);

	bool ConAuth_REQ_LOGIN(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_LOGIN);
	bool ConAuth_REQ_CHARACTER_SELECT(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_CHARACTER_SELECT);

	//Contents Game interface & stubs
	bool ConGame_OnThrStart(DWORD ThrID);
	bool ConGame_OnThrEnd(DWORD ThrID);
	bool ConGame_OnJoin(NID sessID, CID departCID, const PVOID data);
	bool ConGame_OnLeave(NID sessID, CID destCID);
	bool ConGame_OnUpdate();
	bool ConGame_OnDisconnect(NID sessID);

	bool ConGame_REQ_MOVE_CHARACTER(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_MOVE_CHARACTER);
	bool ConGame_REQ_STOP_CHARACTER(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_STOP_CHARACTER);
	bool ConGame_REQ_ATTACK1(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK1);
	bool ConGame_REQ_ATTACK2(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK2);
	bool ConGame_REQ_PICK(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_PICK);
	bool ConGame_REQ_SIT(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_SIT);
	bool ConGame_REQ_PLAYER_RESTART(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_PLAYER_RESTART);
	bool ConGame_REQ_ECHO(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ECHO);
	bool ConGame_REQ_HEARTBEAT(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_HEARTBEAT);

};

