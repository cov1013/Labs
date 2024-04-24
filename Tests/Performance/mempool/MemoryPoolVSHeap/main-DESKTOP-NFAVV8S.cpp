#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <conio.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>

#include "ProFiler.h"
#include "CMemoryPool_TLS.h"

#define dfTHREAD_MAX	4
#define dfMAX			1000

struct st_TEST
{
	char Data[1024 * 256];
};

CMemPool_TLS<st_TEST> g_MemPool = CMemPool_TLS<st_TEST>(0);

unsigned int __stdcall WorkerThread(void* lpParam)
{
	int iCall = 1;
	st_TEST* g_Test[dfMAX];

	while (iCall)
	{
		//-------------------------------------------------------
		// Heap Alloc
		//-------------------------------------------------------
		for (int i = 0; i < dfMAX; i++)
		{
			PRO_BEGIN(L"New");
			g_Test[i] = new st_TEST;
			PRO_END(L"New");
		}

		//-------------------------------------------------------
		// Heap Free
		//-------------------------------------------------------
		for (int i = 0; i < dfMAX; i++)
		{
			PRO_BEGIN(L"Delete");
			delete g_Test[i];
			PRO_END(L"Delete");
		}

		//=======================================================
		for (int i = 0; i < dfMAX; i++)
		{
			g_Test[i] = g_MemPool.Alloc();
		}
		for (int i = 0; i < dfMAX; i++)
		{
			g_MemPool.Free(g_Test[i]);
		}
		//=======================================================

		//-------------------------------------------------------
		// MemPool Alloc
		//-------------------------------------------------------
		for (int i = 0; i < dfMAX; i++)
		{
			PRO_BEGIN(L"Alloc");
			g_Test[i] = g_MemPool.Alloc();
			PRO_END(L"Alloc");
		}

		//-------------------------------------------------------
		// MemPool Free
		//-------------------------------------------------------
		for (int i = 0; i < dfMAX; i++)
		{
			PRO_BEGIN(L"Free");
			g_MemPool.Free(g_Test[i]);
			PRO_END(L"Free");
		}

		--iCall;
	}

	return 0;
}

int main(void)
{
	InitializeProfiler(L"../Profiling", en_PROFILER_UNIT::en_UNIT_NANO);

	HANDLE Threads[dfTHREAD_MAX];
	for (int n = 0; n < dfTHREAD_MAX; n++)
	{
		Threads[n] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
	}

	WaitForMultipleObjects(dfTHREAD_MAX, Threads, true, INFINITE);

	OutputProfilingData();

	ReleaseProfiler();

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	return 0;
}