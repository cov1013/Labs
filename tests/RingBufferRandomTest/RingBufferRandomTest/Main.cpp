/*
* raw data를 랜덤하게 Read, Write하는 테스트
*/
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "RingBuffer.h"
#pragma comment (lib, "Winmm")

constexpr int BufferSize = 1024 * 10;

int main()
{
	timeBeginPeriod(1);
	system("mode con cols=81 lines=50");

	int index = 0;
	cov1013::RingBuffer ringBuffer(BufferSize);
	const char* sendBuffer = "1234567890 abcdefghijklmnopqrstuvwxyz 1234567890 abcdefghijklmnopqrstuvwxyz 12345";
	BYTE recvBuffer[1024];
	BYTE tempBuffer[1024];
	srand(0);

	while (1)
	{
		//-----------------------------------
		// 초기화
		//-----------------------------------
		memset(tempBuffer, 0, 1024);
		memset(recvBuffer, 0, 1024);

		//-----------------------------------
		// SendSize 계산
		//-----------------------------------
		int sendSize = rand() % 82;
		int nextIndex = index + sendSize;
		if (nextIndex > 81)
		{
			sendSize = 81 - index;
		}

		//-----------------------------------
		// Send
		//-----------------------------------
		int result = ringBuffer.Write((const BYTE*)&sendBuffer[index], sendSize);
		index += result;
		if (index >= 81)
		{
			index = 0;
		}

		//-----------------------------------
		// Recv
		//-----------------------------------
		int recvSize = rand() % 50;
		ringBuffer.Peek(tempBuffer, recvSize);
		ringBuffer.Read(recvBuffer, recvSize);
		if (memcmp(tempBuffer, recvBuffer, 1024) == 0)
		{
			printf("%s", recvBuffer);
		}
	}

	timeEndPeriod(1);
	return 0;
}