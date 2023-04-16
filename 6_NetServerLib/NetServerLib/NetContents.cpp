#include "NetSV-include/CNetServer.h"
#include "NetSV-include/NetSV/NetContents.h"

UINT WINAPI Thread_Contents(PVOID param) {
	NetContents* ct = (NetContents*)param;
	ContentsDataBlock* cdb = ct->_cdb;
	CNetServer* sv = ct->_sv;

	sv->_pNetlog->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::FILE, LOGLEVEL::SYS,
		L"Thread Contents Started.\n");
	bool isShutdown = false;
	DWORD prevTime = 0;
	unsigned int mspt = 1000u / cdb->TPS;

	ct->OnContentsThreadStart(GetCurrentThreadId());

	prevTime = timeGetTime();

	while (!isShutdown) {
		//calc mspt and sleep
		cdb->TickCount++;
		DWORD elapsedTime = (timeGetTime() - prevTime);
		if (elapsedTime <= mspt) {
			Sleep(mspt - elapsedTime);
		}

		//save prevtime for tps
		prevTime = timeGetTime();

		//process CDB Queue
		while (true) {
			//dequeue Q
			SessMovingData smd;
			if (cdb->SessQ.Dequeue(&smd) == false) { break; }

			//check contents thread return signal
			if (sv->_isStarted == false &&
				smd.MovingTo == 0 &&
				smd.MovingFrom == 0) {
				isShutdown = true;
				continue;
			}

			//check dest is valid
			if (smd.MovingTo != ct->_cdb->ContentsID) {
				continue;
			}

			//register and init sess
			cdb->ConSessMap.insert({ smd.sessID, smd.sess });
			smd.sess->isMoving = false;
			ct->OnContentsJoin(smd.sessID, smd.MovingFrom, smd.data);
			sv->DecIOCnt(smd.sess);		//From MoveContents
			cdb->ConSessNum++;
		}

		//process sess recv
		for (auto sessiter = cdb->ConSessMap.begin(); sessiter != cdb->ConSessMap.end();) {
			NID sessid = sessiter->first;
			NetSession* sess = sessiter->second;

			// mark that this session is using
			sv->IncIOCnt(sess);

			//check if session is dead
			if (sess->CountSet.isDead == 1) {
				sv->DecIOCnt(sess);
				ct->OnContentsDisconnect(sessid);
				sessiter = cdb->ConSessMap.erase(sessiter);
				cdb->ConSessNum--;
				continue;
			}

			//check sessID is correct
			if (sess->sessID != sessid) {
				sv->DecIOCnt(sess);
				ct->OnContentsDisconnect(sessid);
				sessiter = cdb->ConSessMap.erase(sessiter);
				cdb->ConSessNum--;
				continue;
			}
			//now we can use NetSess safely under here...

			//process sess packetQ
			while ((sess->ContentsID == cdb->ContentsID) && (!sess->isMoving) && (!sess->isDisconned)) {
				SerialBuffer* pkt;
				if (sess->packetQ.Dequeue(&pkt) == false) { break; }
				bool rpcflag = false;
				if (ct->_RPCStub != nullptr) {
					rpcflag = ct->_RPCStub->__RPC__DispatchRPC(sessid, pkt);
				}
				if (!rpcflag) { ct->OnContentsRecv(sessid, pkt); }
				sv->FreePacket(pkt);		//from Worker's recv process
			}
			//unmark that this session is using
			sv->DecIOCnt(sess);
			sessiter++;
		}

		//do contents update
		ct->OnContentsUpdate();

		//erase moved sess from map
		for (auto i : cdb->DelConSessVec) {
			cdb->ConSessMap.erase(i);
			cdb->ConSessNum--;
		}
		cdb->DelConSessVec.clear();
	}
	//do disconnect sess
	//clear map here
	for (auto sessiter : cdb->ConSessMap) {
		ct->OnContentsDisconnect(sessiter.first);
		cdb->ConSessMap.erase(sessiter.first);
	}
	cdb->ConSessMap.clear();
	//clear SessQ
	while (true) {
		//dequeue Q
		SessMovingData smd;
		if (cdb->SessQ.Dequeue(&smd) == false) { break; }
		if (smd.MovingFrom == smd.MovingTo) { continue; }
		sv->Disconnect(smd.sessID);
	}
	cdb->ConSessNum = 0;
	cdb->TickCount = 0;

	ct->OnContentsThreadEnd(GetCurrentThreadId());

	sv->_pNetlog->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::FILE, LOGLEVEL::SYS,
		L"Thread Contents Returned.\n");

	return 0;
}

bool NetContents::MoveContents(NID sessID, CID destCID, PVOID data) {
	if (_sv->_isStarted == false) { return false; }
	if (destCID == _cdb->ContentsID) { return false; }

	//find sess
	auto sessiter = _cdb->ConSessMap.find(sessID);
	if (sessiter == _cdb->ConSessMap.end()) { return false; }
	NetSession* sess = sessiter->second;

	//chcek sess is valid
	_sv->IncIOCnt(sess);	//mark that this sess is using - for MoveContents
	//check if session is dead
	if (sess->CountSet.isDead == 1) {
		_sv->DecIOCnt(sess);
		return false;
	}

	//check sessID is correct
	if (sess->sessID != sessID) {
		_sv->DecIOCnt(sess);
		return false;
	}
	//now we can use NetSess safely under here...
	if (sess->ContentsID != _cdb->ContentsID) { _sv->DecIOCnt(sess); return false; }
	if (sess->isMoving) { _sv->DecIOCnt(sess); return false; }

	//add to del vector
	_cdb->DelConSessVec.push_back(sessID);
	//create SMD
	SessMovingData smd;
	smd.sessID = sessID;
	smd.sess = sess;
	smd.data = data;
	smd.MovingFrom = _cdb->ContentsID;
	smd.MovingTo = destCID;
	//do move
	sess->isMoving = true;
	OnContentsLeave(sessID, destCID);
	if (destCID != 0) {		//to other contents
		sess->ContentsID = destCID;
		ContentsDataBlock* destCDB = _sv->GetCDB(destCID);
		destCDB->SessQ.Enqueue(smd);
	}
	else {					//to worker thread
		SessMovingData* dynsmd = new SessMovingData;
		*dynsmd = smd;
		sess->psmd = dynsmd;
		sess->isMovingToWorkerThread = true;
		CancelIoEx((HANDLE)sess->sock, (LPOVERLAPPED)&sess->recvOvl);
		sess->ContentsID = 0;
	}

	return true;
}

bool NetContents::AttachRPCStub(SuperRPCStub* rpcstub) {
	if (_sv->_isStarted) { return false; }
	_RPCStub = rpcstub;
	_RPCStub->__RPC__sv = _sv;
	_RPCStub->__RPC__Init();
	return true;
}

bool NetContents::AttachRPCProxy(SuperRPCProxy* rpcproxy) {
	if (_sv->_isStarted) { return false; }
	rpcproxy->__RPC__sv = _sv;
	return true;
}
