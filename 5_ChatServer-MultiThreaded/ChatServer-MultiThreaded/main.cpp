#include <iostream>
#include "ChatServer.h"

int main() {
	std::cout << "Hello ChatServer!_MT\n";

	ChatServer_Properties props;
	props.CocurrentThreadNum = 3;
	props.WorkerThreadNum = 12;
	props.MaxSessionNum = 25000;
	props.MaxUserNum = 21000;
	props.MonitorSV_IP = L"127.0.0.1";
	props.MonitorSV_Port = 12300;


	ChatServer csv;
	csv.Init(props);
	csv.Start();

	DWORD prevtime = timeGetTime();
	while (true) {
		DWORD ctime = timeGetTime();
		if (ctime - prevtime >= 1000) {
			csv.PrintLog();
			prevtime = ctime;
		}
		Sleep(50);
		if (GetAsyncKeyState('Q')) {
			printf("stopping...\n");
			csv.Stop();
			//break;
		}
	}

	return 0;
}