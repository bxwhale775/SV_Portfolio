#pragma once

enum class DBTASKTYPE {
	NONE,
	KILL,
	LOGOUT,
	DIE,
	RESTART,
	GETCRYSTAL,
	HEAL,
};

struct DBTask {
	DBTASKTYPE Type = DBTASKTYPE::NONE;
	INT64 AccountNo = -1;
	ClientPos d_cpos = ClientPos();
	int d_int1 = 0;
	int d_int2 = 0;
	int d_int3 = 0;
};

class AsyncDBUpdater {
private:
	DBConnMgr* _mgr = nullptr;
	MemoryPoolTLS<DBTask> _TaskPool = MemoryPoolTLS<DBTask>();
	LFQueue<DBTask*> _TaskQ = LFQueue<DBTask*>();
	HANDLE _hUpdateEv = INVALID_HANDLE_VALUE;
	HANDLE _hThrHandle = INVALID_HANDLE_VALUE;
	unsigned long _UpdateCount = 0;
	friend UINT WINAPI Thread_AsyncDBExecute(PVOID param);
public:
	bool Start(DBConnMgr* dbmgr);
	bool Stop();
	unsigned long GetUpdateCount();
	long GetQueueSize();
	int GetTaskPoolCapacity();
	int GetTaskPoolUseCount();

	bool Write_Logout(INT64 AccountNo, ClientPos cpos, USHORT rotation, int crystal, int hp);
	bool Write_Die(INT64 AccountNo, ClientPos cpos, USHORT rotation, int crystal, int hp);
	bool Write_Restart(INT64 AccountNo, ClientPos cpos, USHORT rotation);
	bool Write_GetCrystal(INT64 AccountNo, int getCrystal, int totalCrystal);
	bool Write_Heal(INT64 AccountNo, int oldhp, int newhp, int sittimesec);

private:
	void RunTask(DBTask* task);
	DBTask* AllocTask(DBTASKTYPE type, INT64 accono);
	void FreeTask(DBTask* task);
};

