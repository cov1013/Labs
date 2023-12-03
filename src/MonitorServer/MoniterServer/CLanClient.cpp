#include "CLanClient.h"

namespace cov1013
{
    CLanClient::CLanClient()
    {
        //-----------------------------------------------------------------------
        // WinSock 초기화
        //-----------------------------------------------------------------------
        WSADATA	wsa;
        if (0 != WSAStartup(MAKEWORD(2, 2), &wsa))
        {
            LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to initialization Winsock (error:%ld)\n", WSAGetLastError());
            return;
        }
    }

    CLanClient::~CLanClient()
    {

    }

    bool CLanClient::Connect(
        const WCHAR* BindIP,
        const WCHAR* ServerIP,
        const WORD		ServerPort,
        const DWORD		NumberOfIOCPWorkerThread,
        const bool		bNagleFlag,
        const bool		bZeroCopyFlag
    )
    {
        //-----------------------------------------------------------------------
        // 소켓 생성
        //-----------------------------------------------------------------------
        m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == m_Socket)
        {
            LOG(L"System", eLOG_LEVEL_ERROR, L"Socket Create Failed (error:%ld)\n", WSAGetLastError());
            return false;
        }
        LOG(L"System", eLOG_LEVEL_SYSTEM, L"Socket Create Successful\n");

        //-----------------------------------------------------------------------
        // 네이글 알고리즘 설정
        //-----------------------------------------------------------------------
        if (!bNagleFlag)
        {
            bool bFlag = true;
            if (SOCKET_ERROR == setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&bFlag, sizeof(bFlag)))
            {
                LOG(L"System", eLOG_LEVEL_ERROR, L"Nalge Setup Failed (error:%ld)\n", WSAGetLastError());
                return false;
            }
        }
        LOG(L"System", eLOG_LEVEL_SYSTEM, L"Nalge Setup Successful\n");

        //-----------------------------------------------------------------------
        // 비동기 출력 설정
        //-----------------------------------------------------------------------
        if (bZeroCopyFlag)
        {
            int iSize = 0;
            if (SOCKET_ERROR == setsockopt(m_Socket, SOL_SOCKET, SO_SNDBUF, (const char*)&iSize, sizeof(iSize)))
            {
                LOG(L"System", eLOG_LEVEL_ERROR, L"ZeroCopy Setup Failed (error:%ld)\n", WSAGetLastError());
                return false;
            }
        }
        LOG(L"System", eLOG_LEVEL_SYSTEM, L"ZeroCopy Setup Successful\n");

        //-----------------------------------------------------------------------
        // RST 송신 설정
        //-----------------------------------------------------------------------
        LINGER Linger;
        Linger.l_onoff = 1;
        Linger.l_linger = 0;
        if (SOCKET_ERROR == setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (const char*)&Linger, sizeof(Linger)))
        {
            LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to Linger setsockpot (error:%ld)\n", WSAGetLastError());
            return false;
        }
        LOG(L"System", eLOG_LEVEL_SYSTEM, L"Linger Setup Successful\n");

        //-----------------------------------------------------------------------
        // Connect
        //-----------------------------------------------------------------------
        SOCKADDR_IN ServerAddr;
        ServerAddr.sin_family = AF_INET;
        ServerAddr.sin_port = htons(ServerPort);
        InetPton(AF_INET, ServerIP, &ServerAddr.sin_addr);
        if (SOCKET_ERROR == connect(m_Socket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
        {
            LOG(L"System", eLOG_LEVEL_ERROR, L"Connect Failed (error:%ld)\n", WSAGetLastError());
            return false;
        }
        LOG(L"System", eLOG_LEVEL_SYSTEM, L"Connect Successful\n");

        //-----------------------------------------------------------------------
        // IOCP 생성 후 등록
        //-----------------------------------------------------------------------
        m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);

        //-----------------------------------------------------------------------
        // IOCP WorkerThread 가동
        //-----------------------------------------------------------------------
        for (int i = 0; i < NumberOfIOCPWorkerThread; i++)
        {
            m_hIOCPWorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, this->IOCPWorkerThread, this, 0, nullptr);
        }

        RecvPost();

        return false;
    }

    bool CLanClient::Disconnect(void)
    {
        InterlockedIncrement16(&m_IOCount);

        if ((bool)m_bReleaseFlag == true)
        {
            if (0 == InterlockedDecrement16(&m_IOCount))
            {
                Release();
            }
            return false;
        }

        m_bDisconnectFlag = true;
        CancelIoEx((HANDLE)m_Socket, NULL);

        if (0 == InterlockedDecrement16(&m_IOCount))
        {
            Release();
        }
        return true;
    }

    bool CLanClient::SendPacket(CPacket* pPacket)
    {
        InterlockedIncrement16(&m_IOCount);

        if ((bool)m_bReleaseFlag == true)
        {
            if (0 == InterlockedDecrement16(&m_IOCount))
            {
                Release();
            }
            return false;
        }

        int iPacketSize = pPacket->GetLanPacketSize();
        int iPacketCount = iPacketSize / df_MSS;
        if (iPacketCount == 0)
        {
            iPacketCount = 1;
        }
        InterlockedAdd(&m_ExpSendBytes, (df_TCP_HEADER_SIZE + df_IP_HEADER_SIZE + df_MAC_HEADER_SIZE) * iPacketCount + iPacketSize);

        pPacket->SetLanHeader();
        pPacket->AddRef();
        m_SendBuffer.Enqueue(pPacket);
        SendPost();

        if (0 == InterlockedDecrement16(&m_IOCount))
        {
            Release();
        }

        return true;
    }

    bool CLanClient::Release(void)
    {
        //--------------------------------------------------------
        // 이미 다른 곳에서 Release 중이었으면 리턴
        //--------------------------------------------------------
        if (true == (bool)InterlockedCompareExchange((long*)&m_bReleaseFlag, 0x00000001, false))
        {
            return false;
        }

        //--------------------------------------------------------
        // 1. Enqueue만하고 SendFlag에 막힌 상태로 Release로 들어와서 
        //    Dequeue하지 못한 패킷 정리
        //--------------------------------------------------------
        CPacket* pPacket;
        while (m_SendBuffer.GetCapacity() != 0)
        {
            m_SendBuffer.Dequeue(&pPacket);
            pPacket->SubRef();
        }

        //--------------------------------------------------------
        // 2. WSASend 실패로 인해 보내지 못한 패킷 정리
        //--------------------------------------------------------
        for (int i = 0; i < m_SendPacketCount; i++)
        {
            m_SendPacketArray[i]->SubRef();
        }

        //--------------------------------------------------------
        // 3. 소켓 리소스 반환 후 RST 송신
        //--------------------------------------------------------
        closesocket(m_Socket);

        OnLeaveServer();

        return true;
    }

    void CLanClient::RecvPost(void)
    {
        DWORD	dwFlags = 0;
        WSABUF	Buffers[2];
        int		iBufferCount;
        int		iLen1;
        int		iLen2;

        // 1. 사용 가능 수신 버퍼 사이즈 얻기
        iLen1 = m_RecvBuffer.GetNonBrokenPutSize();
        iLen2 = m_RecvBuffer.GetFreeSize();

        // 2. 기본 수신 버퍼 등록
        Buffers[0].buf = m_RecvBuffer.GetWritePos();
        Buffers[0].len = iLen1;
        iBufferCount = 1;

        // 3. 수신 버퍼에 공간이 남아있다면 추가 등록
        if (iLen2 > iLen1)
        {
            Buffers[1].buf = m_RecvBuffer.GetBufferPtr();
            Buffers[1].len = iLen2 - iLen1;
            iBufferCount = 2;
        }

        ZeroMemory(&m_RecvOverlapped.Overlapped, sizeof(OVERLAPPED));
        InterlockedIncrement16(&m_IOCount);

        if (SOCKET_ERROR == WSARecv(m_Socket, Buffers, iBufferCount, NULL, &dwFlags, &m_RecvOverlapped.Overlapped, NULL))
        {
            errno_t err = WSAGetLastError();
            if (err != ERROR_IO_PENDING)
            {
                if (err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAECONNRESET && err != WSAESHUTDOWN)
                {
                    // TODO : Log
                }

                if (0 == InterlockedDecrement16(&m_IOCount))
                {
                    Release();
                }
            }
        }
    }

    bool CLanClient::SendPost(void)
    {
        int	iResult;

        do
        {
            if (true == (bool)InterlockedExchange8((CHAR*)&m_bSendFlag, true))
            {
                return false;
            }

            if (m_SendBuffer.GetCapacity() == 0)
            {
                m_bSendFlag = false;

                if (m_SendBuffer.GetCapacity() != 0)
                {
                    continue;
                }
                return false;
            }

            CPacket* pPacket;
            WSABUF	 Buffers[eSEND_PACKET_MAX];
            int		 iBufferCount = 0;

            //--------------------------------------------------------
            // SendQ 에 존재하는 모든 패킷 Dequeue
            //--------------------------------------------------------
            while (m_SendBuffer.GetCapacity() != 0)
            {
                m_SendBuffer.Dequeue(&pPacket);
                m_SendPacketArray[iBufferCount] = pPacket;
                Buffers[iBufferCount].buf = pPacket->GetLanHeaderPtr();
                Buffers[iBufferCount].len = pPacket->GetLanPacketSize();
                iBufferCount++;
            }
            m_SendPacketCount = iBufferCount;

            ZeroMemory(&m_SendOverlapped.Overlapped, sizeof(OVERLAPPED));
            InterlockedIncrement16(&m_IOCount);

            if (SOCKET_ERROR == WSASend(m_Socket, Buffers, iBufferCount, NULL, 0, &m_SendOverlapped.Overlapped, NULL))
            {
                errno_t err = WSAGetLastError();

                if (err != ERROR_IO_PENDING)
                {
                    if (err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAECONNRESET && err != WSAESHUTDOWN)
                    {
                        // TODO : Log
                    }

                    if (0 == InterlockedDecrement16(&m_IOCount))
                    {
                        Release();
                    }
                }
            }

            return true;

        } while (true);
    }

    void CLanClient::RecvProc(const DWORD dwTransferred)
    {
        int iResult;

        // 수신 받은 만큼 링버퍼 포인터 이동
        m_RecvBuffer.MoveWritePos(dwTransferred);

        //--------------------------------------------------------
        // 패킷 처리 시작
        //--------------------------------------------------------
        while (true)
        {
            CPacket::st_PACKET_LAN Packet;

            //--------------------------------------------------------
            // 1. 수신 버퍼에 헤더는 왔는가?
            //--------------------------------------------------------
            int iUseSize = m_RecvBuffer.GetUseSize();
            if (iUseSize < sizeof(CPacket::st_PACKET_LAN::Len))
            {
                break;
            }

            //--------------------------------------------------------
            // 2. 수신 버퍼에서 헤더 Peek
            //--------------------------------------------------------
            m_RecvBuffer.Peek((char*)&Packet, sizeof(CPacket::st_PACKET_LAN::Len));

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
            m_RecvBuffer.MoveReadPos(sizeof(CPacket::st_PACKET_LAN::Len));

            InterlockedIncrement(&m_RecvPacketTPS);

            //--------------------------------------------------------
            // 5. 페이로드를 수신 버퍼에서 직렬화 버퍼로 복사
            //--------------------------------------------------------
            CPacket* pRecvPacket = CPacket::Alloc();
            iResult = m_RecvBuffer.Get(pRecvPacket->GetWritePos(), Packet.Len);
            pRecvPacket->MoveWritePos(iResult);

            //--------------------------------------------------------
            // 6. 컨텐츠에 전달
            //--------------------------------------------------------
            OnRecv(pRecvPacket);
            pRecvPacket->SubRef();
        }

        RecvPost();
    }

    void CLanClient::SendProc(const DWORD dwTransferred)
    {
        InterlockedIncrement(&m_SendCompeltedTPS);
        InterlockedAdd(&m_SendPacketTPS, m_SendPacketCount);

        OnSend(dwTransferred);

        //--------------------------------------------------------
        // 송신 완료 패킷 반환 유도
        //--------------------------------------------------------
        int iCompletedCount = m_SendPacketCount;
        for (int i = 0; i < iCompletedCount; i++)
        {
            m_SendPacketArray[i]->SubRef();

            //--------------------------------------------------------
            // 여기서 Count를 차감해주지 않으면 ReleaseSession에서 
            // 하나의 패킷에 대해 SubRef를 여러 번 호출하게 된다.
            //--------------------------------------------------------
            m_SendPacketCount--;
        }

        m_bSendFlag = false;
        SendPost();
    }

    unsigned int __stdcall CLanClient::IOCPWorkerThread(void* lpParam)
    {
        return ((CLanClient*)lpParam)->IOCPWorkerThread_Procedure();
    }

    int CLanClient::IOCPWorkerThread_Procedure(void)
    {
        DWORD		dwTransferred;
        OVERLAPPED* pOverlapped;
        CLanClient* pLanClient;

        while (true)
        {
            dwTransferred = 0;
            pOverlapped = NULL;
            pLanClient = NULL;

            GetQueuedCompletionStatus(m_hIOCP, &dwTransferred, (PULONG_PTR)&pLanClient, &pOverlapped, INFINITE);

            OnWorkerThreadBegin();

            // IOCPWorkerThread 종료
            if (dwTransferred == 0 && pOverlapped == NULL && pLanClient == NULL)
            {
                break;
            }

            switch (dwTransferred)
            {
                //--------------------------------------------------------
                // 1. WorkerTherad 종료
                //--------------------------------------------------------
            case en_WORKER_JOB_TYPE::en_THREAD_EXIT:
                PostQueuedCompletionStatus(m_hIOCP, en_WORKER_JOB_TYPE::en_THREAD_EXIT, 0, 0);
                OnWorkerThreadEnd();
                return 0;

                //--------------------------------------------------------
                // 2. SendPacket
                //--------------------------------------------------------
            case en_WORKER_JOB_TYPE::en_SEND_PACKET:
                SendPost();

                // SendPacket 함수에서 증가시킨 IOCount 차감
                if (0 == InterlockedDecrement16(&m_IOCount))
                {
                    Release();
                }
                break;

                //--------------------------------------------------------
                // 3. 클라이언트의 연결 종료
                //--------------------------------------------------------
            case 0:
                if (!m_bDisconnectFlag)
                {
                    Disconnect();
                }

                if (0 == InterlockedDecrement16(&m_IOCount))
                {
                    Release();
                }
                break;

                //--------------------------------------------------------
                // 4. I/O 완료 처리
                //--------------------------------------------------------
            default:
                switch (((st_OVERLAPPED*)pOverlapped)->Type)
                {
                case en_OVERLAPPED_TYPE::eOVERLAPPED_TYPE_RECV:
                    if (!m_bDisconnectFlag)
                    {
                        RecvProc(dwTransferred);
                    }
                    break;

                case en_OVERLAPPED_TYPE::eOVERLAPPED_TYPE_SEND:
                    if (!m_bDisconnectFlag)
                    {
                        SendProc(dwTransferred);
                    }
                    break;
                }

                //--------------------------------------------------------
                // Disconnect 했는데 이미 등록된 경우 강제 종료
                //--------------------------------------------------------
                if (m_bDisconnectFlag)
                {
                    CancelIoEx((HANDLE)m_Socket, NULL);
                }

                if (0 == InterlockedDecrement16(&m_IOCount))
                {
                    Release();
                }

                break;
            }

            OnWorkerThreadEnd();
        }

        return 0;
    }
}