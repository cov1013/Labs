#include <stdio.h>
#include "GamePlayer.h"
#include "GameServer.h"

#include "DBConnector/DBConnector/DBConnector.h"

/*
- alignas 넣기
- Accpet / Game / Session 카운팅해서 출력
- Restart 복붙 ㄴㄴ
- 네트워크 함수 실패 로그 남겨야함.
- INVALID_SESSION_ID 확인 코드 빼기.......
- 월요일 오후 2시까지 Stop 눌러서 Accept / Connect Total 확인 해서 스샷 찍어서 보내주기.
*/

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