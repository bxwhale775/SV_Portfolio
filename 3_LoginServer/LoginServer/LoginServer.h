#pragma once
#include "NetSV-include/CNetServer.h"
#include "TinyRouter.h"
#include "DBConnMgr_MT.h"
#include "CommonProtocol.h"
#include "PDHCounter.h"
#include "MonitorClientMgr.h"
#pragma comment(lib, "NetSV-include/lib/NetServerLib.lib")

class NetCallback;

struct LoginServer_Properties : public CNetServer_Properties {
	const WCHAR* MyDomain_Name = L"example.com";
	USHORT ChatServer_Port = 12001;
	USHORT GameServer_Port = 10010;

	const char* DB_HostName = "tcp://127.0.0.1:3306/accountdb";
	const char* DB_UserName = "root";
	const char* DB_Password = "12345678";

	const char* GameDB_HostName = "tcp://127.0.0.1:3306/gamedb";
	const char* GameDB_UserName = "root";
	const char* GameDB_Password = "12345678";

	const char* Redis_IP = "127.0.0.1";
	size_t Redis_Port = 6379;

	const WCHAR* MonitorSV_IP = L"127.0.0.1";
	u_short MonitorSV_Port = 12300;
};

class LoginServer {
public:
	class NetCallback : public Interface_CNetServer_Callback {
	public:
		LoginServer* lsv = nullptr;
		virtual bool OnWorkerThreadStart(DWORD ThrID);
		virtual bool OnWorkerThreadEnd(DWORD ThrID);
		virtual bool OnAccept(const WCHAR* ip, const u_short port);
		virtual bool OnJoin(const NID sessID);
		virtual bool OnDisconnect(const NID sessID);
		virtual bool OnRecv(const NID sessID, SerialBuffer* databuf);
	};
private:
	CNetServer _sv;
	NetCallback _ncbk;
	DWORD _TlsIdxRedis;
	LoginServer_Properties _lprops;
	DBConnMgr_MT _db;
	DBConnMgr_MT _gamedb;
	TinyRouter _Router;

	//monitor
	MonitorClientMgr* _mtrmgr = nullptr;
	ULONGLONG _AuthCount = 0;
	PDHCounter _pdh;
	ULONG _SessCnt = 0;
	
public:
	LoginServer() { _TlsIdxRedis = TlsAlloc(); }
	~LoginServer() { 
		TlsFree(_TlsIdxRedis); 
		if (_mtrmgr != nullptr) { delete _mtrmgr; }
	}

	void Init(LoginServer_Properties props);
	void Start();
	void Stop();
	void PrintLog();

private:
	void InitRouter();
	bool GetIpByDomainName(const WCHAR* in_domain, WCHAR* out_firstIP);
};
