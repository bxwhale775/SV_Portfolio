#pragma once

class NetContents;
UINT WINAPI Thread_Contents(PVOID param);

struct SessMovingData {
	NID sessID = 0;
	NetSession* sess = nullptr;
	PVOID data = nullptr;
	CID MovingFrom = 0;		// depart, sended
	CID MovingTo = 0;		// dest,   recved
};

struct ContentsDataBlock {
	NetContents* ct = nullptr;
	LFQueue<SessMovingData> SessQ;
	UINT ConSessNum = 0;
	UINT TickCount = 0;
	HANDLE ThrHandle = INVALID_HANDLE_VALUE;

	CNetServer* nsv = nullptr;
	std::unordered_map<NID, NetSession*> ConSessMap;
	std::vector<NID> DelConSessVec;
	CID ContentsID = 0;
	unsigned int TPS = 0;
};

class NetContents {
private:
	friend class CNetServer;
	friend struct NetSession;
	friend UINT WINAPI Thread_Contents(PVOID param);
	SuperRPCStub* _RPCStub = nullptr;

protected:
	ContentsDataBlock* _cdb = nullptr;
	CNetServer* _sv = nullptr;

public:
	bool MoveContents(NID sessID, CID destCID, PVOID data);
	bool AttachRPCStub(SuperRPCStub* rpcstub);
	bool AttachRPCProxy(SuperRPCProxy* rpcproxy);

protected:
	virtual bool OnContentsThreadStart(DWORD ThrID) { return false; }
	virtual bool OnContentsThreadEnd(DWORD ThrID) { return false; }
	virtual bool OnContentsJoin(NID sessID, CID departCID, const PVOID data) { return false; }
	virtual bool OnContentsLeave(NID sessID, CID destCID) { return false; }
	virtual bool OnContentsRecv(NID sessID, SerialBuffer* databuf) { return false; }
	virtual bool OnContentsUpdate() { return false; }
	virtual bool OnContentsDisconnect(NID sessID) { return false; }
};
