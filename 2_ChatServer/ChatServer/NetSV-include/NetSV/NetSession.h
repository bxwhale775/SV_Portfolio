#pragma once
#include "ringbuffer.h"
#include "LFStack.h"
#include "LFQueue.h"
#include "Serialbuffer.h"
#include "MemoryPoolTLS.h"

#define IO_SEND 1
#define IO_RECV 2

#define MAX_SEND_PACKETS 256
#define IDX_BYTE 2

#define NETSV_INVALID_NID ULLONG_MAX
#define NETSV_IPSTR_LEN 16

struct SessMovingData;


struct OVERLAPPED_EXT {
	OVERLAPPED ovl;
	int IOType;
};

struct NetSession {
	NID sessID = NETSV_INVALID_NID;
	SOCKET sock = INVALID_SOCKET;
	LFQueue<SerialBuffer*> sendQ;
	CRingBuffer* pRecvQ = nullptr;
	OVERLAPPED_EXT sendOvl = { 0, 0 };
	OVERLAPPED_EXT recvOvl = { 0, 0 };

	union {
		struct __s {
			short isDead;
			short IOCount;
		} CountSet = { 1,1 };
		long IOCandDeadflag;
	};

	char isSending = 0;
	SOCKET ClosePendingSock = INVALID_SOCKET;
	char SendAndDisconn = 0;

	SerialBuffer* sendedPacketArr[MAX_SEND_PACKETS] = { nullptr, };
	int sendedPacketCount = 0;

	CID ContentsID = 0;
	LFQueue<SerialBuffer*> packetQ;
	bool isMoving = false;
	bool isMovingToWorkerThread = false;
	bool isDisconned = false;
	SessMovingData* psmd;

	WCHAR ip[NETSV_IPSTR_LEN];
	u_short port;

	~NetSession() {
		if (pRecvQ != nullptr) { delete pRecvQ; }
	}
};

class NetSessionMgr {
private:
	MemoryPoolTLS<NetSession> _NetSessPool;
	NetSession** _NetSessArr = nullptr;
	int _Capacity = 0;
	LFStack<unsigned int> _UnusedIdx;
	NID _LastGenSessID = 0;
	int _RingbufSize = 0;

public:
	NetSessionMgr(int capacity, int ringbufsize = 256, int NSPool_initblocknum = 0, int NSPool_chunksize = 100);
	~NetSessionMgr();

	NetSession* CreateNewNetSess(SOCKET sock, HANDLE hiocp, WCHAR* ip, u_short port);
	bool DeleteNetSess(NetSession* delsess);
	bool CloseSessSock(NetSession* delsess);
	bool CloseAllSessSock();
	NetSession* GetSess(NID SessID);

	int GetNetSessPoolCapacity();
	int GetNetSessPoolUsingCount();

private:
	unsigned int GetIdx(NID SessID);
	bool GenerateNewID(NID* SessID);
	bool RetrieveID(NID SessID);
};

