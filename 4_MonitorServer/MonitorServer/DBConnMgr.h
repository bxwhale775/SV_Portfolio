#pragma once
#include <Windows.h>
#include <strsafe.h>
#include "NetSV-include/CNetServer.h"
#include "mysql/mysql/jdbc.h"
#pragma comment(lib, "mysql/lib/mysqlcppconn.lib")
#pragma comment(lib, "winmm")


#define SQL_QUERYLEN_MAX 4096

#define DBCONNMGR_ERR_EXE false
#define DBCONNMGR_ERR_EXEQUERY nullptr
#define DBCONNMGR_ERR_EXEUPDATE -10

class DBConnMgr {
private:
	bool _isInited = false;
	sql::Driver* _Driver = nullptr;
	sql::Connection* _Conn = nullptr;
	unsigned int _MaxQueryLen = 0;
	WCHAR* _WideQuery = nullptr;
	char* _Utf8Query = nullptr;
	int _LastErr = 0;
	DWORD _TimeOut = 0;
	Logger* _DBLog = nullptr;
	sql::ConnectOptionsMap* _connection_properties = nullptr;

public:
	DBConnMgr() {}
	DBConnMgr(sql::Driver* driver) { _Driver = driver; }
	~DBConnMgr() {}

	void SetTimeOut(DWORD ms) { _TimeOut = ms; }
	void SetLogger(Logger* logger) { _DBLog = logger; }

	bool Start(const char* hostName, const char* userName, const char* password, unsigned int MaxQueryLen=SQL_QUERYLEN_MAX) {
		if (_isInited) { return true; }
		_LastErr = 0;

		_MaxQueryLen = MaxQueryLen;
		_WideQuery = new WCHAR[_MaxQueryLen];
		_Utf8Query = new char[_MaxQueryLen];

		_connection_properties = new sql::ConnectOptionsMap();
		(*_connection_properties)["hostName"] = hostName;
		(*_connection_properties)["userName"] = userName;
		(*_connection_properties)["password"] = password;
		(*_connection_properties)["OPT_RECONNECT"] = true;

		try {
			if(_Driver == nullptr){ _Driver = get_driver_instance(); }
			_Conn = _Driver->connect(*_connection_properties);
		}
		catch (sql::SQLException& e) {
			_LastErr = e.getErrorCode();
			if (_Conn != nullptr) { delete _Conn; _Conn = nullptr; }
			if (_WideQuery != nullptr) { delete _WideQuery; _WideQuery = nullptr; }
			if (_Utf8Query != nullptr) { delete _Utf8Query; _Utf8Query = nullptr; }
			if (_connection_properties != nullptr) { delete _connection_properties; _connection_properties = nullptr; }
			_isInited = false;
			return false;
		}
		_isInited = true;
		return true;
	}

	bool Stop() {
		if (!_isInited) { return false; }
		_LastErr = 0;

		delete _Conn;
		delete _WideQuery;
		delete _Utf8Query;
		delete _connection_properties;
		_Conn = nullptr;
		_WideQuery = nullptr;
		_Utf8Query = nullptr;
		_connection_properties = nullptr;
		_isInited = false;

		return true;
	}

	//return true if success. otherwise return DBCONNMGR_ERR_EXE.
	bool execute(const WCHAR* query, ...) {
		if (!_isInited) { return false; }
		_LastErr = 0;
		bool ret = DBCONNMGR_ERR_EXE;
		va_list args;
		va_start(args, query);
		bool cvtres = ConvertQuery(query, args);
		va_end(args);
		if (!cvtres) { return false; }

		sql::Statement* stmt = nullptr;
		try {
			stmt = _Conn->createStatement();
			DWORD timeout = StartTimeOutCheck();
			ret = stmt->execute(_Utf8Query);
			EndTimeOutCheck(timeout);
		}
		catch (sql::SQLException& e) {
			_LastErr = e.getErrorCode();
			ret = DBCONNMGR_ERR_EXE;
		}
		if(stmt != nullptr){ delete stmt; }
		return ret;
	}

	//return sql::ResultSet* if success. otherwise return DBCONNMGR_ERR_EXEQUERY.
	//!!! please delete resultset after use !!!
	sql::ResultSet* executeQuery(const WCHAR* query, ...) {
		if (!_isInited) { return nullptr; }
		_LastErr = 0;
		sql::ResultSet* ret = DBCONNMGR_ERR_EXEQUERY;
		va_list args;
		va_start(args, query);
		bool cvtres = ConvertQuery(query, args);
		va_end(args);
		if (!cvtres) { return nullptr; }

		sql::Statement* stmt = nullptr;
		try {
			stmt = _Conn->createStatement();
			DWORD timeout = StartTimeOutCheck();
			ret = stmt->executeQuery(_Utf8Query);
			EndTimeOutCheck(timeout);
		}
		catch (sql::SQLException& e) {
			_LastErr = e.getErrorCode();
			ret = DBCONNMGR_ERR_EXEQUERY;
		}
		if (stmt != nullptr) { delete stmt; }
		return ret;
	}

	//return affected rows count if success. otherwise return DBCONNMGR_ERR_EXEUPDATE.
	int executeUpdate(const WCHAR* query, ...) {
		if (!_isInited) { return -1; }
		_LastErr = 0;
		int ret = DBCONNMGR_ERR_EXEUPDATE;
		va_list args;
		va_start(args, query);
		bool cvtres = ConvertQuery(query, args);
		va_end(args);
		if (!cvtres) { return -1; }

		sql::Statement* stmt = nullptr;
		try {
			stmt = _Conn->createStatement();
			DWORD timeout = StartTimeOutCheck();
			ret = stmt->executeUpdate(_Utf8Query);
			EndTimeOutCheck(timeout);
		}
		catch (sql::SQLException& e) {
			_LastErr = e.getErrorCode();
			ret = DBCONNMGR_ERR_EXEUPDATE;
		}
		if (stmt != nullptr) { delete stmt; }
		return ret;
	}
	
	int GetLastError() {
		return _LastErr;
	}

private:
	bool ConvertQuery(const WCHAR* query, va_list args) {
		//format query and copy to widequery.
		HRESULT vpfres = StringCchVPrintf(_WideQuery, _MaxQueryLen, query, args);
		if (FAILED(vpfres)) {
			return false;
		}

		//convert widequery to utf8.
		int cvtres = WideCharToMultiByte(CP_UTF8, 0, _WideQuery, -1, _Utf8Query, _MaxQueryLen, NULL, NULL);
		if (cvtres == 0) {
			return false;
		}
		return true;
	}
	inline DWORD StartTimeOutCheck() {
		if (_TimeOut == 0) { return 0; }
		return timeGetTime();
	}
	inline void EndTimeOutCheck(DWORD ms) {
		if (_TimeOut == 0) { return; }
		if (timeGetTime() - ms > ms && _DBLog != nullptr) {
			_DBLog->writeLog((UINT)LOGOPT::FILE, LOGLEVEL::ERR,
				L"[Thread: %d]DB is Too Slow. SQL: %s",
				GetCurrentThreadId(), _WideQuery);
		}
		return;
	}
};
