#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include "NetSV-include/NetSV/TypeDefine.h"
#include "NetSV-include/NetSV/ringbuffer.h"
#include "NetSV-include/NetSV/Serialbuffer.h"
#include "NetSV-include/NetSV/NetSession.h"
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm")


NetSessionMgr::NetSessionMgr(int capacity, int ringbufsize, int NSPool_initblocknum, int NSPool_chunksize) :
	_NetSessPool(MemoryPoolTLS<NetSession>(NSPool_initblocknum, NSPool_chunksize)),
	_Capacity(capacity),
	_RingbufSize(ringbufsize)
{
	_NetSessArr = new NetSession*[capacity];
	for (int i = 0; i < capacity; i++) {
		_NetSessArr[i] = nullptr;
	}

	for (int i = capacity - 1; i >= 0; i--) {
		_UnusedIdx.push(i);
	}

	return;
}

NetSessionMgr::~NetSessionMgr() {
	delete[] _NetSessArr;
	return;
}

NetSession* NetSessionMgr::CreateNewNetSess(SOCKET sock, HANDLE hiocp, WCHAR* ip, u_short port) {
	NetSession* newsess = _NetSessPool.Alloc();

	if (!GenerateNewID(&newsess->sessID)) { 
		_NetSessPool.Free(newsess);
		return nullptr;
	}

	if (newsess->pRecvQ == nullptr) {
		newsess->pRecvQ = new CRingBuffer(_RingbufSize);
	}
	newsess->sock = sock;
	newsess->ClosePendingSock = sock;
	CreateIoCompletionPort((HANDLE)sock, hiocp, (ULONG_PTR)newsess, 0);
	SecureZeroMemory(&(newsess->sendOvl), sizeof(newsess->sendOvl));
	SecureZeroMemory(&(newsess->recvOvl), sizeof(newsess->recvOvl));
	newsess->CountSet.isDead = 0;

	wcscpy_s(newsess->ip, ip);
	newsess->port = port;

	_NetSessArr[GetIdx(newsess->sessID)] = newsess;

	return newsess;
}

bool NetSessionMgr::DeleteNetSess(NetSession* delsess) {
	InterlockedIncrement16(&delsess->CountSet.IOCount);
	CloseSessSock(delsess);
	delsess->pRecvQ->ClearBuffer();
	NID delsessid = delsess->sessID;
	SOCKET ClosePendingSock = delsess->ClosePendingSock;
	_NetSessPool.Free(delsess);
	RetrieveID(delsessid);
	closesocket(ClosePendingSock);
	return true;
}

bool NetSessionMgr::CloseSessSock(NetSession* delsess) {
	if (delsess == nullptr) { return false; }
	CancelIoEx((HANDLE)delsess->sock, NULL);
	delsess->sock = INVALID_SOCKET;
	return true;
}

bool NetSessionMgr::CloseAllSessSock() {
	for (int i = 0; i < _Capacity; i++) {
		NetSession* sess = _NetSessArr[i];
		CloseSessSock(sess);
	}
	return true;
}

NetSession* NetSessionMgr::GetSess(NID SessID) {
	return _NetSessArr[GetIdx(SessID)];
}

int NetSessionMgr::GetNetSessPoolCapacity() {
	return _NetSessPool.getCapacity();
}

int NetSessionMgr::GetNetSessPoolUsingCount() {
	return _NetSessPool.getUseCount();
}

unsigned int NetSessionMgr::GetIdx(NID SessID) {
	unsigned int ret;
	ret = (unsigned int)(SessID >> (8 * (8 - IDX_BYTE)));
	return ret;
}

bool NetSessionMgr::GenerateNewID(NID* SessID) {
	if (_UnusedIdx.getSize() <= 0) { return false; }
	NID newidx;
	bool stackret = _UnusedIdx.pop((unsigned int*)&newidx);
	if (!stackret) { return false; }
	newidx = (newidx << (8 * (8 - IDX_BYTE)));
	if(_LastGenSessID > (0xffffffffffffffff >> (8 * IDX_BYTE))) { return false; }
	NID newSID = _LastGenSessID++;
	*SessID = newidx | newSID;
	return true;
}

bool NetSessionMgr::RetrieveID(NID SessID) {
	_UnusedIdx.push(GetIdx(SessID));
	return true;
}
