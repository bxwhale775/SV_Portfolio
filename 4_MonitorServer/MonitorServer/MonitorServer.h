#pragma once
#include <unordered_set>
#include "NetSV-include/CNetServer.h"
#include "DBConnMgr_MT.h"
#include "MonitorProtocol.h"
#pragma comment (lib, "NetSV-include/lib/NetServerLib.lib")

#define DBSAVE_FREQ_SEC 60
#define MONITOR_CLIENT_SESSKEY "ajfw@!cv980dSZ[fje#@fdj123948djf"

class MonitorServer;

struct DataBlock {
	int ValueSum = 0;
	int ValueMax = INT_MIN;
	int ValueMin = INT_MAX;
	int DBCount = 0;
};

struct ServerInfoBlock {
	NID Nid = 0;
	BYTE SvNo = 0;
	std::unordered_map<BYTE, DataBlock> DataMap;
};

struct MonitorServer_Properties {
	u_short SV_Port = 12300;
	u_short CL_Port = 11990;
	const char* DB_HostName = "tcp://127.0.0.1:3306/logdb";
	const char* DB_UserName = "root";
	const char* DB_Password = "12345678";
	const WCHAR* MonitorSV_LoggerName = L"MonitorLog.txt";
};

class MonitorServer {
public:
	class MonitorSVCallback : public Interface_CNetServer_Callback {
	public:
		MonitorServer* msv = nullptr;
		std::vector<NID> tempnidvec;

		virtual bool OnWorkerThreadStart(DWORD ThrID) {
			int ret = msv->_db.AllocDBConn(nullptr);
			if (ret != 0) {
				printf("DBConn Error at Worker Start: %d\n", ret);
			}
			return true;
		}
		virtual bool OnWorkerThreadEnd(DWORD ThrID) {
			msv->_db.FreeDBConn();
			return true;
		}
		virtual bool OnAccept(const WCHAR* ip, const u_short port) {
			return true;
		}
		virtual bool OnJoin(const NID sessID) {
			msv->_SvMap[sessID].Nid = sessID;
			return true;
		}
		virtual bool OnDisconnect(const NID sessID) {
			msv->_SvMap.erase(sessID);
			return true;
		}
		virtual bool OnRecv(const NID sessID, SerialBuffer* databuf) {
			WORD Type;
			BYTE DataType;
			int	DataValue;
			int	TimeStamp;
			int ServerNo;

			databuf->GetData(&Type, sizeof(Type));
			if (Type == en_PACKET_SS_MONITOR_DATA_UPDATE) {
				databuf->GetData(&DataType, sizeof(DataType));
				databuf->GetData(&DataValue, sizeof(DataValue));
				databuf->GetData(&TimeStamp, sizeof(TimeStamp));
				BYTE svno = msv->_SvMap[sessID].SvNo;

				DataBlock& datablock = msv->_SvMap[sessID].DataMap[DataType];
				datablock.ValueSum += DataValue;
				datablock.ValueMax = max(datablock.ValueMax, DataValue);
				datablock.ValueMin = min(datablock.ValueMin, DataValue);
				datablock.DBCount++;

				//Send data to client
				st_PACKET_CS_MONITOR_TOOL_DATA_UPDATE updpkt;
				updpkt.ServerNo = svno;
				updpkt.DataType = DataType;
				updpkt.DataValue = DataValue;
				updpkt.TimeStamp = TimeStamp;
				SerialBuffer* res = msv->_sv_cl.AllocPacket();
				res->PutData(&updpkt, sizeof(updpkt));

				AcquireSRWLockExclusive(&msv->ClMapLock);
				for (auto i : msv->_ClMap) {
					if (i.second) {
						tempnidvec.push_back(i.first);
					}
				}
				ReleaseSRWLockExclusive(&msv->ClMapLock);
				for (auto i : tempnidvec) {
					msv->_sv_cl.SendPacket(i, res);
				}
				tempnidvec.clear();
				msv->_sv_cl.FreePacket(res);

				if (datablock.DBCount == DBSAVE_FREQ_SEC) {
					int dataavg = datablock.ValueSum / DBSAVE_FREQ_SEC;
					int datamax = datablock.ValueMax;
					int datamin = datablock.ValueMin;

					//Send data to DB
					//get dbconn from TLS
					DBConnMgr* dbconn;
					int ret = msv->_db.AllocDBConn(&dbconn);
					if (ret != 0) {
						printf("DBConn Error at: %d\n", ret);
					}

					SYSTEMTIME ctime;
					GetLocalTime(&ctime);
					std::wstring tablename = L"MonitorLog_";
					tablename += std::to_wstring(ctime.wYear);
					if (ctime.wMonth < 10) {
						tablename += L"0";
						tablename += std::to_wstring(ctime.wMonth);
					}
					else {
						tablename += std::to_wstring(ctime.wMonth);
					}
					bool dbret = dbconn->execute(
						L"INSERT INTO `logdb`.`%s` (`LogTime`, `ServerNo`, `Data_Type`, `Data_Avg`, `Data_Min`, `Data_Max`) VALUES (FROM_UNIXTIME(%d), %d, %d, %d, %d, %d);",
						tablename.c_str(), TimeStamp, svno, DataType, dataavg, datamin, datamax);
					if (!dbret) {
						int dberr = dbconn->GetLastError();
						if (dberr == 1146) {
							dbconn->execute(L"CREATE TABLE `%s` LIKE MonitorLog_template",
								tablename.c_str());
							wprintf(L"creating new table: %s\n", tablename.c_str());
							dbret = dbconn->execute(
								L"INSERT INTO `logdb`.`%s` (`LogTime`, `ServerNo`, `Data_Type`, `Data_Avg`, `Data_Min`, `Data_Max`) VALUES (FROM_UNIXTIME(%d), %d, %d, %d, %d, %d);",
								tablename.c_str(), TimeStamp, svno, DataType, dataavg, datamin, datamax);
						}
					}

					datablock.ValueSum = 0;
					datablock.ValueMax = INT_MIN;
					datablock.ValueMin = INT_MAX;
					datablock.DBCount = 0;
				}
			}
			else if (Type == en_PACKET_SS_MONITOR_LOGIN) {
				databuf->GetData(&ServerNo, sizeof(ServerNo));
				msv->_SvMap[sessID].SvNo = ServerNo;
				msv->_pmlog->writeLog((UINT)LOGOPT::FILE | (UINT)LOGOPT::CONSOLE, LOGLEVEL::SYS,
					L"LAN Server Connected: Server #%d\n", ServerNo);
			}
			else {
				_sv->Disconnect(sessID);
			}

			return true;
		}
	};

	class MonitorCLCallback : public Interface_CNetServer_Callback {
	public:
		MonitorServer* msv = nullptr;
		virtual bool OnWorkerThreadStart(DWORD ThrID) {
			return false;
		}
		virtual bool OnWorkerThreadEnd(DWORD ThrID) {
			return false;
		}
		virtual bool OnAccept(const WCHAR* ip, const u_short port) {
			return true;
		}
		virtual bool OnJoin(const NID sessID) {
			AcquireSRWLockExclusive(&msv->ClMapLock);
			msv->_ClMap.insert({ sessID, false });
			ReleaseSRWLockExclusive(&msv->ClMapLock);
			return true;
		}
		virtual bool OnDisconnect(const NID sessID) {
			AcquireSRWLockExclusive(&msv->ClMapLock);
			msv->_ClMap.erase(sessID);
			ReleaseSRWLockExclusive(&msv->ClMapLock);
			return true;
		}
		virtual bool OnRecv(const NID sessID, SerialBuffer* databuf) {
			st_PACKET_CS_MONITOR_TOOL_REQ_LOGIN reqlogin;
			databuf->GetData(&reqlogin, sizeof(reqlogin));
			if (reqlogin.Type != en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN) {
				_sv->Disconnect(sessID);
				return true;
			}
			char tempkey[33];
			strcpy_s(tempkey, sizeof(tempkey), reqlogin.LoginSessionKey);
			tempkey[32] = '\0';
			tempkey[0] = reqlogin.LoginSessionKey[0];
			if (strcmp(tempkey, MONITOR_CLIENT_SESSKEY) != 0) {
				_sv->Disconnect(sessID);
				return true;
			}
			st_PACKET_CS_MONITOR_TOOL_RES_LOGIN reslogin;
			reslogin.Status = dfMONITOR_TOOL_LOGIN_OK;
			SerialBuffer* buf = _sv->AllocPacket();
			buf->PutData(&reslogin, sizeof(reslogin));
			_sv->SendPacket(sessID, buf);
			_sv->FreePacket(buf);

			AcquireSRWLockExclusive(&msv->ClMapLock);
			msv->_ClMap[sessID] = true;
			ReleaseSRWLockExclusive(&msv->ClMapLock);

			WCHAR sessIP[NETSV_IPSTR_LEN];
			_sv->GetIpAddress(sessID, sessIP);
			msv->_pmlog->writeLog((UINT)LOGOPT::FILE | (UINT)LOGOPT::CONSOLE, LOGLEVEL::SYS,
				L"Monitor viewer Connected: %s:%u\n", sessIP, _sv->GetPort(sessID));
			return true;
		}
	};

private:
	CNetServer _sv_sv;	//need init
	CNetServer _sv_cl;	//need init
	DBConnMgr_MT _db;	//need init
	MonitorSVCallback _svcbk;	//need init
	MonitorCLCallback _clcbk;	//need init
	std::unordered_map<NID, ServerInfoBlock> _SvMap;
	std::unordered_map<NID, bool> _ClMap;
	SRWLOCK ClMapLock;	//need init
	MonitorServer_Properties _props;
	bool _isStarted = false;
	HANDLE _hThrSvStat;
	Logger* _pmlog = nullptr;	//need init
	friend UINT WINAPI Thread_ThrSvStat(PVOID param);

public:
	~MonitorServer();

	void Init(MonitorServer_Properties mtsvprops);
	void Start();
	void Stop();

};

