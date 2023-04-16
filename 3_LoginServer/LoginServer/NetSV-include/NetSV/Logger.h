#pragma once
#include <windows.h>

enum class LOGOPT {
	DEFAULT = 0b00000000,
	FILE = 0b00000001,
	CONSOLE = 0b00000010,
	DBGSTR = 0b00000100
};

enum class LOGLEVEL {
	ERR = 0,
	SYS,
	DBG,			//print less then N

};

class Logger {
private:
	wchar_t* _pfilename = nullptr;
	LOGLEVEL _current_loglvl = LOGLEVEL::SYS;
	UINT _default_logopt = (UINT)LOGOPT::CONSOLE;
	SRWLOCK _FileLock;

public:
	Logger(const wchar_t* name);
	~Logger();

	void writeLog(UINT logopt, LOGLEVEL loglvl, const wchar_t* format, ...);
	void setLogLevel(LOGLEVEL lvl);
	void setDefaultLogOpt(UINT opt);
};