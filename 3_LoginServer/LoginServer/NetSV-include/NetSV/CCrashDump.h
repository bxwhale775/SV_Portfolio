#pragma once
#include <iostream>
#include <Windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>
#include <Psapi.h>
#pragma comment(lib, "DbgHelp.Lib")

namespace Procademy {
	class CCrashDump {
	public:
		CCrashDump() {
			_DumpCount = 0;

			_invalid_parameter_handler oldHandler;
			_invalid_parameter_handler newHandler;
			newHandler = myInvalidParameterHandler;

			oldHandler = _set_invalid_parameter_handler(newHandler);  // CRT �Լ��� null ������ ���� �־��� ��
			_CrtSetReportMode(_CRT_WARN, 0);    // CRT ���� �޽��� ǥ�� �ߴ�. �ٷ� ������ ������.
			_CrtSetReportMode(_CRT_ASSERT, 0);  // CRT ���� �޽��� ǥ�� �ߴ�. �ٷ� ������ ������.
			_CrtSetReportMode(_CRT_ERROR, 0);   // CRT ���� �޽��� ǥ�� �ߴ�. �ٷ� ������ ������.

			_CrtSetReportHook(_custom_Report_hook);

			// pure virtual function called ���� �ڵ鷯�� ����� ���� �Լ��� ��ȸ��Ų��.
			_set_purecall_handler(myPurecallHandler);

			SetHandlerDump();
		}
#pragma warning( push )
#pragma warning( disable : 6011)	//ignoring 'deref nullptr error' - Intended crash point.
		static void Crash() {
			int* p = nullptr;
			*p = 1234;
		}
#pragma warning( pop ) 

		static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer) {
			int iWorkingMemory = 0;
			SYSTEMTIME stNowTime;

			long DumpCount = InterlockedIncrement(&_DumpCount);

			// ���� ��¥�� �ð��� �˾ƿ´�.
			WCHAR filename[MAX_PATH];

			GetLocalTime(&stNowTime);
			wsprintf(filename, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d.dmp",
				stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, DumpCount);

			// �޸� ���� ������ �����Ѵ�.
			wprintf(L"\n\n\n!!! Crash Error !!! %d.%d.%d / %d:%d:%d \n",
				stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);
			wprintf(L"Now Save dump file...\n");

			HANDLE hDumpFile = ::CreateFile(filename,
				GENERIC_WRITE,
				FILE_SHARE_WRITE,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			if (hDumpFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInformation;
				MinidumpExceptionInformation.ThreadId = ::GetCurrentThreadId();
				MinidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
				MinidumpExceptionInformation.ClientPointers = TRUE;

				// Writes user-mode minidump information to the specified file.
				MiniDumpWriteDump(GetCurrentProcess(),
					GetCurrentProcessId(),
					hDumpFile,
					MiniDumpWithFullMemory,
					&MinidumpExceptionInformation,   // If the value of this parameter is NULL, no exception information is included in the minidump file.
					NULL,  // If the value of this parameter is NULL, no user-defined information is included in the minidump file.
					NULL); // If the value of this parameter is NULL, no callbacks are performed.

				CloseHandle(hDumpFile);

				wprintf(L"CrashDump Save Finished!");
			}

			return EXCEPTION_EXECUTE_HANDLER;
		}

		static void SetHandlerDump() {
			SetUnhandledExceptionFilter(MyExceptionFilter);
		}

		// Invalid Parameter handler
		static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved) {
			Crash();
		}
		static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue) {
			Crash();
			return true;
		}
		static void myPurecallHandler(void) {
			Crash();
		}
		static long _DumpCount;
	};
}

