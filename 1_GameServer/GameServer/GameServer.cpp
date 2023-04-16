#include "GameServer.h"


UINT64 GameServer::GetNewClientID() {
	return ++_LastGenClientID;
}

CNetRPCProxy& GameServer::GetRPCProxy() {
	return _Proxy;
}

CNetServer& GameServer::GetNetServer() {
	return _nsv;
}

AsyncDBUpdater& GameServer::GetAsyncGameDB() {
	return _AsyncGameDB;
}

GameSVProps& GameServer::GetProps() {
	return _props;
}

MemoryPoolTLS<CrystalObject>* GameServer::GetCrystalObjPool() {
	return _CrystalObjPool;
}

bool GameServer::isOnTickBound(double scale) {
	return (_CON_Game.GetTickCount() % (UINT)(_CON_Game.GetTPS() * scale)) == 0;
}

void GameServer::FlushMapModifyFromUpdate() {
	for (auto& i : _NewDelPendingList) {
		if (i.first) { InsertObjectToMap(i.second); }
		else { EraseObjectFromMap(i.second); }
	}
	_NewDelPendingList.clear();
	return;
}

void GameServer::InsertObjectToMap(BaseObject* bobj) {
	if (_isOnUpdate) { _NewDelPendingList.push_back({ true, bobj }); return; }
	_ObjectMap.insert({ bobj->_ClientID, bobj });
	if (bobj->_ObjType == OBJTYPE::PLAYER) {
		PlayerObject* pobj = static_cast<PlayerObject*>(bobj);
		_PlayerObjMap.insert({ pobj->_Nid, pobj });
	}
	return;
}

void GameServer::EraseObjectFromMap(BaseObject* bobj) {
	if (_isOnUpdate) { _NewDelPendingList.push_back({ false, bobj }); return; }
	_ObjectMap.erase(bobj->_ClientID);
	if (bobj->_ObjType == OBJTYPE::PLAYER) {
		_PlayerObjMap.erase(static_cast<PlayerObject*>(bobj)->_Nid);
	}
	return;
}

void GameServer::InsertObjectToTile(BaseObject* bobj, TilePos tpos) {
	GetTileByPos(tpos)->ObjMap.insert({ bobj->_ClientID, bobj });
	return;
}

void GameServer::EraseObjectFromTile(BaseObject* bobj, TilePos tpos) {
	GetTileByPos(tpos)->ObjMap.erase(bobj->_ClientID);
	return;
}

BaseObject* GameServer::GetBaseObjByClientID(INT64 ClientID) {
	auto iter = _ObjectMap.find(ClientID);
	if (iter == _ObjectMap.end()) { return nullptr; }
	else { return iter->second; }
}

PlayerObject* GameServer::GetPlayerObjByNID(NID nid) {
	auto iter = _PlayerObjMap.find(nid);
	if (iter == _PlayerObjMap.end()) { return nullptr; }
	else { return iter->second; }
}

Tile* GameServer::GetTileByPos(TilePos tpos) {
	if (!isTilePosValid(tpos)) { return nullptr; }
	return &_Tile2DArr[tpos.y][tpos.x];
}

Sector* GameServer::GetSectorByPos(SectorPos spos) {
	if (!isSectorPosValid(spos)) { return nullptr; }
	return &_Sector2DArr[spos.y][spos.x];
}

std::list<PlayerObject*> GameServer::GetPlayerObjListFromSPosVec(const std::vector<SectorPos>* SPosVec, bool excludeDead) {
	std::list<PlayerObject*> ret;

	for (auto& s : *SPosVec) {
		for (auto& t : GetSectorByPos(s)->TileList) {
			for (auto& o : t->ObjMap) {
				if (o.second->_ObjType == OBJTYPE::PLAYER) {
					PlayerObject* pPo = static_cast<PlayerObject*>(o.second);
					if (excludeDead && pPo->isDead()) { continue; }
					ret.push_back(pPo);
				}
			}
		}
	}

	return ret;
}

std::list<MonsterObject*> GameServer::GetMonsterObjListFromSPosVec(const std::vector<SectorPos>* SPosVec, bool excludeDead) {
	std::list<MonsterObject*> ret;

	for (auto& s : *SPosVec) {
		for (auto& t : GetSectorByPos(s)->TileList) {
			for (auto& o : t->ObjMap) {
				if (o.second->_ObjType == OBJTYPE::MONSTER) {
					MonsterObject* pMo = static_cast<MonsterObject*>(o.second);
					if (excludeDead && pMo->isDead()) { continue; }
					ret.push_back(pMo);
				}
			}
		}
	}

	return ret;
}

std::list<CrystalObject*> GameServer::GetCrystalObjListFromSPosVec(const std::vector<SectorPos>* SPosVec) {
	std::list<CrystalObject*> ret;

	for (auto& s : *SPosVec) {
		for (auto& t : GetSectorByPos(s)->TileList) {
			for (auto& o : t->ObjMap) {
				if (o.second->_ObjType == OBJTYPE::CRYSTAL) {
					CrystalObject* pCo = static_cast<CrystalObject*>(o.second);
					ret.push_back(pCo);
				}
			}
		}
	}

	return ret;
}

void GameServer::JoinPlayerToGame(PlayerObject* player, bool revive) {
	//get around info list first
	auto aroundsector = GetAroundSectorPos(player->_SPos, _props.ActiveSectorRange);
	auto aroundplayer = GetPlayerObjListFromSPosVec(&aroundsector);
	auto aroundmonster = GetMonsterObjListFromSPosVec(&aroundsector, true);
	auto aroundcrystal = GetCrystalObjListFromSPosVec(&aroundsector);

	//insert to globalMap
	if(!revive){ InsertObjectToMap(player); }

	//insert to tile
	if (!revive){ InsertObjectToTile(player, player->_TPos); }

	//send self-player info to player
	_Proxy.PACKET_CS_GAME_RES_CREATE_MY_CHARACTER(player->_Nid, 
		player->_ClientID, player->_Chartype, player->_Nickname,
		player->_CPos.x, player->_CPos.y, player->_Rotation, player->_Crystal,
		player->_HP, player->_Exp, player->_Level
	);
	//send other-player info to player
	for (auto i : aroundplayer) {
		if (i->_Nid == player->_Nid) { continue; }
		_Proxy.PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER(player->_Nid,
			i->_ClientID, i->_Chartype, i->_Nickname, i->_CPos.x, i->_CPos.y,
			i->_Rotation, i->_Level, 0, i->_isSit, i->_isDead);
	}
	//send monster info to player
	for (auto i : aroundmonster) {
		_Proxy.PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER(player->_Nid,
			i->_ClientID, i->_CPos.x, i->_CPos.y, i->_Rotation, 0);
	}
	//send crystal info to player
	for (auto i : aroundcrystal) {
		_Proxy.PACKET_CS_GAME_RES_CREATE_CRISTAL(player->_Nid,
			i->_ClientID, i->_CrystalType, i->_CPos.x, i->_CPos.y);
	}
	//send self-player info to other-player
	for (auto i : aroundplayer) {
		if (i->_Nid == player->_Nid) { continue; }
		_Proxy.PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER(i->_Nid,
			player->_ClientID, player->_Chartype, player->_Nickname, player->_CPos.x, player->_CPos.y,
			player->_Rotation, player->_Level, 1, player->_isSit, player->_isDead);
	}
	return;
}

void GameServer::RemovePlayerFromGame(PlayerObject* player) {
	//remove from tile
	EraseObjectFromTile(player, player->_TPos);

	//remove from globalMap
	EraseObjectFromMap(player);

	//alert this player is gone
	player->_isDead = 1;

	//get around info
	auto aroundsector = GetAroundSectorPos(player->_SPos, _props.ActiveSectorRange);
	auto aroundplayer = GetPlayerObjListFromSPosVec(&aroundsector);

	//send self-player deletion to other-player
	for (auto i : aroundplayer) {
		_Proxy.PACKET_CS_GAME_RES_REMOVE_OBJECT(i->_Nid, player->_ClientID);
	}
	return;
}



bool GameServer::Init(GameSVProps props) {
	_props = props;

	_Tile2DArr =  new Tile* [TILE_Y_MAX];
	for (int i = 0; i < TILE_Y_MAX; i++) {
		_Tile2DArr[i] = new Tile [TILE_X_MAX];
	}
	_Sector2DArr = new Sector * [SECTOR_Y_MAX];
	for (int i = 0; i < SECTOR_Y_MAX; i++) {
		_Sector2DArr[i] = new Sector[SECTOR_X_MAX];
	}

	for (int y = 0; y < TILE_Y_MAX; y++) {
		for (int x = 0; x < TILE_X_MAX; x++) {
			GetSectorByPos(TilePosToSectorPos({ x, y }))->TileList.push_back(GetTileByPos({ x, y }));
		}
	}

	_PlayerObjPool = new MemoryPoolTLS<PlayerObject>();
	_CrystalObjPool = new MemoryPoolTLS<CrystalObject>();
	_PlayerDataPool = new MemoryPoolTLS<PlayerData>();

	UINT64 _LastGenClientID = 0;

	_nsv.Init(_props, &_ncbk, 2);
	_nsv.AttachContents(&_CON_Auth, 20);
	_nsv.AttachContents(&_CON_Game, 30);
	_CON_Auth._gsv = this;
	_CON_Game._gsv = this;

	_nsv.AttachRPCProxy(&_Proxy);
	_CON_Auth.AttachRPCStub(&_StubAuth);
	_CON_Game.AttachRPCStub(&_StubGame);
	_StubAuth._gsv = this;
	_StubGame._gsv = this;

	_pRedisClient = new cpp_redis::client();

	_glog = new Logger(_props.GameSVLogger);
	_pdh.Init();

	_isInited = true;
	return true;
}

bool GameServer::Start() {
	//connect DB
	_SyncedLoginDBConn.Start(_props.LoginDB_HostName, _props.LoginDB_UserName, _props.LoginDB_Password);
	_SyncedGameDBConn.Start(_props.GameDB_HostName, _props.GameDB_UserName, _props.GameDB_Password);
	_AsyncGameDBConn.Start(_props.GameDB_HostName, _props.GameDB_UserName, _props.GameDB_Password);
	_AsyncGameDB.Start(&_AsyncGameDBConn);
	try {
		_pRedisClient->connect(_props.Redis_IP, _props.Redis_Port);
	}
	catch (std::exception& e) {
		printf("Redis Error at GStart: %s\n", e.what());
	}

	


	//join monsters to game
	for (int i = 0; i < _props.Monster1Num; i++) {
		MonsterObject* m = new MonsterObject();
		m->Init(this, RESPAWN_MON1_CENTER, RESPAWN_MON1_RANGE);
	}
	for (int i = 0; i < _props.Monster2Num; i++) {
		MonsterObject* m = new MonsterObject();
		m->Init(this, RESPAWN_MON2_CENTER, RESPAWN_MON2_RANGE);
	}
	for (int i = 0; i < _props.Monster3Num; i++) {
		MonsterObject* m = new MonsterObject();
		m->Init(this, RESPAWN_MON3_CENTER, RESPAWN_MON3_RANGE);
	}
	for (int i = 0; i < _props.Monster4Num; i++) {
		MonsterObject* m = new MonsterObject();
		m->Init(this, RESPAWN_MON4_CENTER, RESPAWN_MON4_RANGE);
	}
	for (int i = 0; i < _props.Monster5Num; i++) {
		MonsterObject* m = new MonsterObject();
		m->Init(this, RESPAWN_MON5_CENTER, RESPAWN_MON5_RANGE);
	}
	for (int i = 0; i < _props.Monster6Num; i++) {
		MonsterObject* m = new MonsterObject();
		m->Init(this, RESPAWN_MON6_CENTER, RESPAWN_MON6_RANGE);
	}
	for (int i = 0; i < _props.Monster7Num; i++) {
		MonsterObject* m = new MonsterObject();
		m->Init(this, RESPAWN_MON7_CENTER, RESPAWN_MON7_RANGE);
	}

	//start monitoring
	_mtrmgr.Init(_props.MonitorSV_IP, _props.MonitorSV_Port, 12);
	_mtrmgr.Start();
	_mtrmgr.LoginToMonitor();

	//start server
	_nsv.Start();

	return true;
}

bool GameServer::Stop() {
	//stop server
	_nsv.Stop();

	//disconnect DB
	_SyncedLoginDBConn.Stop();
	_SyncedGameDBConn.Stop();
	_AsyncGameDB.Stop();
	_AsyncGameDBConn.Stop();
	_pRedisClient->disconnect();

	//remove monsters and crystals from game
	for (int y = 0; y < TILE_Y_MAX; y++) {
		for (int x = 0; x < TILE_X_MAX; x++) {
			for (auto& i : _Tile2DArr[y][x].ObjMap) {
				BaseObject* bobj = i.second;
				if (bobj->_ObjType == OBJTYPE::MONSTER) {
					delete (MonsterObject*)bobj;
				}
				else if (bobj->_ObjType == OBJTYPE::CRYSTAL) {
					_CrystalObjPool->Free((CrystalObject*)bobj);
				}
			}
			_Tile2DArr[y][x].ObjMap.clear();
		}
	}
	_ObjectMap.clear();

	//stop monitoring
	_mtrmgr.Disconnect();

	return true;
}

void GameServer::PrintLog() {
	static ULONGLONG PrevAcceptCount = 0;
	static ULONGLONG PrevSendCount = 0;
	static ULONGLONG PrevRecvCount = 0;
	static ULONGLONG PrevC1TCount = 0;
	static ULONGLONG PrevC2TCount = 0;
	static ULONGLONG PrewDBWCount = 0;

	ULONGLONG acceptc = _nsv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_ACCEPT);
	ULONGLONG sendc = _nsv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_SEND);
	ULONGLONG recvc = _nsv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_RECV);
	int gpp = _nsv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_CAP_PKT);
	int gup = _nsv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_USE_PKT);
	int nsp = _nsv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_CAP_NSESS);
	int nspu = _nsv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_USE_NSESS);
	UINT c1s = _nsv.GetCDB(1)->ConSessNum;
	UINT c2s = _nsv.GetCDB(2)->ConSessNum;
	UINT c1tc = _nsv.GetCDB(1)->TickCount;
	UINT c2tc = _nsv.GetCDB(2)->TickCount;
	ULONG asuc = _AsyncGameDB.GetUpdateCount();
	long asqs = _AsyncGameDB.GetQueueSize();
	float syscpu = _pdh.GetSysCpuTime();
	float procpu = _pdh.GetProcessCpuTime();
	LONGLONG sysnpp = _pdh.GetSysNonPagedPool();
	LONGLONG prompb = _pdh.GetProcessPrivateBytes();
	double netrcv = _pdh.GetNetworkRecv();
	double netsnd = _pdh.GetNetworkSend();
	int popc = _PlayerObjPool->getCapacity();
	int copc = _CrystalObjPool->getCapacity();
	int pdpc = _PlayerDataPool->getCapacity();
	int popu = _PlayerObjPool->getUseCount();
	int copu = _CrystalObjPool->getUseCount();
	int pdpu = _PlayerDataPool->getUseCount();
	int asdbpc = _AsyncGameDB.GetTaskPoolCapacity();
	int asdbpu = _AsyncGameDB.GetTaskPoolUseCount();

	ULONGLONG acptps = acceptc - PrevAcceptCount;
	ULONGLONG sndps = sendc - PrevSendCount;
	ULONGLONG rcvps = recvc - PrevRecvCount;
	ULONGLONG c1tps = c1tc - PrevC1TCount;
	ULONGLONG c2tps = c2tc - PrevC2TCount;
	ULONGLONG dbwps = asuc - PrewDBWCount;

	PrevAcceptCount = acceptc;
	PrevSendCount = sendc;
	PrevRecvCount = recvc;
	PrevC1TCount = c1tc;
	PrevC2TCount = c2tc;
	PrewDBWCount = asuc;

	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, 1);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, (int)procpu);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, (int)_pdh.ToMiB(prompb));
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_SESSION, (int)nspu);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, (int)c1s);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, (int)c2s);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, (int)acptps);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, (int)rcvps);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, (int)sndps);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, (int)dbwps);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG, (int)asqs);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, (int)c1tps);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, (int)c2tps);
	_mtrmgr.SendData(dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, (int)gpp);

	std::string line0, line1, line2, line3, line4, line5;

	line0 += "=====================\n";

	line1 += "Auth: "; line1 += std::to_string(c1s);
	line1 += ", Game: "; line1 += std::to_string(c2s);
	line1 += ", AcceptCnt: "; line1 += std::to_string(acceptc);
	line1 += " ("; line1 += std::to_string(acptps);
	line1 += "/s)\n";

	line2 += "PKpool: "; line2 += std::to_string(gpp);
	line2 += ", NSpool: "; line2 += std::to_string(nsp);
	line2 += ", POpool: "; line2 += std::to_string(popc);
	line2 += ", COpool: "; line2 += std::to_string(copc);
	line2 += ", PDpool: "; line2 += std::to_string(pdpc);
	line2 += ", ASTpool: "; line2 += std::to_string(asdbpc);
	line2 += "\n";

	line3 += "PKUsing: "; line3 += std::to_string(gup);
	line3 += ", NSUsing: "; line3 += std::to_string(nspu);
	line3 += ", POUsing: "; line3 += std::to_string(popu);
	line3 += ", COUsing: "; line3 += std::to_string(copu);
	line3 += ", PDUsing: "; line3 += std::to_string(pdpu);
	line3 += ", ASTUsing: "; line3 += std::to_string(asdbpu);
	line3 += "\n";

	line4 += "AuthTPS: "; line4 += std::to_string(c1tps);
	line4 += "/s, GameTPS: "; line4 += std::to_string(c2tps);
	line4 += "/s, Send: "; line4 += std::to_string(sndps);
	line4 += "/s, Recv: "; line4 += std::to_string(rcvps);
	line4 += "/s\n";
	line4 += "DBWriteTPS: "; line4 += std::to_string(dbwps);
	line4 += "/s, DBQueue: "; line4 += std::to_string(asqs);
	line4 += "\n";

	line5 += "SysCPU: "; line5 += std::to_string(syscpu);
	line5 += "%, ProCPU: "; line5 += std::to_string(procpu);
	line5 += "%\n";
	line5 += "SysMemNPP: "; line5 += std::to_string(_pdh.ToMiB(sysnpp));
	line5 += "MiB, ProMemPB: "; line5 += std::to_string(_pdh.ToMiB(prompb));
	line5 += "MiB\n";
	line5 += "NetRecv: "; line5 += std::to_string(_pdh.ToMiB(netrcv));
	line5 += "MiB/s, NetSend: "; line5 += std::to_string(_pdh.ToMiB(netsnd));
	line5 += "MiB/s\n";

	line0 += line1 += line2 += line3 += line4 += line5;
	std::cout << line0 << std::endl;

	return;
}

GameServer::~GameServer() {
	if (_isInited) {
		for (int i = 0; i < TILE_Y_MAX; i++) {
			delete[] _Tile2DArr[i];
		}
		delete[] _Tile2DArr;
		for (int i = 0; i < SECTOR_Y_MAX; i++) {
			delete[] _Sector2DArr[i];
		}
		delete[] _Sector2DArr;

		delete _PlayerObjPool;
		delete _CrystalObjPool;
		delete _PlayerDataPool;
		delete _pRedisClient;
	}
	return;
}

bool GameServer::ConAuth_OnThrStart(DWORD ThrID) {
	return true;
}

bool GameServer::ConAuth_OnThrEnd(DWORD ThrID) {
	return true;
}

bool GameServer::ConAuth_OnJoin(NID sessID, CID departCID, const PVOID data) {
	_NotLogInPlayerMap.insert({ sessID, nullptr });
	return true;
}

bool GameServer::ConAuth_OnLeave(NID sessID, CID destCID) {
	_NotLogInPlayerMap.erase(sessID);
	return true;
}

bool GameServer::ConAuth_OnDisconnect(NID sessID) {
	auto pdata_iter = _NotLogInPlayerMap.find(sessID);
	if (pdata_iter == _NotLogInPlayerMap.end()) { return true; }

	PlayerData* pdata = pdata_iter->second;
	if (pdata != nullptr) { _PlayerDataPool->Free(pdata); }
	_NotLogInPlayerMap.erase(pdata_iter);

	return true;
}

bool GameServer::ConAuth_REQ_LOGIN(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_LOGIN) {
	//check login info from redis
	bool isExist = false;
	std::string str_accno = std::to_string(AccountNo);
	std::future<cpp_redis::reply> get_reply = _pRedisClient->get(str_accno);
	_pRedisClient->sync_commit();

	cpp_redis::reply reply;
	reply = get_reply.get();
	isExist = !reply.is_null();

	//if login info exist
	if (isExist) {
		//query player info from gamedb
		sql::ResultSet* res = _SyncedGameDBConn.executeQuery(L"SELECT * FROM `gamedb`.`character` WHERE `accountno`=%lld;",
			AccountNo);

		//get player info from gamedb
		PlayerData* pdata = nullptr;
		bool isDead;
		int rowcnt = 0;
		while (res->next()) {
			pdata = _PlayerDataPool->Alloc();
			isDead = res->getBoolean("die");
			rowcnt++;

			pdata->AccountNo = AccountNo;
			pdata->Chartype = res->getInt("charactertype");
			pdata->CPos.x = (float)res->getDouble("posx");
			pdata->CPos.y = (float)res->getDouble("posy");
			pdata->Crystal = res->getInt("cristal");
			pdata->Exp = res->getInt64("exp");
			pdata->HP = res->getInt("hp");
			pdata->Level = res->getInt("level");
			pdata->Rotation = res->getInt("rotation");

			//Query and get nickname from accountdb
			sql::ResultSet* nameres = _SyncedLoginDBConn.executeQuery(L"SELECT `usernick` FROM `accountdb`.`account` WHERE `accountno`=%lld;",
				AccountNo);
			nameres->next();
			std::string name_str = nameres->getString("usernick");
			MultiByteToWideChar(CP_UTF8, 0, name_str.c_str(), -1, pdata->Nickname, 20);
			delete nameres;
		}
		delete res;

		//if player is brand-new - build new player
		if (rowcnt == 0) {
			//alloc new playerdata
			pdata = _PlayerDataPool->Alloc();

			//get new start pos
			TilePos newTPos{ 85 + (rand() % 21) ,93 + (rand() % 31) };
			ClientPos newCPos = TilePosToClientPos(newTPos);

			//set playerdata
			pdata->AccountNo = AccountNo;
			pdata->Chartype = 0;
			pdata->CPos = newCPos;
			pdata->Crystal = 0;
			pdata->Exp = 0;
			pdata->HP = INITIAL_HP;
			pdata->Level = 1;
			pdata->Rotation = 0;

			//Query and get nickname from accountdb
			sql::ResultSet* nameres = _SyncedLoginDBConn.executeQuery(L"SELECT `usernick` FROM `accountdb`.`account` WHERE `accountno`=%lld;",
				AccountNo);
			nameres->next();
			std::string name_str = nameres->getString("usernick");
			MultiByteToWideChar(CP_UTF8, 0, name_str.c_str(), -1, pdata->Nickname, 20);
			delete nameres;

			//temp save at NLPMap
			_NotLogInPlayerMap[__RPC__nid] = pdata;

			//send charselect res to player
			_Proxy.PACKET_CS_GAME_RES_LOGIN(__RPC__nid, 2, AccountNo);
		}
		//if player is existing one - send to game contents
		else if (rowcnt == 1) {
			//get session's ip:port
			WCHAR ip[NETSV_IPSTR_LEN];
			u_short port;
			_nsv.GetIpAddress(__RPC__nid, ip);
			port = _nsv.GetPort(__RPC__nid);
			TilePos tpos = ClientPosToTilePos(pdata->CPos);

			//log DB for login
			_SyncedGameDBConn.executeUpdate(L"START TRANSACTION;");
			const WCHAR* query =
				L"INSERT INTO `gamedb`.`gamelog`(`type`, `code`, `accountno`, `param1`, `param2`, `param3`, `param4`, `message`) "
				L"VALUES(%d, %d, %lld, %d, %d, %d, %d, '%s:%d');";
			_SyncedGameDBConn.executeUpdate(query,
				1, 11, pdata->AccountNo, tpos.x, tpos.y, pdata->Crystal, pdata->HP, ip, port);

			if(isDead) {
				//reset pos and dead flag
				TilePos newTPos{ 85 + (rand() % 21) ,93 + (rand() % 31) };
				ClientPos newCPos = TilePosToClientPos(newTPos);
				pdata->CPos = newCPos;
				pdata->Rotation = 0;

				//update pos and Dead flag at gameDB
				const WCHAR* deadquery =
					L"INSERT INTO `gamedb`.`gamelog`(`type`, `code`, `accountno`, `param1`, `param2`) "
					L"VALUES(%d, %d, %lld, %d, %d);";
				_SyncedGameDBConn.executeUpdate(deadquery,
					3, 33, AccountNo, newTPos.x, newTPos.y);
				deadquery = 
					L"UPDATE `gamedb`.`character` SET `die`=%d, `posx`=%f, `posy`=%f, `tilex`=%d, `tiley`=%d, `rotation`=%d WHERE `accountno`=%lld;";
				_SyncedGameDBConn.executeUpdate(deadquery,
					0, newCPos.x, newCPos.y, newTPos.x, newTPos.y, 0, AccountNo);

			}
			_SyncedGameDBConn.executeUpdate(L"COMMIT;");

			//join player to game
			_CON_Auth.MoveContents(__RPC__nid, 2, pdata);

			//send join game res to player
			_Proxy.PACKET_CS_GAME_RES_LOGIN(__RPC__nid, 1, AccountNo);
		}
	}
	//if login info not exist
	else {
		_Proxy.PACKET_CS_GAME_RES_LOGIN(__RPC__nid, 0, AccountNo);
		_nsv.Disconnect(__RPC__nid);
	}

	return true;
}

bool GameServer::ConAuth_REQ_CHARACTER_SELECT(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_CHARACTER_SELECT) {
	auto pdata_iter = _NotLogInPlayerMap.find(__RPC__nid);
	if (pdata_iter == _NotLogInPlayerMap.end()) { _nsv.Disconnect(__RPC__nid); return true; }
	PlayerData* pdata = pdata_iter->second;
	INT64 AccNo = pdata->AccountNo;
	TilePos TPos = ClientPosToTilePos(pdata->CPos);

	//set chartype
	pdata->Chartype = CharacterType;

	//get session's ip:port
	WCHAR ip[NETSV_IPSTR_LEN];
	u_short port;
	_nsv.GetIpAddress(__RPC__nid, ip);
	port = _nsv.GetPort(__RPC__nid);
	TilePos tpos = ClientPosToTilePos(pdata->CPos);

	//log DB for login
	//log DB for create char
	//insert new row at gamedb
	_SyncedGameDBConn.executeUpdate(L"START TRANSACTION;");

	const WCHAR* query =
		L"INSERT INTO `gamedb`.`gamelog`(`type`, `code`, `accountno`, `param1`, `param2`, `param3`, `param4`, `message`) "
		L"VALUES(%d, %d, %lld, %d, %d, %d, %d, '%s:%d'); ";
	_SyncedGameDBConn.executeUpdate(query,
		1, 11, pdata->AccountNo, tpos.x, tpos.y, pdata->Crystal, pdata->HP, ip, port);

	query =
		L"INSERT INTO `gamedb`.`gamelog`(`type`, `code`, `accountno`, `param1`) "
		L"VALUES(%d, %d, %lld, %d); ";
	_SyncedGameDBConn.executeUpdate(query,
		3, 32, pdata->AccountNo, pdata->Chartype);

	query = 
		L"INSERT INTO `gamedb`.`character` VALUES (%lld, %d, %f, %f, %d, %d, %d, %d, %d, %lld, %d, %d); ";
	_SyncedGameDBConn.executeUpdate(query,
		pdata->AccountNo, pdata->Chartype, pdata->CPos.x, pdata->CPos.y, TPos.x, TPos.y, pdata->Rotation, pdata->Crystal, pdata->HP, pdata->Exp, pdata->Level, 0);
	
	_SyncedGameDBConn.executeUpdate(L"COMMIT;");

	//join game
	_CON_Auth.MoveContents(__RPC__nid, 2, pdata);
	_Proxy.PACKET_CS_GAME_RES_CHARACTER_SELECT(__RPC__nid, 1);

	return true;
}



bool GameServer::ConGame_OnThrStart(DWORD ThrID) {

	return true;
}

bool GameServer::ConGame_OnThrEnd(DWORD ThrID) {

	return true;
}

bool GameServer::ConGame_OnJoin(NID sessID, CID departCID, const PVOID data) {
	//get playerdata from auth contents
	PlayerData* pData = (PlayerData*)data;

	//alloc playerobject, set info, join to game
	PlayerObject* pobj = _PlayerObjPool->Alloc();		// will be freed when player disconnects
	pobj->Init(this, sessID, pData);
	JoinPlayerToGame(pobj);
	_PlayerDataPool->Free(pData);		//free playerdata alloced from auth contents

	return true;
}

bool GameServer::ConGame_OnLeave(NID sessID, CID destCID) {
	//there is no situation to move session to other contents.
	return true;
}

bool GameServer::ConGame_OnUpdate() {
	_isOnUpdate = true;
	for (auto& i : _ObjectMap) {
		i.second->Update();
	}
	_isOnUpdate = false;
	FlushMapModifyFromUpdate();
	return true;
}

bool GameServer::ConGame_OnDisconnect(NID sessID) {
	PlayerObject* pobj = GetPlayerObjByNID(sessID);

	//write on DB
	_AsyncGameDB.Write_Logout(pobj->_AccountNo, pobj->_CPos, pobj->_Rotation, pobj->_Crystal, pobj->_HP);

	//Remove Player from game
	RemovePlayerFromGame(pobj);
	_PlayerObjPool->Free(pobj);		//alloced from OnJoin

	return true;
}


bool GameServer::ConGame_REQ_MOVE_CHARACTER(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_MOVE_CHARACTER) {
	//get player object
	PlayerObject* pobj = GetPlayerObjByNID(__RPC__nid);
	if (pobj->_ClientID != ClientID) { _nsv.Disconnect(__RPC__nid); }
	pobj->UpdateLastRecvTime();

	//create CPos and validate
	ClientPos newCPos{ X, Y };
	if(!isClientPosValid(newCPos)) { _nsv.Disconnect(__RPC__nid); }
	if (!isRotationValid(Rotation)) { 
		if (Rotation == 360) { Rotation = 0; }
		else { _nsv.Disconnect(__RPC__nid); }
	}

	//unset sit
	pobj->SetSit(false);

	//backup prev Pos info and update
	TilePos prevTPos = pobj->_TPos;
	SectorPos prevSPos = pobj->_SPos;
	pobj->UpdatePos(newCPos);
	pobj->UpdateRotation(Rotation);
	TilePos newTPos = pobj->_TPos;
	SectorPos newSPos = pobj->_SPos;

	//get new-around player list to send move msg
	auto newAroundSPosVec = GetAroundSectorPos(newSPos, _props.ActiveSectorRange);
	auto AroundPlayerList = GetPlayerObjListFromSPosVec(&newAroundSPosVec);

	//update tile and sector if needed
	if (prevTPos != newTPos) {		//if tile has been changed
		//move tile
		EraseObjectFromTile(pobj, prevTPos);
		InsertObjectToTile(pobj, newTPos);

		if (prevSPos != newSPos) {		//if sector has been changed
			//get prev sector's around sectors
			auto prevAroundSPosVec = GetAroundSectorPos(prevSPos, _props.ActiveSectorRange);

			//get new and del sector
			std::vector<SectorPos> delSPosVec;
			std::vector<SectorPos> newSPosVec;
			GetNewAndDelSectorPos(&prevAroundSPosVec, &newAroundSPosVec, &delSPosVec, &newSPosVec);

			//get new and del player list
			auto DelPlayerList = GetPlayerObjListFromSPosVec(&delSPosVec);
			auto NewPlayerList = GetPlayerObjListFromSPosVec(&newSPosVec);

			//get new and del monster list
			auto DelMonsterList = GetMonsterObjListFromSPosVec(&delSPosVec, true);
			auto NewMonsterList = GetMonsterObjListFromSPosVec(&newSPosVec, true);

			//get new and del Crystal list
			auto DelCrystalList = GetCrystalObjListFromSPosVec(&delSPosVec);
			auto NewCrystalList = GetCrystalObjListFromSPosVec(&newSPosVec);

			//send deletion of player to del-players
			//and deletion of del-players to player
			for (auto& i : DelPlayerList) {
				_Proxy.PACKET_CS_GAME_RES_REMOVE_OBJECT(i->_Nid, pobj->_ClientID);
				_Proxy.PACKET_CS_GAME_RES_REMOVE_OBJECT(pobj->_Nid, i->_ClientID);
			}
			//send creation of player to new-players
			//and creation of new-players to player
			for (auto& i : NewPlayerList) {
				_Proxy.PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER(i->_Nid,
					pobj->_ClientID, pobj->_Chartype, pobj->_Nickname, pobj->_CPos.x, pobj->_CPos.y,
					pobj->_Rotation, pobj->_Level, 0, pobj->_isSit, pobj->_isDead);
				_Proxy.PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER(pobj->_Nid,
					i->_ClientID, i->_Chartype, i->_Nickname, i->_CPos.x, i->_CPos.y,
					i->_Rotation, i->_Level, 0, i->_isSit, i->_isDead);
			}

			//send del-monster to player
			for (auto& i : DelMonsterList) {
				_Proxy.PACKET_CS_GAME_RES_REMOVE_OBJECT(pobj->_Nid, i->_ClientID);
			}
			//send new-monster to player
			for (auto& i : NewMonsterList) {
				_Proxy.PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER(pobj->_Nid,
					i->_ClientID, i->_CPos.x, i->_CPos.y, i->_Rotation, 0);
			}

			//send del-Crystal to player
			for (auto& i : DelCrystalList) {
				_Proxy.PACKET_CS_GAME_RES_REMOVE_OBJECT(pobj->_Nid, i->_ClientID);
			}
			//send new-Crystal to player
			for (auto& i : NewCrystalList) {
				_Proxy.PACKET_CS_GAME_RES_CREATE_CRISTAL(pobj->_Nid,
					i->_ClientID, i->_CrystalType, i->_CPos.x, i->_CPos.y);
			}
		}
	}
	
	//send move msg
	for (auto& i : AroundPlayerList) {
		if (i->_Nid == pobj->_Nid) { continue; }
		_Proxy.PACKET_CS_GAME_RES_MOVE_CHARACTER(i->_Nid,
			pobj->_ClientID, pobj->_CPos.x, pobj->_CPos.y, pobj->_Rotation, VKey, HKey);
	}
	
	return true;
}

bool GameServer::ConGame_REQ_STOP_CHARACTER(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_STOP_CHARACTER) {
	//get player object
	PlayerObject* pobj = GetPlayerObjByNID(__RPC__nid);
	if (pobj->_ClientID != ClientID) { _nsv.Disconnect(__RPC__nid); }
	pobj->UpdateLastRecvTime();

	//create CPos and validate
	ClientPos newCPos{ X, Y };
	if (!isClientPosValid(newCPos)) { _nsv.Disconnect(__RPC__nid); }
	if (!isRotationValid(Rotation)) {
		if (Rotation == 360) { Rotation = 0; }
		else { _nsv.Disconnect(__RPC__nid); }
	}

	//unset sit
	pobj->SetSit(false);

	//backup prev Pos info and update
	TilePos prevTPos = pobj->_TPos;
	SectorPos prevSPos = pobj->_SPos;
	pobj->UpdatePos(newCPos);
	pobj->UpdateRotation(Rotation);
	TilePos newTPos = pobj->_TPos;
	SectorPos newSPos = pobj->_SPos;

	//get new-around player list to send stop msg
	auto newAroundSPosVec = GetAroundSectorPos(newSPos, _props.ActiveSectorRange);
	auto AroundPlayerList = GetPlayerObjListFromSPosVec(&newAroundSPosVec);

	//update tile and sector if needed
	if (prevTPos != newTPos) {		//if tile has been changed
		//move tile
		EraseObjectFromTile(pobj, prevTPos);
		InsertObjectToTile(pobj, newTPos);

		if (prevSPos != newSPos) {		//if sector has been changed
			//get prev sector's around sectors
			auto prevAroundSPosVec = GetAroundSectorPos(prevSPos, _props.ActiveSectorRange);

			//get new and del sector
			std::vector<SectorPos> delSPosVec;
			std::vector<SectorPos> newSPosVec;
			GetNewAndDelSectorPos(&prevAroundSPosVec, &newAroundSPosVec, &delSPosVec, &newSPosVec);

			//get new and del player list
			auto DelPlayerList = GetPlayerObjListFromSPosVec(&delSPosVec);
			auto NewPlayerList = GetPlayerObjListFromSPosVec(&newSPosVec);

			//get new and del monster list
			auto DelMonsterList = GetMonsterObjListFromSPosVec(&delSPosVec, true);
			auto NewMonsterList = GetMonsterObjListFromSPosVec(&newSPosVec, true);

			//get new and del Crystal list
			auto DelCrystalList = GetCrystalObjListFromSPosVec(&delSPosVec);
			auto NewCrystalList = GetCrystalObjListFromSPosVec(&newSPosVec);

			//send deletion of player to del-players
			//and deletion of del-players to player
			for (auto& i : DelPlayerList) {
				_Proxy.PACKET_CS_GAME_RES_REMOVE_OBJECT(i->_Nid, pobj->_ClientID);
				_Proxy.PACKET_CS_GAME_RES_REMOVE_OBJECT(pobj->_Nid, i->_ClientID);
			}
			//send creation of player to new-players
			//and creation of new-players to player
			for (auto& i : NewPlayerList) {
				_Proxy.PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER(i->_Nid,
					pobj->_ClientID, pobj->_Chartype, pobj->_Nickname, pobj->_CPos.x, pobj->_CPos.y,
					pobj->_Rotation, pobj->_Level, 0, pobj->_isSit, pobj->_isDead);
				_Proxy.PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER(pobj->_Nid,
					i->_ClientID, i->_Chartype, i->_Nickname, i->_CPos.x, i->_CPos.y,
					i->_Rotation, i->_Level, 0, i->_isSit, i->_isDead);
			}

			//send del-monster to player
			for (auto& i : DelMonsterList) {
				_Proxy.PACKET_CS_GAME_RES_REMOVE_OBJECT(pobj->_Nid, i->_ClientID);
			}
			//send new-monster to player
			for (auto& i : NewMonsterList) {
				_Proxy.PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER(pobj->_Nid,
					i->_ClientID, i->_CPos.x, i->_CPos.y, i->_Rotation, 0);
			}

			//send del-Crystal to player
			for (auto& i : DelCrystalList) {
				_Proxy.PACKET_CS_GAME_RES_REMOVE_OBJECT(pobj->_Nid, i->_ClientID);
			}
			//send new-Crystal to player
			for (auto& i : NewCrystalList) {
				_Proxy.PACKET_CS_GAME_RES_CREATE_CRISTAL(pobj->_Nid,
					i->_ClientID, i->_CrystalType, i->_CPos.x, i->_CPos.y);
			}
		}
	}

	//send stop msg
	for (auto& i : AroundPlayerList) {
		if (i->_Nid == pobj->_Nid) { continue; }
		_Proxy.PACKET_CS_GAME_RES_STOP_CHARACTER(i->_Nid,
			pobj->_ClientID, pobj->_CPos.x, pobj->_CPos.y, pobj->_Rotation);
	}

	return true;
}

bool GameServer::ConGame_REQ_ATTACK1(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK1) {
	//get player object
	PlayerObject* pobj = GetPlayerObjByNID(__RPC__nid);
	if (pobj->_ClientID != ClientID) { _nsv.Disconnect(__RPC__nid); }
	pobj->UpdateLastRecvTime();

	//unset sit
	pobj->SetSit(false);

	//get around player list to send attack res
	auto AroundSPosVec = GetAroundSectorPos(pobj->_SPos, _props.ActiveSectorRange);
	auto AroundPlayerList = GetPlayerObjListFromSPosVec(&AroundSPosVec);

	//get target monsters from attack range
	std::vector<TilePos> RangeTPosVec = GetTilePosInCircleSector(pobj->_TPos, pobj->_Rotation, 60, 5);
	std::list<MonsterObject*> TargetedMonsterList;
	for (auto& tpos : RangeTPosVec) {
		for (auto& bobj_iter : GetTileByPos(tpos)->ObjMap) {
			BaseObject* bobj = bobj_iter.second;
			if (bobj->_ObjType == OBJTYPE::MONSTER) {
				MonsterObject* mobj = static_cast<MonsterObject*>(bobj);
				if (!mobj->isDead()) {
					TargetedMonsterList.push_back(mobj);
				}
			}
		}
	}

	//send attack res
	for (auto& i : AroundPlayerList) {
		if (i->_Nid == pobj->_Nid) { continue; }
		_Proxy.PACKET_CS_GAME_RES_ATTACK1(i->_Nid, pobj->_ClientID);
	}

	//give damage
	std::list<MonsterObject*> DeadMonsterList;
	for (auto& m : TargetedMonsterList) {
		//give damage to targeted monsters
		bool isDead = m->TakeDamage(pobj, PLAYER_DAMAGE);
		//and send damage res to around players - include me
		for (auto& p : AroundPlayerList) {
			_Proxy.PACKET_CS_GAME_RES_DAMAGE(p->_Nid, pobj->_ClientID, m->_ClientID, PLAYER_DAMAGE);
		}
		//and collect dead monsters
		if (isDead) { DeadMonsterList.push_back(m); }
	}

	//create crystal
	for (auto& m : DeadMonsterList) {
		//create crystal
		CrystalObject* crystal = _CrystalObjPool->Alloc();
		crystal->Init(this, m->_CPos, (BYTE)RandBetween(1, 3));
		InsertObjectToMap(crystal);
		InsertObjectToTile(crystal, crystal->_TPos);
		//send monster dead res to around players - include me
		//send creation of crystal res to around players - include me
		if (pobj->_SPos == m->_SPos) {
			for (auto& p : AroundPlayerList) {
				_Proxy.PACKET_CS_GAME_RES_MONSTER_DIE(p->_Nid, m->_ClientID);
				_Proxy.PACKET_CS_GAME_RES_CREATE_CRISTAL(p->_Nid,
					crystal->_ClientID, crystal->_CrystalType, crystal->_CPos.x, crystal->_CPos.y);
			}
		}
		else {
			auto MonAroundSPosVec = GetAroundSectorPos(m->_SPos, _props.ActiveSectorRange);
			auto MonAroundPlayerList = GetPlayerObjListFromSPosVec(&MonAroundSPosVec);
			for (auto& p : MonAroundPlayerList) {
				_Proxy.PACKET_CS_GAME_RES_MONSTER_DIE(p->_Nid, m->_ClientID);
				_Proxy.PACKET_CS_GAME_RES_CREATE_CRISTAL(p->_Nid,
					crystal->_ClientID, crystal->_CrystalType, crystal->_CPos.x, crystal->_CPos.y);
			}
		}
	}

	return true;
}

bool GameServer::ConGame_REQ_ATTACK2(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ATTACK2) {
	//get player object
	PlayerObject* pobj = GetPlayerObjByNID(__RPC__nid);
	if (pobj->_ClientID != ClientID) { _nsv.Disconnect(__RPC__nid); }
	pobj->UpdateLastRecvTime();

	//unset sit
	pobj->SetSit(false);

	//get around player list to send attack res
	auto AroundSPosVec = GetAroundSectorPos(pobj->_SPos, _props.ActiveSectorRange);
	auto AroundPlayerList = GetPlayerObjListFromSPosVec(&AroundSPosVec);

	//get target monsters from attack range
	std::vector<TilePos> RangeTPosVec = GetTilePosInCircleSector(pobj->_TPos, pobj->_Rotation, 60, 5);
	std::list<MonsterObject*> TargetedMonsterList;
	for (auto& tpos : RangeTPosVec) {
		for (auto& bobj_iter : GetTileByPos(tpos)->ObjMap) {
			BaseObject* bobj = bobj_iter.second;
			if (bobj->_ObjType == OBJTYPE::MONSTER) {
				MonsterObject* mobj = static_cast<MonsterObject*>(bobj);
				if (!mobj->isDead()) {
					TargetedMonsterList.push_back(mobj);
				}
			}
		}
	}

	//send attack res
	for (auto& i : AroundPlayerList) {
		if (i->_Nid == pobj->_Nid) { continue; }
		_Proxy.PACKET_CS_GAME_RES_ATTACK2(i->_Nid, pobj->_ClientID);
	}

	//give damage
	std::list<MonsterObject*> DeadMonsterList;
	for (auto& m : TargetedMonsterList) {
		//give damage to targeted monsters
		bool isDead = m->TakeDamage(pobj, PLAYER_DAMAGE);
		//and send damage res to around players - include me
		for (auto& p : AroundPlayerList) {
			_Proxy.PACKET_CS_GAME_RES_DAMAGE(p->_Nid, pobj->_ClientID, m->_ClientID, PLAYER_DAMAGE);
		}
		//and collect dead monsters
		if (isDead) { DeadMonsterList.push_back(m); }
	}

	//create crystal
	for (auto& m : DeadMonsterList) {
		//create crystal
		CrystalObject* crystal = _CrystalObjPool->Alloc();
		crystal->Init(this, m->_CPos, (BYTE)RandBetween(1, 3));
		InsertObjectToMap(crystal);
		InsertObjectToTile(crystal, crystal->_TPos);
		//send monster dead res to around players - include me
		//send creation of crystal res to around players - include me
		if (pobj->_SPos == m->_SPos) {
			for (auto& p : AroundPlayerList) {
				_Proxy.PACKET_CS_GAME_RES_MONSTER_DIE(p->_Nid, m->_ClientID);
				_Proxy.PACKET_CS_GAME_RES_CREATE_CRISTAL(p->_Nid,
					crystal->_ClientID, crystal->_CrystalType, crystal->_CPos.x, crystal->_CPos.y);
			}
		}
		else {
			auto MonAroundSPosVec = GetAroundSectorPos(m->_SPos, _props.ActiveSectorRange);
			auto MonAroundPlayerList = GetPlayerObjListFromSPosVec(&MonAroundSPosVec);
			for (auto& p : MonAroundPlayerList) {
				_Proxy.PACKET_CS_GAME_RES_MONSTER_DIE(p->_Nid, m->_ClientID);
				_Proxy.PACKET_CS_GAME_RES_CREATE_CRISTAL(p->_Nid,
					crystal->_ClientID, crystal->_CrystalType, crystal->_CPos.x, crystal->_CPos.y);
			}
		}
	}

	return true;
}

bool GameServer::ConGame_REQ_PICK(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_PICK) {
	//get player object
	PlayerObject* pobj = GetPlayerObjByNID(__RPC__nid);
	if (pobj->_ClientID != ClientID) { _nsv.Disconnect(__RPC__nid); }
	pobj->UpdateLastRecvTime();

	//unset sit
	pobj->SetSit(false);

	//get around player list to send pick res
	auto AroundSPosVec = GetAroundSectorPos(pobj->_SPos, _props.ActiveSectorRange);
	auto AroundPlayerList = GetPlayerObjListFromSPosVec(&AroundSPosVec);

	//get crystal on current tile
	auto AroundTPosVec = GetAroundTilePos(pobj->_TPos, 1);
	std::list<CrystalObject*> CrystalList;
	for (auto& tp : AroundTPosVec) {
		Tile* curtile = GetTileByPos(tp);
		for (auto& bobj_iter : curtile->ObjMap) {
			BaseObject* bobj = bobj_iter.second;
			if (bobj->_ObjType == OBJTYPE::CRYSTAL) {
				CrystalList.push_back(static_cast<CrystalObject*>(bobj));
			}
		}
	}

	//send pick res
	for (auto& p : AroundPlayerList) {
		if (p->_Nid == pobj->_Nid) { continue; }
		_Proxy.PACKET_CS_GAME_RES_PICK(p->_Nid, pobj->_ClientID);
	}

	//get crystal
	for (auto& c : CrystalList) {
		//get crystal
		int getcrystal = CrystalTypeToAmount(c->_CrystalType);
		pobj->_Crystal += getcrystal;
		_AsyncGameDB.Write_GetCrystal(pobj->_AccountNo, getcrystal, pobj->_Crystal);
		//send pick crystal res to around player - include me
		for (auto& p : AroundPlayerList) {
			_Proxy.PACKET_CS_GAME_RES_PICK_CRISTAL(p->_Nid, pobj->_ClientID, c->_ClientID, pobj->_Crystal);
		}
		if (c->_SPos != pobj->_SPos) {
			auto CrystalAroundSPosVec = GetAroundSectorPos(c->_SPos, _props.ActiveSectorRange);
			auto CrystalAroundPlayerList = GetPlayerObjListFromSPosVec(&CrystalAroundSPosVec);
			for (auto& p : CrystalAroundPlayerList) {
				_Proxy.PACKET_CS_GAME_RES_REMOVE_OBJECT(p->_Nid, c->_ClientID);
			}
		}
		//remove crystal
		EraseObjectFromTile(c, c->_TPos);
		EraseObjectFromMap(c);
		_CrystalObjPool->Free(c);
	}


	return true;
}

bool GameServer::ConGame_REQ_SIT(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_SIT) {
	//get player object
	PlayerObject* pobj = GetPlayerObjByNID(__RPC__nid);
	if (pobj->_ClientID != ClientID) { _nsv.Disconnect(__RPC__nid); }
	pobj->UpdateLastRecvTime();

	//set sit
	pobj->SetSit(true);

	//get around player list to send sit res
	auto AroundSPosVec = GetAroundSectorPos(pobj->_SPos, _props.ActiveSectorRange);
	auto AroundPlayerList = GetPlayerObjListFromSPosVec(&AroundSPosVec);

	//send sit res
	for (auto& p : AroundPlayerList) {
		if (p->_Nid == pobj->_Nid) { continue; }
		_Proxy.PACKET_CS_GAME_RES_SIT(p->_Nid, pobj->_ClientID);
	}

	return true;
}

bool GameServer::ConGame_REQ_PLAYER_RESTART(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_PLAYER_RESTART) {
	//get player object
	PlayerObject* pobj = GetPlayerObjByNID(__RPC__nid);
	pobj->UpdateLastRecvTime();

	//check player is really dead
	if (!pobj->isDead()) { return true; }
	//reset Dead flag
	pobj->_isDead = false;

	//send player restart res to player
	_Proxy.PACKET_CS_GAME_RES_PLAYER_RESTART(pobj->_Nid);

	//get new random Pos to respawn
	TilePos newTPos{ 85 + (rand() % 21) ,93 + (rand() % 31) };
	ClientPos newCPos = TilePosToClientPos(newTPos);

	//get OLD around players to send deletion of player
	auto oldAroundSPosVec = GetAroundSectorPos(pobj->_SPos, _props.ActiveSectorRange);
	auto oldAroundPlayerList = GetPlayerObjListFromSPosVec(&oldAroundSPosVec);

	//reset player pos - other stats were resetted at dead
	EraseObjectFromTile(pobj, pobj->_TPos);
	pobj->UpdatePos(newCPos);
	pobj->UpdateRotation(0);
	InsertObjectToTile(pobj, pobj->_TPos);
	_AsyncGameDB.Write_Restart(pobj->_AccountNo, pobj->_CPos, pobj->_Rotation);

	//send deletion of player to OLD around players
	for (auto& p : oldAroundPlayerList) {
		_Proxy.PACKET_CS_GAME_RES_REMOVE_OBJECT(p->_Nid, pobj->_ClientID);
	}

	//re-join to game
	JoinPlayerToGame(pobj, true);

	return true;
}

bool GameServer::ConGame_REQ_ECHO(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_ECHO) {
	_Proxy.PACKET_CS_GAME_RES_ECHO(__RPC__nid, AccountoNo, SendTick);
	return true;
}

bool GameServer::ConGame_REQ_HEARTBEAT(dfRPC_STUB_PARAM_CNetRPCStub_PACKET_CS_GAME_REQ_HEARTBEAT) {
	//get player object
	PlayerObject* pobj = GetPlayerObjByNID(__RPC__nid);
	pobj->UpdateLastRecvTime();
	return true;
}
