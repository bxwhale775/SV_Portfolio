#pragma once
#include <Windows.h>

struct Session {
	unsigned __int64 uid;
	INT64 AccountNo = -1;
	int sector = -1;
	unsigned long lastrecvtime = 0;
	SRWLOCK SessLock;

	WCHAR ID[20] = { 0, };
	WCHAR Nickname[20] = { 0, };
	char SessionKey[64] = { 0, };
};
