#include <stdio.h>
#include "CPlayer.h"
#include "CGameServer.h"

/*
- alignas �ֱ�
- Accpet / Game / Session ī�����ؼ� ���
- Restart ���� ����
- ��Ʈ��ũ �Լ� ���� �α� ���ܾ���.
- INVALID_SESSION_ID Ȯ�� �ڵ� ����.......
- ������ ���� 2�ñ��� Stop ������ Accept / Connect Total Ȯ�� �ؼ� ���� �� �����ֱ�.
*/

using namespace cov1013;

CGameServer server;

int main(void)
{
	server.Run();

	return 0;
}