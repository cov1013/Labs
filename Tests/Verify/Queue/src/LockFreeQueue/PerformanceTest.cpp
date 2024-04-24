#pragma comment(lib, "Winmm")

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <time.h>
#include <locale.h>
#include <process.h>
#include <crtdbg.h>

#define __PROFILING__
#include "Profiler.h"

#include "CCrashDump.h"
#include "CQueue.h"
#include "CLockFreeQueue.h"

using namespace cov1013;

#define dfTHREAD_MAX	1
#define dfDATA_MAX		1000000

struct st_DATA
{
	unsigned __int64 Code;
	unsigned __int64 Count;
};

CCrashDump					g_CrashDump;
CQueue<st_DATA*>			g_Queue;
//CLockFreeQueue<st_DATA*>	g_Queue;
bool						g_bLoop = true;


unsigned int __stdcall WorkerThread(LPVOID lpParam)
{
	int callCount = 0;
	st_DATA** Dataes = new st_DATA * [dfDATA_MAX];

	//--------------------------------------------------------------------------
	// 1. Pop 체크
	//--------------------------------------------------------------------------
	for (int i = 0; i < dfDATA_MAX; i++)
	{
		PRO_BEGIN(L"Dequeue");
		g_Queue.Lock();
		g_Queue.Dequeue(&Dataes[i]);
		g_Queue.Unlock();
		PRO_END(L"Dequeue");
	}

	//--------------------------------------------------------------------------
	// 2. 초기값 체크
	//--------------------------------------------------------------------------
	for (int i = 0; i < dfDATA_MAX; i++)
	{
		if (Dataes[i]->Code != 0x5555555555555555)
		{
			CCrashDump::Crash();
		}
		if (Dataes[i]->Count != 0)
		{
			CCrashDump::Crash();
		}
	}

	//--------------------------------------------------------------------------
	// 3. 데이터 변경
	//--------------------------------------------------------------------------
	for (int i = 0; i < dfDATA_MAX; i++)
	{
		InterlockedIncrement(&Dataes[i]->Code);
		InterlockedIncrement(&Dataes[i]->Count);
	}

	//--------------------------------------------------------------------------
	// 4. 데이터 변경 중 다른곳에서 해당 메모리를 사용했는지 체크
	//--------------------------------------------------------------------------
	for (int i = 0; i < dfDATA_MAX; i++)
	{
		if (Dataes[i]->Code != 0x5555555555555556)
		{
			CCrashDump::Crash();
		}
		if (Dataes[i]->Count != 1)
		{
			CCrashDump::Crash();
		}
	}

	//--------------------------------------------------------------------------
	// 5. 다시 초기값으로 변경
	//--------------------------------------------------------------------------
	for (int i = 0; i < dfDATA_MAX; i++)
	{
		InterlockedDecrement(&Dataes[i]->Code);
		InterlockedDecrement(&Dataes[i]->Count);
	}

	//--------------------------------------------------------------------------
	// 6. 초기값으로 변경 중 다른곳에서 해당 메모리를 사용했는지 체크
	//--------------------------------------------------------------------------
	for (int i = 0; i < dfDATA_MAX; i++)
	{
		if (Dataes[i]->Code != 0x5555555555555555)
		{
			CCrashDump::Crash();
		}
		if (Dataes[i]->Count != 0)
		{
			CCrashDump::Crash();
		}
	}

	//--------------------------------------------------------------------------
	// 7. 반환
	//--------------------------------------------------------------------------
	for (int i = 0; i < dfDATA_MAX; i++)
	{
		PRO_BEGIN(L"Enqueue");
		g_Queue.Lock();
		g_Queue.Enqueue(Dataes[i]);
		g_Queue.Unlock();
		PRO_END(L"Enqueue");
	}

	return 0;
}

void Initial(void)
{
	st_DATA* pData;
	for (int i = 0; i < dfDATA_MAX * dfTHREAD_MAX; i++)
	{
		pData = new st_DATA;
		pData->Code = 0x5555555555555555;
		pData->Count = 0;

		g_Queue.Enqueue(pData);
	}
}

int main(void)
{
	setlocale(LC_ALL, "");
	timeBeginPeriod(1);

	InitializeProfiler(L"../ProfileData", en_PROFILER_UNIT::eUNIT_NANO);

	Initial();

	HANDLE hThreads[dfTHREAD_MAX];
	for (int i = 0; i < dfTHREAD_MAX; i++)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
	}

	if (WaitForMultipleObjects(dfTHREAD_MAX, hThreads, TRUE, INFINITE) == WAIT_FAILED)
	{
		wprintf(L"Workder Thread Exit error:%ld\n", GetLastError());
	}

	OutputProfilingData();
	ReleaseProfiler();
	timeEndPeriod(1);

	return 0;
}