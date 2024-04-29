#pragma once
#include <ws2tcpip.h>
#include <winsock2.h>
#include <mstcpip.h>
#include <windows.h>
#include "CrashDumper.h"
#include "Common.h"
#include "RingBuffer.h"
#include "PacketBuffer.h"
#include "Queue.h"
#include "LockFreeQueue.h"

//#define __MULTI_THREAD_DEBUG_MODE__

namespace covEngine
{
	class MMOServer;
	class MMOSession
	{
		friend class MMOServer;

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
		MMOSession();
		virtual ~MMOSession();
		bool Disconnect();
		bool SendPacket(PacketBuffer* pPacket);
		void SetMode_ToGame();

#ifdef __MULTI_THREAD_DEBUG_MODE__
		void SetLog(const int Logic);
#endif

	protected:
		///////////////////////////////////////////////////////////////////
		// AUTH ������ �ű� Ŭ���̾�Ʈ ����.
		// 
		// �ش� �÷��̾� �������� ���� ���ӻ��� ������ �Ҵ� �� �غ�
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_ClientJoin() = 0;

		///////////////////////////////////////////////////////////////////
		// AUTH ��忡�� Ŭ���̾�Ʈ ����
		// 
		// �������������� ����ߴ� �����Ͱ� �ִٸ� ����
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_ClientLeave() = 0;

		///////////////////////////////////////////////////////////////////
		// AUTH ��� ������ Ŭ���̾�Ʈ ��Ŷ ����
		// 
		// �α��� ��Ŷ Ȯ�� �� DB ������ �ε�
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_Packet(PacketBuffer* pPacket) = 0;

		///////////////////////////////////////////////////////////////////
		// GAME ������ �ű� Ŭ���̾�Ʈ ����
		// 
		// ���������� ������ ���� ������ �غ� �� ����
		// (���� �ʿ� ĳ���� �ڱ�, ����Ʈ �غ� ��)
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_ClientJoin() = 0;

		///////////////////////////////////////////////////////////////////
		// GAME ��忡�� Ŭ���̾�Ʈ ����
		// 
		// ���� ���������� �÷��̾� ����
		// (����ʿ��� ����, ��Ƽ���� ��)
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_ClientLeave() = 0;

		///////////////////////////////////////////////////////////////////
		// GAME ��忡�� Ŭ���̾�Ʈ ���� ����
		// 
		// �ش� Ŭ���̾�Ʈ�� ������ ���� ����, ����, ���� ��
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_ClientRelease() = 0;

		///////////////////////////////////////////////////////////////////
		// GAME ��� ������ Ŭ���̾�Ʈ ��Ŷ ����
		// 
		// ���� ���� ��Ŷ ó��
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_Packet(PacketBuffer* pPacket) = 0;


	public:
#ifdef __MULTI_THREAD_DEBUG_MODE__
		st_LOG							m_Logs[st_LOG::en_LOG_MAX];
		short							m_LogIndex;
#endif

	protected:
		en_THREAD_MODE m_eThreadMode;
		short m_IOCount;
		bool m_bExitSocketFlag;
		bool m_bSendFlag;
		bool m_bDisconnectFlag;
		bool m_bToGameFlag;
		int m_SendPacketCount;
		SESSION_ID m_SessionID;
		SESSION_INDEX m_SessionIndex;
		st_OVERLAPPED m_RecvOverlapped;
		st_OVERLAPPED m_SendOverlapped;
		RingBuffer m_RecvBuffer;
		LockFreeQueue<PacketBuffer*> m_SendBuffer;
		Queue<PacketBuffer*> m_CompleteRecvPacket;
		PacketBuffer* m_SendPacketArray[en_SEND_PACKET_MAX];
		SOCKET m_Socket;
		WCHAR m_IP[16];
		WORD m_Port;
	};
}