#include <iostream>
#include "ChatServer.h"
#include "NetSV-include/NetSV/CCrashDump.h"
#pragma comment(lib, "winmm")

Procademy::CCrashDump cdump;

int main() {
	std::cout << "Hello ChatServer!\n";

	ChatServer_Properties props;
	props.CocurrentThreadNum = 1;
	props.WorkerThreadNum = 1;
	props.MaxSessionNum = 17000;
	props.MaxUserNum = 16000;
	props.Port = 11956;
	props.PK_Size = 300;

	ChatServer* csv = new ChatServer;
	csv->Init(props);
	csv->Start();

	DWORD prevtime = timeGetTime();
	while (true) {
		DWORD ctime = timeGetTime();
		if (ctime - prevtime >= 1000) {
			csv->PrintLog();
			prevtime = ctime;
		}
		Sleep(50);
		if (GetAsyncKeyState('Q')) {
			printf("stopping...\n");
			csv->Stop();
			break;
		}
	}
	delete csv;

	return 0;
}
