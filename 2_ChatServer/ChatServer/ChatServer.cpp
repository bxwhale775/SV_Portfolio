#include<time.h>
#include <process.h>
#include <vector>
#include "ChatServer.h"
#include "DBConnMgr_MT.h"
#pragma warning(disable:26812)

#define TIMEOUT_SESS 40000
#define TIMEOUT_USER 40000
#define MSG_MAX_LEN 255

UINT WINAPI Thread_ChatUpdate(PVOID param) {
	srand((unsigned int)time(NULL));
	ChatServer* csv = (ChatServer*)param;
	Task* task;
	bool isShutdown = false;

	while(!isShutdown) {
		//wait thread
		task = nullptr;
		int waitret = WaitForSingleObject(csv->_hUpdateEv, INFINITE);
		if (waitret == WAIT_FAILED) {
			DWORD err = GetLastError();
			csv->_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
				L"WAIT failed: %d\n", err);
			break;
		}
		//do task until taskQ is empty
		while (true) {
			//get task from queue
			if (!csv->_TaskQ.Dequeue(&task)) { break; }

			//do task by task type
			switch (task->type) {
			case en_TASK_CS_TIMEOUT:
			{
				for (auto i : csv->_UserArr) {
					Session* sess = i.second;
					DWORD timeelapsed = timeGetTime() - sess->lastrecvtime;
					if (sess->AccountNo != -1) {	//if sess is login user
						if (timeelapsed > TIMEOUT_USER) {
							csv->_TimeoutUser++;
							csv->_sv.Disconnect(sess->uid);
						}
					}
					else {							//if sess is not login user
						if (timeelapsed > TIMEOUT_SESS) {
							csv->_TimeoutSess++;
							csv->_sv.Disconnect(sess->uid);
						}
					}
				}
			}
				break;
			case en_TASK_CS_SHUTDOWN:
			{
				isShutdown = true;
			}
				break;
			case en_TASK_CS_NEW_SESS:
			{
				csv->_CurrentNetSess++;
				Session* newsess = csv->AllocSess(task->sessID);

				csv->_UserArr.insert({ task->sessID, newsess });
				csv->_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::DBG,
					L"NEWSESS: %d\n", task->sessID);
			}
				break;
			case en_TASK_CS_DEL_SESS:
			{
				Session* delsess = csv->FindSessByUid(task->sessID);
				csv->_CurrentNetSess--;
				if (delsess->AccountNo != -1) {
					csv->_CurrentUser--;
					csv->_Secmgr.deleteSessionFromSector(delsess);
				}
				csv->_AccountNoArr.erase(delsess->AccountNo);
				csv->_UserArr.erase(task->sessID);
				csv->FreeSess(delsess);
				csv->_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::DBG,
					L"DELSESS: %d\n", task->sessID);
			}
				break;
			case en_TASK_CS_NEW_PLAYER:
			{
				Session* sess = csv->FindSessByUid(task->sessID);
				if (sess == nullptr) { break; }

				//limit user nums
				if (csv->_CurrentUser + 1 > csv->_ChatProps.MaxUserNum) {
					csv->_sv.Disconnect(sess->uid);
					break;
				}

				//set session data from packet
				task->buf->GetData(&sess->ID, sizeof(sess->ID));
				task->buf->GetData(&sess->Nickname, sizeof(sess->Nickname));
				task->buf->GetData(&sess->SessionKey, sizeof(sess->SessionKey));
				task->buf->GetData(&sess->AccountNo, sizeof(sess->AccountNo));

				//assign assign accountNo to accountNoArr
				auto account_iter = csv->_AccountNoArr.find(sess->AccountNo);
				if (account_iter != csv->_AccountNoArr.end()) {
					csv->_sv.Disconnect(account_iter->second->uid);
					csv->_AccountNoArr.erase(account_iter);
				}
				csv->_AccountNoArr.insert({ sess->AccountNo, sess });

				//send respond of login
				SerialBuffer* buf = csv->_sv.AllocPacket();
				WORD packettype = en_PACKET_CS_CHAT_RES_LOGIN;
				BYTE status = 1;
				INT64 accountno = sess->AccountNo;
				buf->PutData(&packettype, sizeof(packettype));
				buf->PutData(&status, sizeof(status));
				buf->PutData(&accountno, sizeof(accountno));
				csv->_sv.SendPacket(sess->uid, buf);
				csv->_sv.FreePacket(buf);

				csv->_CurrentUser++;
				csv->_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::DBG,
					L"PK_LOGIN: %d\n", task->sessID);
			}
				break;
			case en_TASK_CS_RECV_FROM_CLIENT:
			{
				//get type of req
				WORD recvtype;
				task->buf->GetData(&recvtype, sizeof(recvtype));
				Session* sess = csv->FindSessByUid(task->sessID);
				sess->lastrecvtime = timeGetTime();

				switch (recvtype) {
				case en_PACKET_CS_CHAT_REQ_LOGIN:
				{
					task->buf->IncUseCount();
					PostQueuedCompletionStatus(csv->_hiocp_login,
						0, (ULONG_PTR)task->sessID, (LPOVERLAPPED)task->buf);
				}
					break;
				case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
				{
					//get data from packet
					INT64 AccountNo;
					WORD SecX;
					WORD SecY;
					task->buf->GetData(&AccountNo, sizeof(AccountNo));
					task->buf->GetData(&SecX, sizeof(SecX));
					task->buf->GetData(&SecY, sizeof(SecY));

					//check data malform
					if (AccountNo != sess->AccountNo ||
						SecX >= SECTOR_W ||
						SecY >= SECTOR_H) {
						csv->_sv.Disconnect(task->sessID);
						break;
					}

					//move sector
					csv->_Secmgr.assignNewSectorIDtoSession(sess, csv->_Secmgr.findSectorIDBySectorPos(SecY, SecX));

					//send respond of sector move
					SerialBuffer* buf = csv->_sv.AllocPacket();
					WORD packettype = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
					buf->PutData(&packettype, sizeof(packettype));
					buf->PutData(&AccountNo, sizeof(AccountNo));
					buf->PutData(&SecX, sizeof(SecX));
					buf->PutData(&SecY, sizeof(SecY));
					csv->_sv.SendPacket(sess->uid, buf);
					csv->_sv.FreePacket(buf);
					csv->_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::DBG,
						L"PK_SEC: %d\n", task->sessID);
				}
					break;
				case en_PACKET_CS_CHAT_REQ_MESSAGE:
				{
					//get data from packet
					INT64 AccountNo;
					WORD MessageLen;		//byte
					WCHAR Message[MSG_MAX_LEN];
					task->buf->GetData(&AccountNo, sizeof(AccountNo));
					task->buf->GetData(&MessageLen, sizeof(MessageLen));

					//check data validate
					if (AccountNo != sess->AccountNo ||
						MessageLen > MSG_MAX_LEN
						) {
						csv->_sv.Disconnect(task->sessID);
						break;
					}

					//get msg data
					int getmsgret = task->buf->GetData(&Message, MSG_MAX_LEN);
					if (getmsgret != MessageLen) {
						csv->_sv.Disconnect(task->sessID);
						break;
					}

					//set response of send chat
					SerialBuffer* buf = csv->_sv.AllocPacket();
					WORD packettype = en_PACKET_CS_CHAT_RES_MESSAGE;
					buf->PutData(&packettype, sizeof(packettype));
					buf->PutData(&AccountNo, sizeof(AccountNo));
					buf->PutData(&sess->ID, sizeof(sess->ID));
					buf->PutData(&sess->Nickname, sizeof(sess->Nickname));
					buf->PutData(&MessageLen, sizeof(MessageLen));
					buf->PutData(&Message, MessageLen);

					//get around session and send packet
					std::vector<NID> sesslist;
					csv->_Secmgr.getSessionsFromAroundSector(sess->sector, &sesslist);
					for (auto i : sesslist) {
						csv->_sv.SendPacket(i, buf);
					}
					csv->_sv.FreePacket(buf);
					csv->_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::DBG,
						L"PK_MSG: %d\n", task->sessID);
				}
					break;
				case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
					break;
				default:
					csv->_sv.Disconnect(task->sessID);
					break;
				}
			}
			break;
			default:
				csv->_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::ERR,
					L"task type invalid\n");
				break;
			}
			csv->FreeTask(task);
			csv->_UpdateThrCount++;
		}
	}

	csv->_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"[Chat] Thread Update Returned.\n");
	return 0;
}
UINT WINAPI Thread_TimeOut(PVOID param) {
	ChatServer* csv = (ChatServer*)param;

	while (!csv->_isShutdown) {
		Task* t = csv->AllocTask(en_TASK_CS_TIMEOUT);
		csv->RunTask(t);
		Sleep(1000);
	}

	csv->_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"[Chat] Thread Timeout Returned.\n");
	return 0;
}
UINT WINAPI Thread_Login(PVOID param) {
	ChatServer* csv = (ChatServer*)param;
	//get and connect redis client
	cpp_redis::client rclient;
	try {
		rclient.connect(csv->_ChatProps.Redis_IP, csv->_ChatProps.Redis_Port);
	}
	catch (std::exception& e) {
		printf("Redis Error at Thread_Login: %s\n", e.what());
		return 0;
	}

	std::string str_accno;
	DWORD transbytes;
	ULONG_PTR cKey;
	OVERLAPPED* ovl;

	while (true) {
		//reset args
		transbytes = 0;
		cKey = 0;
		ovl = nullptr;

		//do gqcs
		BOOL gqcsret = GetQueuedCompletionStatus(csv->_hiocp_login, &transbytes, &cKey, &ovl, INFINITE);
		DWORD err = GetLastError();
		DWORD wsaerr = WSAGetLastError();

		//check shutdown msg
		if (transbytes == 1) {
			break;
		}

		//get sess info from gqcs args
		NID sessID = (NID)cKey;
		SerialBuffer* buf = (SerialBuffer*)ovl;
		INT64 AccountNo;
		buf->GetData(&AccountNo, sizeof(AccountNo));
		buf->PutData(&AccountNo, sizeof(AccountNo));

		str_accno.clear();
		str_accno = std::to_string(AccountNo);

		//check sess info via redis
		bool isExist = false;
		std::future<cpp_redis::reply> get_reply = rclient.get(str_accno);
		rclient.sync_commit();
		
		cpp_redis::reply reply;
		reply = get_reply.get();
		isExist = !reply.is_null();

		//process afterwards
		if (isExist) {
			Task* t = csv->AllocTask(en_TASK_CS_NEW_PLAYER);
			t->sessID = sessID;
			t->buf = buf;
			csv->RunTask(t);
		}
		else {
			csv->_sv.Disconnect(sessID);
		}
		csv->_sv.FreePacket(buf);
	}

	csv->_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"[Chat] Thread Login Returned.\n");
	return 0;
}

void ChatServer::Init(ChatServer_Properties props) {
	_ChatProps = props;
	_ncbk.csv = this;
	_hUpdateEv = CreateEvent(NULL, false, false, NULL);

	_pdh.Init();
	_sv.Init(_ChatProps, &_ncbk);
	return;
}

void ChatServer::Start() {
	_isShutdown = false;

	//start thread
	_hiocp_login = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
	_ThrHandles[0] = (HANDLE)_beginthreadex(NULL, 0, Thread_ChatUpdate, (PVOID)this, 0, NULL);
	_ThrHandles[1] = (HANDLE)_beginthreadex(NULL, 0, Thread_TimeOut, (PVOID)this, 0, NULL);
	_ThrHandles[2] = (HANDLE)_beginthreadex(NULL, 0, Thread_Login, (PVOID)this, 0, NULL);

	//start netserver
	_sv.Start();
	return;
}

void ChatServer::Stop() {
	_isShutdown = true;										//Stop Thread_TimeOut
	_sv.Stop();												//Stop NetServer
	RunTask(AllocTask(en_TASK_CS_SHUTDOWN));				//Stop Thread_Update
	PostQueuedCompletionStatus(_hiocp_login, 1, NULL, NULL);//Stop Thread_Login

	//wait for thread all return
	DWORD timeout = 5000;		//5sec
	DWORD ret;
	_csvlog.writeLog((unsigned int)LOGOPT::DEFAULT, LOGLEVEL::SYS,
		L"[Chat] Wait for all thread down...\n");
	ret = WaitForMultipleObjects(3, _ThrHandles, true, timeout);
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
	static ULONGLONG PrevUpdateCount = 0;

	ULONGLONG acceptc = _sv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_ACCEPT);
	ULONGLONG sendc = _sv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_SEND);
	ULONGLONG recvc = _sv.GetMonitorDataULL(en_NETSV_MTR_TYPE::MTR_ULL_NET_CNT_RECV);
	int gpp = _sv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_CAP_PKT);
	int gup = _sv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_USE_PKT);
	unsigned long long updc = _UpdateThrCount;
	int nsp = _sv.GetMonitorDataInt(en_NETSV_MTR_TYPE::MTR_INT_POOL_CAP_NSESS);
	unsigned int tos = _TimeoutSess;
	unsigned int tou = _TimeoutUser;
	int tkp = _TaskPool.getCapacity();
	int sep = _SessPool.getCapacity();
	int sepu = _CurrentSess;
	int plpu = _CurrentUser;
	int tkq = _TaskQ.getSize();
	float syscpu = _pdh.GetSysCpuTime();
	float procpu = _pdh.GetProcessCpuTime();
	LONGLONG sysnpp = _pdh.GetSysNonPagedPool();
	LONGLONG prompb = _pdh.GetProcessPrivateBytes();
	double netrcv = _pdh.GetNetworkRecv();
	double netsnd = _pdh.GetNetworkSend();

	ULONGLONG acptps = acceptc - PrevAcceptCount;
	ULONGLONG sndps = sendc - PrevSendCount;
	ULONGLONG rcvps = recvc - PrevRecvCount;
	ULONGLONG updps = updc - PrevUpdateCount;

	PrevAcceptCount = acceptc;
	PrevSendCount = sendc;
	PrevRecvCount = recvc;
	PrevUpdateCount = updc;

	if (_mtrmgr == nullptr) {
		_mtrmgr = new MonitorClientMgr();
		_mtrmgr->Init(_ChatProps.MonitorSV_IP, _ChatProps.MonitorSV_Port, 15);
		_mtrmgr->Start();
		_mtrmgr->LoginToMonitor();
	}
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, 1);
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, (int)procpu);
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, (int)_pdh.ToMiB(prompb));
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_CHAT_SESSION, (int)sepu);
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_CHAT_PLAYER, (int)plpu);
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, (int)updps);
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, (int)gpp);
	_mtrmgr->SendData(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, (int)tkp);



	std::string line0, line1, line2, line3, line4, line5;

	line0 += "=====================\n";

	line1 += "Sess: "; line1 += std::to_string(sepu);
	line1 += ", User: "; line1 += std::to_string(plpu);
	line1 += ", AcceptCnt: "; line1 += std::to_string(acceptc);
	line1 += " ("; line1 += std::to_string(acptps);
	line1 += "/s)\n";

	line2 += "Taskpool: "; line2 += std::to_string(tkp);
	line2 += ", Sesspool: "; line2 += std::to_string(sep);
	line2 += ", PKpool: "; line2 += std::to_string(gpp);
	line2 += ", NSpool: "; line2 += std::to_string(nsp);
	line2 += "\n";

	line3 += "TaskQSize: "; line3 += std::to_string(tkq);
	line3 += ", SessUsing: "; line3 += std::to_string(sepu);
	line3 += ", PKUsing: "; line3 += std::to_string(gup);
	line3 += "\n";

	line4 += "Update: "; line4 += std::to_string(updps);
	line4 += "/s, Send: "; line4 += std::to_string(sndps);
	line4 += "/s, Recv: "; line4 += std::to_string(rcvps);
	line4 += "/s\n";

	line4 += "TOSess: "; line4 += std::to_string(tos);
	line4 += "TOUser: "; line4 += std::to_string(tou);

	line5 += "SysCPU: "; line5 += std::to_string(syscpu);
	line5 += "%, ProCPU: "; line5 += std::to_string(procpu);
	line5 += "%\n";
	line5 += "SysMemNPP: "; line5 += std::to_string(_pdh.ToMiB(sysnpp));
	line5 += "MiB, ProMemPB: "; line5 += std::to_string(_pdh.ToMiB(prompb));
	line5 += "MiB\n";
	line5 += "NetRecv: "; line5 += std::to_string(_pdh.ToMiB(netrcv));
	line5 += "MiB/s, NetSend: "; line5 += std::to_string(_pdh.ToMiB(netsnd));
	line5 += "MiB/s\n";

	line0 += line1 += line2 += line3 += line4 += line5;
	std::cout << line0 << std::endl;

	return;
}

Task* ChatServer::AllocTask(en_TASK_TYPE type) {
	Task* t = _TaskPool.Alloc();
	t->type = type;
	t->buf = nullptr;
	return t;
}

void ChatServer::FreeTask(Task* task) {
	if (task->buf != nullptr) { _sv.FreePacket(task->buf); }
	_TaskPool.Free(task);
	return;
}

Session* ChatServer::AllocSess(NID uid) {
	Session* s = _SessPool.Alloc();
	s->uid = uid;
	s->AccountNo = -1;
	s->sector = -1;
	s->lastrecvtime = timeGetTime();
	InterlockedIncrement(&_CurrentSess);
	return s;
}

void ChatServer::FreeSess(Session* sess) {
	_SessPool.Free(sess);
	InterlockedDecrement(&_CurrentSess);
	return;
}

Session* ChatServer::FindSessByUid(NID uid) {
	auto i = _UserArr.find(uid);
	if (i == _UserArr.end()) { return nullptr; }
	return i->second;
}

void ChatServer::RunTask(Task* task) {
	if (task->buf != nullptr) { task->buf->IncUseCount(); }
	_TaskQ.Enqueue(task);
	SetEvent(_hUpdateEv);
	return;
}
