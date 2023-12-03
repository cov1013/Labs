#pragma once
#include <ws2tcpip.h>
#include <winsock2.h>
#include <mstcpip.h>
#include <windows.h>
#include "CCrashDump.h"
#include "Common.h"
#include "MMO_Common.h"
#include "CPacket.h"
#include "CRingBuffer.h"
#include "CQueue.h"
#include "CLockFreeQueue.h"

//#define __MULTI_THREAD_DEBUG_MODE__

namespace cov1013
{
	class CMMOServer;
	class CNetSession
	{
		friend class CMMOServer;

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

#ifdef __MULTI_THREAD_DEBUG_MODE__
		enum en_LOGIC
		{
			//================================================
			// Library
			// 
			// 1000 ~ 9999		: THREAD
			// 10000 ~ 99999	: LOGIC
			//================================================
			ACCEPT_THREAD		= 10000,
			ACCEPT_THREAD_PART1	= 11000,
			ACCEPT_THREAD_PART2 = 12000,
			ACCEPT_THREAD_PART3 = 13000,
			ACCEPT_THREAD_PART4 = 14000,
			ACCEPT_THREAD_PART5 = 15000,

			AUTH_THREAD			= 20000,
			AUTH_THREAD_PART1	= 21000,
			AUTH_THREAD_PART2	= 22000,
			AUTH_THREAD_PART3	= 23000,
			AUTH_THREAD_PART4	= 24000,
			AUTH_THREAD_PART5	= 25000,

			SEND_THREAD			= 30000,

			WORKER_THREAD		= 40000,

			GAME_THREAD			= 50000,
			GAME_THREAD_PART1	= 51000,
			GAME_THREAD_PART2	= 52000,
			GAME_THREAD_PART3	= 53000,
			GAME_THREAD_PART4	= 54000,
			GAME_THREAD_PART5	= 55000,

			NEW_SESSION			= 1000,
			RECV_PROC			= 2000,
			SEND_PROC			= 3000,
			RECV_POST			= 4000,
			SEND_POST			= 5000,
			SEND_PACKET			= 6000,
			SEND_PACKET_PRIVATE = 6500,
			DISCONNECT			= 7000,
			DISCONNECT_PRIVATE	= 7500,
			RELEASE_SESSION		= 8000,

			//================================================
			// Contents
			//================================================
			
		};

		struct st_LOG
		{
			enum
			{
				en_LOG_MAX = 10000,
			};

			DWORD			ID;							// 4	
			int				Logic;						// 4
			DWORD			Time;						// 4

			en_THREAD_MODE	ThreadMode;					// 4
			int				SendPacketCount;			// 4
			int				SendBufferSize;				// 4
			int				CompleleteRecvBufferSize;	// 4
			short			IOCount;					// 2
			bool			bExitSocketFlag;			// 1
			bool			bSendFlag;					// 1
			bool			bDisconnectFlag;			// 1
			bool			bToGameFlag;				// 1
			SESSION_ID		SessionID;					// 8
			SESSION_INDEX	SessionIndex;				// 8
			SOCKET			Socket;						// 8
		};
#endif

	public:
		CNetSession();
		virtual ~CNetSession();
		bool Disconnect(void);
		bool SendPacket(CPacket* pPacket);
		void SetMode_ToGame(void);

#ifdef __MULTI_THREAD_DEBUG_MODE__
		void SetLog(const int Logic);
#endif

	protected:
		///////////////////////////////////////////////////////////////////
		// AUTH 모드로의 신규 클라이언트 입장.
		// 
		// 해당 플레이어 접속으로 인한 게임상의 데이터 할당 및 준비
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_ClientJoin(void) = 0;

		///////////////////////////////////////////////////////////////////
		// AUTH 모드에서 클라이언트 퇴장
		// 
		// 인증과정에서만 사용했던 데이터가 있다면 정리
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_ClientLeave(void) = 0;

		///////////////////////////////////////////////////////////////////
		// AUTH 모드 세션의 클라이언트 패킷 수신
		// 
		// 로그인 패킷 확인 및 DB 데이터 로딩
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_Packet(CPacket* pPacket) = 0;

		///////////////////////////////////////////////////////////////////
		// GAME 모드로의 신규 클라이언트 입장
		// 
		// 게임컨텐츠 진입을 위한 데이터 준비 및 세팅
		// (월드 맵에 캐릭터 박기, 퀘스트 준비 등)
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_ClientJoin(void) = 0;

		///////////////////////////////////////////////////////////////////
		// GAME 모드에서 클라이언트 퇴장
		// 
		// 게임 컨텐츠상의 플레이어 정리
		// (월드맵에서 제거, 파티정리 등)
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_ClientLeave(void) = 0;

		///////////////////////////////////////////////////////////////////
		// GAME 모드에서 클라이언트 연결 종료
		// 
		// 해당 클라이언트의 진정한 접속 종료, 정리, 해제 등
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_ClientRelease(void) = 0;

		///////////////////////////////////////////////////////////////////
		// GAME 모드 세션의 클라이언트 패킷 수신
		// 
		// 실제 게임 패킷 처리
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_Packet(CPacket* pPacket) = 0;


	public:
#ifdef __MULTI_THREAD_DEBUG_MODE__
		st_LOG							m_Logs[st_LOG::en_LOG_MAX];
		short							m_LogIndex;
#endif

	protected:
		en_THREAD_MODE					m_eThreadMode;
		short							m_IOCount;

		bool							m_bExitSocketFlag;
		bool							m_bSendFlag;
		bool							m_bDisconnectFlag;
		bool							m_bToGameFlag;

		int								m_SendPacketCount;

		SESSION_ID						m_SessionID;
		SESSION_INDEX					m_SessionIndex;

		st_OVERLAPPED					m_RecvOverlapped;
		st_OVERLAPPED					m_SendOverlapped;

		CRingBuffer						m_RecvBuffer;
		CLockFreeQueue<CPacket*>		m_SendBuffer;

		CQueue<CPacket*>				m_CompleteRecvPacket;

		CPacket*						m_SendPacketArray[en_SEND_PACKET_MAX];

		SOCKET							m_Socket;
		WCHAR							m_IP[16];
		WORD							m_Port;
	};
}