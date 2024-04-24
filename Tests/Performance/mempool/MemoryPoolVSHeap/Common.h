#pragma once

//#define __LOG__
//#define __GQCSEX__
//#define __PROFILING__
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

enum en_WORKER_JOB_TYPE
{
	en_IO_FAILED = 0x00000000,
	en_THREAD_EXIT = 0xFFFFFFFF,
	en_SEND_PACKET = 0xFFFFFFFE,
	en_JOB = 0xFFFFFFFD
};