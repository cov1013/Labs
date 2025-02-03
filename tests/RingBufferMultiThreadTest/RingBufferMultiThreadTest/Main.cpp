/*
* 서로 다른 스레드가 한 쪽에서는 Read만, 한 쪽에서는 Write만 하는 테스트
*/

#include <conio.h>
#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include <process.h>
#include "RingBuffer.h"
#pragma comment (lib, "Winmm")

constexpr int BufferSize = 1024 * 10;

bool g_bLoop;
cov1013::RingBuffer	g_RingBuffer(BufferSize);

static unsigned int __stdcall SendThread(void* lpParam)
{
	srand(GetCurrentThreadId());

	char	szBuffer[82] = { "1234567890 abcdefghijklmnopqrstuvwxyz 1234567890 abcdefghijklmnopqrstuvwxyz 12345" };
	int		iSendSize;
	int		iBufferIndex = 0;
	int		iBufferNext;
	int		iResult;

	while (!g_bLoop)
	{
		iSendSize = rand() % 82;
		iBufferNext = iBufferIndex + iSendSize;
		if (iBufferNext > 81)
		{
			iSendSize = 81 - iBufferIndex;
		}

		iResult = g_RingBuffer.Write((BYTE*)&szBuffer[iBufferIndex], iSendSize);
		iBufferIndex += iResult;
		if (iBufferIndex >= 81)
		{
			iBufferIndex = 0;
		}
	}

	return 0;
}

static unsigned int __stdcall RecvThread(void* lpParam)
{
	srand(GetCurrentThreadId());

	int	 iRecvSize;
	BYTE szBuffer[1024];

	while (!g_bLoop)
	{
		memset(szBuffer, 0, 1024);
		iRecvSize = rand() % 100;
		g_RingBuffer.Read(szBuffer, iRecvSize);

		printf("%s", szBuffer);
	}

	return 0;
}

int main()
{
	timeBeginPeriod(1);
	system("mode con cols=81 lines=50");

	HANDLE hSendThread = (HANDLE)_beginthreadex(nullptr, 0, SendThread, nullptr, false, nullptr);
	HANDLE hRecvThread = (HANDLE)_beginthreadex(nullptr, 0, RecvThread, nullptr, false, nullptr);

	WCHAR wch = _getwch();
	if (wch == 'q' || wch == 'Q')
	{
		g_bLoop = true;
	}

	WaitForSingleObject(hRecvThread, INFINITE);
	WaitForSingleObject(hSendThread, INFINITE);
	CloseHandle(hRecvThread);
	CloseHandle(hSendThread);

	timeEndPeriod(1);

	return 0;
}