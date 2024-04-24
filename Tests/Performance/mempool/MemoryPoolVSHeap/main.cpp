#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <process.h>
#include "CMemoryPool.h"
#include "CMemoryPool_TLS.h"

#define __PROFILING__
#include "Profiler.h"

#define dfTEST_COUNT	10000
#define dtTEST_SIZE		1024
#define dfTHREAD_COUNT	6

#define SWITCH			1

using namespace cov1013;

struct st_SAMPLE_DATA
{
	char Sample[1024];
};

//CMemoryPool<st_SAMPLE_DATA> g_MemPool = CMemoryPool<st_SAMPLE_DATA>(0);
CMemoryPool_TLS<st_SAMPLE_DATA> g_MemPool = CMemoryPool_TLS<st_SAMPLE_DATA>(0);

unsigned int __stdcall WorkerThread(void* lpParam)
{
	st_SAMPLE_DATA** g_Test;
	g_Test = (st_SAMPLE_DATA**)_aligned_malloc(sizeof(st_SAMPLE*) * dfTEST_COUNT, en_PAGE_ALIGN);

#ifdef SWITCH
	//-------------------------------------------------------
	// Heap Alloc
	//-------------------------------------------------------
	for (int i = 0; i < dfTEST_COUNT; i++)
	{
		PRO_BEGIN(L"New");
		*(g_Test + i) = new st_SAMPLE_DATA;
		PRO_END(L"New");
	}

	//-------------------------------------------------------
	// Heap Free
	//-------------------------------------------------------
	for (int i = 0; i < dfTEST_COUNT; i++)
	{
		PRO_BEGIN(L"Delete");
		delete* (g_Test + i);
		PRO_END(L"Delete");
	}
#else
	for (int i = 0; i < dfTEST_COUNT; i++)
	{
		*(g_Test + i) = g_MemPool.Alloc();
	}

	for (int i = 0; i < dfTEST_COUNT; i++)
	{
		g_MemPool.Free(*(g_Test + i));
	}

	//-------------------------------------------------------
	// MemPool Alloc
	//-------------------------------------------------------
	for (int i = 0; i < dfTEST_COUNT; i++)
	{
		PRO_BEGIN(L"Alloc");
		*(g_Test + i) = g_MemPool.Alloc();
		PRO_END(L"Alloc");
	}

	//-------------------------------------------------------
	// MemPool Free
	//-------------------------------------------------------
	for (int i = 0; i < dfTEST_COUNT; i++)
	{
		PRO_BEGIN(L"Free");
		g_MemPool.Free(*(g_Test + i));
		PRO_END(L"Free");
	}
#endif

	_aligned_free(g_Test);

	return 0;
}

int main(void)
{
	InitializeProfiler(L"../ProfilingData", en_PROFILER_UNIT::eUNIT_NANO);

	HANDLE Threads[dfTHREAD_COUNT];
	for (int n = 0; n < dfTHREAD_COUNT; n++)
	{
		Threads[n] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
	}

	WaitForMultipleObjects(dfTHREAD_COUNT, Threads, true, INFINITE);
	OutputProfilingData();
	ReleaseProfiler();

#ifdef DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	return 0;
}