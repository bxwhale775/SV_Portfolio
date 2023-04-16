#pragma once
#include <Windows.h>

class ElapsedTimeChecker {
private:
	DWORD _Time = 0;
public:
	ElapsedTimeChecker() { Reset(); }
	void Reset() {
		_Time = timeGetTime();
		return;
	}
	bool CheckElapsed(DWORD ms, bool noreset=false) {
		DWORD nowtime = timeGetTime();
		bool isElapsed = (nowtime - _Time) >= ms;
		if (isElapsed && !noreset) { Reset(); }
		return isElapsed;
	}
};