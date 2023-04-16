#include <string>
#include <stdarg.h>
#include <windows.h>
#include <locale.h>
#include "NetSV-include/NetSV/Logger.h"

Logger::Logger(const wchar_t* name) {
	setlocale(LC_ALL, "");
	_wsetlocale(LC_ALL, L"");

	size_t len = wcslen(name) + 1;
	_pfilename = new wchar_t[len];
	wcscpy_s(_pfilename, len, name);
	InitializeSRWLock(&_FileLock);
	return;
}

Logger::~Logger() {
	if (_pfilename != nullptr) { delete[] _pfilename; }
	return;
}

void Logger::writeLog(UINT logopt, LOGLEVEL loglvl, const wchar_t* format, ...) {
	if (loglvl > _current_loglvl) { return; }

	//build loglvl str
	WCHAR* loglvlstr;
	switch (loglvl) {
	case LOGLEVEL::ERR:
		loglvlstr = (WCHAR*)L"ERR";
		break;
	case LOGLEVEL::SYS:
		loglvlstr = (WCHAR*)L"SYS";
		break;
	case LOGLEVEL::DBG:
		loglvlstr = (WCHAR*)L"DBG";
		break;
	default:
		loglvlstr = (WCHAR*)L"UKN";
		break;
	}

	//calc time and build timestr
	constexpr size_t timestrlen = 31;
	SYSTEMTIME ctime;
	GetLocalTime(&ctime);
	WCHAR timestr[timestrlen];
	swprintf_s(timestr, timestrlen, L"[[%3.3s]%04d/%02d/%02d-%02d:%02d:%02d.%03d]"
		, loglvlstr, ctime.wYear, ctime.wMonth, ctime.wDay, ctime.wHour, ctime.wMinute, ctime.wSecond, ctime.wMilliseconds
	);

	//prepare vars
	va_list args;
	int len;
	wchar_t* buf;
	unsigned int using_opt;

	//build log string
	va_start(args, format);
	len = _vscwprintf(format, args) + 1;
	buf = new wchar_t[len];
	vswprintf_s(buf, len, format, args);
	va_end(args);

	//check default opt
	if (logopt == (UINT)LOGOPT::DEFAULT) { using_opt = _default_logopt; }
	else { using_opt = logopt; }

	//write log
	if (using_opt & (UINT)LOGOPT::CONSOLE) {
		wprintf(L"%s %s",timestr, buf);
	}
	if (using_opt & (UINT)LOGOPT::FILE && _pfilename != nullptr) {
		FILE* pfile;
		AcquireSRWLockExclusive(&_FileLock);
		errno_t errcode = _wfopen_s(&pfile, _pfilename, L"a");
		if (pfile != nullptr) {
			fwprintf_s(pfile, L"%s %s",timestr, buf);
			fclose(pfile);
			ReleaseSRWLockExclusive(&_FileLock);
		}
		else {
			ReleaseSRWLockExclusive(&_FileLock);
			this->writeLog((UINT)LOGOPT::CONSOLE | (UINT)LOGOPT::DBGSTR, LOGLEVEL::ERR,
				L"[LOGGER] Failed to open file: %s / Code: %d\n", _pfilename, errcode);
		}
	}
	if (using_opt & (UINT)LOGOPT::DBGSTR) {
		OutputDebugString(timestr);
		OutputDebugString(buf);
	}

	delete[] buf;
	return;
}

void Logger::setLogLevel(LOGLEVEL lvl) {
	_current_loglvl = lvl;
	return;
}

void Logger::setDefaultLogOpt(UINT opt) {
	_default_logopt = opt;
	return;
}
