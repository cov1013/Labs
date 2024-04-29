
#pragma comment(lib, "ws2_32")

#include <ws2tcpip.h>
#include <winsock.h>
#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <wchar.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "DBConnector.h"
#include "DBConnector_TLS.h"

using namespace cov1013;

#define df_THREAD_MAX	2

int					g_iTPS;
bool				g_bLoop;
//DBConnector		g_DBConnector(L"127.0.0.1", L"root", L"0000", L"testdb", 3306);
DBConnector_TLS	g_DBConnector(L"127.0.0.1", L"root", L"0000", L"testdb", 3306);

unsigned int __stdcall WorkerThread(void* lpParam)
{
	int iCnt = 0;
	DWORD dwThreadId = GetCurrentThreadId();

	while (true)
	{
		//g_DBConnector.Lock();
		g_DBConnector.Query(L"INSERT INTO `test` (`ID`, `RoomNum`) VALUES(%d, %d)", dwThreadId, iCnt++);
		//g_DBConnector.Unlock();

		//Sleep(1000);

		++g_iTPS;
	}

	//g_DBConnector.Lock();
	g_DBConnector.Query_Save(L"SELECT `ID`, `RoomNum` FROM `t`");

	MYSQL_ROW Row;
	while ((Row = g_DBConnector.FetchRow()) != nullptr)
	{
		wprintf(L"ThreadID:%d\tRoomNum:%d\n", atoi(Row[0]), atoi(Row[1]));
	}
	g_DBConnector.FreeResult();
	//g_DBConnector.Unlock();

	return 0;
}

int main(void)
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2), &wsa);

	//g_DBConnector.Query(L"INSERT INTO `Test` (`ID`, `RoomNum`) VALUES(%d, %d)", 1, 1);
	//g_DBConnector.Query(L"DELETE FROM `Test`");

	//g_DBConnector.Connect();

	HANDLE hThreads[df_THREAD_MAX];
	for (int i = 0; i < df_THREAD_MAX; i++)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
	}

	while (true)
	{
		Sleep(999);

		wprintf(L"TPS:%d\n", g_iTPS);
		g_iTPS = 0;

		//WCHAR wch = _getwch();
		//if (wch == 'q' || wch == 'Q')
		//{
		//	g_bLoop = true;
		//	break;
		//}
	}

	WaitForMultipleObjects(df_THREAD_MAX, hThreads, TRUE, INFINITE);
	for (int i = 0; i < df_THREAD_MAX; i++)
	{
		CloseHandle(hThreads[i]);
	}

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);


	//DBConnector* pDBConnector = new DBConnector(L"127.0.0.1", L"root", L"0000", L"testdb", 3306);
	//pDBConnector->Connect();

	//DBConnector* pDBConnector2 = new DBConnector(L"127.0.0.1", L"root", L"0000", L"testdb", 3306);
	//pDBConnector2->Connect();

	//DBConnector DBConnector2(L"127.0.0.1", L"root", L"0000", L"testdb", 3306);
	//DBConnector2.Connect();

	//DBConnector.Query(L"INSERT INTO `Test` (`ID`, `RoomNum`) VALUES(%d, %d)", 1, 10);
	//DBConnector.Query_Save(L"Select * From `Test`");

	//MYSQL_ROW row;
	//while ((row = DBConnector.FetchRow()) != NULL)
	//{
	//	printf("%2s %2s %s\n", row[0], row[1], row[2]);
	//}
	//DBConnector.FreeResult();

	return 0;
}