#pragma once
#include "NetSV-include/CNetServer.h"
#include "PDHCounter.h"
#include "Session.h"
#include "Sector.h"
#include "CommonProtocol.h"
#pragma comment (lib, "NetSV-include/lib/NetServerLib.lib")

#define TIMEOUT_SESS 40000
#define TIMEOUT_USER 40000
#define MSG_MAX_LEN 255

struct ChatServer_Properties : public CNetServer_Properties {
	int MaxUserNum = 20000;
	const WCHAR* MonitorSV_IP = L"127.0.0.1";
	u_short MonitorSV_Port = 12300;
};

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
			InterlockedIncrement(&csv->_CurrentSess);

			Session* newsess = csv->AllocSess(sessID);

			AcquireSRWLockExclusive(&csv->_UserArrLock);
			csv->_UserArr.insert({ sessID, newsess });
			ReleaseSRWLockExclusive(&csv->_UserArrLock);
			AcquireSRWLockExclusive(&csv->_DelUserArrLock);
			csv->_DelUserArr.insert({ sessID, newsess });
			ReleaseSRWLockExclusive(&csv->_DelUserArrLock);

			return true;
		}
		virtual bool OnDisconnect(const NID sessID) {
			Session* delsess = csv->FindSessByUid(sessID);
			InterlockedDecrement(&csv->_CurrentSess);

			if (delsess->AccountNo != -1) {
				InterlockedDecrement(&csv->_CurrentUser);
				csv->_Secmgr.deleteSessionFromSector(delsess);
			}
			if (delsess->AccountNo != -2) {
				AcquireSRWLockExclusive(&csv->_AccountNoArrLock);
				csv->_AccountNoArr.erase(delsess->AccountNo);
				ReleaseSRWLockExclusive(&csv->_AccountNoArrLock);
			}
			AcquireSRWLockExclusive(&csv->_UserArrLock);
			csv->_UserArr.erase(sessID);
			ReleaseSRWLockExclusive(&csv->_UserArrLock);
			AcquireSRWLockExclusive(&csv->_DelUserArrLock);
			csv->_DelUserArr.erase(sessID);
			ReleaseSRWLockExclusive(&csv->_DelUserArrLock);

			csv->FreeSess(delsess);

			return true;
		}
		virtual bool OnRecv(const NID sessID, SerialBuffer* databuf) {
			//get type of req
			WORD recvtype;
			*databuf >> recvtype;
			Session* sess = csv->FindSessByUid(sessID);
			sess->lastrecvtime = timeGetTime();

			switch (recvtype) {
			case en_PACKET_CS_CHAT_REQ_LOGIN:
			{
				//limit user nums
				if (csv->_CurrentUser + 1 > (ULONG)csv->_ChatProps.MaxUserNum) {
					csv->_sv.Disconnect(sess->uid);
					break;
				}

				//set session data from packet
				databuf->GetData(&sess->AccountNo, sizeof(sess->AccountNo));
				databuf->GetData(&sess->ID, sizeof(sess->ID));
				databuf->GetData(&sess->Nickname, sizeof(sess->Nickname));
				databuf->GetData(&sess->SessionKey, sizeof(sess->SessionKey));

				//assign accountNo to accountNoArr
				AcquireSRWLockExclusive(&csv->_AccountNoArrLock);
				auto account_iter = csv->_AccountNoArr.find(sess->AccountNo);
				if (account_iter != csv->_AccountNoArr.end()) {
					account_iter->second->AccountNo = -2;
					csv->_sv.Disconnect(account_iter->second->uid);
					csv->_AccountNoArr.erase(account_iter);
				}
				csv->_AccountNoArr.insert({ sess->AccountNo, sess });
				ReleaseSRWLockExclusive(&csv->_AccountNoArrLock);

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

				InterlockedIncrement(&csv->_CurrentUser);
			}
				break;
			case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
			{
				//get data from packet
				INT64 AccountNo;
				WORD SecX;
				WORD SecY;
				databuf->GetData(&AccountNo, sizeof(AccountNo));
				databuf->GetData(&SecX, sizeof(SecX));
				databuf->GetData(&SecY, sizeof(SecY));

				//check data malform
				if (AccountNo != sess->AccountNo ||
					SecX >= SECTOR_W ||
					SecY >= SECTOR_H) {
					csv->_sv.Disconnect(sessID);
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
			}
				break;
			case en_PACKET_CS_CHAT_REQ_MESSAGE:
			{
				//get data from packet
				INT64 AccountNo;
				WORD MessageLen;		//byte
				WCHAR Message[MSG_MAX_LEN];
				databuf->GetData(&AccountNo, sizeof(AccountNo));
				databuf->GetData(&MessageLen, sizeof(MessageLen));

				//check data validate
				if (AccountNo != sess->AccountNo ||
					MessageLen > MSG_MAX_LEN
					) {
					csv->_sv.Disconnect(sessID);
					break;
				}

				//get msg data
				int getmsgret = databuf->GetData(&Message, MSG_MAX_LEN);
				if (getmsgret != MessageLen) {
					csv->_sv.Disconnect(sessID);
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

				std::vector<NID> aroundsesslist;
				csv->_Secmgr.getSessionsFromAroundSector(sess->sector, &aroundsesslist);
				for (auto i : aroundsesslist) {
					csv->_sv.SendPacket(i, buf);
				}
				csv->_sv.FreePacket(buf);
			}
				break;
			case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
				break;
			default:
				csv->_sv.Disconnect(sessID);
				break;
			}

			return true;
		}
	};

	std::unordered_map<NID, Session*> _UserArr;
	std::unordered_map<INT64, Session*> _AccountNoArr;
	std::unordered_map<NID, Session*> _DelUserArr;
	MemoryPoolTLS<Session> _SessPool = MemoryPoolTLS<Session>();

	SRWLOCK _UserArrLock;
	SRWLOCK _AccountNoArrLock;
	SRWLOCK _DelUserArrLock;

	bool _isShutdown = true;
	SectorMgr _Secmgr = SectorMgr();

	CNetServer _sv;
	NetCallBack _ncbk;
	ChatServer_Properties _ChatProps;
	HANDLE _hThrTimeOut = INVALID_HANDLE_VALUE;
	Logger _csvlog = Logger(L"chatsvlog.txt");

	unsigned long _CurrentSess = 0;
	unsigned long _CurrentUser = 0;
	unsigned long _TimeoutUser = 0;
	unsigned long _TimeoutSess = 0;
	PDHCounter pdh;

public:
	void Init(ChatServer_Properties props);
	void Start();
	void Stop();

	void PrintLog();
private:
	friend UINT WINAPI Thread_TimeOut(PVOID param);
	Session* AllocSess(NID uid);
	void FreeSess(Session* sess);

	Session* FindSessByUid(NID uid);
};

