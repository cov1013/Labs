#pragma once
#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <winsock.h>
#include <mstcpip.h>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include "../../netlib/Common.h"
#include "../../netlib/CPDH.h"
#include "../../netlib/CCrashDump.h"
#include "../../netlib/CCpuUsage.h"
#include "../../netlib/CParser.h"
#include "../../netlib/CLogger.h"
#include "../../netlib/Profiler.h"
#include "../../netlib/CMemoryPool.h"
#include "../../netlib/CMemoryPool_TLS.h"
#include "../../netlib/CLockFreeStack.h"
#include "../../netlib/CLockFreeQueue.h"
#include "../../netlib/CRingBuffer.h"
#include "../../netlib/CStreamQ.h"
#include "../../netlib/CPacket.h"

namespace cov1013
{
	class CLanClient
	{
	public:
		enum en_CONFIG
		{
			en_SEND_PACKET_MAX = 500,
			en_IOCP_WORKER_THREAD_MAX = 100,
		};

		enum en_OVERLAPPED_TYPE
		{
			eOVERLAPPED_TYPE_SEND,
			eOVERLAPPED_TYPE_RECV,
		};

		struct st_OVERLAPPED
		{
			en_OVERLAPPED_TYPE	Type;
			WSAOVERLAPPED		Overlapped;
		};

	public:
		CLanClient();
		virtual ~CLanClient();
		bool Connect(
			const WCHAR* BindIP,
			const WCHAR* ServerIP,
			const WORD		ServerPort,
			const DWORD		NumberOfIOCPWorkerThread,
			const bool		bNagleFlag,
			const bool		bZeroCopyFlag
		);
		bool Disconnect(void);
		bool SendPacket(CPacket* pPacket);

	protected:
		virtual void OnEnterJoinServer() = 0;						// 서버와의 연결 성공 후
		virtual void OnLeaveServer() = 0;							// 서버와의 연결이 끊어졌을 때
		virtual void OnRecv(CPacket* pRecvPacket) = 0;				// 하나의 패킷 수신 완료 후
		virtual void OnSend(int sendsize) = 0;						// 패킷 송신 완료 후
		virtual void OnWorkerThreadBegin() = 0;
		virtual void OnWorkerThreadEnd() = 0;
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
		long							m_SendCompeltedTPS;
		long							m_ExpSendBytes;

	private:
		bool							m_bSendFlag;
		bool							m_bDisconnectFlag;

		st_OVERLAPPED					m_RecvOverlapped;
		st_OVERLAPPED					m_SendOverlapped;

		CRingBuffer						m_RecvBuffer;
		CLockFreeQueue<CPacket*>		m_SendBuffer;
		CPacket*						m_SendPacketArray[en_SEND_PACKET_MAX];
		DWORD							m_SendPacketCount;

		WCHAR							m_IP[16];
		WORD							m_Port;
		WCHAR							m_ServerIP[16];
		WORD							m_ServerPort;

		SOCKET							m_Socket;
		HANDLE							m_hIOCP;
		HANDLE							m_hIOCPWorkerThreads[en_IOCP_WORKER_THREAD_MAX];

		alignas(64)
		short							m_bReleaseFlag;
		short							m_IOCount;
	};
}

