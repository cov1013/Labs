#include "CMMOServer.h"

namespace cov1013
{
	///////////////////////////////////////////////////////////////////////////////////
	// 생성자
	///////////////////////////////////////////////////////////////////////////////////
	CMMOServer::CMMOServer()
	{
		m_AuthPlayer = 0;
		m_GamePlayer = 0;
		m_AuthFPS = 0;
		m_GameFPS = 0;
		m_SendFPS = 0;
		m_AcceptTPS	= 0;
		m_AcceptTotal= 0;
		m_DisconnectTotal = 0;
		m_DisconnectTPS = 0;
		m_ExpSendBytes = 0;
		m_RecvPacketTPS = 0;
		m_SendPacketTPS = 0;
		m_bLoop = FALSE;
		m_UniqueKey = 1;

		timeBeginPeriod(1);

		//------------------------------------------------
		// 크래시 덤프 초기화
		//------------------------------------------------
		CCrashDump::CCrashDump();

		//------------------------------------------------
		// 시스템 로거 초기화
		//------------------------------------------------
		CLogger::CLogger(L"../Logs", eLOG_LEVEL_DEBUG);

		//------------------------------------------------
		// 프로파일러 초기화
		//------------------------------------------------
		InitializeProfiler(L"../Profiling", en_PROFILER_UNIT::eUNIT_MILLI);

		//------------------------------------------------
		// WSA 초기화
		//------------------------------------------------
		WSADATA	wsa;
		if (0 != WSAStartup(MAKEWORD(2, 2), &wsa))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to initialization Winsock (error:%ld)\n", WSAGetLastError());
			return;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"Winsock initialization Successful\n");
	}


	///////////////////////////////////////////////////////////////////////////////////
	// 소멸자
	///////////////////////////////////////////////////////////////////////////////////
	CMMOServer::~CMMOServer()
	{
		WSACleanup();
	}


	///////////////////////////////////////////////////////////////////////////////////
	// 서버 시작
	///////////////////////////////////////////////////////////////////////////////////
	bool CMMOServer::Start(
		const wchar_t*			BindIP,
		const unsigned short	BindPort,
		const unsigned int		NumberOfIOCPWorkerThread,
		const unsigned int		NumberOfIOCPActiveThread,
		const unsigned int		SessionMax,
		const unsigned int		PacketSizeMax,
		const bool				bNagleFlag,
		const bool				bZeroCopyFlag,
		const bool				bKeepAliveFlag,
		const bool				bDirectSendFlag
	)
	{
		SOCKADDR_IN ServerAddr;
		InetPton(AF_INET, BindIP, (PVOID)&ServerAddr.sin_addr);
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(BindPort);

		//-----------------------------------------------------------------------
		// 1. 생성할 워커스레드 개수 체크
		//-----------------------------------------------------------------------
		if (NumberOfIOCPWorkerThread > en_WORKER_THREAD_MAX)
		{
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Create Worker Thread Num Over (%d > %d)", NumberOfIOCPWorkerThread, en_WORKER_THREAD_MAX);
			return false;
		}

		//-----------------------------------------------------------------------
		// 2. 생성할 세션 체크
		//-----------------------------------------------------------------------
		if (SessionMax > en_SESSION_MAX)
		{
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Create Session Over (%d > %d)", SessionMax, en_SESSION_MAX);
			return false;
		}

		//-----------------------------------------------------------------------
		// 3. ListenSocket 생성
		//-----------------------------------------------------------------------
		m_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == m_ListenSocket)
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to create ListenSocket (error:%ld)\n", WSAGetLastError());
			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"ListenSocket Creation Successful\n");

		//-----------------------------------------------------------------------
		// 4. Nagle 설정
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
		// 5. KeepAlive 설정 (테스트용)
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
		// 6. ZeroCopy Send 설정
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
		// 7. RST 설정
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
		// 8. Listen
		//-----------------------------------------------------------------------
		if (SOCKET_ERROR == listen(m_ListenSocket, SOMAXCONN))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"listen failed(error:%ld)\n", WSAGetLastError());
			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"listen Successful\n");

		//-----------------------------------------------------------------------
		// 9. 라이브러리 설정 정보 세팅
		//-----------------------------------------------------------------------
		m_NumberOfIOCPWorkerThread	= NumberOfIOCPWorkerThread;
		m_NumberOfIOCPActiveThread	= NumberOfIOCPActiveThread;
		m_SessionMax				= SessionMax;
		m_BindPort					= BindPort;
		m_bZeroCopyFlag				= bZeroCopyFlag;
		m_bNagleFlag				= bNagleFlag;
		m_bKeepAliveFlag			= bKeepAliveFlag;
		m_PacketSizeMax				= PacketSizeMax;
		m_bDirectSendFlag			= bDirectSendFlag;
		wcscpy_s(m_BindIP, 16, BindIP);

		//-----------------------------------------------------------------------
		// 10. 인덱스 세팅
		//-----------------------------------------------------------------------
		for (int i = m_SessionMax; i > 0; i--)
		{
			m_Indexes.Push(i - 1);
		}

		//-----------------------------------------------------------------------
		// 11. IOCP 생성
		//-----------------------------------------------------------------------
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, m_NumberOfIOCPActiveThread);

		//-----------------------------------------------------------------------
		// 12. IOCP WorkerThread 생성
		//-----------------------------------------------------------------------
		for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
		{
			m_hIOCPWorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, this->IOCPWorkerThread, this, 0, nullptr);
		}

		m_bLoop = TRUE;

		//-----------------------------------------------------------------------
		// 13. AcceptThread 생성
		//-----------------------------------------------------------------------
		m_hAcceptThread = (HANDLE)_beginthreadex(nullptr, 0, this->AcceptThread, this, 0, nullptr);

		//-----------------------------------------------------------------------
		// 14. AuthThread 생성
		//-----------------------------------------------------------------------
		m_hAuthThread = (HANDLE)_beginthreadex(nullptr, 0, this->AuthThread, this, 0, nullptr);

		//-----------------------------------------------------------------------
		// 15. GameThread 생성
		//-----------------------------------------------------------------------
		m_hGameThread = (HANDLE)_beginthreadex(nullptr, 0, this->GameThread, this, 0, nullptr);

		//-----------------------------------------------------------------------
		// 16. SendThread 생성
		//-----------------------------------------------------------------------
		m_hSendThread = (HANDLE)_beginthreadex(nullptr, 0, this->SendThread, this, 0, nullptr);

		//-----------------------------------------------------------------------
		// 18. Start
		//-----------------------------------------------------------------------
		LOG(L"System", eLOG_LEVEL_SYSTEM,
			L"Start (IP:%s / Port:%d / WorkerThread:%d / ActiveThread:%d / Session:%d / Nalge:%d / Zerocopy:%d)\n",
			m_BindIP, m_BindPort, m_NumberOfIOCPWorkerThread, m_NumberOfIOCPActiveThread, m_SessionMax, m_bNagleFlag, m_bZeroCopyFlag
		);

		return TRUE;
	}


	///////////////////////////////////////////////////////////////////////////////////
	// 서버 정지
	// 
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::Stop(en_STOP_TYPE eType)
	{
		st_CONNECT_INFO connectInfo;

		switch (eType)
		{
		case en_STOP_TYPE::STOP:
			//--------------------------------------------------------
			// 1. Accept 쓰레드 종료
			//--------------------------------------------------------
			closesocket(m_ListenSocket);
			WaitForSingleObject(m_hAcceptThread, INFINITE);
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Accpet Thread Exit Completed\n");

			//--------------------------------------------------------
			// 2. AcceptSockets 비우기
			//--------------------------------------------------------
			while (m_AcceptSockets.GetCapacity() != 0)
			{
				m_AcceptSockets.Dequeue(&connectInfo);
			}

			//--------------------------------------------------------
			// 3. 전체 세션 연결 끊기
			//--------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				Disconnect(m_Sessions[i]);
			}

			//--------------------------------------------------------
			// 4. 전체 세션이 ReleaseSession으로 인해 반환될 때 까지 무한 대기
			//--------------------------------------------------------
			while ((m_SessionMax - m_Indexes.GetCapacity()) != 0) {};

			//--------------------------------------------------------
			// 5. Thread 동작 플래그 OFF
			//--------------------------------------------------------
			m_bLoop = FALSE;

			//--------------------------------------------------------
			// 6. AuthThread 종료
			//--------------------------------------------------------
			WaitForSingleObject(m_hAuthThread, INFINITE);

			//--------------------------------------------------------
			// 7. GameThread 종료
			//--------------------------------------------------------
			WaitForSingleObject(m_hGameThread, INFINITE);

			//--------------------------------------------------------
			// 8. SendThread 종료
			//--------------------------------------------------------
			WaitForSingleObject(m_hSendThread, INFINITE);

			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Stop Completed\n");
			break;

		case en_STOP_TYPE::RESOURCE_RELEASE:

			//--------------------------------------------------------
			// 9. IOCPWorkerThread 종료
			//--------------------------------------------------------
			PostQueuedCompletionStatus(m_hIOCP, 0, (ULONG_PTR)NULL, NULL);
			WaitForMultipleObjects(m_NumberOfIOCPWorkerThread, m_hIOCPWorkerThreads, TRUE, INFINITE);

			//--------------------------------------------------------
			// 10. OS 리소스 반환
			//--------------------------------------------------------
			CloseHandle(m_hIOCP);
			for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
			{
				CloseHandle(m_hIOCPWorkerThreads[i]);
			}
			CloseHandle(m_hAcceptThread);
			CloseHandle(m_hAuthThread);
			CloseHandle(m_hGameThread);
			CloseHandle(m_hSendThread);
			WSACleanup();

			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Quit Completed\n");
			break;

		case en_STOP_TYPE::QUIT:
			//--------------------------------------------------------
			// 1. Accept 쓰레드 종료
			//--------------------------------------------------------
			closesocket(m_ListenSocket);
			WaitForSingleObject(m_hAcceptThread, INFINITE);
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Accpet Thread Exit Completed\n");

			//--------------------------------------------------------
			// 2. AcceptSockets 비우기
			//--------------------------------------------------------
			while (m_AcceptSockets.GetCapacity() != 0)
			{
				m_AcceptSockets.Dequeue(&connectInfo);
			}

			//--------------------------------------------------------
			// 3. 전체 세션 연결 끊기
			//--------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				Disconnect(m_Sessions[i]);
			}

			//--------------------------------------------------------
			// 4. 전체 세션이 ReleaseSession으로 인해 반환될 때 까지 무한 대기
			//--------------------------------------------------------
			while ((m_SessionMax - m_Indexes.GetCapacity()) != 0) {};

			//--------------------------------------------------------
			// 5. Thread 동작 플래그 OFF
			//--------------------------------------------------------
			m_bLoop = FALSE;

			//--------------------------------------------------------
			// 6. AuthThread 종료
			//--------------------------------------------------------
			WaitForSingleObject(m_hAuthThread, INFINITE);

			//--------------------------------------------------------
			// 7. GameThread 종료
			//--------------------------------------------------------
			WaitForSingleObject(m_hGameThread, INFINITE);

			//--------------------------------------------------------
			// 8. SendThread 종료
			//--------------------------------------------------------
			WaitForSingleObject(m_hSendThread, INFINITE);

			//--------------------------------------------------------
			// 9. IOCPWorkerThread 종료
			//--------------------------------------------------------
			PostQueuedCompletionStatus(m_hIOCP, 0, (ULONG_PTR)NULL, NULL);
			WaitForMultipleObjects(m_NumberOfIOCPWorkerThread, m_hIOCPWorkerThreads, TRUE, INFINITE);

			//--------------------------------------------------------
			// 10. OS 리소스 반환
			//--------------------------------------------------------
			CloseHandle(m_hIOCP);
			for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
			{
				CloseHandle(m_hIOCPWorkerThreads[i]);
			}
			CloseHandle(m_hAcceptThread);
			CloseHandle(m_hAuthThread);
			CloseHandle(m_hGameThread);
			CloseHandle(m_hSendThread);
			WSACleanup();

			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Quit Completed\n");
			break;

		default:
			CCrashDump::Crash();
			break;
		}
	}


	///////////////////////////////////////////////////////////////////////////////////
	// 서버 재시작
	// 
	///////////////////////////////////////////////////////////////////////////////////
	bool CMMOServer::Restart(void)
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

				return false;
			}
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"Nalge Setup Successful\n");

		//-----------------------------------------------------------------------
		// 3. KeepAlive 설정 
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
		// 4. ZeroCopy 설정
		//-----------------------------------------------------------------------
		if (m_bZeroCopyFlag)
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
		// 5. RST 송신 설정
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
		// 6. Bind
		//-----------------------------------------------------------------------
		if (SOCKET_ERROR == bind(m_ListenSocket, (const sockaddr*)&ServerAddr, sizeof(ServerAddr)))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"bind failed (error:%ld)\n", WSAGetLastError());

			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"bind Successful\n");

		//-----------------------------------------------------------------------
		// 7. Listen
		//-----------------------------------------------------------------------
		if (SOCKET_ERROR == listen(m_ListenSocket, SOMAXCONN))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"listen failed(error:%ld)\n", WSAGetLastError());

			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"listen Successful\n");

		//-----------------------------------------------------------------------
		// 8. Thread 재가동
		//-----------------------------------------------------------------------
		m_bLoop = TRUE;
		m_hAcceptThread = (HANDLE)_beginthreadex(nullptr, 0, this->AcceptThread, this, 0, nullptr);
		m_hAuthThread = (HANDLE)_beginthreadex(nullptr, 0, this->AuthThread, this, 0, nullptr);
		m_hGameThread = (HANDLE)_beginthreadex(nullptr, 0, this->GameThread, this, 0, nullptr);
		m_hSendThread = (HANDLE)_beginthreadex(nullptr, 0, this->SendThread, this, 0, nullptr);

		//-----------------------------------------------------------------------
		// 9. Restart
		//-----------------------------------------------------------------------
		LOG(L"System", eLOG_LEVEL_SYSTEM,
			L"Restart (IP:%s / Port:%d / WorkerThread:%d / ActiveThread:%d / Session:%d / Nalge:%d / Zerocopy:%d)\n",
			m_BindIP, m_BindPort, m_NumberOfIOCPWorkerThread, m_NumberOfIOCPActiveThread, m_SessionMax, m_bNagleFlag, m_bZeroCopyFlag
		);

		return true;
	}


	///////////////////////////////////////////////////////////////////////////////////
	// 패킷 송신
	// 
	///////////////////////////////////////////////////////////////////////////////////
	bool CMMOServer::SendPacket(const SESSION_ID SessionID, CPacket* pSendPacket)
	{
		// TODO
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 세션 세팅
	// 
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::SetSession(const int iIndex, CNetSession* pSession)
	{
		m_Sessions[iIndex] = pSession;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 플레이어 상태 수치 얻기
	// 
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::GetPlayerStatusCount(void)
	{
		en_THREAD_MODE eThreadMode;

		for (int i = 0; i < m_SessionMax; i++)
		{
			eThreadMode = m_Sessions[i]->m_eThreadMode;

			if (eThreadMode == en_THREAD_MODE_AUTH_IN)
			{
				m_AuthPlayer++;
				continue;
			}

			if (eThreadMode == en_THREAD_MODE_GAME_IN)
			{
				m_GamePlayer++;
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 세션 수 반환
	// 
	///////////////////////////////////////////////////////////////////////////////////
	const int CMMOServer::GetSessionCount(void)
	{
		return m_SessionMax - m_Indexes.GetCapacity();
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 인덱스 개수 반환
	// 
	///////////////////////////////////////////////////////////////////////////////////
	const int CMMOServer::GetIndexesCount(void)
	{
		return m_Indexes.GetCapacity();
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 새로운 세션 생성
	// 
	///////////////////////////////////////////////////////////////////////////////////
	CNetSession* CMMOServer::NewSession(const st_CONNECT_INFO ConnectInfo)
	{
		SESSION_INDEX	SessionIndex;
		SESSION_ID		SessionID;
		CNetSession*	pSession;

		if (!m_Indexes.Pop(&SessionIndex))
		{
			return NULL;	// 신규 접속 불가능.
		}

		//-----------------------------------------------------------
		// SessionID 생성
		//-----------------------------------------------------------
		SessionID = m_UniqueKey++;
		SessionID <<= 16;
		SessionID |= SessionIndex;

		pSession = m_Sessions[SessionIndex];

		if (pSession->m_eThreadMode != en_THREAD_MODE_NONE)
		{
			CCrashDump::Crash();
		}

		if (pSession->m_IOCount != 0)
		{
			CCrashDump::Crash();
		}

		if (pSession->m_SendPacketCount != 0)
		{
			CCrashDump::Crash();
		}

		pSession->m_bExitSocketFlag = FALSE;
		pSession->m_bSendFlag		= FALSE;
		pSession->m_bDisconnectFlag = FALSE;
		pSession->m_bToGameFlag		= FALSE;

		pSession->m_SessionID		= SessionID;
		pSession->m_SessionIndex	= SessionIndex;

		pSession->m_SendPacketCount = 0;
		pSession->m_RecvBuffer.Clear();

		pSession->m_Socket = ConnectInfo.Socket;
		wcscpy_s(pSession->m_IP, 16, ConnectInfo.IP);
		pSession->m_Port = htons(ConnectInfo.Port);
		CreateIoCompletionPort((HANDLE)pSession->m_Socket, m_hIOCP, (ULONG_PTR)pSession, 0);

		return pSession;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 세션 정리
	// 
	///////////////////////////////////////////////////////////////////////////////////
	bool CMMOServer::ReleaseSession(CNetSession* pSession)
	{
		CPacket* pDregsPacket = NULL;

		//-----------------------------------------------------------
		// 1. 해당 세션을 각 스레드의 처리 대상에서 제외시킨다.
		//-----------------------------------------------------------
		pSession->m_eThreadMode = en_THREAD_MODE_NONE;

		//-----------------------------------------------------------
		// 2. SendFlag에 막혀 보내지못하고 모아두었던 패킷 정리
		//-----------------------------------------------------------
		while (pSession->m_SendBuffer.GetCapacity() != 0)
		{
			pSession->m_SendBuffer.Dequeue(&pDregsPacket);
			pDregsPacket->SubRef();
		}

		//-----------------------------------------------------------
		// 3. 완성된 수신 패킷 중 처리하지 못한 패킷 정리
		//-----------------------------------------------------------
		while (pSession->m_CompleteRecvPacket.GetCapacity() != 0)
		{
			pSession->m_CompleteRecvPacket.Dequeue(&pDregsPacket);
			pDregsPacket->SubRef();
		}

		//-----------------------------------------------------------
		// 4. WSASend 함수 실패로 인해 보내지 못한 패킷 정리
		//-----------------------------------------------------------
		int iCount = pSession->m_SendPacketCount;
		for (int i = 0; i < iCount; i++)
		{
			pSession->m_SendPacketArray[i]->SubRef();
			pSession->m_SendPacketCount--;
		}

		//-----------------------------------------------------------
		// 5. 소켓 반환
		//-----------------------------------------------------------
		closesocket(pSession->m_Socket);

		//-----------------------------------------------------------
		// 6. 인덱스 반환
		//-----------------------------------------------------------
		m_Indexes.Push(pSession->m_SessionIndex);

		m_DisconnectTPS++;
		m_DisconnectTotal++;

		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 세션 연결 끊기
	// 
	///////////////////////////////////////////////////////////////////////////////////
	bool CMMOServer::Disconnect(CNetSession* pSession)
	{
		pSession->m_bDisconnectFlag = TRUE;
		CancelIoEx((HANDLE)pSession->m_Socket, NULL);

		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 수신 등록
	// 
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::RecvPost(CNetSession* pSession)
	{
		DWORD	dwFlags = 0;
		WSABUF	Buffers[2];
		int		iBufferCount;
		int		iLen1;
		int		iLen2;

		iLen1 = pSession->m_RecvBuffer.GetNonBrokenPutSize();
		iLen2 = pSession->m_RecvBuffer.GetFreeSize();

		Buffers[0].buf = pSession->m_RecvBuffer.GetWritePos();
		Buffers[0].len = iLen1;
		iBufferCount = 1;

		if (iLen2 > iLen1)
		{
			Buffers[1].buf = pSession->m_RecvBuffer.GetBufferPtr();
			Buffers[1].len = iLen2 - iLen1;
			iBufferCount = 2;
		}

		ZeroMemory(&pSession->m_RecvOverlapped.Overlapped, sizeof(WSAOVERLAPPED));
		InterlockedIncrement16(&pSession->m_IOCount);

		int iResult = WSARecv(pSession->m_Socket, Buffers, iBufferCount, NULL, &dwFlags, &pSession->m_RecvOverlapped.Overlapped, NULL);
		errno_t err = WSAGetLastError();

		if (SOCKET_ERROR == iResult)
		{
			if (err != ERROR_IO_PENDING)
			{
				if (err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAECONNRESET && err != WSAESHUTDOWN)
				{
					// Log
				}

				if (0 == InterlockedDecrement16(&pSession->m_IOCount))
				{
					pSession->m_bExitSocketFlag = TRUE;
				}
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 송신 등록
	// 
	///////////////////////////////////////////////////////////////////////////////////
	bool CMMOServer::SendPost(CNetSession* pSession)
	{
		CPacket*	pPacket;
		WSABUF		Buffers[eSEND_PACKET_MAX];
		int			iBufferCount = 0;
		int			iTotalPacketSize = 0;

		if (pSession->m_SendBuffer.GetCapacity() == 0)
		{
			pSession->m_bSendFlag = FALSE;
			return FALSE;
		}

		while (pSession->m_SendBuffer.GetCapacity() != 0)
		{
			pSession->m_SendBuffer.Dequeue(&pPacket);
			pSession->m_SendPacketArray[iBufferCount] = pPacket;
			pPacket->Encode();

			Buffers[iBufferCount].buf = pPacket->GetBufferPtr();
			Buffers[iBufferCount].len = pPacket->GetPacketSize();

			iTotalPacketSize += Buffers[iBufferCount].len;
			iBufferCount++;

			if (iBufferCount == eSEND_PACKET_MAX)
			{
				break;
			}
		}
		pSession->m_SendPacketCount = iBufferCount;
		
		int iTotalPacketCount = iTotalPacketSize / df_MSS;
		m_ExpSendBytes += ((df_TCP_HEADER_SIZE + df_IP_HEADER_SIZE + df_MAC_HEADER_SIZE) * iTotalPacketCount) + iTotalPacketSize;

		ZeroMemory(&pSession->m_SendOverlapped.Overlapped, sizeof(WSAOVERLAPPED));
		InterlockedIncrement16(&pSession->m_IOCount);

		//PRO_BEGIN(L"SendPost");
		int iReulst = WSASend(pSession->m_Socket, Buffers, iBufferCount, NULL, 0, &pSession->m_SendOverlapped.Overlapped, NULL);
		errno_t err = WSAGetLastError();
		//PRO_END(L"SendPost");

		if (SOCKET_ERROR == iReulst)
		{
			if (err != ERROR_IO_PENDING)
			{
				if (err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAECONNRESET && err != WSAESHUTDOWN)
				{
					// Log
				}

				if (0 == InterlockedDecrement16(&pSession->m_IOCount))
				{
					pSession->m_bExitSocketFlag = TRUE;
				}
				return FALSE;
			}
		}
		return TRUE;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 수신 완료 프로시저
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::RecvProc(CNetSession* pSession, const DWORD dwTransferred)
	{
		int iResult;
		pSession->m_RecvBuffer.MoveWritePos(dwTransferred);

		while (true)
		{
			//--------------------------------------------------------
			// 1. 수신 버퍼에 헤더는 왔는가?
			//--------------------------------------------------------
			int iUseSize = pSession->m_RecvBuffer.GetUseSize();
			if (iUseSize < CPacket::eHEADER_LEN)
			{
				break;
			}

			//--------------------------------------------------------
			// 2. 수신 버퍼에서 헤더 Peek
			//--------------------------------------------------------
			CPacket::st_PACKET_HEADER Header;
			pSession->m_RecvBuffer.Peek((char*)&Header, CPacket::eHEADER_LEN);

			//--------------------------------------------------------
			// 3. 패킷 Len 확인
			// 
			// 지정한 패킷 사이즈보다 큰 패킷일 경우, 잘못된 세션으로 인식하고 끊는다.
			//--------------------------------------------------------
			if (Header.Len > m_PacketSizeMax)
			{
				//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Packet Length Over - (SessionID:%I64u)\n", pSession->SessionID);
				pSession->Disconnect();
				return;
			}

			//--------------------------------------------------------
			// 4. 수신 버퍼에 패킷이 완성되어있는가?
			//--------------------------------------------------------
			if (iUseSize < (CPacket::eHEADER_LEN + Header.Len))
			{
				break;
			}

			//--------------------------------------------------------
			// 5. 완성된 패킷이라면 직렬화 버퍼로 옮긴다.
			//--------------------------------------------------------
			CPacket* pRecvPacket = CPacket::Alloc();
			iResult = pSession->m_RecvBuffer.Get(pRecvPacket->GetBufferPtr(), CPacket::eHEADER_LEN + Header.Len);
			pRecvPacket->MoveWritePos(iResult - CPacket::eHEADER_LEN);

			//--------------------------------------------------------
			// 6. 디코딩
			//--------------------------------------------------------
			if (pRecvPacket->Decode() == true)
			{
				InterlockedIncrement(&m_RecvPacketTPS);

				//--------------------------------------------------------
				// 여기서 올린 패킷의 참조카운트는 각 스레드에서 패킷 처리 후 차감
				//--------------------------------------------------------
				pRecvPacket->AddRef();
				pSession->m_CompleteRecvPacket.Enqueue(pRecvPacket);
			}
			else
			{
				// OnError();
				CCrashDump::Crash();
				return;
			}
			pRecvPacket->SubRef();
		}

		RecvPost(pSession);
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 송신 완료 프로시저
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::SendProc(CNetSession* pSession, const DWORD dwTransferred)
	{
		InterlockedAdd(&m_SendPacketTPS, pSession->m_SendPacketCount);

		int iCompletedCount = pSession->m_SendPacketCount;
		for (int i = 0; i < iCompletedCount; i++)
		{
			pSession->m_SendPacketArray[i]->SubRef();
			pSession->m_SendPacketCount--;
		}

		pSession->m_bSendFlag = FALSE;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 게임 스레드 로직 처리 
	//
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::OnGame_Update(void)
	{
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 인증 스레드 로직 처리
	//
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::OnAuth_Update(void)
	{
	}

	///////////////////////////////////////////////////////////////////////////////////
	// Accept Thread
	// 
	///////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall CMMOServer::AcceptThread(void* lpParam)
	{
		return ((CMMOServer*)lpParam)->AcceptThread_Procedure();
	}
	int CMMOServer::AcceptThread_Procedure(void)
	{
		st_CONNECT_INFO AcceptInfo;
		SOCKET			ClientSocket;
		SOCKADDR_IN		ClientAddr;
		int				iAddrLen = sizeof(ClientAddr);

		while (true)
		{
			ClientSocket = accept(m_ListenSocket, (SOCKADDR*)&ClientAddr, &iAddrLen);
			if (INVALID_SOCKET == ClientSocket)
			{
				errno_t err = WSAGetLastError();
				if (err != WSAEINTR && err != WSAENOTSOCK)
				{
					LOG(L"Error", eLOG_LEVEL_ERROR, L"ListenSocket error:%d\n", err);
				}
				return 0;
			}

			getpeername(ClientSocket, (SOCKADDR*)&ClientAddr, &iAddrLen);
			InetNtop(AF_INET, (const void*)&ClientAddr.sin_addr, AcceptInfo.IP, 16);

			// 접속 정보 넣고 Enqueue
			AcceptInfo.Port = htons(ClientAddr.sin_port);
			AcceptInfo.Socket = ClientSocket;
			m_AcceptSockets.Enqueue(AcceptInfo);
		}

		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// Auth Thread
	// 
	///////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall CMMOServer::AuthThread(void* lpParam)
	{
		return ((CMMOServer*)lpParam)->AuthThread_Procedure();
	}
	int CMMOServer::AuthThread_Procedure(void)
	{
		st_CONNECT_INFO ConnectInfo;
		CNetSession*	pSession = NULL;
		CPacket*		pRecvPacket;
		en_THREAD_MODE	eThreadMode;
		int				iSessionMax = m_SessionMax;

		int				SleepTime;
		DWORD			dwStartTime;
		DWORD			dwElapsedTime;

		while (m_bLoop)
		{
			dwStartTime = timeGetTime();

			//============================================================================
			// Part 1. 신규 접속자 처리
			// 
			// AccpetSocketQueue에 들어온 정보를 뽑아서 비어있는 세션의 공간에 신규 세션을 할당
			//============================================================================
			int iAccpetCount = 0;
			while (m_AcceptSockets.GetCapacity() != 0)
			{
				//---------------------------------------------------------------------
				// [예외] 프레임 처리 최대치에 도달했는가?
				//---------------------------------------------------------------------
				if (iAccpetCount == en_THREAD_LOGIC_MAX_AUTH_ACCEPT)
				{
					break;
				}

				//---------------------------------------------------------------------
				// 접속 정보 뽑아서 새로운 세션 생성
				//---------------------------------------------------------------------
				m_AcceptSockets.Dequeue(&ConnectInfo);
				pSession = NewSession(ConnectInfo);

				//---------------------------------------------------------------------
				// [예외] 최대 접속자 수를 초과했다면 연결 종료
				//---------------------------------------------------------------------
				if (pSession == NULL)
				{
					closesocket(ConnectInfo.Socket);
					continue;
				}

				//---------------------------------------------------------------------
				// [통과] 인증 처리 시작
				//---------------------------------------------------------------------
				pSession->OnAuth_ClientJoin();
				pSession->m_eThreadMode = en_THREAD_MODE_AUTH_IN;
				RecvPost(pSession);

				m_AcceptTPS++;
				m_AcceptTotal++;
				iAccpetCount++;
			}

			//============================================================================
			// Part 2. AUTH 모드 세션들 패킷 처리
			// 
			// 세션 배열을 모두 돌면서 AUTH 모드 세션을 확인하여 해당 세션의 CompleteRecvPacket을 처리요청
			// * virtaul OnAuth_Packet() 호출 
			// * 실제 처리는 상속자에게 넘김
			// * 기본은 패킷 1개 처리 / 상황에 따라 N개로 조절
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//---------------------------------------------------------------------
				// [예외] 종료 예정 세션인가?
				//---------------------------------------------------------------------
				if (pSession->m_bExitSocketFlag == TRUE)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [예외] AUTH Thread의 처리 대상인가?
				//---------------------------------------------------------------------
				if (pSession->m_eThreadMode != en_THREAD_MODE_AUTH_IN)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [통과] 패킷 처리 시작
				//---------------------------------------------------------------------
				int	iPacketCount = 0;
				while (pSession->m_CompleteRecvPacket.GetCapacity() != 0)
				{
					//---------------------------------------------------------------------
					// [예외] 로그인 완료 패킷을 보낸 상태라면 이후 수신 패킷은 게임 관련 패킷이므로 처리하지 않고 중지.
					//---------------------------------------------------------------------
					if (pSession->m_bToGameFlag == TRUE)
					{
						break;
					}

					//---------------------------------------------------------------------
					// [예외] 최대 처리 패킷 수를 처리했다면 남은 패킷은 다음 프레임에 처리
					//---------------------------------------------------------------------
					if (iPacketCount == en_THREAD_LOGIC_MAX_AUTH_PACKET)
					{
						break;
					}

					pSession->m_CompleteRecvPacket.Dequeue(&pRecvPacket);
					pSession->OnAuth_Packet(pRecvPacket);
					pRecvPacket->SubRef();
					++iPacketCount;
				}
			}

			//============================================================================
			// Part 3. AUTH 모드의 Update
			// 
			// 기본적으로 항시 처리되어야할 컨텐츠 부분 로직
			//============================================================================
			OnAuth_Update();

			//============================================================================
			// Part 4. AUTH 모드의 소켓종료 처리
			// 
			// 세션 배열을 돌면서 AUTH 모드에서 소켓 종료 Flag가 true인 세션을 [GAME 종료단계모드] 모드로 변경한다.
			// 
			// RecvPost 등록 실패시 Release 되어야하는 세션 처리
			// 
			// * 이는 세션 릴리즈 처리의 1단계.
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//---------------------------------------------------------------------
				// [예외] 종료 예정 세션이 아니라면 스킵
				//---------------------------------------------------------------------
				if (pSession->m_bExitSocketFlag == FALSE)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [예외] AUTH Thread 처리 대상이 아니라면 스킵
				//---------------------------------------------------------------------
				eThreadMode = pSession->m_eThreadMode;
				if (eThreadMode != en_THREAD_MODE_AUTH_IN && eThreadMode != en_THREAD_MODE_GAME_IN_READY)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [통과] Release 준비 단계로 변환
				//---------------------------------------------------------------------
				pSession->m_eThreadMode = en_THREAD_MODE_AUTH_OUT_READY;
			}

			//============================================================================
			// Part 5. AUTH에서 GAME 모드 전환
			// 
			// 처리 세션 배열을 돌면서 ToGame 변수가 true이며, AUTH 모드인 세션을 [게임스레드 진입] 모드로 변경.
			// 
			// * ToGameFlag는 컨텐츠 쪽에서 변경 요청해준다.
			// * virtual OnAuth_ClientLeave() 호출
			//============================================================================
			int iChangeCount = 0;
			for (int i = 0; i < iSessionMax; i++)
			{
				//---------------------------------------------------------------------
				// [예외] 게임 스레드에 넘길 세션 수 초과시 다음 프레임에서 처리한다.
				//---------------------------------------------------------------------
				if (iChangeCount == en_THREAD_LOGIC_MAX_AUTH_TO_GAME)
				{
					break;
				}

				pSession = m_Sessions[i];

				//---------------------------------------------------------------------
				// [예외] ToGameFlag가 꺼져있다면 스킵
				//---------------------------------------------------------------------
				if (pSession->m_bToGameFlag == FALSE)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [예외] 처리 대상이 아니라면 스킵
				//---------------------------------------------------------------------
				if (pSession->m_eThreadMode != en_THREAD_MODE_AUTH_IN)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [통과] Game Thread 준비 단계로 전환
				//---------------------------------------------------------------------
				pSession->m_eThreadMode = en_THREAD_MODE_GAME_IN_READY;
				pSession->OnAuth_ClientLeave();
				++iChangeCount;
			}

			//---------------------------------------------------------------------
			// Sleep Time 계산
			//---------------------------------------------------------------------
			dwElapsedTime = timeGetTime() - dwStartTime;
			SleepTime = en_THREAD_FREAM_MAX_AUTH - dwElapsedTime;
			if (SleepTime < 0)
			{
				SleepTime = 0;
			}

			m_AuthFPS++;
			Sleep(SleepTime);
		}

		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// SendThread
	// 
	///////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall CMMOServer::SendThread(void* lpParam)
	{
		return ((CMMOServer*)lpParam)->SendThread_Procedure();
	}
	int CMMOServer::SendThread_Procedure(void)
	{
		CNetSession*	pSession;
		en_THREAD_MODE	eThreadMode;
		int				SleepTime;
		int				iSessionMax = m_SessionMax;
		DWORD			dwStartTime;
		DWORD			dwElapsedTime;

		while (m_bLoop)
		{
			dwStartTime = timeGetTime();

			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				eThreadMode = pSession->m_eThreadMode;
				
				//---------------------------------------------------------------------
				// [예외] 스레드 처리 대상이 아니라면 스킵
				//---------------------------------------------------------------------
				if (eThreadMode == en_THREAD_MODE_NONE)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [예외] 종료 대상 세션인가?
				// -> SendThread에서 릴리즈 준비 단계로 만들어놨지만, GameThread에서 해당 세션에 대한 Release를 하지 않은 경우.
				//---------------------------------------------------------------------
				if (eThreadMode == en_THREAD_MODE_RELEASE_READY_COM)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [예외] Release 준비중인 세션인가?
				// -> 세션의 상태가 Release 준비중이라는 건, IOCount가 0이라는 뜻. 즉, 등록되어있는 I/O는 없다는 뜻
				// -> SendFlag는 NewSession에서 FALSE로 갱신해줌.
				//---------------------------------------------------------------------
				if (eThreadMode == en_THREAD_MODE_AUTH_OUT_READY || eThreadMode == en_THREAD_MODE_GAME_OUT_READY)
				{
					pSession->m_eThreadMode = en_THREAD_MODE_RELEASE_READY_COM;
					continue;
				}

				//---------------------------------------------------------------------
				// [예외] 이미 송신중인가?
				//---------------------------------------------------------------------
				if (pSession->m_bSendFlag == TRUE)
				{
					continue;
				}

				pSession->m_bSendFlag = TRUE;

				//---------------------------------------------------------------------
				// [예외] SendThread의 처리 대상인가?
				//---------------------------------------------------------------------
				if (eThreadMode != en_THREAD_MODE_AUTH_IN && eThreadMode != en_THREAD_MODE_GAME_IN_READY && eThreadMode != en_THREAD_MODE_GAME_IN )
				{
					pSession->m_bSendFlag = FALSE;
					continue;
				}

				//---------------------------------------------------------------------
				// [통과] Send 등록
				//---------------------------------------------------------------------
				SendPost(pSession);
			}

			dwElapsedTime = timeGetTime() - dwStartTime;
			SleepTime = en_THREAD_FREAM_MAX_SEND - dwElapsedTime;
			if (SleepTime < 0)
			{
				SleepTime = 0;
			}

			m_SendFPS++;
			Sleep(SleepTime);
		}

		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// IOCP Worker Thread
	// 
	///////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall CMMOServer::IOCPWorkerThread(void* lpParam)
	{
		return ((CMMOServer*)lpParam)->IOCPWorkerThread_Procedure();
	}
	int CMMOServer::IOCPWorkerThread_Procedure(void)
	{
		DWORD			dwTransferred;
		WSAOVERLAPPED*	pOverlapped;
		CNetSession*	pSession;

		while (true)
		{
			dwTransferred	= 0;
			pOverlapped		= NULL;
			pSession		= NULL;
			 
			GetQueuedCompletionStatus(m_hIOCP, &dwTransferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);

			//PRO_BEGIN(L"IOCP_WorkerThread");

			//------------------------------------------------------
			// [예외] IOCP WorkerThread 종료
			//------------------------------------------------------
			if (dwTransferred == 0 && pOverlapped == NULL && pSession == NULL)
			{
				PostQueuedCompletionStatus(m_hIOCP, 0, (ULONG_PTR)NULL, NULL);
				//PRO_END(L"IOCP_WorkerThread");

				return 0;
			}

			//------------------------------------------------------
			// [예외] 클라이언트가 연결을 끊었는가?
			//------------------------------------------------------
			if (dwTransferred == 0 && pOverlapped != NULL)
			{
				//---------------------------------------------------------------------
				// [예외] DisconnectFlag가 False인 경우, 바로 끊어주기 위해 Disconnect
				//---------------------------------------------------------------------
				if (!pSession->m_bDisconnectFlag)
				{
					pSession->Disconnect();
				}

				//---------------------------------------------------------------------
				// 이전 스레드에서 등록할 때 올린 IOCount 차감
				//---------------------------------------------------------------------
				if (0 == InterlockedDecrement16(&pSession->m_IOCount))
				{
					pSession->m_bExitSocketFlag = TRUE;
				}
			}
			//------------------------------------------------------
			// [통과] I/O 완료 처리
			//------------------------------------------------------
			else
			{
				switch (((CNetSession::st_OVERLAPPED*)pOverlapped)->Type)
				{
				//------------------------------------------------------
				// Recv 완료
				//------------------------------------------------------
				case CNetSession::st_OVERLAPPED::en_RECV:
					if (!pSession->m_bDisconnectFlag)
					{
						RecvProc(pSession, dwTransferred);
					}
					break;

				//------------------------------------------------------
				// Send 완료
				//------------------------------------------------------
				case CNetSession::st_OVERLAPPED::en_SEND:
					if (!pSession->m_bDisconnectFlag)
					{
						SendProc(pSession, dwTransferred);
					}
					break;
				}

				//--------------------------------------------------------
				// [예외] DisconnectFlag를 무시하고 등록된 경우 다시 Cancel
				//--------------------------------------------------------
				if (pSession->m_bDisconnectFlag)
				{
					CancelIoEx((HANDLE)pSession->m_Socket, NULL);
				}

				//------------------------------------------------------
				// 이전 I/O 등록시 물고있는 IOCount 차감
				//------------------------------------------------------
				if (0 == InterlockedDecrement16(&pSession->m_IOCount))
				{
					pSession->m_bExitSocketFlag = TRUE;
				}
			}

			//PRO_END(L"IOCP_WorkerThread");
		}

		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// Game Thread
	// 
	///////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall CMMOServer::GameThread(void* lpParam)
	{
		return ((CMMOServer*)lpParam)->GameThread_Procedure();
	}
	int CMMOServer::GameThread_Procedure(void)
	{
		CNetSession*	pSession;
		int				iSessionMax = m_SessionMax;
		int				SleepTime;
		DWORD			dwStartTime;
		DWORD			dwElapsedTime;

		while (m_bLoop)
		{
			dwStartTime = timeGetTime();

			//============================================================================
			// Part 1. Game 모드로 전환
			// 
			// 세션 배열을 돌면서 TO_GAME 모드의 세션을 GAME 모드로 변경한다.
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//------------------------------------------------------
				// [예외] 종료 예정 세션이라면 스킵
				//------------------------------------------------------
				if (pSession->m_bExitSocketFlag == TRUE)
				{
					continue;
				}

				//------------------------------------------------------
				// [예외] en_THREAD_MODE_GAME_IN_READY 상태가 아니면 스킵
				//------------------------------------------------------
				if (pSession->m_eThreadMode != en_THREAD_MODE_GAME_IN_READY)
				{
					continue;
				}

				//------------------------------------------------------
				// [오류체크] ToGameFlag가 꺼져있을 수 없음.
				//------------------------------------------------------
				if (pSession->m_bToGameFlag != TRUE)
				{
					CCrashDump::Crash();
				}

				//------------------------------------------------------
				// [통과] GameThread 처리 대상으로 전환
				//------------------------------------------------------
				pSession->OnGame_ClientJoin();
				pSession->m_eThreadMode = en_THREAD_MODE_GAME_IN;
			}

			//============================================================================
			// Part 2. GAME 모드 세션들 패킷 처리 세션
			// 
			// 배열을 모두 돌면서 GAME 모드 세션을 확인하여 해당 세션의 CompleteRecvPacket을 처리 요청
			// 
			// * virtual OnGame_Packet() 호출
			// * 실제 처리는 상속자에게 넘김
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//------------------------------------------------------
				// [예외] 종료 예정 세션이라면 스킵
				//------------------------------------------------------
				if (pSession->m_bExitSocketFlag == TRUE)
				{
					continue;
				}

				//------------------------------------------------------
				// [예외] GameThread의 처리 대상이 아니라면 스킵
				//------------------------------------------------------
				if (pSession->m_eThreadMode != en_THREAD_MODE_GAME_IN)
				{
					continue;
				}

				int			iPacketCount = 0;
				CPacket*	pRecvPacket = NULL;

				while (pSession->m_CompleteRecvPacket.GetCapacity() != 0)
				{
					if (iPacketCount == en_THREAD_LOGIC_MAX_GAME_PACKET)
					{
						break;
					}

					pSession->m_CompleteRecvPacket.Dequeue(&pRecvPacket);
					pSession->OnGame_Packet(pRecvPacket);
					pRecvPacket->SubRef();
					++iPacketCount;
				}
			}

			//============================================================================
			// Part 3. GAME 모드의 Update
			// 
			// 메시지와 별도로 항시 처리되어야 할 게임 컨텐츠 부분 로직
			//============================================================================
			OnGame_Update();

			//============================================================================
			// Part 4. GAME 모드의 소켓종료 처리
			// 
			// 세션 배열을 돌면서 GAME 모드에서 소켓 종료 Flag가 true인 세션을 [GAME 종료단계모드] 모드로 변경한다.
			// 
			// * 이는 세션 릴리즈 처리의 1단계.
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//------------------------------------------------------
				// [예외] 종료 예정 세션이 아니라면 스킵
				//------------------------------------------------------
				if (pSession->m_bExitSocketFlag == FALSE)
				{
					continue;
				}

				//------------------------------------------------------
				// [예외] GameThread의 처리 대상이 아니라면 스킵
				//------------------------------------------------------
				if (pSession->m_eThreadMode != en_THREAD_MODE_GAME_IN)
				{
					continue;
				}

				//------------------------------------------------------
				// [통과] Release 준비 단계로 변경
				//------------------------------------------------------
				pSession->m_eThreadMode = en_THREAD_MODE_GAME_OUT_READY;
			}

			//============================================================================
			//Part 5. [GAME 종료단계모드] 릴리즈 단계돌입
			//
			// 세션 배열을 돌면서 [GAME 종료준비완료] 세션을 찾아 정리한다.
			//
			//* virtual OnGame_ClientLeave() 호출
			//* virtual OnGame_ClientRelease() 호출
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//------------------------------------------------------
				// [예외] Release 완료 상태가 아니라면 스킵
				//------------------------------------------------------
				if (pSession->m_eThreadMode != en_THREAD_MODE_RELEASE_READY_COM)
				{
					continue;
				}

				//------------------------------------------------------
				// [통과] Release 처리 시작
				//------------------------------------------------------
				pSession->OnGame_ClientLeave();
				pSession->OnGame_ClientRelease();
				ReleaseSession(pSession);
			}

			//------------------------------------------------------
			// Sleep Time 계산
			//------------------------------------------------------
			dwElapsedTime = timeGetTime() - dwStartTime;
			SleepTime = en_THREAD_FREAM_MAX_GAME - dwElapsedTime;
			if (SleepTime < 0)
			{
				SleepTime = 0;
			}
			m_GameFPS++;
			Sleep(SleepTime); 
		}

		return 0;
	}
}