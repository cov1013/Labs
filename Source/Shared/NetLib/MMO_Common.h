#pragma once
enum en_THREAD_MODE
{
	en_THREAD_MODE_NONE = 0,				// 세션 미사용 상태

	en_THREAD_MODE_AUTH = 10000,
	en_THREAD_MODE_AUTH_IN,					// 연결 후 AUTH 스레드 상태
	en_THREAD_MODE_AUTH_OUT_READY,

	en_THREAD_MODE_GAME = 20000,
	en_THREAD_MODE_GAME_IN_READY,			// GAME 스레드로 전환 대기 상태
	en_THREAD_MODE_GAME_IN,					// GAME 스레드 상태
	en_THREAD_MODE_GAME_OUT_READY,			// GAME 스레드에서의 종료준비

	en_THREAD_MODE_RELEASE_READY_COM,		// RELEASE 준비 완료 -> 바로 삭제 가능
};