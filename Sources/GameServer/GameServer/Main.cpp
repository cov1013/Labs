#include <stdio.h>
#include "GamePlayer.h"
#include "GameServer.h"

#include "DBConnector/DBConnector/DBConnector.h"

/*
- alignas �ֱ�
- Accpet / Game / Session ī�����ؼ� ���
- Restart ���� ����
- ��Ʈ��ũ �Լ� ���� �α� ���ܾ���.
- INVALID_SESSION_ID Ȯ�� �ڵ� ����.......
- ������ ���� 2�ñ��� Stop ������ Accept / Connect Total Ȯ�� �ؼ� ���� �� �����ֱ�.
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