#pragma once
#pragma comment (lib, "Dbghelp.lib")
#include <crtdbg.h>
#include <direct.h>
#include <Dbghelp.h>
#include <psapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#define CRASH() cov1013::CCrashDump::Crash()

namespace cov1013
{
	class CCrashDump
	{
	public:
		CCrashDump()
		{
			_DumpCount = 0;

			_invalid_parameter_handler oldHandler, newHandler;
			newHandler = myInvalidParameterHandler;

			oldHandler = _set_invalid_parameter_handler(newHandler);	// crt함수에 null 포인터 등을 넣었을 때
			_CrtSetReportMode(_CRT_WARN, 0);							// CRT 오류 메시지 표시 중단. 바로 덤프로 남도록.
			_CrtSetReportMode(_CRT_ASSERT, 0);							// CRT 오류 메시지 표시 중단. 바로 덤프로 남도록.
			_CrtSetReportMode(_CRT_ERROR, 0);							// CRT 오류 메시지 표시 중단. 바로 덤프로 남도록.

			_CrtSetReportHook(_custom_Report_hook);

			//-----------------------------------------------------------------------
			// pure virtual function called 에러 핸들러를 사용자 정의 함수로 우회시킨다.
			//-----------------------------------------------------------------------
			_set_purecall_handler(myPurecallHandler);

			SetHandlerDump();
		};

		~CCrashDump()
		{

		}

		static void Crash(void)
		{
			*((int*)(0x00000000)) = 0;
		}

		static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
		{
			int iResult = _mkdir("../Dump");

			int iWorkingMemory = 0;
			SYSTEMTIME stNowTime;

			long DumpCount = InterlockedIncrement(&_DumpCount);

			//----------------------------------------------------------
			// 현재 프로세스의 메모리 사용량을 얻어온다.
			//----------------------------------------------------------
			HANDLE hProcess = 0;
			PROCESS_MEMORY_COUNTERS pmc;

			hProcess = GetCurrentProcess();

			if (NULL == hProcess)
				return 0;

			if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
			{
				iWorkingMemory = (int)(pmc.WorkingSetSize / 1024 / 1024);
			}
			CloseHandle(hProcess);

			//----------------------------------------------------------
			// 현재 날짜와 시간을 알아온다.
			//----------------------------------------------------------
			WCHAR filename[MAX_PATH];

			GetLocalTime(&stNowTime);
			wsprintf(
				filename,
				L"../Dump/Dump_%d%02d%02d_%02d.%02d.%02d_%d_%dMB.dmp"
				, stNowTime.wYear
				, stNowTime.wMonth
				, stNowTime.wDay
				, stNowTime.wHour
				, stNowTime.wMinute
				, stNowTime.wSecond
				, DumpCount
				, iWorkingMemory
			);
			wprintf(L"\n\n\n!!! Crash Error!!! %d.%d.%d / %d:%d:%d \n"
				, stNowTime.wYear
				, stNowTime.wMonth
				, stNowTime.wDay
				, stNowTime.wHour
				, stNowTime.wMinute
				, stNowTime.wSecond
			);
			wprintf(L"Now Save dump file...\n");

			HANDLE hDumpFile = ::CreateFileW(
				filename,
				GENERIC_WRITE,				// 프로세스 내에서 파일을 읽을 일이 없다.
				FILE_SHARE_WRITE,			// 다른 프로세스에서 이 파일에 동시 쓰기 접근 가능
				NULL,
				CREATE_ALWAYS,				// 항상 새로운 파일 생성
				FILE_ATTRIBUTE_NORMAL,		// 보통 파일로 지정
				NULL
			);

			if (hDumpFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInformation;

				MinidumpExceptionInformation.ThreadId = ::GetCurrentThreadId();
				MinidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
				MinidumpExceptionInformation.ClientPointers = TRUE;

				MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithFullMemory, &MinidumpExceptionInformation, NULL, NULL);
				CloseHandle(hDumpFile);
				wprintf(L"CrashDump Svae Finish!\n");
			}

			return EXCEPTION_EXECUTE_HANDLER;
		}

		static void SetHandlerDump(void)
		{
			SetUnhandledExceptionFilter(MyExceptionFilter);
		}

		static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int	line, uintptr_t	pReserved)
		{
			Crash();
		}

		static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue)
		{
			Crash();
			return true;
		}

		static void myPurecallHandler(void)
		{
			Crash();
		}

		static long _DumpCount;
	};
}

long cov1013::CCrashDump::_DumpCount = 0;