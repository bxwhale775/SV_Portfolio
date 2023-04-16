#include <iostream>
#include "GameServer.h"

int main() {
    std::cout << "Hello GDBug_GameServer!\n";

    GameSVProps props;
    props.Port = 11955;
    props.CocurrentThreadNum = 2;
    props.WorkerThreadNum = 4;
    props.MaxSessionNum = 6000;
	props.PK_Size = 300;
	props.LogFileName = L"GDBug_GameServer_Log.txt";

	props.LoginDB_HostName = "tcp://192.168.0.2:3306/accountdb";
	props.LoginDB_UserName = "root";
	props.LoginDB_Password = "1111";

	props.GameDB_HostName = "tcp://192.168.0.2:3306/gamedb";
	props.GameDB_UserName = "root";
	props.GameDB_Password = "1111";

	props.ActiveSectorRange = 3;

	props.Monster2Num = 100;

    GameServer gsv;
    gsv.Init(props);
    gsv.Start();

	DWORD prevtime = timeGetTime();
	while (true) {
		DWORD ctime = timeGetTime();
		if (ctime - prevtime >= 1000) {
			gsv.PrintLog();
			prevtime = ctime;
		}
		Sleep(50);
		if (GetAsyncKeyState('Q')) {
			printf("stopping...\n");
			gsv.Stop();
			break;
		}
	}

    return 0;
}