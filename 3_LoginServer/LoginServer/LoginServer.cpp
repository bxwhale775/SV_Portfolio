#include "LoginServer.h"

void LoginServer::Init(LoginServer_Properties props) {
	_lprops = props;
	_lprops.disableLinger = true;
	_ncbk.lsv = this;
	_sv.Init(_lprops, &_ncbk);
	_db.Init(_lprops.DB_HostName, _lprops.DB_UserName, _lprops.DB_Password);
	_gamedb.Init(_lprops.GameDB_HostName, _lprops.GameDB_UserName, _lprops.GameDB_Password);
	_pdh.Init();
	InitRouter();
	return;
}

void LoginServer::Start() {
	_sv.Start();
	return;
}

void LoginServer::Stop() {
	_sv.Stop();
	return;
}

void LoginServer::PrintLog() {
	static ULONGLONG PrevAcceptCount = 0;
	static ULONGLONG PrevSendCount = 0;
	static ULONGLONG PrevRecvCount = 0;
	static ULONGLONG PrevAuthCount = 0;

	ULONGLONG acceptc = _sv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_ACCEPT);
	ULONGLONG sendc = _sv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_SEND);
	ULONGLONG recvc = _sv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_RECV);
	int gpp = _sv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_CAP_PKT);
	int gup = _sv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_USE_PKT);
	ULONGLONG updc = _AuthCount;
	ULONG sesc = _SessCnt;
	int nsp = _sv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_CAP_NSESS);
	int sepu = _sv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_USE_NSESS);

	ULONGLONG acptps = acceptc - PrevAcceptCount;
	ULONGLONG sndps = sendc - PrevSendCount;
	ULONGLONG rcvps = recvc - PrevRecvCount;
	ULONGLONG authps = updc - PrevAuthCount;

	PrevAcceptCount = acceptc;
	PrevSendCount = sendc;
	PrevRecvCount = recvc;
	PrevAuthCount = updc;

	if (_mtrmgr == nullptr) {
		_mtrmgr = new MonitorClientMgr();
		_mtrmgr->Init(_lprops.MonitorSV_IP, _lprops.MonitorSV_Port, 5);
		_mtrmgr->Start();
		_mtrmgr->LoginToMonitor();
	}
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, 1);
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, (int)_pdh.GetProcessCpuTime());
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, (int)_pdh.ToMiB(_pdh.GetProcessPrivateBytes()));
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_LOGIN_SESSION, (int)sepu);
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, (int)authps);
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, (int)gpp);

	std::string line0, line1, line2, line3, line4, line5;

	line0 += "=====================\n";

	line1 += "NetSess: "; line1 += std::to_string(sepu);
	line1 += ", LoginSess: "; line1 += std::to_string(sesc);
	line1 += ", AcceptCnt: "; line1 += std::to_string(acceptc);
	line1 += " ("; line1 += std::to_string(acptps);
	line1 += "/s)\n";

	line2 += "PKpool: "; line2 += std::to_string(gpp);
	line2 += ", NSpool: "; line2 += std::to_string(nsp);
	line2 += "\n";

	line3 += "AuthRate: "; line3 += std::to_string(authps);
	line3 += "/s, PKUsing: "; line3 += std::to_string(gup);
	line3 += ", NetSessUsing: "; line3 += std::to_string(sepu);
	line3 += "\n";

	line4 += "Send: "; line4 += std::to_string(sndps);
	line4 += "/s, Recv: "; line4 += std::to_string(rcvps);
	line4 += "/s\n";

	line5 += "SysCPU: "; line5 += std::to_string(_pdh.GetSysCpuTime());
	line5 += "%, ProCPU: "; line5 += std::to_string(_pdh.GetProcessCpuTime());
	line5 += "%\n";
	line5 += "SysMemNPP: "; line5 += std::to_string(_pdh.ToMiB(_pdh.GetSysNonPagedPool()));
	line5 += "MiB, ProMemPB: "; line5 += std::to_string(_pdh.ToMiB(_pdh.GetProcessPrivateBytes()));
	line5 += "MiB\n";
	line5 += "NetRecv: "; line5 += std::to_string(_pdh.ToMiB(_pdh.GetNetworkRecv()));
	line5 += "MiB/s, NetSend: "; line5 += std::to_string(_pdh.ToMiB(_pdh.GetNetworkSend()));
	line5 += "MiB/s\n";

	line0 += line1 += line2 += line3 += line4 += line5;
	std::cout << line0 << std::endl;

	return;
}

void LoginServer::InitRouter() {
	WCHAR myip[NETSV_IPSTR_LEN];
	GetIpByDomainName(_lprops.MyDomain_Name, myip);
	_Router.AddRoute(L"0.0.0.0", L"0.0.0.0", myip);
	_Router.AddRoute(L"127.0.0.0", L"255.0.0.0", L"127.0.0.1");
	_Router.AddRoute(L"10.0.1.0", L"255.255.255.0", L"10.0.1.1");
	_Router.AddRoute(L"10.0.2.0", L"255.255.255.0", L"10.0.2.1");
	wprintf(L"Default Gateway: %s\n", myip);
	return;
}

bool LoginServer::GetIpByDomainName(const WCHAR* in_domain, WCHAR* out_firstIP) {
	// setup hints
	ADDRINFOW hints;
	SecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;

	// get ip address list from domain name
	ADDRINFOW* result;
	int ret = GetAddrInfo(in_domain, NULL, &hints, &result);
	if (ret != 0) {
		return false;
	}

	// get first ip from ip list
	ADDRINFOW* ptr;
	int cnt = 0;
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		SOCKADDR_IN* addr_ip = (SOCKADDR_IN*)ptr->ai_addr;
		InetNtop(AF_INET, &addr_ip->sin_addr, out_firstIP, NETSV_IPSTR_LEN);
		cnt++;
		break;
	}

	// free result
	FreeAddrInfo(result);

	if (cnt == 0) { return false; }
	else { return true; }
}

bool LoginServer::NetCallback::OnWorkerThreadStart(DWORD ThrID) {
	//alloc dbconn to TLS
	int ret = lsv->_db.AllocDBConn(nullptr);
	if (ret != 0) {
		printf("DBConn Error at WStart: %d\n", ret);
		return true;
	}

	//alloc game dbconn to TLS
	ret = lsv->_gamedb.AllocDBConn(nullptr);
	if (ret != 0) {
		printf("DBConn Error at WStart: %d\n", ret);
		return true;
	}

	//alloc redis client to TLS
	cpp_redis::client* rclient = new cpp_redis::client;
	try {
		rclient->connect(lsv->_lprops.Redis_IP, lsv->_lprops.Redis_Port);
	}
	catch (std::exception& e) {
		printf("Redis Error at WStart: %s\n", e.what());
		return true;
	}
	TlsSetValue(lsv->_TlsIdxRedis, rclient);

	return true;
}

bool LoginServer::NetCallback::OnWorkerThreadEnd(DWORD ThrID) {
	//free and delete dbconn from TLS
	lsv->_db.FreeDBConn();
	//free and delete game dbconn from TLS
	lsv->_gamedb.FreeDBConn();
	//free and delete redis client from TLS
	cpp_redis::client* rclient = (cpp_redis::client*)TlsGetValue(lsv->_TlsIdxRedis);
	delete rclient;
	return true;
}

bool LoginServer::NetCallback::OnAccept(const WCHAR* ip, const u_short port) {
	return true;
}

bool LoginServer::NetCallback::OnJoin(const NID sessID) {
	InterlockedIncrement(&lsv->_SessCnt);
	return true;
}

bool LoginServer::NetCallback::OnDisconnect(const NID sessID) {
	InterlockedDecrement(&lsv->_SessCnt);
	return true;
}

bool LoginServer::NetCallback::OnRecv(const NID sessID, SerialBuffer* databuf) {
	//get dbconn from TLS
	DBConnMgr* dbconn;
	int ret = lsv->_db.AllocDBConn(&dbconn);
	if (ret != 0) {
		printf("DBConn Error at: %d\n", ret);
	}

	//get game dbconn from TLS
	DBConnMgr* gamedbconn;
	ret = lsv->_gamedb.AllocDBConn(&gamedbconn);
	if (ret != 0) {
		printf("DBConn Error at: %d\n", ret);
	}

	//get redis client from TLS
	cpp_redis::client* rclient = (cpp_redis::client*)TlsGetValue(lsv->_TlsIdxRedis);

	//get data from packet
	WORD pktype = 0;
	INT64 AccountNo;
	char SessionKey[64];
	databuf->GetData(&pktype, sizeof(pktype));
	databuf->GetData(&AccountNo, sizeof(AccountNo));
	databuf->GetData(&SessionKey, sizeof(SessionKey));

	//check packet type
	if (pktype != en_PACKET_CS_LOGIN_REQ_LOGIN) {
		printf("Invalid LoginReq Packet Type: %d\n", pktype);
		lsv->_sv.Disconnect(sessID);
		return true;
	}

	//check DB by AccountNo
	sql::ResultSet* dbres = dbconn->executeQuery(
		L"SELECT * FROM `account` WHERE `accountno` = %lld;",
		AccountNo
	);

	//get data from DB result
	std::string straccountno;
	std::string userid;
	std::string userpass;
	std::string usernick;
	int rowcnt = 0;
	while (dbres->next()) {
		straccountno = dbres->getString("accountno");
		userid = dbres->getString("userid");
		userpass = dbres->getString("userpass");
		usernick = dbres->getString("usernick");
		rowcnt++;
	}
	delete dbres;

	//set req status
	BYTE reqstatus = dfLOGIN_STATUS_OK;
	if (rowcnt == 0) {
		reqstatus = dfLOGIN_STATUS_FAIL;
	}

	//convert some data for packet/redis
	WCHAR ID[20];
	MultiByteToWideChar(CP_UTF8, 0, userid.c_str(), -1, ID, 20);
	WCHAR NICK[20];
	MultiByteToWideChar(CP_UTF8, 0, usernick.c_str(), -1, NICK, 20);
	std::string SesskeyStr = std::string(SessionKey, 64);

	//do route for game/chat server
	WCHAR sessip[NETSV_IPSTR_LEN];
	WCHAR svip[NETSV_IPSTR_LEN];
	lsv->_sv.GetIpAddress(sessID, sessip);
	lsv->_Router.Route(sessip, svip);

	//register login info at redis
	if (reqstatus == dfLOGIN_STATUS_OK) {
		rclient->set(straccountno, SesskeyStr);
		rclient->expire(straccountno, 600);
	}

	//write log at gamedb
	const WCHAR* query =
		L"INSERT INTO `gamedb`.`gamelog`(`type`, `code`, `accountno`, `servername`, `param1`, `message`) "
		L"VALUES(%d, %d, %lld, '%s', %u, '%s:%d'); ";
	gamedbconn->executeUpdate(query,
		100, 101, AccountNo, L"Login", reqstatus, sessip, lsv->_sv.GetPort(sessID));

	//build packet
	WORD reqpktype = en_PACKET_CS_LOGIN_RES_LOGIN;

	SerialBuffer* buf = lsv->_sv.AllocPacket();
	buf->PutData(&reqpktype, sizeof(reqpktype));
	buf->PutData(&AccountNo, sizeof(AccountNo));
	buf->PutData(&reqstatus, sizeof(reqstatus));
	buf->PutData(&ID, sizeof(ID));
	buf->PutData(&NICK, sizeof(NICK));
	buf->PutData(svip, sizeof(svip));
	buf->PutData(&lsv->_lprops.GameServer_Port, sizeof(lsv->_lprops.GameServer_Port));
	buf->PutData(svip, sizeof(svip));
	buf->PutData(&lsv->_lprops.ChatServer_Port, sizeof(lsv->_lprops.ChatServer_Port));

	//send packet
	rclient->sync_commit();
	if (90000 <= AccountNo) { lsv->_sv.SendPacket(sessID, buf, true, false); }
	else { lsv->_sv.SendPacket(sessID, buf, true, true); }
	lsv->_sv.FreePacket(buf);

	//monitor
	InterlockedIncrement(&lsv->_AuthCount);

	return true;
}
