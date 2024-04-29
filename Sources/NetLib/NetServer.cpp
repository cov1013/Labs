#include "NetServer.h"

namespace covEngine
{
	NetServer::NetServer()
	{
		m_AcceptTPS = 0;
		m_AcceptTotal = 0;
		m_DisconnectTotal = 0;
		m_DisconnectTPS = 0;
		m_RecvPacketTPS = 0;
		m_SendPacketTPS = 0;
		m_ExpSendBytes = 0;
		m_NumberOfIOCPWorkerThread = 0;
		m_NumberOfIOCPActiveThread = 0;
		m_SessionMax = 0;
		m_bZeroCopyFlag = false;
		m_bNagleFlag = false;
		m_bKeepAliveFlag = false;
		m_bSelfSendPostFlag = false;
		m_UniqueKey = 1;
		m_ListenSocket = INVALID_SOCKET;
		m_hIOCP = nullptr;
		m_hAcceptThread = nullptr;
		memset(m_Sessions, 0, sizeof(st_SESSION*) * en_SESSION_MAX);
		memset(m_hIOCPWorkerThreads, 0, sizeof(HANDLE) * en_WORKER_THREAD_MAX);
		WSADATA	wsa;
		if (0 != WSAStartup(MAKEWORD(2, 2), &wsa))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to initialization Winsock (error:%ld)\n", WSAGetLastError());

			return;
		}

		LOG(L"System", eLOG_LEVEL_SYSTEM, L"Winsock initialization Successful\n");
	}

	NetServer::~NetServer() 
	{
		
	}

	bool NetServer::Start(
		const WCHAR* BindIP,
		const WORD BindPort,
		const int iNumberOfIOCPWorkerThread,
		const int iNumberOfIOCPActiveThread,
		const bool bNagleFlag,
		const bool bZeroCopyFlag,
		const int iSessionMax,
		const int iPacketSizeMax,
		const bool bSelfSendPostFlag,
		const bool bKeepAliveFlag
	)
	{
		SOCKADDR_IN ServerAddr;
		InetPton(AF_INET, BindIP, (PVOID)&ServerAddr.sin_addr);
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(BindPort);

		if (iNumberOfIOCPWorkerThread > en_WORKER_THREAD_MAX)
		{
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Create Worker Thread Num Over (%d > %d)", iNumberOfIOCPWorkerThread, en_WORKER_THREAD_MAX);
			OnError(en_ERROR_CONFIG_OVER_THREAD);

			return false;
		}

		if (iSessionMax > en_SESSION_MAX)
		{
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Create Session Over (%d > %d)", iSessionMax, en_SESSION_MAX);
			OnError(en_ERROR_CONFIG_OVER_SESSION);

			return false;
		}

		m_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == m_ListenSocket)
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to create ListenSocket (error:%ld)\n", WSAGetLastError());
			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"ListenSocket Creation Successful\n");

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

		LINGER Linger;
		Linger.l_onoff = 1;
		Linger.l_linger = 0;
		if (SOCKET_ERROR == setsockopt(m_ListenSocket, SOL_SOCKET, SO_LINGER, (const char*)&Linger, sizeof(Linger)))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to Linger setsockpot (error:%ld)\n", WSAGetLastError());
			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"Linger Setup Successful\n");

		if (SOCKET_ERROR == bind(m_ListenSocket, (const sockaddr*)&ServerAddr, sizeof(ServerAddr)))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"bind failed (error:%ld)\n", WSAGetLastError());
			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"bind Successful\n");

		if (SOCKET_ERROR == listen(m_ListenSocket, SOMAXCONN))
		{
			LOG(L"System", eLOG_LEVEL_ERROR, L"listen failed(error:%ld)\n", WSAGetLastError());
			return false;
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"listen Successful\n");

		m_NumberOfIOCPWorkerThread = iNumberOfIOCPWorkerThread;
		m_NumberOfIOCPActiveThread = iNumberOfIOCPActiveThread;
		m_SessionMax = iSessionMax;
		m_BindPort = BindPort;
		m_bZeroCopyFlag = bZeroCopyFlag;
		m_bNagleFlag = bNagleFlag;
		m_bKeepAliveFlag = bKeepAliveFlag;
		m_PacketSizeMax = iPacketSizeMax;
		m_bSelfSendPostFlag = bSelfSendPostFlag;
		wcscpy_s(m_BindIP, 16, BindIP);

		for (int i = 0; i < m_SessionMax; i++)
		{
			m_Sessions[i] = (st_SESSION*)_aligned_malloc(sizeof(st_SESSION), en_CACHE_ALIGN);
			new (m_Sessions[i]) st_SESSION();
		}

		for (int i = m_SessionMax; i > 0; i--)
		{
			m_Indexes.Push(i - 1);
		}

		LOG(L"System", eLOG_LEVEL_SYSTEM,
			L"Start (IP:%s / Port:%d / WorkerThread:%d / ActiveThread:%d / Session:%d / Nalge:%d / Zerocopy:%d)\n",
			m_BindIP, m_BindPort, m_NumberOfIOCPWorkerThread, m_NumberOfIOCPActiveThread, m_SessionMax, m_bNagleFlag, m_bZeroCopyFlag
		);

		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, m_NumberOfIOCPActiveThread);
		m_hAcceptThread = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, this, 0, nullptr);
		for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
		{
			m_hIOCPWorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, IOCPWorkerThread, this, 0, nullptr);
		}

		return true;
	}

	void NetServer::Stop(en_STOP_TYPE eType)
	{
		switch (eType)
		{
		case en_STOP_TYPE::STOP:

			//--------------------------------------------------------
			// 1. Accept ������ ����
			//--------------------------------------------------------
			closesocket(m_ListenSocket);
			WaitForSingleObject(m_hAcceptThread, INFINITE);

			//--------------------------------------------------------
			// 2. ���� ���� ����
			//--------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				Disconnect(m_Sessions[i]);
			}
			while ((m_SessionMax - m_Indexes.GetCapacity()) != 0) {};

			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Stop Completed\n");
			break;

		case en_STOP_TYPE::RESOURCE_RELEASE:

			//--------------------------------------------------------
			// ��Ŀ������ ����
			//--------------------------------------------------------
			PostQueuedCompletionStatus(m_hIOCP, en_THREAD_EXIT, (ULONG_PTR)nullptr, nullptr);
			WaitForMultipleObjects(m_NumberOfIOCPWorkerThread, m_hIOCPWorkerThreads, TRUE, INFINITE);
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"All Worker Thread Exit Completed\n");

			//--------------------------------------------------------
			// ���ҽ� ����
			//--------------------------------------------------------
			CloseHandle(m_hIOCP);
			CloseHandle(m_hAcceptThread);
			for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
			{
				CloseHandle(m_hIOCPWorkerThreads[i]);
			}

			//-----------------------------------------------------------------------
			// ���� �޸� ��ȯ
			//-----------------------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				m_Sessions[i]->~st_SESSION();
				_aligned_free(m_Sessions[i]);
			}

			//-----------------------------------------------------------------------
			// WinSock ����
			//-----------------------------------------------------------------------
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
			// 2. ���� ���� ����
			//--------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				Disconnect(m_Sessions[i]);
			}
			while ((m_SessionMax - m_Indexes.GetCapacity()) != 0) {};
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"All Session Out Complete\n");

			//--------------------------------------------------------
			// ��Ŀ������ ����
			//--------------------------------------------------------
			PostQueuedCompletionStatus(m_hIOCP, en_THREAD_EXIT, (ULONG_PTR)nullptr, nullptr);
			WaitForMultipleObjects(m_NumberOfIOCPWorkerThread, m_hIOCPWorkerThreads, TRUE, INFINITE);
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"All Worker Thread Exit Completed\n");

			//--------------------------------------------------------
			// ���ҽ� ����
			//--------------------------------------------------------
			CloseHandle(m_hIOCP);
			CloseHandle(m_hAcceptThread);
			for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
			{
				CloseHandle(m_hIOCPWorkerThreads[i]);
			}

			//-----------------------------------------------------------------------
			// ���� �޸� ��ȯ
			//-----------------------------------------------------------------------
			for (int i = 0; i < m_SessionMax; i++)
			{
				m_Sessions[i]->~st_SESSION();
				_aligned_free(m_Sessions[i]);
			}

			//-----------------------------------------------------------------------
			// WinSock ����
			//-----------------------------------------------------------------------
			WSACleanup();

			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Quit Completed\n");

			break;

		default:
			CrashDumper::Crash();
			break;
		}

	}

	bool NetServer::Restart(void)
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
			OnError(en_ERROR_WINSOCK_LISTEN_SOCK);

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
				OnError(en_ERROR_WINSOCK_SET_NAGLE);

				return false;
			}
		}
		LOG(L"System", eLOG_LEVEL_SYSTEM, L"Nalge Setup Successful\n");

		//-----------------------------------------------------------------------
		// KeepAlive ���� 
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
		// 3. �񵿱� ��� ����
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
		// 4. RST �۽� ����
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
		// AccpetThread �簡��
		//-----------------------------------------------------------------------
		m_hAcceptThread = (HANDLE)_beginthreadex(nullptr, 0, this->AcceptThread, this, 0, nullptr);

		return true;
	}

	bool NetServer::SendPacket(const SESSION_ID SessionID, PacketBuffer* pPacket)
	{
		st_SESSION* pSession = m_Sessions[GET_SESSION_INDEX(SessionID)];
		InterlockedIncrement16(&pSession->IOCount);

		// ReleaseFlag�� ���� Ȯ������ ������, �̹� ������ ������ SessionID�� �޸𸮿� ���������Ƿ�, �ش� SessionID�� ĳġ�ϰ� ����� �� ����.
		if ((bool)pSession->bReleaseFlag == true)
		{
			if (0 == InterlockedDecrement16(&pSession->IOCount))
			{
				ReleaseSession(pSession);
			}
			return false;
		}

		// SessionID�� ���� Ȯ���� ���, Release ���� NewSession���� ReleaseFlag�� false�� �Ǵ� ��Ȳ�� ĳġ�ϰ� ����� �� ����.
		if (pSession->SessionID != SessionID)
		{
			if (0 == InterlockedDecrement16(&pSession->IOCount))
			{
				ReleaseSession(pSession);
			}
			return false;
		}
		pPacket->Encode();
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

	bool NetServer::Disconnect(const SESSION_ID SessionID)
	{
		st_SESSION* pSession = m_Sessions[GET_SESSION_INDEX(SessionID)];
		InterlockedIncrement16(&pSession->IOCount);

		// ReleaseFlag�� Ȯ������ ������, �̹� ������ ������ SessionID�� �޸𸮿� ���������Ƿ�, �ش� SessionID�� ĳġ�ϰ� ����� �� ����.
		if ((bool)pSession->bReleaseFlag == true)
		{
			if (0 == InterlockedDecrement16(&pSession->IOCount))
			{
				ReleaseSession(pSession);
			}
			return false;
		}

		// SessionID�� ���� Ȯ���� ���, Release ���� NewSession���� ReleaseFlag�� false�� �Ǵ� ��Ȳ�� ĳġ�ϰ� ����� �� ����.
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

	const int NetServer::GetSessionCount(void)
	{
		return m_SessionMax - m_Indexes.GetCapacity();
	}

	const int NetServer::GetIndexesCount(void)
	{
		return m_Indexes.GetCapacity();
	}

	NetServer::st_SESSION* NetServer::NewSession(const SOCKET ClientSocket)
	{
		SOCKADDR_IN	ClientAddr;
		int addrLen = sizeof(ClientAddr);
		getpeername(ClientSocket, (sockaddr*)&ClientAddr, &addrLen);

		SESSION_INDEX SessionIndex;
		if (!m_Indexes.Pop(&SessionIndex))
		{
			return nullptr;	// �ű� ���� ���� �Ұ���
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

	bool NetServer::ReleaseSession(st_SESSION* pSession)
	{
		// �ʱ�ȭ �� ���� ���� ������ �ε��� ���ÿ� ��ȯ�Ǵ� ���� ����.
		if (pSession->SessionIndex == df_INVALID_SESSION_INDEX)
		{
			return false;
		}

		// �̹� �ٸ� ������ Release ���̾����� ����
		if (true == (bool)InterlockedCompareExchange((long*)&pSession->bReleaseFlag, 0x00000001, false))
		{
			return false;
		}

		SESSION_ID SessionID = pSession->SessionID;

		// 1. Enqueue���ϰ� SendFlag�� ���� ���·� Release�� ���ͼ� Dequeue���� ���� ��Ŷ ����
		PacketBuffer* pPacket;
		while (pSession->SendQ.GetCapacity() != 0)
		{
			pSession->SendQ.Dequeue(&pPacket);
			pPacket->SubRef();
		}

		// 2. WSASend ���з� ���� ������ ���� ��Ŷ ����
		int iCount = pSession->PacketCount;
		for (int i = 0; i < iCount; i++)
		{
			pSession->Packets[i]->SubRef();
		}

		// 3. ���� ���ҽ� ��ȯ �� RST �۽�
		closesocket(pSession->Socket);

		// 4. ���� �ε��� ��ȯ
		m_Indexes.Push(pSession->SessionIndex);

		OnClientLeave(SessionID);

		InterlockedIncrement(&m_DisconnectTPS);
		InterlockedIncrement(&m_DisconnectTotal);

		return true;
	}

	// ���� ���� ���� (���ο�)
	bool NetServer::Disconnect(st_SESSION* pSession)
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

	// WSARecv ���
	void NetServer::RecvPost(st_SESSION* pSession)
	{
		DWORD	dwFlags = 0;
		WSABUF	Buffers[2];
		int		iBufferCount;
		int		iLen1;
		int		iLen2;

		// ��� ���� ���� ���� ������ ���
		iLen1 = pSession->RecvQ.GetNonBrokenPutSize();
		iLen2 = pSession->RecvQ.GetFreeSize();

		// �⺻ ���� ���� ���
		Buffers[0].buf = pSession->RecvQ.GetWritePos();
		Buffers[0].len = iLen1;
		iBufferCount = 1;

		// ���� ���ۿ� ������ �����ִٸ� �߰� ���
		if (iLen2 > iLen1)
		{
			Buffers[1].buf = pSession->RecvQ.GetBufferPtr();
			Buffers[1].len = iLen2 - iLen1;
			iBufferCount = 2;
		}

		ZeroMemory(&pSession->RecvOverlapped.Overlapped, sizeof(WSAOVERLAPPED));
		InterlockedIncrement16(&pSession->IOCount);

		if (SOCKET_ERROR == WSARecv(pSession->Socket, Buffers, iBufferCount, NULL, &dwFlags, &pSession->RecvOverlapped.Overlapped, NULL))
		{
			errno_t err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				if (err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAECONNRESET && err != WSAESHUTDOWN)
				{
					LOG(L"Error", eLOG_LEVEL_DEBUG, L"### WSARecv error (%d) - SessionID:%d | SessionIndex:%d | IP:%s | Port:%d\n", err, pSession->SessionID, pSession->SessionIndex, pSession->IP, pSession->Port);
				}

				if (0 == InterlockedDecrement16(&pSession->IOCount))
				{
					ReleaseSession(pSession);
				}
			}
		}
	}

	// WSASend ���
	bool NetServer::SendPost(st_SESSION* pSession)
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

			PacketBuffer*	pPacket;
			WSABUF		Buffers[en_SEND_PACKET_MAX];
			int			iBufferCount = 0;
			int			iPacketSize = 0;
			int			iPacketCount = 0;

			while (pSession->SendQ.GetCapacity() != 0)
			{
				 pSession->SendQ.Dequeue(&pPacket);
				pSession->Packets[iBufferCount] = pPacket;
				Buffers[iBufferCount].buf = pPacket->GetBufferPtr();
				Buffers[iBufferCount].len = pPacket->GetPacketSize();

				iPacketSize += Buffers[iBufferCount].len;
				iBufferCount++;

				if (iBufferCount == en_SEND_PACKET_MAX)
				{
					break;
				}
			}

			iPacketCount = iPacketSize / df_MSS;
			InterlockedAdd(&m_ExpSendBytes, (df_TCP_HEADER_SIZE + df_IP_HEADER_SIZE + df_MAC_HEADER_SIZE)* iPacketCount + iPacketSize);
			pSession->PacketCount = iBufferCount;

			ZeroMemory(&pSession->SendOverlapped.Overlapped, sizeof(WSAOVERLAPPED));
			InterlockedIncrement16(&pSession->IOCount);

			PRO_BEGIN(L"WSASend");
			int iReulst = WSASend(pSession->Socket, Buffers, iBufferCount, NULL, 0, &pSession->SendOverlapped.Overlapped, NULL);
			errno_t err = WSAGetLastError();
			PRO_END(L"WSASend");

			if (SOCKET_ERROR == iReulst)
			{
				if (err != ERROR_IO_PENDING)
				{
					if (err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAECONNRESET && err != WSAESHUTDOWN)
					{
						LOG(L"Error", eLOG_LEVEL_DEBUG, L"### WSASend error (%d) - SessionID:%d | SessionIndex:%d | IP:%s | Port:%d\n", err, pSession->SessionID, pSession->SessionIndex, pSession->IP, pSession->Port);
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

	// ���� �Ϸ� ó��
	void NetServer::RecvProc(st_SESSION* pSession, const DWORD dwTransferred)
	{
		int iResult;

		// ���� ���� ��ŭ ������ ������ �̵�
		pSession->RecvQ.MoveWritePos(dwTransferred);

		// ��Ŷ ó�� ����
		while (true)
		{
			// ���� ���ۿ� ����� �Դ°�?
			int iUseSize = pSession->RecvQ.GetUseSize();
			if (iUseSize < PacketBuffer::eHEADER_LEN)
			{
				break;
			}

			// ���� ���ۿ��� ��� Peek
			PacketBuffer::st_PACKET_HEADER Header;
			pSession->RecvQ.Peek((char*)&Header, PacketBuffer::eHEADER_LEN);

			// ��Ŷ Len Ȯ��.
			if (Header.Len > m_PacketSizeMax)
			{
				LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Packet Length Over - (SessionID:%I64u)\n", pSession->SessionID);
				Disconnect(pSession);

				return;
			}

			// ���� ���ۿ� ��Ŷ�� �ϼ��Ǿ��ִ°�?
			if (iUseSize < (PacketBuffer::eHEADER_LEN + Header.Len))
			{
				break;
			}

			// �ϼ��� ��Ŷ�̶��, ���̺귯�� ����� �����ϰ� ����ȭ ���۷� �ű��.
			PacketBuffer* pRecvPacket = PacketBuffer::Alloc();

			iResult = pSession->RecvQ.Get(pRecvPacket->GetBufferPtr(), PacketBuffer::eHEADER_LEN + Header.Len);
			pRecvPacket->MoveWritePos(iResult - PacketBuffer::eHEADER_LEN);	// ��� ���̸� �� ���̷ε� ������ ��ŭ WritePos �̵�

			// ���ڵ�
			if (pRecvPacket->Decode() == true)
			{
				InterlockedIncrement(&m_RecvPacketTPS);
				OnRecv(pSession->SessionID, pRecvPacket);
			}
			else
			{
				LOG(L"Error", eLOG_LEVEL_ERROR, L"Packet Decode Error - SessionID:%d | SessionIndex:%d | IP:%s | Port:%d | )\n", pSession->SessionID, pSession->SessionIndex, pSession->IP, pSession->Port);
				OnError(en_ERROR_DECODE_FAILD, pSession->SessionID);
			}

			pRecvPacket->SubRef();
		}
		RecvPost(pSession);
	}

	void NetServer::SendProc(st_SESSION* pSession, const DWORD dwTransferred)
	{
		InterlockedAdd(&m_SendPacketTPS, pSession->PacketCount);
		OnSend(pSession->SessionID, dwTransferred);

		int iCompletedCount = pSession->PacketCount;
		for (int i = 0; i < iCompletedCount; i++)
		{
			pSession->Packets[i]->SubRef();
			pSession->PacketCount--;
		}

		pSession->bSendFlag = false;
		SendPost(pSession);
	}

	unsigned int __stdcall NetServer::AcceptThread(void* lpParam)
	{
		return ((NetServer*)lpParam)->AcceptThread_Procedure();
	}

	int NetServer::AcceptThread_Procedure(void)
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
				if (err != WSAEINTR && err != WSAENOTSOCK)
				{
					LOG(L"Error", eLOG_LEVEL_ERROR, L"ListenSocket error:%d\n", err);
					//OnError(en_ERROR_WINSOCK_LISTEN_SOCK);
				}

				return 0;
			}

			st_SESSION* pSession = NewSession(ClientSocket);
			if (nullptr == pSession)
			{
				OnError(en_ERROR_WINSOCK_OVER_CONNECT);
				closesocket(ClientSocket);
				continue;
			}

			OnClientJoin(pSession->SessionID);

			RecvPost(pSession);

			// NewSession���� �ø� IOCount ����
			if (0 == InterlockedDecrement16(&pSession->IOCount))
			{
				ReleaseSession(pSession);
			}

			m_AcceptTotal++;
			m_AcceptTPS++;
		}
	}

	unsigned int __stdcall NetServer::IOCPWorkerThread(void* lpParam)
	{
		return ((NetServer*)lpParam)->IOCPWorkerThread_Procedure();
	}
	int NetServer::IOCPWorkerThread_Procedure(void)
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
			PRO_BEGIN(L"IOCPWorkerThread");

			// [����] Therad ����
			if (dwTransferred == en_THREAD_EXIT && pSession == nullptr && pOverlapped == nullptr)
			{
				PostQueuedCompletionStatus(m_hIOCP, en_THREAD_EXIT, (ULONG_PTR)nullptr, nullptr);
				OnWorkerThreadEnd();
				PRO_END(L"IOCPWorkerThread");
				return 0;
			}

			switch (dwTransferred)
			{
			// �ۼ��� ����
			case en_IO_FAILED:

				if (!pSession->bDisconnectFlag)
				{
					Disconnect(pSession->SessionID);
				}

				// ���� I/O ��Ͻ� �����ִ� IOCount ����
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

				// SendPacket �Լ����� �����ִ� IOCount ����
				if (0 == InterlockedDecrement16(&pSession->IOCount))
				{
					ReleaseSession(pSession);
				}
				break;

				//--------------------------------------------------------
				// 4. I/O �Ϸ� ó��
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
				// Disconnect �ߴµ� �̹� ��ϵ� ��� ���� ����
				//--------------------------------------------------------
				if (pSession->bDisconnectFlag)
				{
					CancelIoEx((HANDLE)pSession->Socket, NULL);
				}

				// ���� I/O ��Ͻ� �����ִ� IOCount ����
				if (0 == InterlockedDecrement16(&pSession->IOCount))
				{
					ReleaseSession(pSession);
				}

				break;
			}

			OnWorkerThreadEnd();
			PRO_END(L"IOCPWorkerThread");
		}

		return 0;
	}
}