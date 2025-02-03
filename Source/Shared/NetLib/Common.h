#pragma once

/*
- alignas 넣기
- Accpet / Game / Session 카운팅해서 출력
- Restart 복붙 ㄴㄴ
- 네트워크 함수 실패 로그 남겨야함.
- INVALID_SESSION_ID 확인 코드 빼기.......
- 월요일 오후 2시까지 Stop 눌러서 Accept / Connect Total 확인 해서 스샷 찍어서 보내주기.
*/
namespace cov1013
{
//#define __LOG__
//#define __GQCSEX__
#define __PROFILING__
//#define __SAFE_MODE__
//#define __KEEP_ALIVE__
#define __TLS_MODE__

#define CAS(Dest, Ex, Com)			InterlockedCompareExchange64((volatile LONG64*)Dest, (LONG64)Ex, (LONG64)Com)																	
#define DCAS(Dest, ExH, ExL, Com)	InterlockedCompareExchange128((volatile LONG64*)Dest,(LONG64)ExH,(LONG64)ExL,(LONG64*)Com)

#define df_INVALID_SESSION_ID		-1
#define df_INVALID_SESSION_INDEX	-1
#define GET_SESSION_INDEX(a)		(a & 0x000000000000FFFF)

#define df_TCP_HEADER_SIZE			(20)
#define df_IP_HEADER_SIZE			(20)
#define df_MAC_HEADER_SIZE			(14)
#define df_MSS						(1460)

	typedef unsigned __int64	SESSION_ID;
	typedef short				SESSION_INDEX;

	enum en_STOP_TYPE
	{
		STOP = 1,
		RESOURCE_RELEASE = 2,
		QUIT = 3
	};

	enum en_ERROR_CODE
	{
		en_ERROR_WINSOCK = 10000,
		en_ERROR_WINSOCK_INIT,
		en_ERROR_WINSOCK_BIND,
		en_ERROR_WINSOCK_LISTEN_SOCK,
		en_ERROR_WINSOCK_SET_NAGLE,
		en_ERROR_WINSOCK_SET_RST,
		en_ERROR_WINSOCK_OVER_CONNECT,

		en_ERROR_RING_BUF = 20000,
		en_ERROR_RING_BUF_RECV,
		en_ERROR_RING_BUF_SEND,

		en_ERROR_CONFIG = 30000,
		en_ERROR_CONFIG_OVER_THREAD,
		en_ERROR_CONFIG_OVER_SESSION,

		en_ERROR_NET_PACKET = 40000,
		en_ERROR_DECODE_FAILD = 50000,
		en_ERROR_DB_CONNECT_FAILD = 60000,
	};

	enum en_CONPIG
	{
		en_CAS_ALIGN = 16,
		en_CACHE_ALIGN = 64,
		en_PAGE_ALIGN = 4096,

		en_PADDING_SIZE = 56,

		en_SESSION_MAX = 20000,
		en_WORKER_THREAD_MAX = 100,
		en_SEND_PACKET_MAX = 500,
		en_GQCS_MAX = 1000,
		en_PACKET_LEN_MAX = 1024,
	};

	enum en_WORKER_JOB_TYPE : unsigned int
	{
		en_IO_FAILED = 0x00000000,
		en_THREAD_EXIT = 0xFFFFFFFF,
		en_SEND_PACKET = 0xFFFFFFFE,
		en_JOB = 0xFFFFFFFD
	};

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
}

