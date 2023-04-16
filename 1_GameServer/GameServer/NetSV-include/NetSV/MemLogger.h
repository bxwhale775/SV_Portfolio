#pragma once
#include <Windows.h>

////////////////////////////////////////////////////////
//Do something here if you need for MemLog


////////////////////////////////////////////////////////

#define LOGBUF_SIZE 0x0000FFFF
#define LOGBUFIDX_SIZE 0x000000FF

struct ThrLog {
	DWORD IDX;
	DWORD ThreadID;
	///////////////////////////////////
	//Modify here if you need
	const WCHAR* msg;
	const char* func;
	PVOID ptr1;
	PVOID ptr2;
	///////////////////////////////////
};

extern ThrLog tlog[LOGBUFIDX_SIZE + 1][LOGBUF_SIZE + 1];
extern ULONG tlog_idx;

#define Log_Def \
ULONG ctlog, ltlog, lt1log, lt2log;

#define Log_Init(CTLOGIDX, LTLOGIDX, LT1IDX, LT2IDX) \
CTLOGIDX = InterlockedIncrement(&tlog_idx); \
LTLOGIDX = CTLOGIDX & 0x00FFFFFF; \
LT1IDX = LTLOGIDX & 0x00FF0000; \
LT2IDX = LTLOGIDX & 0x0000FFFF; \
LT1IDX >>= 16; \
tlog[LT1IDX][LT2IDX].ThreadID = GetCurrentThreadId();

#define Log_Set \
Log_Init(ctlog, ltlog, lt1log, lt2log)

#define Log_Idx \
tlog[lt1log][lt2log].IDX = ctlog;

#define Log_Data(CATEGORY, DATA) \
tlog[lt1log][lt2log].CATEGORY = DATA;

///////////////////////////////////
//Modify here if you need
#define Log_Str(MESG, P1, P2)	\
Log_Set \
Log_Idx \
Log_Data(msg, MESG) \
Log_Data(func, __FUNCTION__) \
Log_Data(ptr1, P1) \
Log_Data(ptr2, P2)

///////////////////////////////////