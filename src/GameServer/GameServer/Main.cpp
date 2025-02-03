#include <stdio.h>
#include "GamePlayer.h"
#include "GameServer.h"

#include "DBConnector/DBConnector/DBConnector.h"

using namespace cov1013;

GameServer server;

int main(void)
{
	//DBConnector	DBConnector(L"127.0.0.1", L"root", L"0000", L"testdb", 3306);
	//if (DBConnector.Connect() == false)
	//{
	//	return 0;
	//}

	server.Run();

	return 0;
}