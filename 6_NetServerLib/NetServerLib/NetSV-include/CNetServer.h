#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <unordered_map>
#include "NetSV/TypeDefine.h"
#include "NetSV/NetHeader.h"
#include "NetSV/Logger.h"
#include "NetSV/Serialbuffer.h"
#include "NetSV/NetSession.h"
#include "NetSV/RPCSuperClass.h"
#include "NetSV/NetContents.h"
#include "NetSV/SerialBufferIO.h"

class CNetServer;

class Interface_CNetServer_Callback {
public:
	CNetServer* _sv = nullptr;

	virtual bool OnWorkerThreadStart(DWORD ThrID) {
		return false;
	}
	virtual bool OnWorkerThreadEnd(DWORD ThrID) {
		return false;
	}
	virtual bool OnAccept(const WCHAR* ip, const u_short port) {
		return false;
	}
	virtual bool OnJoin(const NID sessID) {
		return false;
	}
	virtual bool OnDisconnect(const NID sessID) {
		return false;
	}
	virtual bool OnRecv(const NID sessID, SerialBuffer* databuf) {
		return false;
	}
	virtual bool OnContentsJoin(NID sessID, CID departCID, const PVOID data) {
		return false;
	}
	virtual bool OnContentsLeave(NID sessID, CID destCID) {
		return false;
	}
};

struct CNetServer_Properties {
	u_short Port = 12001;
	DWORD CocurrentThreadNum = 6;
	DWORD WorkerThreadNum = 6;
	DWORD MaxSessionNum = 30000;
	BYTE PacketCode = 0x77;
	BYTE EncryptKey = 0x32;
	bool disableNagle = false;
	bool disableLinger = false;
	bool disableEncrypt = false;
	const wchar_t* LogFileName = L"Netlog.txt";
	int PKPool_InitBlockNum = 0;
	int PKPool_ChunkSize = 100;
	int PK_Size = 100;
	int NSPool_InitBlockNum = 0;
	int NSPool_ChunkSize = 100;
	int NS_RingBufSize = 2048;
	bool Client_RunAsClient = false;
	bool Client_ManualDispatch = false;
	const WCHAR* Client_IP = L"127.0.0.1";
	u_short Client_Port = 12300;
};

class CNetServer {
private:
	//callback stuff
	Interface_CNetServer_Callback* cbk = nullptr;

	//packet stuff
	MemoryPoolTLS<SerialBuffer>* _SBPool = nullptr;

	//RPC stuff
	SuperRPCStub* _RPCStub = nullptr;

	//contents thread stuff
	ContentsDataBlock* _CDBArray = nullptr;
	CID _LastGenCID = 0;
	UINT _AllocedCDB = 0;

	//session stuff
	NetSessionMgr* _NetSessMgr = nullptr;
	int _MaxSession = 0;

	//networking stuff
	HANDLE _hiocp = INVALID_HANDLE_VALUE;
	SOCKET _Listen_Sock = INVALID_SOCKET;
	u_short _Port = 0;

	//Networking thread stuff
	HANDLE* _ThrHandles = nullptr;
	int _CocurrentThreadNum = 0;
	int _WorkerThreadNum = 0;

	//misc stuff
	CNetServer_Properties _props;
	bool _isInited = false;
	bool _isStarted = false;
	Logger* _pNetlog = nullptr;

	//client stuff
	bool _usingDefaultCallback = false;
	NetSession* _ClientCon = nullptr;

	//monitor
	unsigned long long _AcceptCount = 0;
	unsigned long long _SendCount = 0;
	unsigned long long _RecvCount = 0;

public:
	CNetServer() {}
	~CNetServer();

	bool Init(
		CNetServer_Properties props,
		Interface_CNetServer_Callback* Callback,
		UINT AllocContentsNum = 0
	);
	bool Start();
	bool Stop();
	bool AttachRPCStub(SuperRPCStub* rpcstub);
	bool AttachRPCProxy(SuperRPCProxy* rpcproxy);

	ContentsDataBlock* GetCDB(CID cid);
	CID AttachContents(NetContents* ct, unsigned int tps = 50);
	bool MoveContents(NID sessID, CID destCID, PVOID data);

	bool SendPacket(NID sessID, SerialBuffer* packet, bool async = true, bool sendanddisconnect = false);
	bool Disconnect(NID sessID);
	bool ManualDispatch();

	bool GetIpAddress(NID sessID, WCHAR* str);
	u_short GetPort(NID sessID);

	SerialBuffer* AllocPacket();
	void FreePacket(SerialBuffer* delbuf);

	//monitor
	int GetMonitorDataInt(en_NETSV_MTR_TYPE type);
	ULONGLONG GetMonitorDataULL(en_NETSV_MTR_TYPE type);

private:
	friend UINT WINAPI Thread_Accept(PVOID param);
	friend UINT WINAPI Thread_Worker(PVOID param);
	friend UINT WINAPI Thread_Contents(PVOID param);
	friend class NetContents;

	bool PostSend(NetSession* sess);
	bool PostRecv(NetSession* sess);
	void IncIOCnt(NetSession* sess);
	void DecIOCnt(NetSession* sess);
};


