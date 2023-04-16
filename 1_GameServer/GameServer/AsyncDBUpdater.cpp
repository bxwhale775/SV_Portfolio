#include "NetSV-include/CNetServer.h"
#include "DBConnMgr_MT.h"
#include "Pos.h"
#include "AsyncDBUpdater.h"


UINT WINAPI Thread_AsyncDBExecute(PVOID param) {
	AsyncDBUpdater* asyncdb = (AsyncDBUpdater*)param;
	DBConnMgr* dbc = asyncdb->_mgr;
	DBTask* task;
	bool isShutdown = false;

	while (!isShutdown) {
		//wait thread
		task = nullptr;
		int waitret = WaitForSingleObject(asyncdb->_hUpdateEv, INFINITE);
		if (waitret == WAIT_FAILED) {
			break;
		}
		//do task until taskQ is empty
		while (true) {
			//get task from queue
			if (!asyncdb->_TaskQ.Dequeue(&task)) { break; }

			switch (task->Type) {
			case DBTASKTYPE::KILL:
			{
				isShutdown = true;
			}
			break;
			case DBTASKTYPE::LOGOUT:
			{
				ClientPos cpos = task->d_cpos;
				TilePos tpos = ClientPosToTilePos(cpos);
				dbc->execute(L"START TRANSACTION;");
				const WCHAR* query =
					L"UPDATE `gamedb`.`character` SET "
					L"`posx` = '%f' , "
					L"`posy` = '%f' , "
					L"`tilex` = '%d' , "
					L"`tiley` = '%d' , "
					L"`rotation` = '%d' , "
					L"`cristal` = '%d' , "
					L"`hp` = '%d' "
					L"WHERE `accountno` = '%lld';";
				dbc->execute(query,
					cpos.x, cpos.y, tpos.x, tpos.y, task->d_int1, task->d_int2, task->d_int3, task->AccountNo);
				query = 
					L"INSERT INTO `gamedb`.`gamelog`(`type`, `code`, `accountno`, `param1`, `param2`, `param3`, `param4`) "
					L"VALUES(%d, %d, %lld, %d, %d, %d, %d);";
				dbc->execute(query,
					1, 12, task->AccountNo, tpos.x, tpos.y, task->d_int2, task->d_int3);
				dbc->execute(L"COMMIT;");
			}
			break;
			case DBTASKTYPE::DIE:
			{
				ClientPos cpos = task->d_cpos;
				TilePos tpos = ClientPosToTilePos(cpos);
				dbc->execute(L"START TRANSACTION;");
				const WCHAR* query =
					L"UPDATE `gamedb`.`character` SET "
					L"`cristal` = '%d' , "
					L"`hp` = '%d' , "
					L"`die` = '%d' "
					L"WHERE `accountno` = '%lld';";
				dbc->execute(query,
					task->d_int2, task->d_int3, 1, task->AccountNo);
				query =
					L"INSERT INTO `gamedb`.`gamelog`(`type`, `code`, `accountno`, `param1`, `param2`, `param3`) "
					L"VALUES(%d, %d, %lld, %d, %d, %d);";
				dbc->execute(query,
					3, 31, task->AccountNo, tpos.x, tpos.y, task->d_int2);
				dbc->execute(L"COMMIT;");
			}
			break;
			case DBTASKTYPE::RESTART:
			{
				ClientPos cpos = task->d_cpos;
				TilePos tpos = ClientPosToTilePos(cpos);
				dbc->execute(L"START TRANSACTION;");
				const WCHAR* query =
					L"UPDATE `gamedb`.`character` SET "
					L"`posx` = '%f' , "
					L"`posy` = '%f' , "
					L"`tilex` = '%d' , "
					L"`tiley` = '%d' , "
					L"`rotation` = '%d' , "
					L"`die` = '%d' "
					L"WHERE `accountno` = '%lld';";
				dbc->execute(query,
					cpos.x, cpos.y, tpos.x, tpos.y, task->d_int1, 0, task->AccountNo);
				query = 
					L"INSERT INTO `gamedb`.`gamelog`(`type`, `code`, `accountno`, `param1`, `param2`) "
					L"VALUES(%d, %d, %lld, %d, %d);";
				dbc->execute(query,
					3, 33, task->AccountNo, tpos.x, tpos.y);
				dbc->execute(L"COMMIT;");
			}
			break;
			case DBTASKTYPE::GETCRYSTAL:
			{
				dbc->execute(L"START TRANSACTION;");
				const WCHAR* query =
					L"UPDATE `gamedb`.`character` SET "
					L"`cristal` = '%d' "
					L"WHERE `accountno` = '%lld';";
				dbc->execute(query,
					task->d_int2, task->AccountNo);
				query = 
					L"INSERT INTO `gamedb`.`gamelog`(`type`, `code`, `accountno`, `param1`, `param2`) "
					L"VALUES(%d, %d, %lld, %d, %d);";
				dbc->execute(query,
					4, 41, task->AccountNo, task->d_int1, task->d_int2);
				dbc->execute(L"COMMIT;");
			}
			break;
			case DBTASKTYPE::HEAL:
			{
				dbc->execute(L"START TRANSACTION;");
				const WCHAR* query =
					L"UPDATE `gamedb`.`character` SET "
					L"`hp` = '%d' "
					L"WHERE `accountno` = '%lld';";
				dbc->execute(query,
					task->d_int2, task->AccountNo);
				query = 
					L"INSERT INTO `gamedb`.`gamelog`(`type`, `code`, `accountno`, `param1`, `param2`, `param3`) "
					L"VALUES(%d, %d, %lld, %d, %d, %d);";
				dbc->execute(query,
					5, 51, task->AccountNo, task->d_int1, task->d_int2, task->d_int3);
				dbc->execute(L"COMMIT;");
			}
			break;
			default:
				break;
			}
			asyncdb->FreeTask(task);
			InterlockedIncrement(&asyncdb->_UpdateCount);
		}
	}
	return 0;
}

bool AsyncDBUpdater::Start(DBConnMgr* dbmgr) {
	_mgr = dbmgr;

	_hUpdateEv = CreateEvent(NULL, false, false, NULL);
	_hThrHandle = (HANDLE)_beginthreadex(NULL, 0, Thread_AsyncDBExecute, (PVOID)this, 0, NULL);
	return true;
}

bool AsyncDBUpdater::Stop() {
	DBTask* task = AllocTask(DBTASKTYPE::KILL, -1);
	RunTask(task);

	DWORD timeout = 5000;
	DWORD waitret = WaitForSingleObject(_hThrHandle, timeout);
	if (waitret == WAIT_TIMEOUT) {}

	_mgr = nullptr;
	CloseHandle(_hUpdateEv);
	_UpdateCount = 0;
	return true;
}

unsigned long AsyncDBUpdater::GetUpdateCount() { 
	return _UpdateCount; 
}

long AsyncDBUpdater::GetQueueSize() {
	return _TaskQ.getSize();
}

int AsyncDBUpdater::GetTaskPoolCapacity() {
	return _TaskPool.getCapacity();
}

int AsyncDBUpdater::GetTaskPoolUseCount() {
	return _TaskPool.getUseCount();
}


bool AsyncDBUpdater::Write_Logout(INT64 AccountNo, ClientPos cpos, USHORT rotation, int crystal, int hp) {
	DBTask* t = AllocTask(DBTASKTYPE::LOGOUT, AccountNo);
	t->d_cpos = cpos;
	t->d_int1 = rotation;
	t->d_int2 = crystal;
	t->d_int3 = hp;
	RunTask(t);
	return true;
}

bool AsyncDBUpdater::Write_Die(INT64 AccountNo, ClientPos cpos, USHORT rotation, int crystal, int hp) {
	DBTask* t = AllocTask(DBTASKTYPE::DIE, AccountNo);
	t->d_cpos = cpos;
	t->d_int1 = rotation;
	t->d_int2 = crystal;
	t->d_int3 = hp;
	RunTask(t);
	return true;
}

bool AsyncDBUpdater::Write_Restart(INT64 AccountNo, ClientPos cpos, USHORT rotation) {
	DBTask* t = AllocTask(DBTASKTYPE::RESTART, AccountNo);
	t->d_cpos = cpos;
	t->d_int1 = rotation;
	RunTask(t);
	return true;
}

bool AsyncDBUpdater::Write_GetCrystal(INT64 AccountNo, int getCrystal, int totalCrystal) {
	DBTask* t = AllocTask(DBTASKTYPE::GETCRYSTAL, AccountNo);
	t->d_int1 = getCrystal;
	t->d_int2 = totalCrystal;
	RunTask(t);
	return true;
}

bool AsyncDBUpdater::Write_Heal(INT64 AccountNo, int oldhp, int newhp, int sittimesec) {
	DBTask* t = AllocTask(DBTASKTYPE::HEAL, AccountNo);
	t->d_int1 = oldhp;
	t->d_int2 = newhp;
	t->d_int3 = sittimesec;
	RunTask(t);
	return true;
}

void AsyncDBUpdater::RunTask(DBTask* task) {
	_TaskQ.Enqueue(task);
	SetEvent(_hUpdateEv);
	return;
}

DBTask* AsyncDBUpdater::AllocTask(DBTASKTYPE type, INT64 accono) {
	DBTask* t = _TaskPool.Alloc();
	t->Type = type;
	t->AccountNo = accono;
	return t;
}

void AsyncDBUpdater::FreeTask(DBTask* task) {
	_TaskPool.Free(task);
	return;
}
