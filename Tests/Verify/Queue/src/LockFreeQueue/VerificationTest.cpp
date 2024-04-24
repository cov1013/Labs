#pragma comment(lib, "Winmm")
#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <time.h>
#include <locale.h>
#include <process.h>
#include <crtdbg.h>

#include "CMemoryPool.h"
#include "CLockFreeQueue.h"
#include "CQueue.h"
#include "CCrashDump.h"

using namespace cov1013;

#define dfTHREAD_MAX	4
#define dfDATA_MAX		1

struct st_DATA
{
	unsigned __int64 Code;
	unsigned __int64 Count;
};

CCrashDump					g_CrashDump;
//CLockFreeQueue<st_DATA*>	g_Queue;
CQueue<st_DATA*>			g_Queue;
bool						g_bShutdown = false;

unsigned int __stdcall MoniteringThread(LPVOID lpParam)
{
	while (!g_bShutdown)
	{
		wprintf(L"=========================================\n");
		wprintf(L"MemoryPool Capacity:%d | UseCount:%d\n", g_Queue.m_pMemoryPool->GetCapacity(), g_Queue.m_pMemoryPool->GetUseCount());
		wprintf(L"=========================================\n\n\n");
		Sleep(999);
	}
	return 0;
}

unsigned int __stdcall WorkerThread(LPVOID lpParam)
{
	st_DATA* Dataes[dfDATA_MAX];

	while (!g_bShutdown)
	{
		// 1. Pop 체크
		for (int i = 0; i < dfDATA_MAX; i++)
		{
			g_Queue.Lock();
			bool bFlag = g_Queue.Dequeue(&(Dataes[i]));
			g_Queue.Unlock();

			if (!bFlag)
			{
				CRASH();
			}
		}

		// 2. 초기값 체크
		for (int i = 0; i < dfDATA_MAX; i++)
		{
			if (Dataes[i]->Code != 0x5555555555555555)
			{
				CRASH();
			}
			if (Dataes[i]->Count != 0)
			{
				CRASH();
			}
		}

		// 3. 데이터 변경
		for (int i = 0; i < dfDATA_MAX; i++)
		{
			InterlockedIncrement(&Dataes[i]->Code);
			InterlockedIncrement(&Dataes[i]->Count);
		}

		// 4. 데이터 변경 중 다른곳에서 해당 메모리를 사용했는지 체크
		for (int i = 0; i < dfDATA_MAX; i++)
		{
			if (Dataes[i]->Code != 0x5555555555555556)
			{
				CRASH();
			}
			if (Dataes[i]->Count != 1)
			{
				CRASH();
			}
		}

		// 5. 다시 초기값으로 변경
		for (int i = 0; i < dfDATA_MAX; i++)
		{
			InterlockedDecrement(&Dataes[i]->Code);
			InterlockedDecrement(&Dataes[i]->Count);
		}

		// 6. 초기값으로 변경 중 다른곳에서 해당 메모리를 사용했는지 체크
		for (int i = 0; i < dfDATA_MAX; i++)
		{
			if (Dataes[i]->Code != 0x5555555555555555)
			{
				CRASH();
			}
			if (Dataes[i]->Count != 0)
			{
				CRASH();
			}
		}

		// 7. 반환
		for (int i = 0; i < dfDATA_MAX; i++)
		{
			g_Queue.Lock();
			g_Queue.Enqueue(Dataes[i]);
			g_Queue.Unlock();
		}
	}

	return 0;
}

void Initial(void)
{
	for (int i = 0; i < dfDATA_MAX * dfTHREAD_MAX; i++)
	{
		st_DATA* pData = new st_DATA;
		pData->Code = 0x5555555555555555;
		pData->Count = 0;
		g_Queue.Enqueue(pData);
	}
}

int main(void)
{
	setlocale(LC_ALL, "");
	timeBeginPeriod(1);

	Initial();

	HANDLE hMoniteringThread = (HANDLE)_beginthreadex(nullptr, 0, MoniteringThread, nullptr, 0, nullptr);
	HANDLE hThreads[dfTHREAD_MAX];
	for (int i = 0; i < dfTHREAD_MAX; i++)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
	}

	WCHAR ch;
	while (!g_bShutdown)
	{
		ch = _getwch();
		if (ch == 'q' || ch == 'Q')
		{
			g_bShutdown = true;
		}
	}

	wprintf(L"---------------------------------------------------------\n");
	wprintf(L"스레드 종료 대기...\n");
	if (WaitForSingleObject(hMoniteringThread, INFINITE) == WAIT_FAILED)
	{
		wprintf(L"Monitering Thread Exit error:%ld\n", GetLastError());
	}
	wprintf(L"### Monitering Thread 종료 완료\n");

	if (WaitForMultipleObjects(dfTHREAD_MAX, hThreads, TRUE, INFINITE) == WAIT_FAILED)
	{
		wprintf(L"Workder Thread Exit error:%ld\n", GetLastError());
	}
	wprintf(L"### Worker Thread 종료 완료\n");
	wprintf(L"---------------------------------------------------------\n");

	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	timeEndPeriod(1);
	return 0;
}