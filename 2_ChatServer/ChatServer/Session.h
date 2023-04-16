#pragma once
#include <Windows.h>

struct Session {
	unsigned __int64 uid;
	INT64 AccountNo = -1;
	int sector = -1;
	unsigned long lastrecvtime = 0;

	WCHAR ID[20];
	WCHAR Nickname[20];
	char SessionKey[64];
};
