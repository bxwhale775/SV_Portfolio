#include "MonitorServer.h"
#include "MonitorClientMgr.h"
#include "PDHCounter.h"

UINT WINAPI Thread_ThrSvStat(PVOID param) {
	MonitorServer* msv = (MonitorServer*)param;
	MonitorClientMgr cli;
	PDHCounter pdh;

	cli.Init(L"127.0.0.1", msv->_props.SV_Port, 0);
	cli.Start();
	cli.LoginToMonitor();

	pdh.Init();

	while (msv->_isStarted) {
		cli.SendData(dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, (int)pdh.GetSysCpuTime());
		cli.SendData(dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, (int)pdh.ToMiB(pdh.GetSysNonPagedPool()));
		cli.SendData(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, (int)pdh.ToKiB(pdh.GetNetworkRecv()));
		cli.SendData(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, (int)pdh.ToKiB(pdh.GetNetworkSend()));
		cli.SendData(dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, (int)pdh.GetSysAvailableMem());
		Sleep(1000);
	}
	cli.Disconnect();

	return 0;
}

MonitorServer::~MonitorServer() {
	if (_pmlog != nullptr) {
		delete _pmlog;
		return;
	}
}

void MonitorServer::Init(MonitorServer_Properties mtsvprops) {
	//init server sv
	CNetServer_Properties svprops;
	svprops.Port = mtsvprops.SV_Port;
	svprops.CocurrentThreadNum = 1;
	svprops.WorkerThreadNum = 1;
	svprops.MaxSessionNum = 200;
	svprops.disableEncrypt = true;
	_svcbk.msv = this;
	_sv_sv.Init(svprops, &_svcbk);


	//init client sv
	CNetServer_Properties clprops;
	clprops.Port = mtsvprops.CL_Port;
	clprops.CocurrentThreadNum = 1;
	clprops.WorkerThreadNum = 1;
	clprops.MaxSessionNum = 200;
	clprops.PacketCode = 109;
	clprops.EncryptKey = 30;
	_clcbk.msv = this;
	_sv_cl.Init(clprops, &_clcbk);

	//init db
	_db.Init(mtsvprops.DB_HostName, mtsvprops.DB_UserName, mtsvprops.DB_Password);

	//init CLMap lock
	InitializeSRWLock(&ClMapLock);

	//init MonitorLogger
	if (_pmlog == nullptr) {
		_pmlog = new Logger(_props.MonitorSV_LoggerName);
	}

	return;
}

void MonitorServer::Start() {
	//start server
	_sv_sv.Start();
	_sv_cl.Start();

	//start serverstat thread
	_isStarted = true;
	_hThrSvStat = (HANDLE)_beginthreadex(NULL, 0, Thread_ThrSvStat, (PVOID)this, 0, NULL);

	return;
}

void MonitorServer::Stop() {
	//stop serverstat thread
	_isStarted = false;
	DWORD timeout = 10000;		//10sec
	DWORD ret;
	ret = WaitForMultipleObjects(1, &_hThrSvStat, true, timeout);
	if (ret == WAIT_TIMEOUT) {
		printf("Thread_ThrSvStat Timed out to exit.\n");
	}

	//stop server
	_sv_sv.Stop();
	_sv_cl.Stop();
	return;
}
