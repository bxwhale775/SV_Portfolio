#pragma once
#include <map>
#include <set>
#include <unordered_set>
#include "NetSV-include/CNetServer.h"
#include "Session.h"
#include "Sector.h"
#include "CommonProtocol.h"
#include "MonitorClientMgr.h"
#include "PDHCounter.h"
#pragma comment (lib, "NetSV-include/lib/NetServerLib.lib")

struct ChatServer_Properties : public CNetServer_Properties {
	unsigned int MaxUserNum = 20000;

	const char* Redis_IP = "127.0.0.1";
	size_t Redis_Port = 6379;

	const WCHAR* MonitorSV_IP = L"127.0.0.1";
	u_short MonitorSV_Port = 12300;
};

struct Task {
	en_TASK_TYPE type;
	NID sessID;
	SerialBuffer* buf = nullptr;
};

//chatserver ST
class ChatServer {
public:
	class NetCallBack : public Interface_CNetServer_Callback {
	public:
		ChatServer* csv = nullptr;
		virtual bool OnWorkerThreadStart(DWORD ThrID) {
			printf("Worker Start!: %d\n", ThrID);
			return true;
		}
		virtual bool OnWorkerThreadEnd(DWORD ThrID) {
			printf("Worker End!: %d\n", ThrID);
			return true;
		}
		virtual bool OnAccept(const WCHAR* ip, const u_short port) {
			return true;
		}
		virtual bool OnJoin(const NID sessID) {
			Task* t = csv->AllocTask(en_TASK_CS_NEW_SESS);
			t->sessID = sessID;
			csv->RunTask(t);
			return true;
		}
		virtual bool OnDisconnect(const NID sessID) {
			Task* t = csv->AllocTask(en_TASK_CS_DEL_SESS);
			t->sessID = sessID;
			csv->RunTask(t);
			return true;
		}
		virtual bool OnRecv(const NID sessID, SerialBuffer* databuf) {
			Task* t = csv->AllocTask(en_TASK_CS_RECV_FROM_CLIENT);
			t->sessID = sessID;
			t->buf = databuf;
			csv->RunTask(t);
			return true;
		}
	};
private:
	MemoryPoolTLS<Task> _TaskPool = MemoryPoolTLS<Task>();
	LFQueue<Task*> _TaskQ = LFQueue<Task*>();
	HANDLE _hUpdateEv;
	ChatServer_Properties _ChatProps;

	MemoryPoolTLS<Session> _SessPool = MemoryPoolTLS<Session>();
	std::map<NID, Session*> _UserArr;
	std::map<INT64, Session*> _AccountNoArr;
	int _MaxUser;
	SectorMgr _Secmgr = SectorMgr();

	HANDLE _hiocp_login;
	HANDLE _ThrHandles[3];
	friend UINT WINAPI Thread_ChatUpdate(PVOID param);
	friend UINT WINAPI Thread_TimeOut(PVOID param);
	friend UINT WINAPI Thread_Login(PVOID param);
	bool _isShutdown = false;

	CNetServer _sv;
	Logger _csvlog = Logger(L"chatsvlog.txt");
	NetCallBack _ncbk;

	//monitor
	MonitorClientMgr* _mtrmgr = nullptr;
	PDHCounter _pdh;
	unsigned long _CurrentNetSess = 0;
	unsigned long _CurrentUser = 0;
	unsigned long _TimeoutUser = 0;
	unsigned long _TimeoutSess = 0;
	unsigned long _CurrentSess = 0;
	unsigned long _LoginIocpPoolCount = 0;
	unsigned long long _UpdateThrCount = 0;

	unsigned long long _PrevAcceptCount = 0;
	unsigned long long _PrevSendCount = 0;
	unsigned long long _PrevRecvCount = 0;
	unsigned long long _PrevUpdateCount = 0;

public:
	void Init(ChatServer_Properties props);
	void Start();
	void Stop();

	void PrintLog();

private:
	Task* AllocTask(en_TASK_TYPE type = en_TASK_CS_NULL);
	void FreeTask(Task* task);

	Session* AllocSess(NID uid);
	void FreeSess(Session* sess);

	Session* FindSessByUid(NID uid);

	void RunTask(Task* task);
};
