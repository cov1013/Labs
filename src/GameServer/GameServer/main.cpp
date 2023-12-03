#include <stdio.h>
#include "CPlayer.h"
#include "CGameServer.h"

/*
- alignas 넣기
- Accpet / Game / Session 카운팅해서 출력
- Restart 복붙 ㄴㄴ
- 네트워크 함수 실패 로그 남겨야함.
- INVALID_SESSION_ID 확인 코드 빼기.......
- 월요일 오후 2시까지 Stop 눌러서 Accept / Connect Total 확인 해서 스샷 찍어서 보내주기.
*/

using namespace cov1013;

CGameServer server;

int main(void)
{
	server.Run();

	return 0;
}