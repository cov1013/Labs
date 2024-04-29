#pragma once
#include "Common.h"
#include "PerformaceDataHelper.h"
#include "CrashDumper.h"
#include "CPUMeter.h"
#include "ConfigLoader.h"
#include "Logger.h"
#include "Profiler.h"
#include "MemoryPool.h"
#include "MemoryPool_TLS.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"
#include "RingBuffer.h"
#include "PacketBuffer.h"

namespace covEngine
{
	class LanClient
	{
	public:
		enum en_CONFIG
		{
			en_SEND_PACKET_MAX			= 500,
			en_IOCP_WORKER_THREAD_MAX	= 100,
		};

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

	public:
		LanClient();
		virtual ~LanClient();
		bool Connect(
			const WCHAR*	BindIP,
			const WCHAR*	ServerIP,
			const WORD		ServerPort,
			const DWORD		NumberOfIOCPWorkerThread,
			const bool		bNagleFlag,
			const bool		bZeroCopyFlag
		);
		void Disconnect(const bool bQuit = false);
		void SendPacket(PacketBuffer* pPacket);
		void Stop(void);

	protected:
		virtual void OnEnterJoinServer(void) = 0;		// 서버와의 연결 성공 후
		virtual void OnLeaveServer(void) = 0;			// 서버와의 연결이 끊어졌을 때
		virtual void OnRecv(PacketBuffer* pRecvPacket) = 0;	// 하나의 패킷 수신 완료 후
		virtual void OnSend(int sendsize) = 0;			// 패킷 송신 완료 후
		virtual void OnWorkerThreadBegin(void) = 0;
		virtual void OnWorkerThreadEnd(void) = 0;
		virtual void OnError(int errorcode, wchar_t* szMsg) = 0;

	private:
		bool Release(void);
		void RecvPost(void);
		bool SendPost(void);
		void RecvProc(const DWORD dwTransferred);
		void SendProc(const DWORD dwTransferred);
		static unsigned int __stdcall	IOCPWorkerThread(void* lpParam);
		int								IOCPWorkerThread_Procedure(void);

	public:
		long							m_RecvPacketTPS;
		long							m_SendPacketTPS;
		long							m_ExpSendBytes;

		// Read
		alignas(en_CACHE_ALIGN)
		WCHAR							m_BindIP[16];
		WORD							m_BindPort;
		WCHAR							m_ServerIP[16];
		WORD							m_ServerPort;
		DWORD							m_NumberOfIOCPWorkerThread;
		bool							m_bNagleFlag;
		bool							m_bZeroCopyFlag;

	private:
		alignas(en_CACHE_ALIGN)
		short							m_bReleaseFlag;
		short							m_IOCount;

		bool							m_bFlag;
		bool							m_bSendFlag;
		bool							m_bDisconnectFlag;

		st_OVERLAPPED					m_RecvOverlapped;
		st_OVERLAPPED					m_SendOverlapped;

		RingBuffer						m_RecvBuffer;
		LockFreeQueue<PacketBuffer*>		m_SendBuffer;
		PacketBuffer*						m_SendPacketArray[en_SEND_PACKET_MAX];
		DWORD							m_SendPacketCount;

		HANDLE							m_hIOCP;

		SOCKET							m_Socket;
		HANDLE							m_hIOCPWorkerThreads[en_IOCP_WORKER_THREAD_MAX];
	};
}

