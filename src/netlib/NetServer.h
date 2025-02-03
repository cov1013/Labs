#pragma once
#pragma comment(lib, "Winmm")
#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <winsock.h>
#include <mstcpip.h>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <time.h>
#include "Common.h"
#include "PDH.h"
#include "CrashDumper.h"
#include "CPUMeter.h"
#include "ConfigLoader.h"
#include "Logger.h"
#include "Profiler.h"
#include "MemoryPool.h"
#include "MemoryPool_TLS.h"
#include "ConcurrencyStack.h"
#include "ConcurrencyQueue.h"
#include "RingBuffer.h"
#include "PacketBuffer.h"

//#define __MULTI_THREAD_DEBUG_MODE__

namespace cov1013
{
	class NetServer
	{
	public:
		struct st_OVERLAPPED
		{
			enum en_TYPE
			{
				en_RECV,
				en_SEND
			};
			WSAOVERLAPPED	Overlapped;
			en_TYPE			Type;
		};

		enum en_STOP_TYPE
		{
			STOP = 1,
			RESOURCE_RELEASE = 2,
			QUIT = 3
		};

#ifdef __MULTI_THREAD_DEBUG_MODE__
		//=================================================================
		// 
		//=================================================================
		enum en_LOGIC
		{
			// netlib
			ACCPET_THREAD = 1000,
			WORKER_THREAD = 2000,
			NEW_SESSION = 10000,
			RECV_PROC = 20000,
			SEND_PROC = 30000,
			RECV_POST = 40000,
			SEND_POST = 50000,
			SEND_PACKET = 60000,
			DISCONNECT = 70000,
			DISCONNECT_PRIVATE = 75000,
			RELEASE = 80000,

			// contents
			JOB_RECV_MSG = 90000,
			JOB_PROC_LOGIN = 100000,
			JOB_PROC_SEC_MOVE = 110000,
			JOB_PROC_CHAT_MSG = 120000,
			JOB_PROC_REDIS_COM = 130000,
		};

		struct st_LOG
		{
			enum
			{
				en_LOG_MAX = 100,
			};

			DWORD			ID;				// 4		
			int				Logic;			// 4
			DWORD			Time;			// 4
			int				PacketCount;	// 4
			int				SendQCapacity;	// 4
			short			IOCount;		// 2
			bool			bReleaseFlag;	// 1
											// (1)
			SESSION_ID		SessionID;		// 8
			SESSION_INDEX	SessionIndex;	// 8
			SOCKET			Socket;			// 8
		};
#endif

#ifdef __MULTI_THREAD_DEBUG_MODE__
		void SetLog(const int iLogic)
		{
			WORD Index = InterlockedIncrement16(&m_LogIndex);
			Index %= st_LOG::en_LOG_MAX;

			m_Logs[Index].Time = timeGetTime();
			m_Logs[Index].ID = GetCurrentThreadId();
			m_Logs[Index].Logic = iLogic;
			m_Logs[Index].PacketCount = PacketCount;
			m_Logs[Index].SendQCapacity = SendQ.GetCapacity();
			m_Logs[Index].IOCount = IOCount;
			m_Logs[Index].bReleaseFlag = bReleaseFlag;
			m_Logs[Index].SessionID = SessionID;
			m_Logs[Index].SessionIndex = SessionIndex;
			m_Logs[Index].Socket = Socket;
		}
#endif

		struct st_SESSION
		{
			st_SESSION()	// 처음 세션 생성시 아래 항목 필수 초기화
			{
				SessionIndex = df_INVALID_SESSION_INDEX;
				SessionID = df_INVALID_SESSION_ID;
				RecvOverlapped.Type = st_OVERLAPPED::en_RECV;
				SendOverlapped.Type = st_OVERLAPPED::en_SEND;
				PacketCount = 0;
				bSendFlag = false;
				bDisconnectFlag = false;
				bReleaseFlag = false;
				IOCount = 0;
			}

			SESSION_INDEX					SessionIndex;
			SESSION_ID						SessionID;
			st_OVERLAPPED					RecvOverlapped;
			st_OVERLAPPED					SendOverlapped;
			int								PacketCount;
			PacketBuffer*					Packets[en_SEND_PACKET_MAX];
			ConcurrencyQueue<PacketBuffer*>	SendQ;
			RingBuffer						RecvQ;
			bool							bSendFlag;
			bool							bDisconnectFlag;
			SOCKET							Socket;
			WORD							Port;
			WCHAR							IP[16];

			alignas(en_CACHE_ALIGN)
			short							bReleaseFlag;
			short							IOCount;

#ifdef __MULTI_THREAD_DEBUG_MODE__
			st_LOG						m_Logs[st_LOG::en_LOG_MAX];
			short						m_LogIndex;
#endif

		};

	public:
		NetServer();
		virtual ~NetServer();
		bool Start(
			const WCHAR* BindIP,
			const WORD BindPort,
			const int iNumberOfIOCPWorkerThread,
			const int iNumberOfIOCPActiveThread,
			const bool bNagleFlag,
			const bool bZeroCopyFlag,
			const int iSessionMax,
			const int iPacketSizeMax,
			const bool bSelfSendPostFlag = true,
			const bool bKeepAliveFlag = false
		);
		void Stop(en_STOP_TYPE eType);
		bool Restart(void);
		bool SendPacket(const SESSION_ID SessionID, PacketBuffer* pPacket);
		bool Disconnect(const SESSION_ID SessionID);
		const int GetSessionCount(void);
		const int GetIndexesCount(void);

	protected:
		virtual bool OnConnectionRequest(const WCHAR* ConnectIP, const WORD ConnectPORT) = 0;
		virtual void OnClientJoin(const SESSION_ID SessionID) = 0;
		virtual void OnClientLeave(const SESSION_ID SessionID) = 0;
		virtual void OnRecv(const SESSION_ID SessionID, PacketBuffer* pRecvPacket) = 0;
		virtual void OnSend(const SESSION_ID SessionID, const DWORD dwTransferred) = 0;
		virtual void OnWorkerThreadBegin(void) = 0;
		virtual void OnWorkerThreadEnd(void) = 0;
		virtual void OnError(const en_ERROR_CODE eErrCode, const SESSION_ID SessionID = df_INVALID_SESSION_ID) = 0;

	private:
		st_SESSION* NewSession(const SOCKET ConnectSocket);
		bool ReleaseSession(st_SESSION* pSession);
		bool Disconnect(st_SESSION* pSession);
		void RecvPost(st_SESSION* pSession);
		bool SendPost(st_SESSION* pSession);
		void RecvProc(st_SESSION* pSession, const DWORD dwTransferred);
		void SendProc(st_SESSION* pSession, const DWORD dwTransferred);

		static unsigned int __stdcall			AcceptThread(void* lpParam);
		int										AcceptThread_Procedure(void);
		static unsigned int __stdcall			IOCPWorkerThread(void* lpParam);
		int										IOCPWorkerThread_Procedure(void);

	public:
		unsigned long							m_AcceptTPS;
		unsigned __int64						m_AcceptTotal;
		unsigned __int64						m_DisconnectTotal;
		alignas(en_CACHE_ALIGN) long			m_DisconnectTPS;
		alignas(en_CACHE_ALIGN) long			m_RecvPacketTPS;
		alignas(en_CACHE_ALIGN) long			m_SendPacketTPS;
		alignas(en_CACHE_ALIGN) long			m_ExpSendBytes;

		alignas(en_CACHE_ALIGN)
		int										m_NumberOfIOCPWorkerThread;
		int										m_NumberOfIOCPActiveThread;
		int										m_SessionMax;
		int										m_PacketSizeMax;
		WORD									m_BindPort;
		bool									m_bZeroCopyFlag;
		bool									m_bNagleFlag;
		bool									m_bKeepAliveFlag;
		bool									m_bSelfSendPostFlag;
		WCHAR									m_BindIP[16];

	private:
		st_SESSION* m_Sessions[en_SESSION_MAX];
		ConcurrencyStack<short> m_Indexes;
		unsigned __int64 m_UniqueKey;
		SOCKET m_ListenSocket;
		HANDLE m_hIOCP;
		HANDLE m_hAcceptThread;
		HANDLE m_hIOCPWorkerThreads[en_WORKER_THREAD_MAX];
	};
}