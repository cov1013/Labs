#pragma once
enum en_THREAD_MODE
{
	en_THREAD_MODE_NONE = 0,				// ���� �̻�� ����

	en_THREAD_MODE_AUTH = 10000,
	en_THREAD_MODE_AUTH_IN,					// ���� �� AUTH ������ ����
	en_THREAD_MODE_AUTH_OUT_READY,

	en_THREAD_MODE_GAME = 20000,
	en_THREAD_MODE_GAME_IN_READY,			// GAME ������� ��ȯ ��� ����
	en_THREAD_MODE_GAME_IN,					// GAME ������ ����
	en_THREAD_MODE_GAME_OUT_READY,			// GAME �����忡���� �����غ�

	en_THREAD_MODE_RELEASE_READY_COM,		// RELEASE �غ� �Ϸ� -> �ٷ� ���� ����
};