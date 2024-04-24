#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <conio.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>
#include "CPDH.h"

#define __PROFILE_ON__
#include "ProFiler.h"
#include "CMemoryPool_TLS.h"

#define dfTHREAD_MAX	4
#define dfCOUNT			10000

CPDH g_PDH(L"MemoryPoolVsHeap");

struct st_SAMPLE_DATA
{
	char Sample[1024];
};

CMemoryPool_TLS<st_SAMPLE_DATA> g_MemPool;

unsigned int __stdcall WorkerThread(void* lpParam)
{
	st_SAMPLE_DATA** g_Test;
	g_Test = (st_SAMPLE_DATA**)_aligned_malloc(sizeof(st_SAMPLE*) * dfCOUNT, en_PAGE_ALIGN);

	for (int i = 0; i < dfCOUNT; i++)
	{
		*(g_Test + i) = g_MemPool.Alloc();
	}

	for (int i = 0; i < dfCOUNT; i++)
	{
		g_MemPool.Free(*(g_Test + i));
	}

	//-------------------------------------------------------
	// MemPool Alloc
	//-------------------------------------------------------
	WCHAR szTag[255] = { 0 };
	for (int i = 0; i < dfCOUNT; i++)
	{
		PRO_BEGIN(L"Alloc");
		g_Test[i] = g_MemPool.Alloc();
		PRO_END(L"Alloc");
	}

	//-------------------------------------------------------
	// MemPool Free
	//-------------------------------------------------------
	for (int i = 0; i < dfCOUNT; i++)
	{
		g_MemPool.Free(*(g_Test + i));
	}

	_aligned_free(g_Test);

	return 0;
}

int main(void)
{
	InitializeProfiler(L"../Profiling", en_PROFILER_UNIT::en_UNIT_NANO);

	HANDLE Threads[dfTHREAD_MAX];
	for (int n = 0; n < dfTHREAD_MAX; n++)
	{
		Threads[n] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
		SetThreadAffinityMask(Threads[n], 0x10 + (n+1));
	}

	WaitForMultipleObjects(dfTHREAD_MAX, Threads, true, INFINITE);

	OutputProfilingData();
	ReleaseProfiler();

#ifdef DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	return 0;
}
