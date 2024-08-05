#pragma once
#pragma comment(lib, "winmm")
#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <winsock2.h>
#include <mstcpip.h>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include "Common.h"
#include "MMO_Common.h"
#include "CPDH.h"
#include "CCrashDump.h"
#include "CCpuUsage.h"
#include "CParser.h"
#include "CLogger.h"
#include "Profiler.h"
#include "CMemoryPool.h"
#include "CMemoryPool_TLS.h"
#include "CLockFreeStack.h"
#include "CLockFreeQueue.h"
#include "CRingBuffer.h"
#include "CPacket.h"
#include "CNetSession.h"

namespace cov1013
{
	class CMMOServer
	{
	public:
		//=================================================================
		// 라이브러리 에러 코드
		//=================================================================
		enum en_ERROR_CODE
		{

		};

		//=================================================================
		// 정지 타입
		//=================================================================
		enum en_STOP_TYPE
		{
			STOP = 1,
			RESOURCE_RELEASE = 2,
			QUIT = 3
		};

		//=================================================================
		// 라이브러리 기본 구성
		//=================================================================
		enum en_CONFIG
		{
			eSESSION_MAX			= 20000,	
			eSEND_PACKET_MAX		= 500,		
			eIOCP_WORKER_THREAD_MAX = 100,		
		};

		//=================================================================
		// 스레드별 프레임 처리 제한
		//=================================================================
		enum en_THREAD_LOGIC_MAX
		{
			en_THREAD_LOGIC_MAX_AUTH_ACCEPT = 100,
			en_THREAD_LOGIC_MAX_AUTH_TO_GAME = 100,

			en_THREAD_LOGIC_MAX_AUTH_PACKET = 200,
			en_THREAD_LOGIC_MAX_GAME_PACKET = 200,
		};

		//=================================================================
		// 스레드별 프레임 제한
		// ms 단위
		//=================================================================
		enum en_THREAD_FREAM_MAX
		{
			en_THREAD_FREAM_MAX_AUTH = 10,	// 100 Frame
			en_THREAD_FREAM_MAX_GAME = 10,	// 100 Frame
			en_THREAD_FREAM_MAX_SEND = 10,	// 100 Frame
		};

		//=================================================================
		// Accept 처리용 구조체
		//=================================================================
		struct st_CONNECT_INFO
		{
			WCHAR	IP[16];
			WORD	Port;
			SOCKET	Socket;
		};

	public:
		CMMOServer();
		virtual ~CMMOServer();
		bool Start(
			const wchar_t*			BindIP,						
			const unsigned short	BindPort,					
			const unsigned int		NumberOfIOCPWorkerThread,	
			const unsigned int		NumberOfIOCPActiveThread,	
			const unsigned int		SessionMax,					
			const unsigned int		PacketSizeMax,				
			const bool				bNagleFlag		= TRUE,			
			const bool				bZeroCopyFlag	= FALSE,		
			const bool				bKeepAliveFlag	= FALSE,
			const bool				bDirectSendFlag = TRUE
		);
		void Stop(en_STOP_TYPE eType);
		bool Restart(void);
		bool SendPacket(const SESSION_ID SessionID, CPacket* pPacket);
		void SetSession(const int iIndex, CNetSession* pSession);
		void GetPlayerStatusCount(void);
		const int GetSessionCount(void);
		const int GetIndexesCount(void);

	private:
		CNetSession*					NewSession(const st_CONNECT_INFO ConnectInfo);
		bool							ReleaseSession(CNetSession* pSession);
		bool							Disconnect(CNetSession* pSession);
		void							RecvPost(CNetSession* pSession);
		bool							SendPost(CNetSession* pSession);
		void							RecvProc(CNetSession* pSession, const DWORD dwTransferred);
		void							SendProc(CNetSession* pSession, const DWORD dwTransferred);

		void							OnGame_Update(void);
		void							OnAuth_Update(void);

		static unsigned int __stdcall	AcceptThread(void* lpParam);
		int								AcceptThread_Procedure(void);
		static unsigned int __stdcall	AuthThread(void* lpParam);
		int								AuthThread_Procedure(void);
		static unsigned int __stdcall	SendThread(void* lpParam);
		int								SendThread_Procedure(void);
		static unsigned int __stdcall	IOCPWorkerThread(void* lpParam);
		int								IOCPWorkerThread_Procedure(void);
		static unsigned int __stdcall	GameThread(void* lpParam);
		int								GameThread_Procedure(void);

	public:
		//=================================================================
		// 모니터링 프로퍼티
		//=================================================================
		int								m_AuthPlayer;				
		int								m_GamePlayer;				
		int								m_AuthFPS;					
		int								m_GameFPS;					
		int								m_SendFPS;						
		int								m_AcceptTPS;				
		unsigned __int64				m_AcceptTotal;				
		unsigned __int64				m_DisconnectTotal;			
		int								m_DisconnectTPS;			
		int								m_ExpSendBytes;				

								long	m_RecvPacketTPS;			
		alignas(en_CACHE_ALIGN) long	m_SendPacketTPS;			

		//=================================================================
		// 라이브러리 설정 정보
		//=================================================================
		alignas(en_CACHE_ALIGN)
		int								m_NumberOfIOCPWorkerThread;
		int								m_NumberOfIOCPActiveThread;
		int								m_SessionMax;			
		int								m_PacketSizeMax;
		WORD							m_BindPort;
		bool							m_bNagleFlag;
		bool							m_bZeroCopyFlag;
		bool							m_bKeepAliveFlag;
		bool							m_bDirectSendFlag;
		WCHAR							m_BindIP[16];				

		CQueue<st_CONNECT_INFO>			m_AcceptSockets;

	private:
		bool							m_bLoop;

		unsigned __int64				m_UniqueKey;
		CNetSession*					m_Sessions[eSESSION_MAX];

		CLockFreeStack<SESSION_INDEX>	m_Indexes;

		SOCKET							m_ListenSocket;
		HANDLE							m_hIOCP;
		HANDLE							m_hIOCPWorkerThreads[eIOCP_WORKER_THREAD_MAX];
		HANDLE							m_hAcceptThread;
		HANDLE							m_hAuthThread;
		HANDLE							m_hGameThread;
		HANDLE							m_hSendThread;
	};
}