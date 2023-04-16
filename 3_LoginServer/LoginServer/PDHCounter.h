#pragma once
#include <string>
#include <Windows.h>
#include <Pdh.h>
#include <Shlwapi.h>
#include <strsafe.h>
#pragma comment(lib,"Pdh.lib")
#pragma comment(lib, "Shlwapi.lib")

#define df_PDH_ETHERNET_MAX 8

enum class enCpuUtilType {
	en_CpuUtil_Total,
	en_CpuUtil_Kernel,
	en_CpuUtil_User,
};

struct st_ETHERNET {
	bool           _bUse;
	WCHAR          _szName[128];
	PDH_HCOUNTER   _pdh_Counter_Network_RecvBytes;
	PDH_HCOUNTER   _pdh_Counter_Network_SendBytes;
};

class PDHCounter {
private:
	//mem & global var
	PDH_HQUERY _QsysMNPP;
	PDH_HQUERY _QsysMAVM;
	PDH_HQUERY _QproMPB;

	PDH_HCOUNTER _CsysMNPP;
	PDH_HCOUNTER _CsysMAVM;
	PDH_HCOUNTER _CproMPB;

	std::wstring _CproMPBStr = L"\\Process(";

	bool _isInited = false;
	//mem & global var/
	
	//cpu var
	HANDLE  _hProcess;
	int     _iNumberOfProcessors;

	float   _fProcessorTotal;
	float   _fProcessorUser;
	float   _fProcessorKernel;

	float   _fProcessTotal;
	float   _fProcessUser;
	float   _fProcessKernel;

	ULARGE_INTEGER  _ftProcessor_LastKernel;
	ULARGE_INTEGER  _ftProcessor_LastUser;
	ULARGE_INTEGER  _ftProcessor_LastIdle;

	ULARGE_INTEGER  _ftProcess_LastKernel;
	ULARGE_INTEGER  _ftProcess_LastUser;
	ULARGE_INTEGER  _ftProcess_LastTime;
	//cpu var/

	//net var
	st_ETHERNET _EthernetStruct[df_PDH_ETHERNET_MAX] = { 0, };   // 랜카드 별 PDH 정보
	double _pdh_value_Network_RecvBytes = 0.0;      // 총 Recv Bytes - 모든 이더넷의 Recv 수치 합산
	double _pdh_value_Network_SendBytes = 0.0;      // 총 Send Bytes - 모든 이더넷의 Send 수치 합산
	PDH_HQUERY _QNetRecv;
	PDH_HQUERY _QNetSend;

	int iCnt = 0;
	bool bErr = false;
	WCHAR* szCur = NULL;
	WCHAR* szCounters = NULL;
	WCHAR* szInterfaces = NULL;
	DWORD dwCounterSize = 0, dwInterfaceSize = 0;
	WCHAR szQuery[1024] = { 0, };
	//net var/


public:
	//PDHCounter();
	~PDHCounter() {
		if (szCounters != NULL) { delete[] szCounters; }
		if (szInterfaces != NULL) { delete[] szInterfaces; }
		return;
	}

	bool Init() {
		//CPU////////
		//get my process handles
		_hProcess = GetCurrentProcess();

		//get cpu cores
		SYSTEM_INFO SystemInfo;
		GetSystemInfo(&SystemInfo);
		_iNumberOfProcessors = SystemInfo.dwNumberOfProcessors;

		//init cpu vars
		_fProcessorTotal = 0;
		_fProcessorUser = 0;
		_fProcessorKernel = 0;

		_fProcessTotal = 0;
		_fProcessUser = 0;
		_fProcessKernel = 0;

		_ftProcessor_LastKernel.QuadPart = 0;
		_ftProcessor_LastUser.QuadPart = 0;
		_ftProcessor_LastIdle.QuadPart = 0;

		_ftProcess_LastUser.QuadPart = 0;
		_ftProcess_LastKernel.QuadPart = 0;
		_ftProcess_LastTime.QuadPart = 0;

		UpdateSysCpuTime();
		UpdateProCpuTime();
		//CPU////////

		//NET/////////
		// PDH enum Object를 사용하는 방법.
		// 모든 이더넷 이름이 나오지만 실제 사용중인 (가상)이더넷 등등을 확인불가함.

		// PdhEnumObjectItems를 통해서 "Network Interface" 항목에서 얻을 수 있는
		// 측정항목(Counters) / 인터페이스 항목(Interfaces)을 얻음.
		// 그런데 그 개수나 길이를 모르기 때문에 
		// 먼저 버퍼의 길이를 알기 위해서 Out Buffer 인자들을 NULL 포인터로 넣어서 
		// 사이즈만 확인한다.
		PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);

		szCounters = new WCHAR[dwCounterSize];
		szInterfaces = new WCHAR[dwInterfaceSize];

		// 버퍼의 동적할당 후 다시 호출!
		//
		// szCounters와 szInterfaces 버퍼에는 여러개의 문자열이 쭉쭉쭉 들어온다.
		// 2차원 배열도 아니고 - 그냥 NULL로 끝나는 문자열들이 
		// dwCounterSize, dwInterfaceSize 길이만큼 줄줄이 들어있음.
		// 이를 문자열 단위로 끊어서 개수를 확인해야함. aaa\0bbb\0ccc\0ddd <-이딴 식
		if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS) {
			delete[] szCounters;
			delete[] szInterfaces;
			return false;
		}

		iCnt = 0;
		szCur = szInterfaces;

		// szInterfaces에서 문자열 단위로 끊으면서, 이름을 복사받는다.
		PdhOpenQuery(NULL, NULL, &_QNetRecv);
		PdhOpenQuery(NULL, NULL, &_QNetSend);
		for (; *szCur != L'\0' && iCnt < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, iCnt++) {
			_EthernetStruct[iCnt]._bUse = true;
			_EthernetStruct[iCnt]._szName[0] = L'\0';

			wcscpy_s(_EthernetStruct[iCnt]._szName, szCur);

			szQuery[0] = L'\0';
			StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);
			PdhAddCounter(_QNetRecv, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes);

			szQuery[0] = L'\0';
			StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);
			PdhAddCounter(_QNetSend, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes);
		}
		UpdateNetRecvStats();
		UpdateNetSendStats();
		//NET/////////
		
		//MEM/////////////
		//complete PDH query string for process object
		_CproMPBStr.append(GetProcessName());
		_CproMPBStr.append(L")\\Private Bytes");

		//init PDHQuery
		PdhOpenQuery(NULL, NULL, &_QsysMNPP);
		PdhOpenQuery(NULL, NULL, &_QsysMAVM);
		PdhOpenQuery(NULL, NULL, &_QproMPB);

		// add counter to query
		PdhAddCounter(_QsysMNPP, L"\\Memory\\Pool Nonpaged Bytes", NULL, &_CsysMNPP);
		PdhAddCounter(_QsysMAVM, L"\\Memory\\Available MBytes", NULL, &_CsysMAVM);
		PdhAddCounter(_QproMPB, _CproMPBStr.c_str(), NULL, &_CproMPB);

		//first update
		PdhCollectQueryData(_QsysMNPP);
		PdhCollectQueryData(_QsysMAVM);
		PdhCollectQueryData(_QproMPB);
		//MEM/////////////

		_isInited = true;

		return true;
	}

	float GetSysCpuTime(enCpuUtilType type = enCpuUtilType::en_CpuUtil_Total) {
		if (!_isInited) { return 0.0f; }
		UpdateSysCpuTime();
		switch (type) {
		case enCpuUtilType::en_CpuUtil_Total:
			return _fProcessorTotal;
			break;
		case enCpuUtilType::en_CpuUtil_Kernel:
			return _fProcessorKernel;
			break;
		case enCpuUtilType::en_CpuUtil_User:
			return _fProcessorUser;
			break;
		default:
			break;
		}
		return 0.0f;
	}

	float GetProcessCpuTime(enCpuUtilType type = enCpuUtilType::en_CpuUtil_Total) {
		if (!_isInited) { return 0.0f; }
		UpdateProCpuTime();
		switch (type) {
		case enCpuUtilType::en_CpuUtil_Total:
			return _fProcessTotal;
			break;
		case enCpuUtilType::en_CpuUtil_Kernel:
			return _fProcessKernel;
			break;
		case enCpuUtilType::en_CpuUtil_User:
			return _fProcessUser;
			break;
		default:
			break;
		}
		return 0.0f;
	}

	LONGLONG GetSysNonPagedPool() {
		if (!_isInited) { return 0; }
		PdhCollectQueryData(_QsysMNPP);
		PDH_FMT_COUNTERVALUE val;
		PdhGetFormattedCounterValue(_CsysMNPP, PDH_FMT_LARGE, NULL, &val);
		return val.largeValue;
	}

	LONGLONG GetSysAvailableMem() {
		if (!_isInited) { return 0; }
		PdhCollectQueryData(_QsysMAVM);
		PDH_FMT_COUNTERVALUE val;
		PdhGetFormattedCounterValue(_CsysMAVM, PDH_FMT_LARGE, NULL, &val);
		return val.largeValue;
	}

	LONGLONG GetProcessPrivateBytes() {
		if (!_isInited) { return 0; }
		PdhCollectQueryData(_QproMPB);
		PDH_FMT_COUNTERVALUE val;
		PdhGetFormattedCounterValue(_CproMPB, PDH_FMT_LARGE, NULL, &val);
		return val.largeValue;
	}

	double GetNetworkRecv() {
		UpdateNetRecvStats();
		return _pdh_value_Network_RecvBytes;
	}

	double GetNetworkSend() {
		UpdateNetSendStats();
		return _pdh_value_Network_SendBytes;
	}

	static double ToMiB(LONGLONG bytes) {
		return ((double)bytes / 1048576.0);
	}
	static double ToMiB(double bytes) {
		return ((double)bytes / 1048576.0);
	}
	static double ToKiB(double bytes) {
		return ((double)bytes / 1024.0);
	}
private:
	void UpdateSysCpuTime() {
		// 프로세서 사용률을 갱신한다.
		// 
		// 본래의 사용 구조체는 FILETIME이지만 ULARGE_INTEGER와 구조가 같으므로 이를 사용함.
		// FILETIME 구조체는 100ns 단위의 시간 단위를 표현하는 구조체임.
		ULARGE_INTEGER Idle;
		ULARGE_INTEGER Kernel;
		ULARGE_INTEGER User;


		// 시스템 사용 시간을 구한다.
		// 아이들 타임 / 커널 사용 타임 (아이들 포함) / 유저 사용 타임
		if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false) {
			return;
		}

		// 커널 타임에는 아이들 타임이 포함됨.
		ULONGLONG KernelDiff = Kernel.QuadPart - _ftProcessor_LastKernel.QuadPart;
		ULONGLONG UserDiff = User.QuadPart - _ftProcessor_LastUser.QuadPart;
		ULONGLONG IdleDiff = Idle.QuadPart - _ftProcessor_LastIdle.QuadPart;

		ULONGLONG Total = KernelDiff + UserDiff;

		if (Total == 0) {
			_fProcessorUser = 0.0f;
			_fProcessorKernel = 0.0f;
			_fProcessorTotal = 0.0f;
		}
		else {
			// 커널 타임에 아이들 타임이 있으므로 빼서 계산.
			_fProcessorTotal = (float)((double)(Total - IdleDiff) / Total * 100.0f);
			_fProcessorUser = (float)((double)UserDiff / Total * 100.0f);
			_fProcessorKernel = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
		}

		_ftProcessor_LastKernel = Kernel;
		_ftProcessor_LastUser = User;
		_ftProcessor_LastIdle = Idle;

		return;
	}
	
	void UpdateProCpuTime() {
		// 지정된 프로세스 사용률을 갱신한다.
		ULARGE_INTEGER Kernel;
		ULARGE_INTEGER User;
		ULARGE_INTEGER None;
		ULARGE_INTEGER NowTime;
		ULONGLONG KernelDiff;
		ULONGLONG UserDiff;
		ULONGLONG TimeDiff;


		// 현재의 100ns단위 시간을 구한다 - UTC 시간
		//
		// 프로세스 사용률 판단공식:
		// a = 샘플간격의 시스템 시간을 구함. (그냥 실제로 지나간 시간)
		// b = 프로세스의 CPU 사용 시간을 구함.
		// a : 100 = b : 사용률  <-공식으로 사용률을 구함.

		// 얼마의 시간이 지났는지 100나노세컨드 시간을 구함.
		GetSystemTimeAsFileTime((LPFILETIME)&NowTime);


		// 해당 프로세스가 사용한 시간을 구함.
		// 두번째, 세번째는 실행, 종료 시간으로 미사용.
		GetProcessTimes(_hProcess, (LPFILETIME)&None, (LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);


		// 이전에 저장된 프로세스 시간과의 차를 구해서 실제로 얼마의 시간이 지났는지 확인.
		// 그리고 실제 지나온 시간으로 나누면 사용률이 나옴.
		TimeDiff = NowTime.QuadPart - _ftProcess_LastTime.QuadPart;
		UserDiff = User.QuadPart - _ftProcess_LastUser.QuadPart;
		KernelDiff = Kernel.QuadPart - _ftProcess_LastKernel.QuadPart;
		
		ULONGLONG Total = KernelDiff + UserDiff;
		if (Total == 0) {
			_fProcessorUser = 0.0f;
			_fProcessorKernel = 0.0f;
			_fProcessorTotal = 0.0f;
		}
		else {
			_fProcessTotal = (float)(Total / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
			_fProcessKernel = (float)(KernelDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
			_fProcessUser = (float)(UserDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
		}

		_ftProcess_LastTime = NowTime;
		_ftProcess_LastKernel = Kernel;
		_ftProcess_LastUser = User;

		return;
	}
	
	void UpdateNetRecvStats() {
		PdhCollectQueryData(_QNetRecv);
		_pdh_value_Network_RecvBytes = 0.0;
		PDH_FMT_COUNTERVALUE CounterValue;
		PDH_STATUS Status;
		for (int iCnt = 0; iCnt < df_PDH_ETHERNET_MAX; iCnt++) {
			if (_EthernetStruct[iCnt]._bUse) {
				Status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes, PDH_FMT_DOUBLE, NULL, &CounterValue);
				if (Status == 0) {
					_pdh_value_Network_RecvBytes += CounterValue.doubleValue;
				}
			}
		}
	}

	void UpdateNetSendStats() {
		PdhCollectQueryData(_QNetSend);
		_pdh_value_Network_SendBytes = 0.0;
		PDH_FMT_COUNTERVALUE CounterValue;
		PDH_STATUS Status;
		for (int iCnt = 0; iCnt < df_PDH_ETHERNET_MAX; iCnt++) {
			if (_EthernetStruct[iCnt]._bUse) {
				Status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes, PDH_FMT_DOUBLE, NULL, &CounterValue);
				if (Status == 0) {
					_pdh_value_Network_SendBytes += CounterValue.doubleValue;
				}
			}
		}
	}

	std::wstring GetProcessName() {
		std::wstring ret;
		WCHAR buf[MAX_PATH];
		GetModuleFileName(NULL, buf, MAX_PATH);
		WCHAR* filename = PathFindFileName(buf);
		PathRemoveExtension(filename);
		ret.append(filename);
		return ret;
	}
};