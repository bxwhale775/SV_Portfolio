#include "NetSV-include/CNetServer.h"
#include <process.h>

class DefaultCallback : public Interface_CNetServer_Callback {
public:
	virtual bool OnAccept(const WCHAR* ip, const u_short port) { return true; }
};

//Thread Proc
UINT WINAPI Thread_Accept(PVOID param) {
	CNetServer* sv = (CNetServer*)param;

	sv->_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"Thread Accept Started.\n");

	while (true) {
		//set cAddr and accept
		int ret = 0;
		SOCKADDR_IN cAddr;
		int cAddrLen = sizeof(cAddr);
		SOCKET newsock = accept(sv->_Listen_Sock, (SOCKADDR*)&cAddr, &cAddrLen);
		if (newsock == INVALID_SOCKET) {
			ret = WSAGetLastError();
			if (ret == WSAEINTR || ret == WSAENOTSOCK) {
				break;
			}
			else {
				sv->_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
					L"Accept fail. CODE: %u\n", ret);
				continue;
			}
		}
		sv->_AcceptCount++;

		//get ip and port
		WCHAR ip[NETSV_IPSTR_LEN];
		InetNtop(AF_INET, &cAddr.sin_addr, ip, NETSV_IPSTR_LEN);
		u_short port = ntohs(cAddr.sin_port);

		//check connection via OnAccept
		if (sv->cbk->OnAccept(ip, port) == false) {
			closesocket(newsock);
			continue;
		}

		//create new sess
		NetSession* newsess = sv->_NetSessMgr->CreateNewNetSess(newsock, sv->_hiocp, ip, port);
		if (newsess == nullptr) {
			closesocket(newsock);
			continue;
		}

		//post recv and call OnJoin
		newsess->ContentsID = 0;
		newsess->isMoving = false;
		newsess->isMovingToWorkerThread = false;
		newsess->isDisconned = false;
		newsess->SendAndDisconn = 0;
		sv->cbk->OnJoin(newsess->sessID);
		sv->PostRecv(newsess);
		sv->DecIOCnt(newsess);
	}

	sv->_pNetlog->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::FILE, LOGLEVEL::SYS,
		L"Thread Accept Returned.\n");
	return 0;
}

UINT WINAPI Thread_Worker(PVOID param) {
	CNetServer* sv = (CNetServer*)param;
	sv->_pNetlog->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::FILE, LOGLEVEL::SYS,
		L"Thread Worker Started.\n");

	DWORD transbytes;
	ULONG_PTR cKey;
	OVERLAPPED_EXT* ovl;
	bool isShutdown = false;
	DWORD gqcswait = INFINITE;

	sv->cbk->OnWorkerThreadStart(GetCurrentThreadId());

	while (true) {
		//reset args
		transbytes = 0;
		cKey = 0;
		ovl = nullptr;

		//do gqcs
		BOOL gqcsret = GetQueuedCompletionStatus(sv->_hiocp, &transbytes, &cKey, (OVERLAPPED**)&ovl, gqcswait);
		DWORD err = GetLastError();
		DWORD wsaerr = WSAGetLastError();

		//get Session from cKey. / if cKey is null, sess is null too
		NetSession* sess = (NetSession*)cKey;

		//check GQCS failed
		if (gqcsret == 0) {			//if GQCS failed
			if (ovl == nullptr) {	//IOCP dequeue error / TIMEOUT
				if (err == WAIT_TIMEOUT) {
					if (isShutdown) { break; }
					sv->_pNetlog->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::FILE, LOGLEVEL::ERR,
						L"GQCS Failed - Timed out\n");
				}
				else {
					sv->_pNetlog->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::FILE, LOGLEVEL::ERR,
						L"GQCS Failed - Other. CODE: %u\n", err);
				}
				sv->_pNetlog->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::FILE, LOGLEVEL::SYS,
					L"Thread Worker Ended with error.\n");
				return -1;
			}
			else {					//OvIO error
				if (ovl->IOType == IO_SEND) {
					if ((wsaerr != ERROR_NETNAME_DELETED) &&
						(wsaerr != WSA_OPERATION_ABORTED)) {
						sv->_pNetlog->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::FILE, LOGLEVEL::ERR,
							L"GQCS Failed - Send OvIO error. Port: %d / CODE: %u / WSACODE: %u\n", sess->port, err, wsaerr);
					}
				}
				else if (ovl->IOType == IO_RECV) {
					//handling for CancelIoEx from MoveContents
					if (wsaerr == ERROR_OPERATION_ABORTED &&
						sess != nullptr &&
						sess->isMovingToWorkerThread) {
						sess->isMovingToWorkerThread = false;
						sess->isMoving = false;
						sess->ContentsID = 0;
						SessMovingData smd = *sess->psmd;
						sv->cbk->OnContentsJoin(sess->sessID, smd.MovingFrom, smd.data);
						while ((sess->ContentsID == 0) && (!sess->isMoving) && (!sess->isDisconned)) {
							SerialBuffer* pkt;
							if (sess->packetQ.Dequeue(&pkt) == false) { break; }
							bool rpcflag = false;
							if (sv->_RPCStub != nullptr) {
								rpcflag = sv->_RPCStub->__RPC__DispatchRPC(sess->sessID, pkt);
							}
							if (!rpcflag) { sv->cbk->OnRecv(sess->sessID, pkt); }
							sv->FreePacket(pkt);		//from Worker's recv process
						}
						delete sess->psmd;
						sv->PostRecv(sess);
						sv->DecIOCnt(sess);
					}
					if ((wsaerr != ERROR_NETNAME_DELETED) &&
						(wsaerr != WSA_OPERATION_ABORTED)) {
						sv->_pNetlog->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::FILE, LOGLEVEL::ERR,
							L"GQCS Failed - Recv OvIO error. Port: %d / CODE: %u, WSACODE: %u\n", sess->port, err, wsaerr);
					}
				}
			}
		}

		//if GQCS done
		else {
			//check GQCS received return signal
			if (ovl == nullptr) {
				if (!sv->_isStarted) {
					isShutdown = true;
					gqcswait = 0;
				}
				continue;
			}
			//if SendPost with async is called
			else if ((ULONG_PTR)ovl == (ULONG_PTR)0x01) {
				sv->PostSend(sess);
			}
			//if Send is done
			else if (ovl->IOType == IO_SEND) {
				// decrease usecnt and free packet
				for (int i = 0; i < sess->sendedPacketCount; i++) {
					sv->FreePacket(sess->sendedPacketArr[i]);
				}
				InterlockedAdd64((LONG64*)&sv->_SendCount, sess->sendedPacketCount);
				sess->sendedPacketCount = 0;

				//check sendanddisconnect
				if (sess->SendAndDisconn == 1) {
					if (sess->sendQ.getSize() <= 0) {
						sv->Disconnect(sess->sessID);
					}
				}

				// unflag issending
				char prevSendState = InterlockedExchange8(&sess->isSending, 0);
				if (prevSendState == 0) {
					sv->_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
						L"double send?\n");
				}

				//try send again
				sv->PostSend(sess);
			}
			//if Recv is done
			else if (ovl->IOType == IO_RECV) {
				//if received something
				if (transbytes > 0) {
					//move head for recvQ
					sess->pRecvQ->MoveHead(transbytes);

					unsigned int recvcnt = 0;
					//process all packet
					while (true) {
						NetHeader header;
						if (sess->pRecvQ->GetUseSize() < sizeof(NetHeader)) { break; }
						sess->pRecvQ->Peek((char*)&header, sizeof(NetHeader));
						if (sess->pRecvQ->GetUseSize() < (int)(sizeof(NetHeader) + header.len)) { break; }
						sess->pRecvQ->MoveTail(sizeof(NetHeader));
						if (header.code != sv->_props.PacketCode) {
							sv->_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
								L"Recv Packet Failed - Packet code is invalid\n");
							sv->Disconnect(sess->sessID);
							break;
						}

						SerialBuffer* databuf = sv->AllocPacket();
						int deqsize = 0;
						deqsize = sess->pRecvQ->Dequeue(databuf->GetBufferHeadPtr(), header.len);
						databuf->MoveWritePos(deqsize);
						databuf->PutHeader((char*)&header);
						if (!sv->_props.disableEncrypt) {
							if (!databuf->Decrypt(sv->_props.EncryptKey)) {
								sv->_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
									L"Recv Packet Failed - Packet Decrypt failed\n");
								sv->Disconnect(sess->sessID);
								sv->FreePacket(databuf);
								break;
							}
						}
						sess->packetQ.Enqueue(databuf);
						recvcnt++;
					}
					if (sess->ContentsID == 0 && !sv->_props.Client_ManualDispatch) {
						if (sess->isMovingToWorkerThread) {
							sess->isMovingToWorkerThread = false;
							sess->isMoving = false;
							SessMovingData smd = *sess->psmd;
							sv->cbk->OnContentsJoin(sess->sessID, smd.MovingFrom, smd.data);
							delete sess->psmd;
							sv->DecIOCnt(sess);
						}
						while ((sess->ContentsID == 0) && (!sess->isMoving) && (!sess->isDisconned)) {
							SerialBuffer* pkt;
							if (sess->packetQ.Dequeue(&pkt) == false) { break; }
							bool rpcflag = false;
							if (sv->_RPCStub != nullptr) {
								rpcflag = sv->_RPCStub->__RPC__DispatchRPC(sess->sessID, pkt);
							}
							if (!rpcflag) { sv->cbk->OnRecv(sess->sessID, pkt); }
							sv->FreePacket(pkt);		//from Worker's recv process
						}
					}
					InterlockedAdd64((LONG64*)&sv->_RecvCount, recvcnt);
					sv->PostRecv(sess);
				}
				//if received nothing
				else if (transbytes == 0) {
					//pass - leave iocnt decrease
				}
			}
		}

		sv->DecIOCnt(sess);
	}
	sv->cbk->OnWorkerThreadEnd(GetCurrentThreadId());
	PostQueuedCompletionStatus(sv->_hiocp, 0, 0, nullptr);

	sv->_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"Thread Worker Returned.\n");
	return 0;
}


//CNetserver func
CNetServer::~CNetServer() {
	if (_NetSessMgr != nullptr) { delete _NetSessMgr; }
	if (_SBPool != nullptr) { delete _SBPool; }
	if (_CDBArray != nullptr) { delete[] _CDBArray; }
	if (_pNetlog != nullptr) { delete _pNetlog; }
	if (_usingDefaultCallback) { delete cbk; }
	return;
}

bool CNetServer::Init(CNetServer_Properties props, Interface_CNetServer_Callback* Callback, UINT AllocContentsNum) {
	//init properties
	_props = props;

	//init callback
	if (Callback == nullptr) {
		cbk = new DefaultCallback();
		_usingDefaultCallback = true;
	}
	else {
		cbk = Callback;
	}
	cbk->_sv = this;

	//init Logger
	if (_pNetlog == nullptr) { _pNetlog = new Logger(_props.LogFileName); }
	
	//init managers
	_NetSessMgr = new NetSessionMgr(
		_props.Client_RunAsClient ? 1 : _props.MaxSessionNum,
		_props.NS_RingBufSize,
		_props.NSPool_InitBlockNum,
		_props.NSPool_ChunkSize
	);
	_SBPool = new MemoryPoolTLS<SerialBuffer>(_props.PKPool_InitBlockNum, _props.PKPool_ChunkSize);

	timeBeginPeriod(1);

	int ret;

	//WSA Startup
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
			L"WSAStartup failed\n");
		return false;
	}

	//Create listen socket
	_Listen_Sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_Listen_Sock == INVALID_SOCKET) {
		ret = WSAGetLastError();
		_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
			L"Listen Socket create fail: %d\n", ret);
		return false;
	}

	//if this is client, init is done here.
	if (props.Client_RunAsClient) {
		_isInited = true;
		return true;
	}

	//turn off Manual Dispach
	_props.Client_ManualDispatch = false;

	//Set linger option to listen socket
	if (!_props.disableLinger) {
		LINGER lng;
		lng.l_onoff = 1;
		lng.l_linger = 0;
		ret = setsockopt(_Listen_Sock, SOL_SOCKET, SO_LINGER, (char*)&lng, sizeof(lng));
		if (ret == SOCKET_ERROR) {
			ret = WSAGetLastError();
			_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
				L"Listen Socket Set option LINGER failed: %d\n", ret);
			return false;
		}
	}

	//Turn off nagle if option is true
	if (_props.disableNagle) {
		DWORD optval = 1;
		ret = setsockopt(_Listen_Sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
		if (ret == SOCKET_ERROR) {
			ret = WSAGetLastError();
			_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
				L"Listen Socket Set option NODELAY failed: %d\n", ret);
			return false;
		}
	}

	//create listen socket addr
	SOCKADDR_IN sAddr;
	memset(&sAddr, 0, sizeof(sAddr));
	sAddr.sin_family = AF_INET;
	sAddr.sin_port = htons(_props.Port);
	sAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind listen socket
	ret = bind(_Listen_Sock, (SOCKADDR*)&sAddr, sizeof(sAddr));
	if (ret == SOCKET_ERROR) {
		ret = WSAGetLastError();
		_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
			L"Listen Socket Bind fail: %d\n", ret);
		return false;
	}

	//Alloc CDB
	_AllocedCDB = AllocContentsNum;
	if (_AllocedCDB != 0) {
		_CDBArray = new ContentsDataBlock[_AllocedCDB];
	}

	_isInited = true;
	return true;
}

bool CNetServer::Start() {
	if (!_isInited) { return false; }
	if (_isStarted) { return false; }

	int Accept_Thr_Slot = 1;
	if (_props.Client_RunAsClient) {
		Accept_Thr_Slot = 0;
	}

	//create IOCP
	_hiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, _props.CocurrentThreadNum);

	int ret;
	//start listening when this is server
	if (!_props.Client_RunAsClient) {
		ret = listen(_Listen_Sock, SOMAXCONN);
		if (ret == SOCKET_ERROR) {
			ret = WSAGetLastError();
			_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
				L"Listen fail: %d\n", ret);
			return false;
		}
	}
	//do connect when this is client
	else {
		//create addr for connect
		SOCKADDR_IN cAddr;
		memset(&cAddr, 0, sizeof(cAddr));
		cAddr.sin_family = AF_INET;
		cAddr.sin_port = htons(_props.Client_Port);
		InetPton(AF_INET, _props.Client_IP, &cAddr.sin_addr);

		//do connect
		ret = connect(_Listen_Sock, (SOCKADDR*)&cAddr, sizeof(cAddr));
		if (ret == SOCKET_ERROR) {
			ret = WSAGetLastError();
			_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
				L"Connect fail: %d\n", ret);
			return false;
		}

		//create session

		//get ip and port
		WCHAR ip[NETSV_IPSTR_LEN];
		InetNtop(AF_INET, &cAddr.sin_addr, ip, NETSV_IPSTR_LEN);
		u_short port = ntohs(cAddr.sin_port);

		//create new sess
		NetSession* newsess = _NetSessMgr->CreateNewNetSess(_Listen_Sock, _hiocp, ip, port);

		//call OnJoin and post recv
		newsess->ContentsID = 0;
		newsess->isMoving = false;
		newsess->isMovingToWorkerThread = false;
		newsess->isDisconned = false;
		newsess->SendAndDisconn = 0;
		_ClientCon = newsess;
		cbk->OnJoin(newsess->sessID);
		PostRecv(newsess);
		DecIOCnt(newsess);
	}

	//new for thread handles array
	_ThrHandles = new HANDLE[(size_t)_props.WorkerThreadNum + (size_t)Accept_Thr_Slot];

	_isStarted = true;

	//create and start Accept Thread when this is server
	if (!_props.Client_RunAsClient) {
		HANDLE ThrA;
		ThrA = (HANDLE)_beginthreadex(NULL, 0, Thread_Accept, (PVOID)this, 0, NULL);
		_ThrHandles[0] = ThrA;
	}

	//create and start Worker Thread
	for (DWORD i = 0; i < _props.WorkerThreadNum; i++) {
		HANDLE ThrW;
		ThrW = (HANDLE)_beginthreadex(NULL, 0, Thread_Worker, (PVOID)this, 0, NULL);
		_ThrHandles[i + Accept_Thr_Slot] = ThrW;
	}

	//create and start Contents Thread
	for (unsigned int i = 1; i <= _LastGenCID; i++) {
		ContentsDataBlock* cdb = GetCDB(i);
		cdb->ThrHandle = (HANDLE)_beginthreadex(NULL, 0, Thread_Contents, (PVOID)cdb->ct, 0, NULL);
	}

	return true;
}

bool CNetServer::Stop() {
	if (!_isInited) { return false; }
	if (!_isStarted) { return false; }
	_isStarted = false;

	int Accept_Thr_Slot = 1;
	if (_props.Client_RunAsClient) {
		Accept_Thr_Slot = 0;
	}

	//close listen sock - make err to listensock and accept Thread will return
	if (!_props.Client_RunAsClient) {
		closesocket(_Listen_Sock);
	}

	//close netsess->sock - make err to netsess->sock and will be deleted by worker thread
	_NetSessMgr->CloseAllSessSock();

	//send first return signal to worker thread via IOCP
	PostQueuedCompletionStatus(_hiocp, 0, 0, nullptr);

	//wait for all worker thread return
	DWORD timeout = 10000;		//10sec
	DWORD ret;
	_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"Wait for all worker thread down...\n");
	ret = WaitForMultipleObjects(_props.WorkerThreadNum + Accept_Thr_Slot, _ThrHandles, true, timeout);
	if (ret == WAIT_TIMEOUT) {
		_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
			L"Worker thread down wait Timed out. Skipping...\n");
	}

	delete[] _ThrHandles;

	//wait for all contents thread return
	HANDLE* tempContThrHdles = new HANDLE[_LastGenCID];

	SessMovingData killersmd;
	killersmd.MovingFrom = 0;
	killersmd.MovingTo = 0;
	int handleidx = 0;
	for (unsigned int i = 1; i <= _LastGenCID; i++) {
		tempContThrHdles[handleidx] = GetCDB(i)->ThrHandle;
		handleidx++;
		GetCDB(i)->SessQ.Enqueue(killersmd);
	}
	_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"Wait for all Contents thread down...\n");
	ret = WaitForMultipleObjects(_LastGenCID, tempContThrHdles, true, timeout);
	if (ret == WAIT_TIMEOUT) {
		_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
			L"Contents thread down wait Timed out. Skipping...\n");
	}
	delete[] tempContThrHdles;

	_pNetlog->writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"All Thread Returned.\n");

	return true;
}

bool CNetServer::AttachRPCStub(SuperRPCStub* rpcstub) {
	if (_isStarted) { return false; }
	_RPCStub = rpcstub;
	_RPCStub->__RPC__sv = this;
	_RPCStub->__RPC__Init();
	return true;
}

bool CNetServer::AttachRPCProxy(SuperRPCProxy* rpcproxy) {
	if (_isStarted) { return false; }
	rpcproxy->__RPC__sv = this;
	return true;
}

bool CNetServer::SendPacket(NID sessID, SerialBuffer* packet, bool async, bool sendanddisconnect) {
	if (!_isStarted) { return false; }

	NetSession* sess = nullptr;
	if (sessID == NETSV_INVALID_NID) {
		if (_props.Client_RunAsClient) {
			sess = _ClientCon;
			sessID = sess->sessID;
		}
		else {
			return false;
		}
	}

	//get netsess via sessid
	if (sess == nullptr) {
		sess = _NetSessMgr->GetSess(sessID);
	}

	// mark that this session is using
	IncIOCnt(sess);

	//check if session is dead
	if (sess->CountSet.isDead == 1) {
		DecIOCnt(sess);
		return false;
	}

	//check sess is correct
	if (sess->sessID != sessID) {
		DecIOCnt(sess);
		return false;
	}
	//now we can use sess safely under here...

	//build header and set to packet
	NetHeader header;
	header.code = _props.PacketCode;
	header.len = packet->GetDataSize();
	header.randkey = rand();
	packet->PutHeader((char*)&header);
	if (!_props.disableEncrypt) {
		packet->Encrypt(_props.EncryptKey);
	}

	//inc packet usecount for sendQ & eq packet
	packet->IncUseCount();
	sess->sendQ.Enqueue(packet);

	//do send and disconnect if enabled
	if (sendanddisconnect) {
		sess->SendAndDisconn = 1;
	}

	//post send request
	if (async) {
		IncIOCnt(sess);
		BOOL ret = PostQueuedCompletionStatus(_hiocp, 0, (ULONG_PTR)sess, (LPOVERLAPPED)0x01);
		if (ret == 0) {
			printf("Async Post Fail\n");
			DecIOCnt(sess);
		}
	}
	else {
		PostSend(sess);
	}

	//unmark that this session is using
	DecIOCnt(sess);

	return true;
}

bool CNetServer::Disconnect(NID sessID) {
	if (!_isStarted) { return false; }

	NetSession* sess = nullptr;
	if (sessID == NETSV_INVALID_NID) {
		if (_props.Client_RunAsClient) {
			sess = _ClientCon;
			sessID = sess->sessID;
		}
		else {
			return false;
		}
	}

	//get netsess via sessid
	if (sess == nullptr) {
		sess = _NetSessMgr->GetSess(sessID);
	}

	// mark that this session is using
	IncIOCnt(sess);

	//check if session is dead
	if (sess->CountSet.isDead == 1) {
		DecIOCnt(sess);
		return false;
	}

	//check sess is correct
	if (sess->sessID != sessID) {
		DecIOCnt(sess);
		return false;
	}
	//now we can use sess safely under here...

	//close socket - release of sess will done at worker thread
	sess->isDisconned = true;
	_NetSessMgr->CloseSessSock(sess);

	//unmark that this session is using
	DecIOCnt(sess);
	return true;
}

bool CNetServer::ManualDispatch() {
	if (!_isStarted) { return false; }

	NetSession* sess = nullptr;
	if (_props.Client_RunAsClient && _props.Client_ManualDispatch) {
		sess = _ClientCon;
	}
	else {
		return false;
	}

	// mark that this session is using
	IncIOCnt(sess);

	//check if session is dead
	if (sess->CountSet.isDead == 1) {
		DecIOCnt(sess);
		return false;
	}

	//now we can use sess safely under here...

	while (!sess->isDisconned) {
		SerialBuffer* pkt;
		if (sess->packetQ.Dequeue(&pkt) == false) { break; }
		bool rpcflag = false;
		if (_RPCStub != nullptr) {
			rpcflag = _RPCStub->__RPC__DispatchRPC(sess->sessID, pkt);
		}
		if (!rpcflag) { cbk->OnRecv(sess->sessID, pkt); }
		FreePacket(pkt);		//from Worker's recv process
	}

	//unmark that this session is using
	DecIOCnt(sess);

	return true;
}

bool CNetServer::GetIpAddress(NID sessID, WCHAR* str) {
	if (!_isStarted) { return false; }

	NetSession* sess = nullptr;
	if (sessID == NETSV_INVALID_NID) {
		if (_props.Client_RunAsClient) {
			sess = _ClientCon;
			sessID = sess->sessID;
		}
		else {
			return false;
		}
	}

	//get netsess via sessid
	if (sess == nullptr) {
		sess = _NetSessMgr->GetSess(sessID);
	}

	// mark that this session is using
	IncIOCnt(sess);

	//check if session is dead
	if (sess->CountSet.isDead == 1) {
		DecIOCnt(sess);
		return false;
	}

	//check sess is correct
	if (sess->sessID != sessID) {
		DecIOCnt(sess);
		return false;
	}
	//now we can use sess safely under here...

	wcscpy_s(str, NETSV_IPSTR_LEN, sess->ip);

	//unmark that this session is using
	DecIOCnt(sess);

	return true;
}

u_short CNetServer::GetPort(NID sessID) {
	if (!_isStarted) { return 0; }

	NetSession* sess = nullptr;
	if (sessID == NETSV_INVALID_NID) {
		if (_props.Client_RunAsClient) {
			sess = _ClientCon;
			sessID = sess->sessID;
		}
		else {
			return false;
		}
	}

	//get netsess via sessid
	if (sess == nullptr) {
		sess = _NetSessMgr->GetSess(sessID);
	}

	// mark that this session is using
	IncIOCnt(sess);

	//check if session is dead
	if (sess->CountSet.isDead == 1) {
		DecIOCnt(sess);
		return false;
	}

	//check sess is correct
	if (sess->sessID != sessID) {
		DecIOCnt(sess);
		return false;
	}
	//now we can use sess safely under here...

	u_short ret = sess->port;

	//unmark that this session is using
	DecIOCnt(sess);

	return ret;
}

ContentsDataBlock* CNetServer::GetCDB(CID cid) {
	if (cid > _AllocedCDB || cid == 0) { return nullptr; }
	return &_CDBArray[cid - 1];
}

CID CNetServer::AttachContents(NetContents* ct, unsigned int tps) {
	if (!_isInited) { return 0; }
	if (_AllocedCDB < _LastGenCID + 1) { return 0; }

	_LastGenCID++;
	CID newcid = _LastGenCID;
	ContentsDataBlock* cdb = GetCDB(newcid);
	ct->_cdb = cdb;
	ct->_sv = this;

	cdb->ct = ct;
	cdb->nsv = this;
	cdb->ContentsID = newcid;
	cdb->TPS = tps;

	return newcid;
}

bool CNetServer::MoveContents(NID sessID, CID destCID, PVOID data) {
	if (_isStarted == false || 
		destCID == 0 || 
		_props.Client_RunAsClient)
	{ return false; }

	//get netsess via sessid
	NetSession* sess = _NetSessMgr->GetSess(sessID);

	// mark that this session is using
	IncIOCnt(sess);

	//check if session is dead
	if (sess->CountSet.isDead == 1) {
		DecIOCnt(sess);
		return false;
	}

	//check sess is correct
	if (sess->sessID != sessID) {
		DecIOCnt(sess);
		return false;
	}
	//now we can use sess safely under here...
	if (sess->ContentsID != 0) { DecIOCnt(sess); return false; }
	if (sess->isMoving) { DecIOCnt(sess); return false; }

	//create SMD
	SessMovingData smd;
	smd.sessID = sessID;
	smd.sess = sess;
	smd.data = data;
	smd.MovingFrom = 0;
	smd.MovingTo = destCID;

	//do move
	sess->isMoving = true;
	cbk->OnContentsLeave(sessID, destCID);
	sess->ContentsID = destCID;
	ContentsDataBlock* destCDB = GetCDB(destCID);
	destCDB->SessQ.Enqueue(smd);

	return true;
}

bool CNetServer::PostSend(NetSession* sess) {
	//return if there is no packet to send
	if (sess->sendQ.getSize() <= 0) { return false; }

	//return if there is another send io
	char prevSendState = InterlockedExchange8(&sess->isSending, 1);
	if (prevSendState == 1) { return false; }

	//mark that sess has send io
	IncIOCnt(sess);

	//set WSABUFS and SendedPacketArr
	WSABUF wsabufarr[MAX_SEND_PACKETS];
	int wsabufcount = 0;
	for (int i = 0; i < MAX_SEND_PACKETS; i++) {
		if (sess->sendQ.Dequeue(&sess->sendedPacketArr[i]) == false) { break; }
		wsabufarr[i].buf = sess->sendedPacketArr[i]->GetBufferPtr();
		wsabufarr[i].len = sess->sendedPacketArr[i]->GetDataSize();
		wsabufcount++;
	}

	// return if there is no packet
	if (wsabufcount == 0) {
		InterlockedExchange8(&sess->isSending, 0);
		DecIOCnt(sess);
		return false;
	}

	//set WSABUFcount
	sess->sendedPacketCount = wsabufcount;

	//ready for ovio
	SecureZeroMemory(&(sess->sendOvl), sizeof(sess->sendOvl));
	sess->sendOvl.IOType = IO_SEND;

	//do send
	int ret;
	ret = WSASend(sess->sock, wsabufarr, wsabufcount, NULL, 0, (OVERLAPPED*)&sess->sendOvl, NULL);

	//do error handling
	if (ret == SOCKET_ERROR) {
		int wsalasterr = WSAGetLastError();
		if (wsalasterr != WSA_IO_PENDING) {
			for (int i = 0; i < sess->sendedPacketCount; i++) {
				FreePacket(sess->sendedPacketArr[i]);
			}
			sess->sendedPacketCount = 0;
			InterlockedExchange8(&sess->isSending, 0);
			DecIOCnt(sess);
			if (wsalasterr != WSAECONNRESET &&
				wsalasterr != WSAENOTSOCK &&
				wsalasterr != WSAECONNABORTED) {
				_pNetlog->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::FILE, LOGLEVEL::ERR,
					L"Send Error. Port: %d / CODE: %u\n", sess->port, wsalasterr);
			}
			return false;
		}
	}

	return true;
}

bool CNetServer::PostRecv(NetSession* sess) {
	//mark that sess has recv io
	IncIOCnt(sess);

	//Set WSABufs with recvQ
	WSABUF wsabufarr[2];
	wsabufarr[0].buf = sess->pRecvQ->GetHeadBufferPtr();
	wsabufarr[1].buf = sess->pRecvQ->GetBufferPtr();
	sess->pRecvQ->GetSizeForWSARecv(&wsabufarr[0].len, &wsabufarr[1].len);

	//ready for ovio
	SecureZeroMemory(&(sess->recvOvl), sizeof(sess->recvOvl));
	sess->recvOvl.IOType = IO_RECV;

	//do recv
	DWORD flag = 0;
	int ret;
	ret = WSARecv(sess->sock, wsabufarr, 2, NULL, &flag, (OVERLAPPED*)&sess->recvOvl, NULL);

	//do error handling
	if (ret == SOCKET_ERROR) {
		int wsalasterr = WSAGetLastError();
		if (wsalasterr != WSA_IO_PENDING) {
			DecIOCnt(sess);
			if (wsalasterr != WSAECONNRESET &&
				wsalasterr != WSAENOTSOCK &&
				wsalasterr != WSAECONNABORTED) {
				_pNetlog->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::FILE, LOGLEVEL::ERR,
					L"Recv Error. Port: %d / CODE: %u\n", sess->port, wsalasterr);
			}
			return false;
		}
	}

	return true;
}

void CNetServer::IncIOCnt(NetSession* sess) {
	InterlockedIncrement16(&sess->CountSet.IOCount);
	return;
}

void CNetServer::DecIOCnt(NetSession* sess) {
	short decedcnt = InterlockedDecrement16(&sess->CountSet.IOCount);
	//check and release sess
	if (decedcnt == 0) {
		long prevDeadState = InterlockedCompareExchange(&sess->IOCandDeadflag, 1, 0);
		if (prevDeadState == 0 && sess->CountSet.isDead == 1) {
			//sess is safe from under here...
			if ((sess->ContentsID == 0) && (!sess->isMoving)) {
				cbk->OnDisconnect(sess->sessID);
			}
			//free packets
			while (true) {
				SerialBuffer* sb;
				if (sess->sendQ.Dequeue(&sb) == false) { break; }
				FreePacket(sb);
			}
			while (true) {
				SerialBuffer* sb;
				if (sess->packetQ.Dequeue(&sb) == false) { break; }
				FreePacket(sb);
			}
			// release sess
			_NetSessMgr->DeleteNetSess(sess);
		}
	}
	return;
}

SerialBuffer* CNetServer::AllocPacket() {
	SerialBuffer* ret = _SBPool->Alloc();
	if (ret->Init(_props.PK_Size) == false) {
		ret->Clear();
	}
	return ret;
}

void CNetServer::FreePacket(SerialBuffer* delbuf) {
	long deccnt = delbuf->DecUseCount();
	if (deccnt == 0) {
		_SBPool->Free(delbuf);
	}
	return;
}

int CNetServer::GetMonitorDataInt(en_NETSV_MTR_TYPE type) {
	int ret = 0;

	switch (type) {
	case en_NETSV_MTR_TYPE::MTR_INT_POOL_CAP_NSESS:
		ret = _NetSessMgr->GetNetSessPoolCapacity();
		break;
	case en_NETSV_MTR_TYPE::MTR_INT_POOL_USE_NSESS:
		ret = _NetSessMgr->GetNetSessPoolUsingCount();
		break;
	case en_NETSV_MTR_TYPE::MTR_INT_POOL_CAP_PKT:
		ret = _SBPool->getCapacity();
		break;
	case en_NETSV_MTR_TYPE::MTR_INT_POOL_USE_PKT:
		ret = _SBPool->getUseCount();
		break;
	default:
		break;
	}

	return ret;
}

ULONGLONG CNetServer::GetMonitorDataULL(en_NETSV_MTR_TYPE type) {
	ULONGLONG ret = 0;

	switch (type) {
	case en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_ACCEPT:
		ret = _AcceptCount;
		break;
	case en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_SEND:
		ret = _SendCount;
		break;
	case en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_RECV:
		ret = _RecvCount;
		break;
	default:
		break;
	}

	return ret;
}
