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
#include "CStack.h"
#include "CLockFreeStack.h"

using namespace cov1013;

#define dfTHREAD_MAX	6
#define dfDATA_MAX		1000000

struct st_DATA
{
	unsigned __int64 Code;
	unsigned __int64 Count;
};

CCrashDump					g_CrashDump;
//CStack<int>				g_Stack;
CLockFreeStack<int>			g_Stack;
bool						g_bLoop = true;

long						g_TPS;


unsigned int __stdcall WorkerThread(LPVOID lpParam)
{
	int callCount = 0;
	st_DATA** Dataes = new st_DATA*[dfDATA_MAX];

	int a;

	while (true)
	{
		//g_Stack.Lock();
		g_Stack.Push(10);
		g_Stack.Pop(&a);
		//g_Stack.Unlock();
		InterlockedIncrement(&g_TPS);
	}

	////--------------------------------------------------------------------------
	//// 1. Pop 체크
	////--------------------------------------------------------------------------
	//for (int i = 0; i < dfDATA_MAX; i++)
	//{
	//	PRO_BEGIN(L"Pop");
	//	//g_Stack.Lock();
	//	g_Stack.Pop(&Dataes[i]);
	//	//g_Stack.Unlock();
	//	PRO_END(L"Pop");
	//}

	////--------------------------------------------------------------------------
	//// 2. 초기값 체크
	////--------------------------------------------------------------------------
	//for (int i = 0; i < dfDATA_MAX; i++)
	//{
	//	if (Dataes[i]->Code != 0x5555555555555555)
	//	{
	//		CCrashDump::Crash();
	//	}
	//	if (Dataes[i]->Count != 0)
	//	{
	//		CCrashDump::Crash();
	//	}
	//}

	////--------------------------------------------------------------------------
	//// 3. 데이터 변경
	////--------------------------------------------------------------------------
	//for (int i = 0; i < dfDATA_MAX; i++)
	//{
	//	InterlockedIncrement(&Dataes[i]->Code);
	//	InterlockedIncrement(&Dataes[i]->Count);
	//}

	////--------------------------------------------------------------------------
	//// 4. 데이터 변경 중 다른곳에서 해당 메모리를 사용했는지 체크
	////--------------------------------------------------------------------------
	//for (int i = 0; i < dfDATA_MAX; i++)
	//{
	//	if (Dataes[i]->Code != 0x5555555555555556)
	//	{
	//		CCrashDump::Crash();
	//	}
	//	if (Dataes[i]->Count != 1)
	//	{
	//		CCrashDump::Crash();
	//	}
	//}

	////--------------------------------------------------------------------------
	//// 5. 다시 초기값으로 변경
	////--------------------------------------------------------------------------
	//for (int i = 0; i < dfDATA_MAX; i++)
	//{
	//	InterlockedDecrement(&Dataes[i]->Code);
	//	InterlockedDecrement(&Dataes[i]->Count);
	//}

	////--------------------------------------------------------------------------
	//// 6. 초기값으로 변경 중 다른곳에서 해당 메모리를 사용했는지 체크
	////--------------------------------------------------------------------------
	//for (int i = 0; i < dfDATA_MAX; i++)
	//{
	//	if (Dataes[i]->Code != 0x5555555555555555)
	//	{
	//		CCrashDump::Crash();
	//	}
	//	if (Dataes[i]->Count != 0)
	//	{
	//		CCrashDump::Crash();
	//	}
	//}

	////--------------------------------------------------------------------------
	//// 7. 반환
	////--------------------------------------------------------------------------
	//for (int i = 0; i < dfDATA_MAX; i++)
	//{
	//	PRO_BEGIN(L"Push");
	//	//g_Stack.Lock();
	//	g_Stack.Push(Dataes[i]);
	//	//g_Stack.Unlock();
	//	PRO_END(L"Push");
	//}

	return 0;
}

void Initial(void)
{
	//st_DATA* pData;
	//for (int i = 0; i < dfDATA_MAX * dfTHREAD_MAX; i++)
	//{
	//	pData = new st_DATA;
	//	pData->Code = 0x5555555555555555;
	//	pData->Count = 0;

	//	g_Stack.Push(pData);
	//}
}

int main(void)
{
	setlocale(LC_ALL, "");
	timeBeginPeriod(1);

	InitializeProfiler(L"../ProfileData", en_PROFILER_UNIT::eUNIT_NANO);

	//Initial();

	HANDLE hThreads[dfTHREAD_MAX];
	for (int i = 0; i < dfTHREAD_MAX; i++)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
	}

	while (true)
	{
		wprintf(L"TPS : %d\n", g_TPS);
		g_TPS = 0;
		Sleep(999);
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