#include <process.h>
#include <iostream>
#include "ChatServer.h"

UINT WINAPI Thread_TimeOut(PVOID param) {
	ChatServer* csv = (ChatServer*)param;
	std::vector<NID> delnid;

	while (!csv->_isShutdown) {
		DWORD nowtime = timeGetTime();
		AcquireSRWLockShared(&csv->_DelUserArrLock);
		for (auto& i : csv->_DelUserArr) {
			Session* sess = i.second;
			DWORD lastrecvtime = sess->lastrecvtime;
			DWORD timeelapsed = 0;
			if (nowtime > lastrecvtime) { timeelapsed = nowtime - lastrecvtime; }
			if (sess->AccountNo != -1) {	//if sess is login user
				if (timeelapsed > TIMEOUT_USER) {
					csv->_TimeoutUser++;	// not using LOCK - this var is only modify here
					delnid.push_back(sess->uid);
				}
			}
			else {							//if sess is not login user
				if (timeelapsed > TIMEOUT_SESS) {
					csv->_TimeoutSess++;	// not using LOCK - this var is only modify here
					delnid.push_back(sess->uid);
				}
			}
		}
		ReleaseSRWLockShared(&csv->_DelUserArrLock);
		for (auto i : delnid) {
			csv->_sv.Disconnect(i);
		}

		Sleep(1000);
	}

	csv->_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"[Chat] Thread Timeout Returned.\n");

	return 0;
}

void ChatServer::Init(ChatServer_Properties props) {
	_ChatProps = props;
	_ncbk.csv = this;

	pdh.Init();
	_sv.Init(_ChatProps, &_ncbk);
	return;
}

void ChatServer::Start() {
	_isShutdown = false;

	//start thread
	_hThrTimeOut = (HANDLE)_beginthreadex(NULL, 0, Thread_TimeOut, (PVOID)this, 0, NULL);

	//start netserver
	_sv.Start();
	return;
}

void ChatServer::Stop() {
	_isShutdown = true;
	_sv.Stop();

	//wait for timeout thread return
	//DWORD timeout = INFINITE;
	DWORD timeout = 5000;		//5sec
	DWORD ret;
	_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"[Chat] Wait for all thread down...\n");
	ret = WaitForSingleObject(_hThrTimeOut, timeout);
	if (ret == WAIT_TIMEOUT) {
		_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
			L"[Chat] Wait Timed out. Skipping...\n");
	}
	_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"[Chat] All thread Returned.\n");

	return;
}

void ChatServer::PrintLog() {
	static ULONGLONG PrevAcceptCount = 0;
	static ULONGLONG PrevSendCount = 0;
	static ULONGLONG PrevRecvCount = 0;
	
	ULONGLONG acceptc = _sv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_ACCEPT);
	ULONGLONG sendc = _sv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_SEND);
	ULONGLONG recvc = _sv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_RECV);
	int gpp = _sv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_CAP_PKT);
	int gup = _sv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_USE_PKT);
	int nsp = _sv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_CAP_NSESS);
	unsigned int tos = _TimeoutSess;
	unsigned int tou = _TimeoutUser;
	int sep = _SessPool.getCapacity();
	int sepu = _CurrentSess;

	ULONGLONG acptps = acceptc - PrevAcceptCount;
	ULONGLONG sndps = sendc - PrevSendCount;
	ULONGLONG rcvps = recvc - PrevRecvCount;

	PrevAcceptCount = acceptc;
	PrevSendCount = sendc;
	PrevRecvCount = recvc;

	std::string line0, line1, line2, line3, line4, line5, line6;

	line0 += "=====================\n";

	line1 += "Sess: "; line1 += std::to_string(sepu);
	line1 += ", User: "; line1 += std::to_string(_CurrentUser);
	line1 += ", AcceptCnt: "; line1 += std::to_string(acceptc);
	line1 += " ("; line1 += std::to_string(acptps);
	line1 += "/s)\n";

	line2 += "Sesspool: "; line2 += std::to_string(sep);
	line2 += ", PKpool: "; line2 += std::to_string(gpp);
	line2 += ", NSpool: "; line2 += std::to_string(nsp);
	line2 += "\n";

	line3 += "SessUsing: "; line3 += std::to_string(sepu);
	line3 += ", PKUsing: "; line3 += std::to_string(gup);
	line3 += "\n";

	line4 += "Send: "; line4 += std::to_string(sndps);
	line4 += "/s, Recv: "; line4 += std::to_string(rcvps);
	line4 += "/s\n";

	line5 += "TOSess: "; line5 += std::to_string(tos);
	line5 += ", TOUser: "; line5 += std::to_string(tou);
	line5 += "\n";

	line6 += "SysCPU: "; line6 += std::to_string(pdh.GetSysCpuTime());
	line6 += "%, ProCPU: "; line6 += std::to_string(pdh.GetProcessCpuTime());
	line6 += "%\n";
	line6 += "SysMemNPP: "; line6 += std::to_string(pdh.ToMiB(pdh.GetSysNonPagedPool()));
	line6 += "MiB, ProMemPB: "; line6 += std::to_string(pdh.ToMiB(pdh.GetProcessPrivateBytes()));
	line6 += "MiB\n";
	line6 += "NetRecv: "; line6 += std::to_string(pdh.ToMiB(pdh.GetNetworkRecv()));
	line6 += "MiB/s, NetSend: "; line6 += std::to_string(pdh.ToMiB(pdh.GetNetworkSend()));
	line6 += "MiB/s\n";

	line0 += line1 += line2 += line3 += line4 += line5 += line6;
	std::cout << line0 << std::endl;

	return;
}

Session* ChatServer::AllocSess(NID uid) {
	Session* s = _SessPool.Alloc();
	s->uid = uid;
	s->AccountNo = -1;
	s->sector = -1;
	s->lastrecvtime = timeGetTime();
	InitializeSRWLock(&s->SessLock);
    return s;
}

void ChatServer::FreeSess(Session* sess) {
	_SessPool.Free(sess);
	//delete sess;
}

Session* ChatServer::FindSessByUid(NID uid) {
	Session* ret = nullptr;
	AcquireSRWLockShared(&_UserArrLock);
	auto i = _UserArr.find(uid);
	if (i != _UserArr.end()) { ret = i->second; }
	ReleaseSRWLockShared(&_UserArrLock);
	return ret;
}

