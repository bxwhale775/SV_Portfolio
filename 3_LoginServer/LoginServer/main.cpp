#include <iostream>
#include "LoginServer.h"
#include "NetSV-include/NetSV/CCrashDump.h"

Procademy::CCrashDump dumper;

int main() {
	std::cout << "Hello GDBug_LoginServer!!\n";

	LoginServer_Properties props;
	props.PK_Size = 200;
	props.Port = 11950;
	props.CocurrentThreadNum = 2;
	props.WorkerThreadNum = 2;
	props.MaxSessionNum = 17000;
	props.LogFileName = L"GDBug_LoginServer_Log.txt";

	props.DB_HostName = "tcp://192.168.0.2:3306/accountdb";
	props.DB_UserName = "root";
	props.DB_Password = "1111";

	props.GameDB_HostName = "tcp://192.168.0.2:3306/gamedb";
	props.GameDB_UserName = "root";
	props.GameDB_Password = "1111";

	props.MyDomain_Name = L"example.com";
	props.ChatServer_Port = 11956;
	props.GameServer_Port = 11955;

	LoginServer lsv;
	lsv.Init(props);
	lsv.Start();

	DWORD prevtime = timeGetTime();
	while (true) {
		DWORD ctime = timeGetTime();
		if (ctime - prevtime >= 1000) {
			lsv.PrintLog();
			prevtime = ctime;
		}
		Sleep(50);
		if (GetAsyncKeyState('Q')) {
			printf("stopping...\n");
			lsv.Stop();
			break;
		}
	}


	return 0;
}
