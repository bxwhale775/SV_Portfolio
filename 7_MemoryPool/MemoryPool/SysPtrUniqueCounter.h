#pragma once
#include <windows.h>

class SysPtrUniqueCounter {
private:
	unsigned int _available_bit = 0;
	unsigned int _unavailable_bit = 0;
	ULONG64 _Normalizer = ULLONG_MAX;
public:
	SysPtrUniqueCounter(int gap=0) {
		//get available bit for pointer;
		SYSTEM_INFO SystemInfo;
		GetSystemInfo(&SystemInfo);
		ULONG64 MaxAddr = (ULONG64)(SystemInfo.lpMaximumApplicationAddress);
		ULONG64 AndComp = 0x8000000000000000u;
		for (;;) {
			if ((MaxAddr & AndComp) == 0) { 
				_available_bit++; 
				AndComp >>= 1;
			}
			else { break; }
		}
		_available_bit -= gap;
		//set how much shift _count var
		_unavailable_bit = 64 - _available_bit;

		//set _normalizer var
		_Normalizer <<= _available_bit;
		_Normalizer >>= _available_bit;
		return;
	}

	PVOID makePtrUnique(PVOID ptr, ULONG64 idx) {
		return (PVOID)((ULONG64)ptr | (idx << _unavailable_bit));
	}

	PVOID makeNormalPtr(PVOID ptr) {
		return (PVOID)((ULONG64)ptr & _Normalizer);
	}

	ULONG64 extractIdx(PVOID ptr) {
		return (ULONG64)ptr >> _unavailable_bit;
	}
};

