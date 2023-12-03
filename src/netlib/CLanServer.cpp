
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
#include "CLanServer.h"

namespace cov1013
{
	//////////////////////////////////////////////////////////////////////
	// 생성자
	//////////////////////////////////////////////////////////////////////
	CLanServer::CLanServer()
	{
		//-----------------------------------------------------------------------
		// 모니터링 프로퍼티 초기화
		//-----------------------------------------------------------------------
		m_AcceptTPS = 0;
		m_AcceptTotal = 0;
		m_DisconnectTotal = 0;
		m_DisconnectTPS = 0;
		m_RecvPacketTPS = 0;
		m_SendPacketTPS = 0;
		m_ExpSendBytes = 0;

		//-----------------------------------------------------------------------
		// 라이브러리 설정 정보 초기화
		//-----------------------------------------------------------------------
		m_NumberOfIOCPWorkerThread = 0;
		m_NumberOfIOCPActiveThread = 0;
		m_SessionMax = 0;
		m_bZeroCopyFlag = false;
		m_bNagleFlag = false;
		m_bKeepAliveFlag = false;
		m_bSelfSendPostFlag = false;

		//-----------------------------------------------------------------------
		// 내부 요소 초기화
		//-----------------------------------------------------------------------
		m_UniqueKey = 1;
		m_ListenSocket = INVALID_SOCKET;
		m_hIOCP = nullptr;
		m_hAcceptThread = nullptr;
		memset(m_Sessions, 0, sizeof(st_SESSION*) * en_SESSION_MAX);
		memset(m_hIOCPWorkerThreads, 0, sizeof(HANDLE) * en_WORKER_THREAD_MAX);

		//-----------------------------------------------------------------------
		// 덤프 클래스 초기화
		//-----------------------------------------------------------------------
		CCrashDump::CCrashDump();

		//-----------------------------------------------------------------------
		// 시스템 로거 클래스 초기화
		//-----------------------------------------------------------------------
		CLogger::CLogger(L"../Logs_Lanserver", eLOG_LEVEL_DEBUG);

		//-----------------------------------------------------------------------
		// 프로파일러 초기화
		//-----------------------------------------------------------------------
		InitializeProfiler(L"../Profiling_Lanserver", en_PROFILER_UNIT::eUNIT_NANO);

		//-----------------------------------------------------------------------
		// WinSock 초기화
		//-----------------------------------------------------------------------
		WSADATA	wsa;
		if (0 != WSAStartup(MAKEWORD(2, 2), &wsa))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to initialization Winsock (error:%ld)\n", WSAGetLastError());
			return;
		}

		LOG(L"System", eLOG_LEVEL_SYSTEM, L"Winsock initialization Successful\n");
	}

	//////////////////////////////////////////////////////////////////////
	// 소멸자
	// 
	//////////////////////////////////////////////////////////////////////
	CLanServer::~CLanServer()
	{
	}

	//////////////////////////////////////////////////////////////////////
	// 서버 시작
	// 
	//////////////////////////////////////////////////////////////////////
	bool CLanServer::Start(
		const WCHAR* BindIP,
		const WORD BindPort,
		const int iNumberOfIOCPWorkerThread,
		const int iNumberOfIOCPActiveThread,
		const bool bNagleFlag,
		const bool bZeroCopyFlag,
		const int iSessionMax,
		const bool bSelfSendPostFlag,
		const bool bKeepAliveFlag
	)
	{
		SOCKADDR_IN ServerAddr;
		InetPton(AF_INET, BindIP, (PVOID)&ServerAddr.sin_addr);
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(BindPort);

		//-----------------------------------------------------------------------
		// 생성할 워커스레드 개수 체크
		//-----------------------------------------------------------------------
		if (iNumberOfIOCPWorkerThread > en_WORKER_THREAD_MAX)
		{
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Create Worker Thread Num Over (%d > %d)", iNumberOfIOCPWorkerThread, en_WORKER_THREAD_MAX);
			OnError(en_ERROR_CONFIG_OVER_THREAD);

			return false;
		}

		//-----------------------------------------------------------------------
		// 생성할 세션수 체크
		//-----------------------------------------------------------------------
		if (iSessionMax > en_SESSION_MAX)
		{
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Create Session Over (%d > %d)", iSessionMax, en_SESSION_MAX);
			OnError(en_ERROR_CONFIG_OVER_SESSION);

			return false;
		}

		//-----------------------------------------------------------------------
		// 1. ListenSocket 생성
		//-----------------------------------------------------------------------
		m_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == m_ListenSocket)
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to create ListenSocket (error:%ld)\n", WSAGetLastError());
			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"ListenSocket Creation Successful\n");

		//-----------------------------------------------------------------------
		// 2. 네이글 알고리즘 설정
		//-----------------------------------------------------------------------
		if (!bNagleFlag)
		{
			bool bFlag = true;
			if (SOCKET_ERROR == setsockopt(m_ListenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&bFlag, sizeof(bFlag)))
			{
				LOG(L"System", eLOG_LEVEL_ERROR, L"Nalge Setup Failed (error:%ld)\n", WSAGetLastError());
				return false;
			}
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"Nalge Setup Successful\n");

		//-----------------------------------------------------------------------
		// KeepAlive 설정 (테스트용)
		//-----------------------------------------------------------------------
		if (bKeepAliveFlag)
		{
			DWORD dwResult;
			tcp_keepalive KeepAlive;
			KeepAlive.onoff = 1;
			KeepAlive.keepalivetime = 10000;
			KeepAlive.keepaliveinterval = 1000;
			if (0 != WSAIoctl(m_ListenSocket, SIO_KEEPALIVE_VALS, &KeepAlive, sizeof(KeepAlive), 0, 0, &dwResult, NULL, NULL))
			{
				LOG(L"System", eLOG_LEVEL_ERROR, L"KeepAlive Setup Failed (error:%ld)\n", WSAGetLastError());
				return false;
			}
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"KeepAlive Setup Successful\n");
		}

		//-----------------------------------------------------------------------
		// 3. 비동기 출력 설정
		//-----------------------------------------------------------------------
		if (bZeroCopyFlag)
		{
			int iSize = 0;
			if (SOCKET_ERROR == setsockopt(m_ListenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&iSize, sizeof(iSize)))
			{
				LOG(L"System", eLOG_LEVEL_ERROR, L"ZeroCopy Setup Failed (error:%ld)\n", WSAGetLastError());
				return false;
			}
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"ZeroCopy Setup Successful\n");

		//-----------------------------------------------------------------------
		// 4. RST 송신 설정
		//-----------------------------------------------------------------------
		LINGER Linger;
		Linger.l_onoff = 1;
		Linger.l_linger = 0;
		if (SOCKET_ERROR == setsockopt(m_ListenSocket, SOL_SOCKET, SO_LINGER, (const char*)&Linger, sizeof(Linger)))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to Linger setsockpot (error:%ld)\n", WSAGetLastError());
			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"Linger Setup Successful\n");

		//-----------------------------------------------------------------------
		// 5. Bind
		//-----------------------------------------------------------------------
		if (SOCKET_ERROR == bind(m_ListenSocket, (const sockaddr*)&ServerAddr, sizeof(ServerAddr)))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"bind failed (error:%ld)\n", WSAGetLastError());
			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"bind Successful\n");

		//-----------------------------------------------------------------------
		// 6. Listen
		//-----------------------------------------------------------------------
		if (SOCKET_ERROR == listen(m_ListenSocket, SOMAXCONN))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"listen failed(error:%ld)\n", WSAGetLastError());
			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"listen Successful\n");

		//-----------------------------------------------------------------------
		// 8. 라이브러리 설정 정보 세팅
		//-----------------------------------------------------------------------
		m_NumberOfIOCPWorkerThread = iNumberOfIOCPWorkerThread;
		m_NumberOfIOCPActiveThread = iNumberOfIOCPActiveThread;
		m_SessionMax = iSessionMax;
		m_BindPort = BindPort;
		m_bZeroCopyFlag = bZeroCopyFlag;
		m_bNagleFlag = bNagleFlag;
		m_bKeepAliveFlag = bKeepAliveFlag;
		m_bSelfSendPostFlag = bSelfSendPostFlag;
		wcscpy_s(m_BindIP, 16, BindIP);

		//-----------------------------------------------------------------------
		// 9. 세션 세팅
		//-----------------------------------------------------------------------
		for (int i = 0; i < m_SessionMax; i++)
		{
			m_Sessions[i] = (st_SESSION*)_aligned_malloc(sizeof(st_SESSION), en_CACHE_ALIGN);
			new (m_Sessions[i]) st_SESSION();
		}

		//-----------------------------------------------------------------------
		// 10. 인덱스 세팅
		//-----------------------------------------------------------------------
		for (int i = m_SessionMax; i > 0; i--)
		{
			m_Indexes.Push(i - 1);
		}

		//-----------------------------------------------------------------------
		// 11. Start
		//-----------------------------------------------------------------------
		LOG(L"System", eLOG_LEVEL_SYSTEM,
			L"Start (IP:%s / Port:%d / WorkerThread:%d / ActiveThread:%d / Session:%d / Nalge:%d / Zerocopy:%d)\n",
			m_BindIP, m_BindPort, m_NumberOfIOCPWorkerThread, m_NumberOfIOCPActiveThread, m_SessionMax, m_bNagleFlag, m_bZeroCopyFlag
		);

		//-----------------------------------------------------------------------
		// 1. IOCP 생성
		//-----------------------------------------------------------------------
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, m_NumberOfIOCPActiveThread);

		//-----------------------------------------------------------------------
		// 2. AccpetThread 가동
		//-----------------------------------------------------------------------
		m_hAcceptThread = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, this, 0, nullptr);

		//-----------------------------------------------------------------------
		// 3. WorkerThread 가동
		//-----------------------------------------------------------------------
		for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
		{
			m_hIOCPWorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, IOCPWorkerThread, this, 0, nullptr);
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// Network I/O 정지
	//////////////////////////////////////////////////////////////////////
	void CLanServer::Stop(int StopCode)
	{
		switch (StopCode)
		{
		case 1:

			//--------------------------------------------------------
			// 1. Accept 쓰레드 종료
			//--------------------------------------------------------
			closesocket(m_ListenSocket);
			WaitForSingleObject(m_hAcceptThread, INFINITE);
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Accpet Thread Exit Completed\n");

			//--------------------------------------------------------
			// 2. 세션 연결 끊기
			//--------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				Disconnect(m_Sessions[i]);
			}
			while ((m_SessionMax - m_Indexes.GetCapacity()) != 0) {};
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"All Session Out Complete\n");

			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Stop Completed\n");

			break;
		case 2:

			//--------------------------------------------------------
			// 워커스레드 종료
			//--------------------------------------------------------
			PostQueuedCompletionStatus(m_hIOCP, en_THREAD_EXIT, (ULONG_PTR)nullptr, nullptr);
			WaitForMultipleObjects(m_NumberOfIOCPWorkerThread, m_hIOCPWorkerThreads, TRUE, INFINITE);
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"All Worker Thread Exit Completed\n");

			//--------------------------------------------------------
			// 리소스 정리
			//--------------------------------------------------------
			CloseHandle(m_hIOCP);
			CloseHandle(m_hAcceptThread);
			for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
			{
				CloseHandle(m_hIOCPWorkerThreads[i]);
			}

			//-----------------------------------------------------------------------
			// 세션 메모리 반환
			//-----------------------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				m_Sessions[i]->~st_SESSION();
				_aligned_free(m_Sessions[i]);
			}

			//-----------------------------------------------------------------------
			// WinSock 정리
			//-----------------------------------------------------------------------
			WSACleanup();

			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Quit Completed\n");

			break;
		case 3: 

			//--------------------------------------------------------
			// 1. Accept 쓰레드 종료
			//--------------------------------------------------------
			closesocket(m_ListenSocket);
			WaitForSingleObject(m_hAcceptThread, INFINITE);
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Accpet Thread Exit Completed\n");

			//--------------------------------------------------------
			// 2. 세션 연결 끊기
			//--------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				Disconnect(m_Sessions[i]);
			}
			while ((m_SessionMax - m_Indexes.GetCapacity()) != 0) {};
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"All Session Out Complete\n");

			//--------------------------------------------------------
			// 워커스레드 종료
			//--------------------------------------------------------
			PostQueuedCompletionStatus(m_hIOCP, en_THREAD_EXIT, (ULONG_PTR)nullptr, nullptr);
			WaitForMultipleObjects(m_NumberOfIOCPWorkerThread, m_hIOCPWorkerThreads, TRUE, INFINITE);
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"All Worker Thread Exit Completed\n");

			//--------------------------------------------------------
			// 리소스 정리
			//--------------------------------------------------------
			CloseHandle(m_hIOCP);
			CloseHandle(m_hAcceptThread);
			for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
			{
				CloseHandle(m_hIOCPWorkerThreads[i]);
			}

			//-----------------------------------------------------------------------
			// 세션 메모리 반환
			//-----------------------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				m_Sessions[i]->~st_SESSION();
				_aligned_free(m_Sessions[i]);
			}

			//-----------------------------------------------------------------------
			// WinSock 정리
			//-----------------------------------------------------------------------
			WSACleanup();

			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Quit Completed\n");

			break;
		}
	}

	//////////////////////////////////////////////////////////////////////
	// Network I/O 재시작
	//////////////////////////////////////////////////////////////////////
	bool CLanServer::Restart(void)
	{
		SOCKADDR_IN ServerAddr;
		InetPton(AF_INET, m_BindIP, (PVOID)&ServerAddr.sin_addr);
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(m_BindPort);

		//-----------------------------------------------------------------------
		// 1. ListenSocket 생성
		//-----------------------------------------------------------------------
		m_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == m_ListenSocket)
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to create ListenSocket (error:%ld)\n", WSAGetLastError());
			OnError(en_ERROR_WINSOCK_LISTEN_SOCK);

			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"ListenSocket Creation Successful\n");

		//-----------------------------------------------------------------------
		// 2. 네이글 알고리즘 설정
		//-----------------------------------------------------------------------
		if (!m_bNagleFlag)
		{
			bool bFlag = true;
			if (SOCKET_ERROR == setsockopt(m_ListenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&bFlag, sizeof(bFlag)))
			{
				LOG(L"System", eLOG_LEVEL_ERROR, L"Nalge Setup Failed (error:%ld)\n", WSAGetLastError());
				OnError(en_ERROR_WINSOCK_SET_NAGLE);

				return false;
			}
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"Nalge Setup Successful\n");

		//-----------------------------------------------------------------------
		// KeepAlive 설정 
		//-----------------------------------------------------------------------
		if (m_bKeepAliveFlag)
		{
			DWORD dwResult;
			tcp_keepalive KeepAlive;
			KeepAlive.onoff = 1;
			KeepAlive.keepalivetime = 10000;
			KeepAlive.keepaliveinterval = 1000;
			if (0 != WSAIoctl(m_ListenSocket, SIO_KEEPALIVE_VALS, &KeepAlive, sizeof(KeepAlive), 0, 0, &dwResult, NULL, NULL))
			{
				LOG(L"System", eLOG_LEVEL_ERROR, L"KeepAlive Setup Failed (error:%ld)\n", WSAGetLastError());
				return false;
			}
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"KeepAlive Setup Successful\n");
		}

		//-----------------------------------------------------------------------
		// 3. 비동기 출력 설정
		//-----------------------------------------------------------------------
		if (m_bZeroCopyFlag)
		{
			int iSize = 0;
			if (SOCKET_ERROR == setsockopt(m_ListenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&iSize, sizeof(iSize)))
			{
				LOG(L"System", eLOG_LEVEL_ERROR, L"ZeroCopy Setup Failed (error:%ld)\n", WSAGetLastError());
				OnError(en_ERROR_WINSOCK_SET_NAGLE);

				return false;
			}
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"ZeroCopy Setup Successful\n");

		//-----------------------------------------------------------------------
		// 4. RST 송신 설정
		//-----------------------------------------------------------------------
		LINGER Linger;
		Linger.l_onoff = 1;
		Linger.l_linger = 0;
		if (SOCKET_ERROR == setsockopt(m_ListenSocket, SOL_SOCKET, SO_LINGER, (const char*)&Linger, sizeof(Linger)))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to Linger setsockpot (error:%ld)\n", WSAGetLastError());
			OnError(en_ERROR_WINSOCK_SET_RST);

			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"Linger Setup Successful\n");

		//-----------------------------------------------------------------------
		// 5. Bind
		//-----------------------------------------------------------------------
		if (SOCKET_ERROR == bind(m_ListenSocket, (const sockaddr*)&ServerAddr, sizeof(ServerAddr)))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"bind failed (error:%ld)\n", WSAGetLastError());
			OnError(en_ERROR_WINSOCK_BIND);

			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"bind Successful\n");

		//-----------------------------------------------------------------------
		// 6. Listen
		//-----------------------------------------------------------------------
		if (SOCKET_ERROR == listen(m_ListenSocket, SOMAXCONN))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"listen failed(error:%ld)\n", WSAGetLastError());
			OnError(en_ERROR_WINSOCK_LISTEN_SOCK);

			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"listen Successful\n");

		//-----------------------------------------------------------------------
		// 10. Restart
		//-----------------------------------------------------------------------
		LOG(L"System", eLOG_LEVEL_SYSTEM,
			L"Restart (IP:%s / Port:%d / WorkerThread:%d / ActiveThread:%d / Session:%d / Nalge:%d / Zerocopy:%d)\n",
			m_BindIP, m_BindPort, m_NumberOfIOCPWorkerThread, m_NumberOfIOCPActiveThread, m_SessionMax, m_bNagleFlag, m_bZeroCopyFlag
		);

		//-----------------------------------------------------------------------
		// AccpetThread 재가동
		//-----------------------------------------------------------------------
		m_hAcceptThread = (HANDLE)_beginthreadex(nullptr, 0, this->AcceptThread, this, 0, nullptr);

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// 패킷 송신
	//////////////////////////////////////////////////////////////////////
	bool CLanServer::SendPacket(const SESSION_ID SessionID, CPacket* pPacket)
	{
		st_SESSION* pSession = m_Sessions[GET_SESSION_INDEX(SessionID)];
		InterlockedIncrement16(&pSession->IOCount);

		//-----------------------------------------------------------
		// ReleaseFlag를 확인하지 않으면, 이미 끊어진 세션의 SessionID가
		// 메모리에 남아있으므로, 해당 SessionID를 캐치하고 진행될 수 있음.
		//-----------------------------------------------------------
		if ((bool)pSession->bReleaseFlag == true)
		{
			if (0 == InterlockedDecrement16(&pSession->IOCount))
			{
				ReleaseSession(pSession);
			}
			return false;
		}

		//-----------------------------------------------------------
		// SessionID를 먼저 확인할 경우, Release 이후 NewSession에서
		// ReleaseFlag가 false가 되는 상황을 캐치하고 진행될 수 있음.
		//-----------------------------------------------------------
		if (pSession->SessionID != SessionID)
		{
			if (0 == InterlockedDecrement16(&pSession->IOCount))
			{
				ReleaseSession(pSession);
			}
			return false;
		}

		pPacket->SetLanHeader();
		pPacket->AddRef();
		pSession->SendQ.Enqueue(pPacket);

		if (m_bSelfSendPostFlag)
		{
			SendPost(pSession);

			if (0 == InterlockedDecrement16(&pSession->IOCount))
			{
				ReleaseSession(pSession);
			}
		}
		else
		{
			PostQueuedCompletionStatus(m_hIOCP, en_SEND_PACKET, (ULONG_PTR)pSession, nullptr);
		}

		return true;
	}

	/////////////////////////////////////////////////////////////////////////
	// 세션 연결 종료
	// 
	/////////////////////////////////////////////////////////////////////////
	bool CLanServer::Disconnect(const SESSION_ID SessionID)
	{
		st_SESSION* pSession = m_Sessions[GET_SESSION_INDEX(SessionID)];
		InterlockedIncrement16(&pSession->IOCount);

		//-----------------------------------------------------------
		// ReleaseFlag를 확인하지 않으면, 이미 끊어진 세션의 SessionID가
		// 메모리에 남아있으므로, 해당 SessionID를 캐치하고 진행될 수 있음.
		//-----------------------------------------------------------
		if ((bool)pSession->bReleaseFlag == true)
		{
			if (0 == InterlockedDecrement16(&pSession->IOCount))
			{
				ReleaseSession(pSession);
			}
			return false;
		}

		//-----------------------------------------------------------
		// SessionID를 먼저 확인할 경우, Release 이후 NewSession에서
		// ReleaseFlag가 false가 되는 상황을 캐치하고 진행될 수 있음.
		//-----------------------------------------------------------
		if (pSession->SessionID != SessionID)
		{
			if (0 == InterlockedDecrement16(&pSession->IOCount))
			{
				ReleaseSession(pSession);
			}
			return false;
		}

		pSession->bDisconnectFlag = true;
		CancelIoEx((HANDLE)pSession->Socket, NULL);

		if (0 == InterlockedDecrement16(&pSession->IOCount))
		{
			ReleaseSession(pSession);
		}

		return true;
	}

	/////////////////////////////////////////////////////////////////////////
	// 세션수 출력
	/////////////////////////////////////////////////////////////////////////
	const int CLanServer::GetSessionCount(void)
	{
		return m_SessionMax - m_Indexes.GetCapacity();
	}

	/////////////////////////////////////////////////////////////////////////
	// 인덱스수 출력
	/////////////////////////////////////////////////////////////////////////
	const int CLanServer::GetIndexesCount(void)
	{
		return m_Indexes.GetCapacity();
	}

	/////////////////////////////////////////////////////////////////////////
	// 세션 생성
	/////////////////////////////////////////////////////////////////////////
	CLanServer::st_SESSION* CLanServer::NewSession(const SOCKET ClientSocket)
	{
		SOCKADDR_IN	ClientAddr;
		int addrLen = sizeof(ClientAddr);
		getpeername(ClientSocket, (sockaddr*)&ClientAddr, &addrLen);

		SESSION_INDEX SessionIndex;
		if (!m_Indexes.Pop(&SessionIndex))
		{
			return nullptr;	// 신규 세션 생성 불가능
		}

		st_SESSION* pSession = m_Sessions[SessionIndex];

		InterlockedIncrement16(&pSession->IOCount);

		SESSION_ID SessionID = m_UniqueKey++;
		SessionID <<= 16;
		SessionID |= SessionIndex;

		pSession->SessionID = SessionID;
		pSession->SessionIndex = SessionIndex;
		pSession->RecvQ.Clear();
		pSession->bSendFlag = false;
		pSession->bDisconnectFlag = false;
		pSession->PacketCount = 0;

		pSession->Socket = ClientSocket;
		pSession->Port = htons(ClientAddr.sin_port);
		InetNtop(AF_INET, (const void*)&ClientAddr.sin_addr, pSession->IP, 16);

		CreateIoCompletionPort((HANDLE)pSession->Socket, m_hIOCP, (ULONG_PTR)pSession, 0);

		pSession->bReleaseFlag = false;

		return pSession;
	}

	/////////////////////////////////////////////////////////////////////////
	// 세션 정리
	/////////////////////////////////////////////////////////////////////////
	bool CLanServer::ReleaseSession(st_SESSION* pSession)
	{
		//--------------------------------------------------------
		// 초기화 된 적이 없는 세션이 인덱스 스택에 반환되는 것을 방지.
		//--------------------------------------------------------
		if (pSession->SessionIndex == df_INVALID_SESSION_INDEX)
		{
			return false;
		}

		//--------------------------------------------------------
		// 이미 다른 곳에서 Release 중이었으면 리턴
		//--------------------------------------------------------
		if (true == (bool)InterlockedCompareExchange((long*)&pSession->bReleaseFlag, 0x00000001, false))
		{
			return false;
		}

		SESSION_ID SessionID = pSession->SessionID;

		//--------------------------------------------------------
		// 1. Enqueue만하고 SendFlag에 막힌 상태로 Release로 들어와서 
		//    Dequeue하지 못한 패킷 정리
		//--------------------------------------------------------
		CPacket* pPacket;
		while (pSession->SendQ.GetCapacity() != 0)
		{
			pSession->SendQ.Dequeue(&pPacket);
			pPacket->SubRef();
		}

		//--------------------------------------------------------
		// 2. WSASend 실패로 인해 보내지 못한 패킷 정리
		//--------------------------------------------------------
		for (int i = 0; i < pSession->PacketCount; i++)
		{
			pSession->Packets[i]->SubRef();
		}

		//--------------------------------------------------------
		// 3. 소켓 리소스 반환 후 RST 송신
		//--------------------------------------------------------
		closesocket(pSession->Socket);

		//--------------------------------------------------------
		// 4. 세션 인덱스 반환
		//--------------------------------------------------------
		m_Indexes.Push(pSession->SessionIndex);

		//--------------------------------------------------------
		// L7 계층에 세션 종료에 대한 통보
		//--------------------------------------------------------
		OnClientLeave(SessionID);

		InterlockedIncrement(&m_DisconnectTPS);
		InterlockedIncrement(&m_DisconnectTotal);

		return true;
	}

	/////////////////////////////////////////////////////////////////////////
	// 세션 연결 종료 (내부용)
	/////////////////////////////////////////////////////////////////////////
	bool CLanServer::Disconnect(st_SESSION* pSession)
	{
		InterlockedIncrement16(&pSession->IOCount);

		if ((bool)pSession->bReleaseFlag == true)
		{
			if (0 == InterlockedDecrement16(&pSession->IOCount))
			{
				ReleaseSession(pSession);
			}
			return false;
		}

		pSession->bDisconnectFlag = true;
		CancelIoEx((HANDLE)pSession->Socket, NULL);

		if (0 == InterlockedDecrement16(&pSession->IOCount))
		{
			ReleaseSession(pSession);
		}
		return true;
	}

	/////////////////////////////////////////////////////////////////////////
	// WSARecv 등록
	/////////////////////////////////////////////////////////////////////////
	void CLanServer::RecvPost(st_SESSION* pSession)
	{
		DWORD	dwFlags = 0;
		WSABUF	Buffers[2];
		int		iBufferCount;
		int		iLen1;
		int		iLen2;

		//--------------------------------------------------------
		// 1. 사용 가능 수신 버퍼 사이즈 얻기
		//--------------------------------------------------------
		iLen1 = pSession->RecvQ.GetNonBrokenPutSize();
		iLen2 = pSession->RecvQ.GetFreeSize();

		//--------------------------------------------------------
		// 2. 기본 수신 버퍼 등록
		//--------------------------------------------------------
		Buffers[0].buf = pSession->RecvQ.GetWritePos();
		Buffers[0].len = iLen1;
		iBufferCount = 1;

		//--------------------------------------------------------
		// 3. 수신 버퍼에 공간이 남아있다면 추가 등록
		//--------------------------------------------------------
		if (iLen2 > iLen1)
		{
			Buffers[1].buf = pSession->RecvQ.GetBufferPtr();
			Buffers[1].len = iLen2 - iLen1;
			iBufferCount = 2;
		}

		ZeroMemory(&pSession->RecvOverlapped.Overlapped, sizeof(OVERLAPPED));
		InterlockedIncrement16(&pSession->IOCount);

		if (SOCKET_ERROR == WSARecv(pSession->Socket, Buffers, iBufferCount, NULL, &dwFlags, &pSession->RecvOverlapped.Overlapped, NULL))
		{
			errno_t err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				if (err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAECONNRESET && err != WSAESHUTDOWN)
				{
					//LOG(L"Error", eLOG_LEVEL_DEBUG, L"### WSARecv error (%d) - SessionID:%d | SessionIndex:%d | IP:%s | Port:%d\n", err, pSession->SessionID, pSession->SessionIndex, pSession->IP, pSession->Port);
				}

				if (0 == InterlockedDecrement16(&pSession->IOCount))
				{
					ReleaseSession(pSession);
				}
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// WSASend 등록
	/////////////////////////////////////////////////////////////////////////
	bool CLanServer::SendPost(st_SESSION* pSession)
	{
		do
		{
			if (true == (bool)InterlockedExchange8((CHAR*)&pSession->bSendFlag, true))
			{
				return false;
			}

			if (pSession->SendQ.GetCapacity() == 0)
			{
				pSession->bSendFlag = false;

				if (pSession->SendQ.GetCapacity() != 0)
				{
					continue;
				}
				return false;
			}

			CPacket* pPacket;
			WSABUF	 Buffers[en_SEND_PACKET_MAX];
			int		 iBufferCount = 0;
			int		 iPacketSize = 0;
			int		 iPacketCount = 0;

			while (pSession->SendQ.GetCapacity() != 0)
			{
				pSession->SendQ.Dequeue(&pPacket);
				pSession->Packets[iBufferCount] = pPacket;
				Buffers[iBufferCount].buf = pPacket->GetLanHeaderPtr();
				Buffers[iBufferCount].len = pPacket->GetLanPacketSize();

				iPacketSize += Buffers[iBufferCount].len;
				iBufferCount++;

				if (iBufferCount == en_SEND_PACKET_MAX)
				{
					break;
				}
			}

			iPacketCount = iPacketSize / df_MSS;
			InterlockedAdd(&m_ExpSendBytes, (df_TCP_HEADER_SIZE + df_IP_HEADER_SIZE + df_MAC_HEADER_SIZE) * iPacketCount + iPacketSize);
			pSession->PacketCount = iBufferCount;

			ZeroMemory(&pSession->SendOverlapped.Overlapped, sizeof(OVERLAPPED));
			InterlockedIncrement16(&pSession->IOCount);

			int iResult = WSASend(pSession->Socket, Buffers, iBufferCount, NULL, 0, &pSession->SendOverlapped.Overlapped, NULL);
			errno_t err = WSAGetLastError();

			  
			if (SOCKET_ERROR == iResult)
			{
				if (err != ERROR_IO_PENDING)
				{
					if (err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAECONNRESET && err != WSAESHUTDOWN)
					{
						//LOG(L"Error", eLOG_LEVEL_DEBUG, L"### WSASend error (%d) - SessionID:%d | SessionIndex:%d | IP:%s | Port:%d\n", err, pSession->SessionID, pSession->SessionIndex, pSession->IP, pSession->Port);
					}

					if (0 == InterlockedDecrement16(&pSession->IOCount))
					{
						ReleaseSession(pSession);
					}
					return false;
				}
			}

			return true;

		} while (true);
	}

	/////////////////////////////////////////////////////////////////////////
	// 수신 완료 처리
	/////////////////////////////////////////////////////////////////////////
	void CLanServer::RecvProc(st_SESSION* pSession, const DWORD dwTransferred)
	{
		int iResult;

		//--------------------------------------------------------
		// 수신 받은 만큼 링버퍼 포인터 이동
		//--------------------------------------------------------
		pSession->RecvQ.MoveWritePos(dwTransferred);

		//--------------------------------------------------------
		// 패킷 처리 시작
		//--------------------------------------------------------
		while (true)
		{
			CPacket::st_PACKET_LAN Packet;

			//--------------------------------------------------------
			// 1. 수신 버퍼에 헤더는 왔는가?
			//--------------------------------------------------------
			int iUseSize = pSession->RecvQ.GetUseSize();
			if (iUseSize < sizeof(CPacket::st_PACKET_LAN::Len))
			{
				break;
			}

			//--------------------------------------------------------
			// 2. 수신 버퍼에서 헤더 Peek
			//--------------------------------------------------------
			pSession->RecvQ.Peek((char*)&Packet, sizeof(CPacket::st_PACKET_LAN::Len));

			//--------------------------------------------------------
			// 3. 수신 버퍼에 패킷이 완성됐는가?
			//--------------------------------------------------------
			if (iUseSize < (sizeof(CPacket::st_PACKET_LAN::Len) + Packet.Len))
			{
				break;
			}

			//--------------------------------------------------------
			// 4. 완성된 메세지라면 수신 버퍼에서 헤더 삭제
			//--------------------------------------------------------
			pSession->RecvQ.MoveReadPos(sizeof(CPacket::st_PACKET_LAN::Len));
			InterlockedIncrement(&m_RecvPacketTPS);

			//--------------------------------------------------------
			// 5. 페이로드를 수신 버퍼에서 직렬화 버퍼로 복사
			//--------------------------------------------------------
			CPacket* pRecvPacket = CPacket::Alloc();
			iResult = pSession->RecvQ.Get(pRecvPacket->GetWritePos(), Packet.Len);
			pRecvPacket->MoveWritePos(iResult);

			//--------------------------------------------------------
			// 6. 컨텐츠에 전달
			//--------------------------------------------------------
			OnRecv(pSession->SessionID, pRecvPacket);
			pRecvPacket->SubRef();
		}

		RecvPost(pSession);
	}

	/////////////////////////////////////////////////////////////////////////
	// 송신 완료 처리
	/////////////////////////////////////////////////////////////////////////
	void CLanServer::SendProc(st_SESSION* pSession, const DWORD dwTransferred)
	{
		InterlockedAdd(&m_SendPacketTPS, pSession->PacketCount);
		OnSend(pSession->SessionID, dwTransferred);

		//--------------------------------------------------------
		// 송신 완료 패킷 반환 유도
		//--------------------------------------------------------
		int iCompletedCount = pSession->PacketCount;
		for (int i = 0; i < iCompletedCount; i++)
		{
			pSession->Packets[i]->SubRef();

			//--------------------------------------------------------
			// 여기서 Count를 차감해주지 않으면 ReleaseSession에서 
			// 하나의 패킷에 대해 SubRef를 여러 번 호출하게 된다.
			//--------------------------------------------------------
			pSession->PacketCount--;
		}

		pSession->bSendFlag = false;
		SendPost(pSession);
	}

	/////////////////////////////////////////////////////////////////////////
	// AccpetThread
	// 
	/////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall CLanServer::AcceptThread(void* lpParam)
	{
		return ((CLanServer*)lpParam)->AcceptThread_Procedure();
	}
	int CLanServer::AcceptThread_Procedure(void)
	{
		SOCKET		ClientSocket;
		SOCKADDR_IN	ClientAddr;
		int			addrlen = sizeof(ClientAddr);

		while (true)
		{
			ClientSocket = accept(m_ListenSocket, (sockaddr*)&ClientAddr, &addrlen);

			if (INVALID_SOCKET == ClientSocket)
			{
				errno_t err = WSAGetLastError();
				if (err != WSAEINTR)
				{
					LOG(L"Error", eLOG_LEVEL_ERROR, L"ListenSocket error:%d\n", err);
					OnError(en_ERROR_WINSOCK_LISTEN_SOCK);
				}

				return 0;
			}

			//--------------------------------------------------------
			// 초과 연결 처리
			//--------------------------------------------------------
			st_SESSION* pSession = NewSession(ClientSocket);
			if (nullptr == pSession)
			{
				closesocket(ClientSocket);
				OnError(en_ERROR_WINSOCK_OVER_CONNECT);
				continue;
			}

			OnClientJoin(pSession->SessionID);
			RecvPost(pSession);

			//--------------------------------------------------------
			// 세션 생성시 올린 IOCount 차감
			//--------------------------------------------------------
			if (0 == InterlockedDecrement16(&pSession->IOCount))
			{
				ReleaseSession(pSession);
			}

			m_AcceptTPS++;
			m_AcceptTotal++;
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// IOCP Worker Thread
	// 
	/////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall CLanServer::IOCPWorkerThread(void* lpParam)
	{
		return ((CLanServer*)lpParam)->IOCPWorkerThread_Procedure();
	}
	int CLanServer::IOCPWorkerThread_Procedure(void)
	{
		DWORD		dwTransferred;
		OVERLAPPED* pOverlapped;
		st_SESSION* pSession;

		while (true)
		{
			dwTransferred = 0;
			pOverlapped = nullptr;
			pSession = nullptr;

			GetQueuedCompletionStatus(m_hIOCP, &dwTransferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
			OnWorkerThreadBegin();
			PRO_BEGIN(L"WorkerThread");

			//--------------------------------------------------------
			// 1. WorkerTherad 종료
			//--------------------------------------------------------
			if (dwTransferred == en_THREAD_EXIT && pSession == nullptr && pOverlapped == nullptr)
			{
				PostQueuedCompletionStatus(m_hIOCP, en_THREAD_EXIT, (ULONG_PTR)nullptr, nullptr);
				OnWorkerThreadEnd();
				PRO_END(L"IOCPWorkerThread");
				return 0;
			}

			switch (dwTransferred)
			{
			//--------------------------------------------------------
			// 2. 송수신 실패
			//--------------------------------------------------------
			case en_IO_FAILED:
				if (!pSession->bDisconnectFlag)
				{
					Disconnect(pSession->SessionID);
				}

				if (0 == InterlockedDecrement16(&pSession->IOCount))
				{
					ReleaseSession(pSession);
				}
				break;

				//--------------------------------------------------------
				// 3. SendPacket
				//--------------------------------------------------------
			case (DWORD)en_SEND_PACKET:
				SendPost(pSession);

				// SendPacket 함수에서 증가시킨 IOCount 차감
				if (0 == InterlockedDecrement16(&pSession->IOCount))
				{
					ReleaseSession(pSession);
				}
				break;

				//--------------------------------------------------------
				// 4. I/O 완료 처리
				//--------------------------------------------------------
			default:
				switch (((st_OVERLAPPED*)pOverlapped)->Type)
				{
				case st_OVERLAPPED::en_RECV:
					if (!pSession->bDisconnectFlag)
					{
						RecvProc(pSession, dwTransferred);
					}
					break;

				case st_OVERLAPPED::en_SEND:
					if (!pSession->bDisconnectFlag)
					{
						SendProc(pSession, dwTransferred);
					}
					break;
				}

				//--------------------------------------------------------
				// Disconnect 했는데 이미 등록된 경우 강제 종료
				//--------------------------------------------------------
				if (pSession->bDisconnectFlag)
				{
					CancelIoEx((HANDLE)pSession->Socket, NULL);
				}

				// 이전 I/O 등록시 물고있는 IOCount 차감
				if (0 == InterlockedDecrement16(&pSession->IOCount))
				{
					ReleaseSession(pSession);
				}

				break;
			}

			OnWorkerThreadEnd();
			PRO_END(L"WorkerThread");
		}

		return 0;
	}
}

