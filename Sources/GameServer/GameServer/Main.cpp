#include <stdio.h>
#include "GamePlayer.h"
#include "GameServer.h"

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
	server.Run();

	return 0;
}