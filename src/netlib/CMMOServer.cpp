#include "CMMOServer.h"

namespace cov1013
{
	///////////////////////////////////////////////////////////////////////////////////
	// ������
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
		// ũ���� ���� �ʱ�ȭ
		//------------------------------------------------
		CCrashDump::CCrashDump();

		//------------------------------------------------
		// �ý��� �ΰ� �ʱ�ȭ
		//------------------------------------------------
		CLogger::CLogger(L"../Logs", eLOG_LEVEL_DEBUG);

		//------------------------------------------------
		// �������Ϸ� �ʱ�ȭ
		//------------------------------------------------
		InitializeProfiler(L"../Profiling", en_PROFILER_UNIT::eUNIT_MILLI);

		//------------------------------------------------
		// WSA �ʱ�ȭ
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
	// �Ҹ���
	///////////////////////////////////////////////////////////////////////////////////
	CMMOServer::~CMMOServer()
	{
		WSACleanup();
	}


	///////////////////////////////////////////////////////////////////////////////////
	// ���� ����
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
		// 1. ������ ��Ŀ������ ���� üũ
		//-----------------------------------------------------------------------
		if (NumberOfIOCPWorkerThread > en_WORKER_THREAD_MAX)
		{
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Create Worker Thread Num Over (%d > %d)", NumberOfIOCPWorkerThread, en_WORKER_THREAD_MAX);
			return false;
		}

		//-----------------------------------------------------------------------
		// 2. ������ ���� üũ
		//-----------------------------------------------------------------------
		if (SessionMax > en_SESSION_MAX)
		{
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Create Session Over (%d > %d)", SessionMax, en_SESSION_MAX);
			return false;
		}

		//-----------------------------------------------------------------------
		// 3. ListenSocket ����
		//-----------------------------------------------------------------------
		m_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == m_ListenSocket)
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to create ListenSocket (error:%ld)\n", WSAGetLastError());
			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"ListenSocket Creation Successful\n");

		//-----------------------------------------------------------------------
		// 4. Nagle ����
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
		// 5. KeepAlive ���� (�׽�Ʈ��)
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
		// 6. ZeroCopy Send ����
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
		// 7. RST ����
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
		// 9. ���̺귯�� ���� ���� ����
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
		// 10. �ε��� ����
		//-----------------------------------------------------------------------
		for (int i = m_SessionMax; i > 0; i--)
		{
			m_Indexes.Push(i - 1);
		}

		//-----------------------------------------------------------------------
		// 11. IOCP ����
		//-----------------------------------------------------------------------
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, m_NumberOfIOCPActiveThread);

		//-----------------------------------------------------------------------
		// 12. IOCP WorkerThread ����
		//-----------------------------------------------------------------------
		for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
		{
			m_hIOCPWorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, this->IOCPWorkerThread, this, 0, nullptr);
		}

		m_bLoop = TRUE;

		//-----------------------------------------------------------------------
		// 13. AcceptThread ����
		//-----------------------------------------------------------------------
		m_hAcceptThread = (HANDLE)_beginthreadex(nullptr, 0, this->AcceptThread, this, 0, nullptr);

		//-----------------------------------------------------------------------
		// 14. AuthThread ����
		//-----------------------------------------------------------------------
		m_hAuthThread = (HANDLE)_beginthreadex(nullptr, 0, this->AuthThread, this, 0, nullptr);

		//-----------------------------------------------------------------------
		// 15. GameThread ����
		//-----------------------------------------------------------------------
		m_hGameThread = (HANDLE)_beginthreadex(nullptr, 0, this->GameThread, this, 0, nullptr);

		//-----------------------------------------------------------------------
		// 16. SendThread ����
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
	// ���� ����
	// 
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::Stop(en_STOP_TYPE eType)
	{
		st_CONNECT_INFO connectInfo;

		switch (eType)
		{
		case en_STOP_TYPE::STOP:
			//--------------------------------------------------------
			// 1. Accept ������ ����
			//--------------------------------------------------------
			closesocket(m_ListenSocket);
			WaitForSingleObject(m_hAcceptThread, INFINITE);
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Accpet Thread Exit Completed\n");

			//--------------------------------------------------------
			// 2. AcceptSockets ����
			//--------------------------------------------------------
			while (m_AcceptSockets.GetCapacity() != 0)
			{
				m_AcceptSockets.Dequeue(&connectInfo);
			}

			//--------------------------------------------------------
			// 3. ��ü ���� ���� ����
			//--------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				Disconnect(m_Sessions[i]);
			}

			//--------------------------------------------------------
			// 4. ��ü ������ ReleaseSession���� ���� ��ȯ�� �� ���� ���� ���
			//--------------------------------------------------------
			while ((m_SessionMax - m_Indexes.GetCapacity()) != 0) {};

			//--------------------------------------------------------
			// 5. Thread ���� �÷��� OFF
			//--------------------------------------------------------
			m_bLoop = FALSE;

			//--------------------------------------------------------
			// 6. AuthThread ����
			//--------------------------------------------------------
			WaitForSingleObject(m_hAuthThread, INFINITE);

			//--------------------------------------------------------
			// 7. GameThread ����
			//--------------------------------------------------------
			WaitForSingleObject(m_hGameThread, INFINITE);

			//--------------------------------------------------------
			// 8. SendThread ����
			//--------------------------------------------------------
			WaitForSingleObject(m_hSendThread, INFINITE);

			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Stop Completed\n");
			break;

		case en_STOP_TYPE::RESOURCE_RELEASE:

			//--------------------------------------------------------
			// 9. IOCPWorkerThread ����
			//--------------------------------------------------------
			PostQueuedCompletionStatus(m_hIOCP, 0, (ULONG_PTR)NULL, NULL);
			WaitForMultipleObjects(m_NumberOfIOCPWorkerThread, m_hIOCPWorkerThreads, TRUE, INFINITE);

			//--------------------------------------------------------
			// 10. OS ���ҽ� ��ȯ
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
			// 1. Accept ������ ����
			//--------------------------------------------------------
			closesocket(m_ListenSocket);
			WaitForSingleObject(m_hAcceptThread, INFINITE);
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Accpet Thread Exit Completed\n");

			//--------------------------------------------------------
			// 2. AcceptSockets ����
			//--------------------------------------------------------
			while (m_AcceptSockets.GetCapacity() != 0)
			{
				m_AcceptSockets.Dequeue(&connectInfo);
			}

			//--------------------------------------------------------
			// 3. ��ü ���� ���� ����
			//--------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				Disconnect(m_Sessions[i]);
			}

			//--------------------------------------------------------
			// 4. ��ü ������ ReleaseSession���� ���� ��ȯ�� �� ���� ���� ���
			//--------------------------------------------------------
			while ((m_SessionMax - m_Indexes.GetCapacity()) != 0) {};

			//--------------------------------------------------------
			// 5. Thread ���� �÷��� OFF
			//--------------------------------------------------------
			m_bLoop = FALSE;

			//--------------------------------------------------------
			// 6. AuthThread ����
			//--------------------------------------------------------
			WaitForSingleObject(m_hAuthThread, INFINITE);

			//--------------------------------------------------------
			// 7. GameThread ����
			//--------------------------------------------------------
			WaitForSingleObject(m_hGameThread, INFINITE);

			//--------------------------------------------------------
			// 8. SendThread ����
			//--------------------------------------------------------
			WaitForSingleObject(m_hSendThread, INFINITE);

			//--------------------------------------------------------
			// 9. IOCPWorkerThread ����
			//--------------------------------------------------------
			PostQueuedCompletionStatus(m_hIOCP, 0, (ULONG_PTR)NULL, NULL);
			WaitForMultipleObjects(m_NumberOfIOCPWorkerThread, m_hIOCPWorkerThreads, TRUE, INFINITE);

			//--------------------------------------------------------
			// 10. OS ���ҽ� ��ȯ
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
	// ���� �����
	// 
	///////////////////////////////////////////////////////////////////////////////////
	bool CMMOServer::Restart(void)
	{
		SOCKADDR_IN ServerAddr;
		InetPton(AF_INET, m_BindIP, (PVOID)&ServerAddr.sin_addr);
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(m_BindPort);

		//-----------------------------------------------------------------------
		// 1. ListenSocket ����
		//-----------------------------------------------------------------------
		m_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == m_ListenSocket)
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to create ListenSocket (error:%ld)\n", WSAGetLastError());

			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"ListenSocket Creation Successful\n");

		//-----------------------------------------------------------------------
		// 2. ���̱� �˰��� ����
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
		// 3. KeepAlive ���� 
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
		// 4. ZeroCopy ����
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
		// 5. RST �۽� ����
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
		// 8. Thread �簡��
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
	// ��Ŷ �۽�
	// 
	///////////////////////////////////////////////////////////////////////////////////
	bool CMMOServer::SendPacket(const SESSION_ID SessionID, CPacket* pSendPacket)
	{
		// TODO
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// ���� ����
	// 
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::SetSession(const int iIndex, CNetSession* pSession)
	{
		m_Sessions[iIndex] = pSession;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// �÷��̾� ���� ��ġ ���
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
	// ���� �� ��ȯ
	// 
	///////////////////////////////////////////////////////////////////////////////////
	const int CMMOServer::GetSessionCount(void)
	{
		return m_SessionMax - m_Indexes.GetCapacity();
	}

	///////////////////////////////////////////////////////////////////////////////////
	// �ε��� ���� ��ȯ
	// 
	///////////////////////////////////////////////////////////////////////////////////
	const int CMMOServer::GetIndexesCount(void)
	{
		return m_Indexes.GetCapacity();
	}

	///////////////////////////////////////////////////////////////////////////////////
	// ���ο� ���� ����
	// 
	///////////////////////////////////////////////////////////////////////////////////
	CNetSession* CMMOServer::NewSession(const st_CONNECT_INFO ConnectInfo)
	{
		SESSION_INDEX	SessionIndex;
		SESSION_ID		SessionID;
		CNetSession*	pSession;

		if (!m_Indexes.Pop(&SessionIndex))
		{
			return NULL;	// �ű� ���� �Ұ���.
		}

		//-----------------------------------------------------------
		// SessionID ����
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
	// ���� ����
	// 
	///////////////////////////////////////////////////////////////////////////////////
	bool CMMOServer::ReleaseSession(CNetSession* pSession)
	{
		CPacket* pDregsPacket = NULL;

		//-----------------------------------------------------------
		// 1. �ش� ������ �� �������� ó�� ��󿡼� ���ܽ�Ų��.
		//-----------------------------------------------------------
		pSession->m_eThreadMode = en_THREAD_MODE_NONE;

		//-----------------------------------------------------------
		// 2. SendFlag�� ���� ���������ϰ� ��Ƶξ��� ��Ŷ ����
		//-----------------------------------------------------------
		while (pSession->m_SendBuffer.GetCapacity() != 0)
		{
			pSession->m_SendBuffer.Dequeue(&pDregsPacket);
			pDregsPacket->SubRef();
		}

		//-----------------------------------------------------------
		// 3. �ϼ��� ���� ��Ŷ �� ó������ ���� ��Ŷ ����
		//-----------------------------------------------------------
		while (pSession->m_CompleteRecvPacket.GetCapacity() != 0)
		{
			pSession->m_CompleteRecvPacket.Dequeue(&pDregsPacket);
			pDregsPacket->SubRef();
		}

		//-----------------------------------------------------------
		// 4. WSASend �Լ� ���з� ���� ������ ���� ��Ŷ ����
		//-----------------------------------------------------------
		int iCount = pSession->m_SendPacketCount;
		for (int i = 0; i < iCount; i++)
		{
			pSession->m_SendPacketArray[i]->SubRef();
			pSession->m_SendPacketCount--;
		}

		//-----------------------------------------------------------
		// 5. ���� ��ȯ
		//-----------------------------------------------------------
		closesocket(pSession->m_Socket);

		//-----------------------------------------------------------
		// 6. �ε��� ��ȯ
		//-----------------------------------------------------------
		m_Indexes.Push(pSession->m_SessionIndex);

		m_DisconnectTPS++;
		m_DisconnectTotal++;

		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// ���� ���� ����
	// 
	///////////////////////////////////////////////////////////////////////////////////
	bool CMMOServer::Disconnect(CNetSession* pSession)
	{
		pSession->m_bDisconnectFlag = TRUE;
		CancelIoEx((HANDLE)pSession->m_Socket, NULL);

		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// ���� ���
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
	// �۽� ���
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
	// ���� �Ϸ� ���ν���
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::RecvProc(CNetSession* pSession, const DWORD dwTransferred)
	{
		int iResult;
		pSession->m_RecvBuffer.MoveWritePos(dwTransferred);

		while (true)
		{
			//--------------------------------------------------------
			// 1. ���� ���ۿ� ����� �Դ°�?
			//--------------------------------------------------------
			int iUseSize = pSession->m_RecvBuffer.GetUseSize();
			if (iUseSize < CPacket::eHEADER_LEN)
			{
				break;
			}

			//--------------------------------------------------------
			// 2. ���� ���ۿ��� ��� Peek
			//--------------------------------------------------------
			CPacket::st_PACKET_HEADER Header;
			pSession->m_RecvBuffer.Peek((char*)&Header, CPacket::eHEADER_LEN);

			//--------------------------------------------------------
			// 3. ��Ŷ Len Ȯ��
			// 
			// ������ ��Ŷ ������� ū ��Ŷ�� ���, �߸��� �������� �ν��ϰ� ���´�.
			//--------------------------------------------------------
			if (Header.Len > m_PacketSizeMax)
			{
				//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Packet Length Over - (SessionID:%I64u)\n", pSession->SessionID);
				pSession->Disconnect();
				return;
			}

			//--------------------------------------------------------
			// 4. ���� ���ۿ� ��Ŷ�� �ϼ��Ǿ��ִ°�?
			//--------------------------------------------------------
			if (iUseSize < (CPacket::eHEADER_LEN + Header.Len))
			{
				break;
			}

			//--------------------------------------------------------
			// 5. �ϼ��� ��Ŷ�̶�� ����ȭ ���۷� �ű��.
			//--------------------------------------------------------
			CPacket* pRecvPacket = CPacket::Alloc();
			iResult = pSession->m_RecvBuffer.Get(pRecvPacket->GetBufferPtr(), CPacket::eHEADER_LEN + Header.Len);
			pRecvPacket->MoveWritePos(iResult - CPacket::eHEADER_LEN);

			//--------------------------------------------------------
			// 6. ���ڵ�
			//--------------------------------------------------------
			if (pRecvPacket->Decode() == true)
			{
				InterlockedIncrement(&m_RecvPacketTPS);

				//--------------------------------------------------------
				// ���⼭ �ø� ��Ŷ�� ����ī��Ʈ�� �� �����忡�� ��Ŷ ó�� �� ����
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
	// �۽� �Ϸ� ���ν���
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
	// ���� ������ ���� ó�� 
	//
	///////////////////////////////////////////////////////////////////////////////////
	void CMMOServer::OnGame_Update(void)
	{
	}

	///////////////////////////////////////////////////////////////////////////////////
	// ���� ������ ���� ó��
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

			// ���� ���� �ְ� Enqueue
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
			// Part 1. �ű� ������ ó��
			// 
			// AccpetSocketQueue�� ���� ������ �̾Ƽ� ����ִ� ������ ������ �ű� ������ �Ҵ�
			//============================================================================
			int iAccpetCount = 0;
			while (m_AcceptSockets.GetCapacity() != 0)
			{
				//---------------------------------------------------------------------
				// [����] ������ ó�� �ִ�ġ�� �����ߴ°�?
				//---------------------------------------------------------------------
				if (iAccpetCount == en_THREAD_LOGIC_MAX_AUTH_ACCEPT)
				{
					break;
				}

				//---------------------------------------------------------------------
				// ���� ���� �̾Ƽ� ���ο� ���� ����
				//---------------------------------------------------------------------
				m_AcceptSockets.Dequeue(&ConnectInfo);
				pSession = NewSession(ConnectInfo);

				//---------------------------------------------------------------------
				// [����] �ִ� ������ ���� �ʰ��ߴٸ� ���� ����
				//---------------------------------------------------------------------
				if (pSession == NULL)
				{
					closesocket(ConnectInfo.Socket);
					continue;
				}

				//---------------------------------------------------------------------
				// [���] ���� ó�� ����
				//---------------------------------------------------------------------
				pSession->OnAuth_ClientJoin();
				pSession->m_eThreadMode = en_THREAD_MODE_AUTH_IN;
				RecvPost(pSession);

				m_AcceptTPS++;
				m_AcceptTotal++;
				iAccpetCount++;
			}

			//============================================================================
			// Part 2. AUTH ��� ���ǵ� ��Ŷ ó��
			// 
			// ���� �迭�� ��� ���鼭 AUTH ��� ������ Ȯ���Ͽ� �ش� ������ CompleteRecvPacket�� ó����û
			// * virtaul OnAuth_Packet() ȣ�� 
			// * ���� ó���� ����ڿ��� �ѱ�
			// * �⺻�� ��Ŷ 1�� ó�� / ��Ȳ�� ���� N���� ����
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//---------------------------------------------------------------------
				// [����] ���� ���� �����ΰ�?
				//---------------------------------------------------------------------
				if (pSession->m_bExitSocketFlag == TRUE)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [����] AUTH Thread�� ó�� ����ΰ�?
				//---------------------------------------------------------------------
				if (pSession->m_eThreadMode != en_THREAD_MODE_AUTH_IN)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [���] ��Ŷ ó�� ����
				//---------------------------------------------------------------------
				int	iPacketCount = 0;
				while (pSession->m_CompleteRecvPacket.GetCapacity() != 0)
				{
					//---------------------------------------------------------------------
					// [����] �α��� �Ϸ� ��Ŷ�� ���� ���¶�� ���� ���� ��Ŷ�� ���� ���� ��Ŷ�̹Ƿ� ó������ �ʰ� ����.
					//---------------------------------------------------------------------
					if (pSession->m_bToGameFlag == TRUE)
					{
						break;
					}

					//---------------------------------------------------------------------
					// [����] �ִ� ó�� ��Ŷ ���� ó���ߴٸ� ���� ��Ŷ�� ���� �����ӿ� ó��
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
			// Part 3. AUTH ����� Update
			// 
			// �⺻������ �׽� ó���Ǿ���� ������ �κ� ����
			//============================================================================
			OnAuth_Update();

			//============================================================================
			// Part 4. AUTH ����� �������� ó��
			// 
			// ���� �迭�� ���鼭 AUTH ��忡�� ���� ���� Flag�� true�� ������ [GAME ����ܰ���] ���� �����Ѵ�.
			// 
			// RecvPost ��� ���н� Release �Ǿ���ϴ� ���� ó��
			// 
			// * �̴� ���� ������ ó���� 1�ܰ�.
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//---------------------------------------------------------------------
				// [����] ���� ���� ������ �ƴ϶�� ��ŵ
				//---------------------------------------------------------------------
				if (pSession->m_bExitSocketFlag == FALSE)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [����] AUTH Thread ó�� ����� �ƴ϶�� ��ŵ
				//---------------------------------------------------------------------
				eThreadMode = pSession->m_eThreadMode;
				if (eThreadMode != en_THREAD_MODE_AUTH_IN && eThreadMode != en_THREAD_MODE_GAME_IN_READY)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [���] Release �غ� �ܰ�� ��ȯ
				//---------------------------------------------------------------------
				pSession->m_eThreadMode = en_THREAD_MODE_AUTH_OUT_READY;
			}

			//============================================================================
			// Part 5. AUTH���� GAME ��� ��ȯ
			// 
			// ó�� ���� �迭�� ���鼭 ToGame ������ true�̸�, AUTH ����� ������ [���ӽ����� ����] ���� ����.
			// 
			// * ToGameFlag�� ������ �ʿ��� ���� ��û���ش�.
			// * virtual OnAuth_ClientLeave() ȣ��
			//============================================================================
			int iChangeCount = 0;
			for (int i = 0; i < iSessionMax; i++)
			{
				//---------------------------------------------------------------------
				// [����] ���� �����忡 �ѱ� ���� �� �ʰ��� ���� �����ӿ��� ó���Ѵ�.
				//---------------------------------------------------------------------
				if (iChangeCount == en_THREAD_LOGIC_MAX_AUTH_TO_GAME)
				{
					break;
				}

				pSession = m_Sessions[i];

				//---------------------------------------------------------------------
				// [����] ToGameFlag�� �����ִٸ� ��ŵ
				//---------------------------------------------------------------------
				if (pSession->m_bToGameFlag == FALSE)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [����] ó�� ����� �ƴ϶�� ��ŵ
				//---------------------------------------------------------------------
				if (pSession->m_eThreadMode != en_THREAD_MODE_AUTH_IN)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [���] Game Thread �غ� �ܰ�� ��ȯ
				//---------------------------------------------------------------------
				pSession->m_eThreadMode = en_THREAD_MODE_GAME_IN_READY;
				pSession->OnAuth_ClientLeave();
				++iChangeCount;
			}

			//---------------------------------------------------------------------
			// Sleep Time ���
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
				// [����] ������ ó�� ����� �ƴ϶�� ��ŵ
				//---------------------------------------------------------------------
				if (eThreadMode == en_THREAD_MODE_NONE)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [����] ���� ��� �����ΰ�?
				// -> SendThread���� ������ �غ� �ܰ�� ����������, GameThread���� �ش� ���ǿ� ���� Release�� ���� ���� ���.
				//---------------------------------------------------------------------
				if (eThreadMode == en_THREAD_MODE_RELEASE_READY_COM)
				{
					continue;
				}

				//---------------------------------------------------------------------
				// [����] Release �غ����� �����ΰ�?
				// -> ������ ���°� Release �غ����̶�� ��, IOCount�� 0�̶�� ��. ��, ��ϵǾ��ִ� I/O�� ���ٴ� ��
				// -> SendFlag�� NewSession���� FALSE�� ��������.
				//---------------------------------------------------------------------
				if (eThreadMode == en_THREAD_MODE_AUTH_OUT_READY || eThreadMode == en_THREAD_MODE_GAME_OUT_READY)
				{
					pSession->m_eThreadMode = en_THREAD_MODE_RELEASE_READY_COM;
					continue;
				}

				//---------------------------------------------------------------------
				// [����] �̹� �۽����ΰ�?
				//---------------------------------------------------------------------
				if (pSession->m_bSendFlag == TRUE)
				{
					continue;
				}

				pSession->m_bSendFlag = TRUE;

				//---------------------------------------------------------------------
				// [����] SendThread�� ó�� ����ΰ�?
				//---------------------------------------------------------------------
				if (eThreadMode != en_THREAD_MODE_AUTH_IN && eThreadMode != en_THREAD_MODE_GAME_IN_READY && eThreadMode != en_THREAD_MODE_GAME_IN )
				{
					pSession->m_bSendFlag = FALSE;
					continue;
				}

				//---------------------------------------------------------------------
				// [���] Send ���
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
			// [����] IOCP WorkerThread ����
			//------------------------------------------------------
			if (dwTransferred == 0 && pOverlapped == NULL && pSession == NULL)
			{
				PostQueuedCompletionStatus(m_hIOCP, 0, (ULONG_PTR)NULL, NULL);
				//PRO_END(L"IOCP_WorkerThread");

				return 0;
			}

			//------------------------------------------------------
			// [����] Ŭ���̾�Ʈ�� ������ �����°�?
			//------------------------------------------------------
			if (dwTransferred == 0 && pOverlapped != NULL)
			{
				//---------------------------------------------------------------------
				// [����] DisconnectFlag�� False�� ���, �ٷ� �����ֱ� ���� Disconnect
				//---------------------------------------------------------------------
				if (!pSession->m_bDisconnectFlag)
				{
					pSession->Disconnect();
				}

				//---------------------------------------------------------------------
				// ���� �����忡�� ����� �� �ø� IOCount ����
				//---------------------------------------------------------------------
				if (0 == InterlockedDecrement16(&pSession->m_IOCount))
				{
					pSession->m_bExitSocketFlag = TRUE;
				}
			}
			//------------------------------------------------------
			// [���] I/O �Ϸ� ó��
			//------------------------------------------------------
			else
			{
				switch (((CNetSession::st_OVERLAPPED*)pOverlapped)->Type)
				{
				//------------------------------------------------------
				// Recv �Ϸ�
				//------------------------------------------------------
				case CNetSession::st_OVERLAPPED::en_RECV:
					if (!pSession->m_bDisconnectFlag)
					{
						RecvProc(pSession, dwTransferred);
					}
					break;

				//------------------------------------------------------
				// Send �Ϸ�
				//------------------------------------------------------
				case CNetSession::st_OVERLAPPED::en_SEND:
					if (!pSession->m_bDisconnectFlag)
					{
						SendProc(pSession, dwTransferred);
					}
					break;
				}

				//--------------------------------------------------------
				// [����] DisconnectFlag�� �����ϰ� ��ϵ� ��� �ٽ� Cancel
				//--------------------------------------------------------
				if (pSession->m_bDisconnectFlag)
				{
					CancelIoEx((HANDLE)pSession->m_Socket, NULL);
				}

				//------------------------------------------------------
				// ���� I/O ��Ͻ� �����ִ� IOCount ����
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
			// Part 1. Game ���� ��ȯ
			// 
			// ���� �迭�� ���鼭 TO_GAME ����� ������ GAME ���� �����Ѵ�.
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//------------------------------------------------------
				// [����] ���� ���� �����̶�� ��ŵ
				//------------------------------------------------------
				if (pSession->m_bExitSocketFlag == TRUE)
				{
					continue;
				}

				//------------------------------------------------------
				// [����] en_THREAD_MODE_GAME_IN_READY ���°� �ƴϸ� ��ŵ
				//------------------------------------------------------
				if (pSession->m_eThreadMode != en_THREAD_MODE_GAME_IN_READY)
				{
					continue;
				}

				//------------------------------------------------------
				// [����üũ] ToGameFlag�� �������� �� ����.
				//------------------------------------------------------
				if (pSession->m_bToGameFlag != TRUE)
				{
					CCrashDump::Crash();
				}

				//------------------------------------------------------
				// [���] GameThread ó�� ������� ��ȯ
				//------------------------------------------------------
				pSession->OnGame_ClientJoin();
				pSession->m_eThreadMode = en_THREAD_MODE_GAME_IN;
			}

			//============================================================================
			// Part 2. GAME ��� ���ǵ� ��Ŷ ó�� ����
			// 
			// �迭�� ��� ���鼭 GAME ��� ������ Ȯ���Ͽ� �ش� ������ CompleteRecvPacket�� ó�� ��û
			// 
			// * virtual OnGame_Packet() ȣ��
			// * ���� ó���� ����ڿ��� �ѱ�
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//------------------------------------------------------
				// [����] ���� ���� �����̶�� ��ŵ
				//------------------------------------------------------
				if (pSession->m_bExitSocketFlag == TRUE)
				{
					continue;
				}

				//------------------------------------------------------
				// [����] GameThread�� ó�� ����� �ƴ϶�� ��ŵ
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
			// Part 3. GAME ����� Update
			// 
			// �޽����� ������ �׽� ó���Ǿ�� �� ���� ������ �κ� ����
			//============================================================================
			OnGame_Update();

			//============================================================================
			// Part 4. GAME ����� �������� ó��
			// 
			// ���� �迭�� ���鼭 GAME ��忡�� ���� ���� Flag�� true�� ������ [GAME ����ܰ���] ���� �����Ѵ�.
			// 
			// * �̴� ���� ������ ó���� 1�ܰ�.
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//------------------------------------------------------
				// [����] ���� ���� ������ �ƴ϶�� ��ŵ
				//------------------------------------------------------
				if (pSession->m_bExitSocketFlag == FALSE)
				{
					continue;
				}

				//------------------------------------------------------
				// [����] GameThread�� ó�� ����� �ƴ϶�� ��ŵ
				//------------------------------------------------------
				if (pSession->m_eThreadMode != en_THREAD_MODE_GAME_IN)
				{
					continue;
				}

				//------------------------------------------------------
				// [���] Release �غ� �ܰ�� ����
				//------------------------------------------------------
				pSession->m_eThreadMode = en_THREAD_MODE_GAME_OUT_READY;
			}

			//============================================================================
			//Part 5. [GAME ����ܰ���] ������ �ܰ赹��
			//
			// ���� �迭�� ���鼭 [GAME �����غ�Ϸ�] ������ ã�� �����Ѵ�.
			//
			//* virtual OnGame_ClientLeave() ȣ��
			//* virtual OnGame_ClientRelease() ȣ��
			//============================================================================
			for (int i = 0; i < iSessionMax; i++)
			{
				pSession = m_Sessions[i];

				//------------------------------------------------------
				// [����] Release �Ϸ� ���°� �ƴ϶�� ��ŵ
				//------------------------------------------------------
				if (pSession->m_eThreadMode != en_THREAD_MODE_RELEASE_READY_COM)
				{
					continue;
				}

				//------------------------------------------------------
				// [���] Release ó�� ����
				//------------------------------------------------------
				pSession->OnGame_ClientLeave();
				pSession->OnGame_ClientRelease();
				ReleaseSession(pSession);
			}

			//------------------------------------------------------
			// Sleep Time ���
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