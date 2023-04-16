#pragma once
#include <winsock2.h>
#include <Windows.h>
#include <unordered_set>
#include "DBConnMgr.h"
#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis_lib/cpp_redis.lib")
#pragma comment (lib, "cpp_redis_lib/tacopie.lib")
#pragma comment (lib, "ws2_32.lib")

class DBConnMgr_MT {
private:
	DWORD _TlsIdxDBC;
	sql::Driver* _Driver = nullptr;
	bool _isInited = false;

	const char* _hostName = nullptr;
	const char* _userName = nullptr;
	const char* _password = nullptr;
	unsigned int _MaxQryLen = 0;

	std::unordered_set<DBConnMgr*> _UsingDBConnSet;
	SRWLOCK _UsingDBConnSetLock;
public:
	DBConnMgr_MT() { 
		_TlsIdxDBC = TlsAlloc(); 
		InitializeSRWLock(&_UsingDBConnSetLock);
	};
	~DBConnMgr_MT() { TlsFree(_TlsIdxDBC); };

	//init DBConnMgr_MT
	void Init(const char* hostName, const char* userName, const char* password, unsigned int MaxQueryLen = SQL_QUERYLEN_MAX) {
		_Driver = get_driver_instance();
		_hostName = hostName;
		_userName = userName;
		_password = password;
		_MaxQryLen = MaxQueryLen;
		_isInited = true;
		return;
	}

	//delete all dbconn via set
	void WipeAllDBConn() {
		AcquireSRWLockExclusive(&_UsingDBConnSetLock);
		for (auto i : _UsingDBConnSet) {
			i->Stop();
			delete i;
		}
		_UsingDBConnSet.clear();
		ReleaseSRWLockExclusive(&_UsingDBConnSetLock);
		return;
	}

	//get or create DBConn from TLS. retrun 0 if success.
	//!!! please call FreeDBConn func after use !!!
	int AllocDBConn(DBConnMgr** ret) {
		if (!_isInited) { return -1; }
		int err = 0;

		//get or create dbconn from TLS
		DBConnMgr* dbc = (DBConnMgr*)TlsGetValue(_TlsIdxDBC);
		if (dbc == nullptr) {
			dbc = new DBConnMgr(_Driver);
			if (!dbc->Start(_hostName, _userName, _password, _MaxQryLen)) {
				err = dbc->GetLastError();
			}
			TlsSetValue(_TlsIdxDBC, dbc);
			//assign new dbconn to set - for WipeAllDBConn()
			AcquireSRWLockExclusive(&_UsingDBConnSetLock);
			_UsingDBConnSet.insert(dbc);
			ReleaseSRWLockExclusive(&_UsingDBConnSetLock);
		}

		if (ret != nullptr) { *ret = dbc; }
		return err;
	}

	//free and delete DBConn from TLS.
	bool FreeDBConn() {
		if (!_isInited) { return false; }

		//get dbconn from TLS
		DBConnMgr* dbc = (DBConnMgr*)TlsGetValue(_TlsIdxDBC);
		if (dbc == nullptr) { return false; }
		//erase dbconn from set - for WipeAllDBConn()
		AcquireSRWLockExclusive(&_UsingDBConnSetLock);
		_UsingDBConnSet.erase(dbc);
		ReleaseSRWLockExclusive(&_UsingDBConnSetLock);
		//stop and delete dbconn
		dbc->Stop();
		delete dbc;
		TlsSetValue(_TlsIdxDBC, nullptr);

		return true;
	}
};

