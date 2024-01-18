
#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <winsock.h>
#include <mstcpip.h>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include "LanClient.h"

namespace cov1013
{
    ///////////////////////////////////////////////////////////////////////////
    // 생성자
    ///////////////////////////////////////////////////////////////////////////
    LanClient::LanClient()
    {
        //-----------------------------------------------------------------------
        // 1. 멤버 변수 초기화
        //-----------------------------------------------------------------------
        m_bFlag                 = false;
        m_bSendFlag             = false;
        m_bDisconnectFlag       = false;
        m_bReleaseFlag          = false;
        m_IOCount               = 0;
        m_RecvOverlapped.Type   = st_OVERLAPPED::en_RECV;
        m_SendOverlapped.Type   = st_OVERLAPPED::en_SEND;
        m_SendPacketCount       = 0;
        m_Socket                = INVALID_SOCKET;

        ////-----------------------------------------------------------------------
        //// 2. 덤프 클래스 초기화
        ////-----------------------------------------------------------------------
        //CrashDumper::CrashDumper();

        ////-----------------------------------------------------------------------
        //// 3. 시스템 로거 클래스 초기화
        ////-----------------------------------------------------------------------
        //Logger::Logger(L"../Logs_LanClient", eLOG_LEVEL_DEBUG);

        ////-----------------------------------------------------------------------
        //// 4. 프로파일러 초기화
        ////-----------------------------------------------------------------------
        //InitializeProfiler(L"../Profiling_LanClient", en_PROFILER_UNIT::eUNIT_NANO);

        //-----------------------------------------------------------------------
        // 5. WinSock 초기화
        //-----------------------------------------------------------------------
        WSADATA	wsa;
        if (0 != WSAStartup(MAKEWORD(2, 2), &wsa))
        {
            LOG(L"System", eLOG_LEVEL_ERROR, L"Failed to initialization Winsock (error:%ld)\n", WSAGetLastError());
            return;
        }
        LOG(L"System", eLOG_LEVEL_SYSTEM, L"Winsock initialization Successful\n");
    }

    ///////////////////////////////////////////////////////////////////////////
    // 소멸자
    ///////////////////////////////////////////////////////////////////////////
    LanClient::~LanClient()
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    // 접속
    ///////////////////////////////////////////////////////////////////////////
    bool LanClient::Connect(
        const WCHAR*    BindIP,
        const WCHAR*    ServerIP,
        const WORD		ServerPort,
        const DWORD		NumberOfIOCPWorkerThread,
        const bool		bNagleFlag,
        const bool		bZeroCopyFlag
    )
    {
        //-----------------------------------------------------------------------
        // 생성된 소켓이 없는 경우에만 소켓 생성한다.
        // 만약 접속에 실패할 경우 이전에 생성해놨던 소켓으로 접속 재시도.
        //-----------------------------------------------------------------------
        if (m_Socket == INVALID_SOCKET)
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
            m_bNagleFlag = bNagleFlag;
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
            m_bZeroCopyFlag = bZeroCopyFlag;
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
        }

        //-----------------------------------------------------------------------
        // Connect
        //-----------------------------------------------------------------------
        SOCKADDR_IN ServerAddr;
        ServerAddr.sin_family = AF_INET;
        ServerAddr.sin_port = htons(ServerPort);
        InetPton(AF_INET, ServerIP, &ServerAddr.sin_addr);
        if (SOCKET_ERROR == connect(m_Socket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
        {
            errno_t err = WSAGetLastError();
            if (err != WSAECONNREFUSED)
            {
                LOG(L"System", eLOG_LEVEL_ERROR, L"Connect Failed (error:%ld)\n", WSAGetLastError());
            }
            return false;
        }

        LOG(L"System", eLOG_LEVEL_SYSTEM, L"Connect Successful\n");

        //-----------------------------------------------------------------------
        // 연결 정보 저장
        //-----------------------------------------------------------------------
        SOCKADDR_IN	Addr;
        int addrLen = sizeof(Addr);
        getpeername(m_Socket, (sockaddr*)&Addr, &addrLen);
        wcscpy_s(m_BindIP, 16, BindIP);
        wcscpy_s(m_ServerIP, 16, ServerIP);
        m_ServerPort = ServerPort;
        m_BindPort = htons(Addr.sin_port);

        //-----------------------------------------------------------------------
        // 접속에 한 번이라도 성공했다면, IOCP를 생성하지 않아야한다.
        //-----------------------------------------------------------------------
        if (m_bFlag == false)
        {
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
            m_NumberOfIOCPWorkerThread = NumberOfIOCPWorkerThread;

            m_bFlag = true;
        }

        //-----------------------------------------------------------------------
        // 등록/재등록
        //-----------------------------------------------------------------------
        CreateIoCompletionPort((HANDLE)m_Socket, m_hIOCP, (ULONG_PTR)this, 0);

        InterlockedIncrement16(&m_IOCount);
        m_bDisconnectFlag = false;
        m_bReleaseFlag = false;

        //-----------------------------------------------------------------------
        // 컨텐츠에게 서버 연결 완료 알리기
        //-----------------------------------------------------------------------
        OnEnterJoinServer();

        //-----------------------------------------------------------------------
        // 수신 등록
        //-----------------------------------------------------------------------
        RecvPost();

        if (0 == InterlockedDecrement16(&m_IOCount))
        {
            Release();
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    // 연결 끊기
    ///////////////////////////////////////////////////////////////////////////
    void LanClient::Disconnect(const bool bQuit)
    {
        InterlockedIncrement16(&m_IOCount);

        if ((bool)m_bReleaseFlag == true)
        {
            if (0 == InterlockedDecrement16(&m_IOCount))
            {
                Release();
            }
            return;
        }

        m_bDisconnectFlag = true;
        CancelIoEx((HANDLE)m_Socket, NULL);

        if (0 == InterlockedDecrement16(&m_IOCount))
        {
            Release();
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // 패킷 송신
    ///////////////////////////////////////////////////////////////////////////
    void LanClient::SendPacket(PacketBuffer* pPacket)
    {
        InterlockedIncrement16(&m_IOCount);

        if ((bool)m_bReleaseFlag == true)
        {
            if (0 == InterlockedDecrement16(&m_IOCount))
            {
                Release();
            }
            return;
        }

        pPacket->SetLanHeader();
        pPacket->AddRef();
        m_SendBuffer.Enqueue(pPacket);
        SendPost();

        if (0 == InterlockedDecrement16(&m_IOCount))
        {
            Release();
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // 클라이언트 정지
    ///////////////////////////////////////////////////////////////////////////
    void LanClient::Stop(void)
    {
        Disconnect();
        while (m_IOCount != 0);

        //--------------------------------------------------------
        // IOCP Worker Thread 종료 대기
        //--------------------------------------------------------
        PostQueuedCompletionStatus(m_hIOCP, 0, (ULONG_PTR)nullptr, nullptr);
        WaitForMultipleObjects(m_NumberOfIOCPWorkerThread, m_hIOCPWorkerThreads, TRUE, INFINITE);

        //--------------------------------------------------------
        // 리소스 정리
        //--------------------------------------------------------
        for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
        {
            CloseHandle(m_hIOCPWorkerThreads[i]);
        }
        CloseHandle(m_hIOCP);
    }

    ///////////////////////////////////////////////////////////////////////////
    // 정리
    ///////////////////////////////////////////////////////////////////////////
    bool LanClient::Release(void)
    {
        //--------------------------------------------------------
        // 이미 다른 곳에서 Release 중이었으면 리턴
        //--------------------------------------------------------
        if (true == (bool)InterlockedCompareExchange((long*)&m_bReleaseFlag, 0x00000001, false))
        {
            return false;
        }

        //--------------------------------------------------------
        // 1. SendFlag에 막혀 보내지 못한 패킷 정리
        //--------------------------------------------------------
        PacketBuffer* pPacket;
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
        // 5. 소켓 리소스 반환 후 RST 송신
        //--------------------------------------------------------
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
        OnLeaveServer();

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    // 수신 등록
    ///////////////////////////////////////////////////////////////////////////
    void LanClient::RecvPost(void)
    {
        DWORD	dwFlags = 0;
        WSABUF	Buffers[2];
        int		iBufferCount;
        int		iLen1;
        int		iLen2;

        //--------------------------------------------------------
        // 1. 사용 가능 수신 버퍼 사이즈 얻기
        //--------------------------------------------------------
        iLen1 = m_RecvBuffer.GetNonBrokenPutSize();
        iLen2 = m_RecvBuffer.GetFreeSize();

        //--------------------------------------------------------
        // 2. 기본 수신 버퍼 등록
        //--------------------------------------------------------
        Buffers[0].buf = m_RecvBuffer.GetWritePos();
        Buffers[0].len = iLen1;
        iBufferCount = 1;

        //--------------------------------------------------------
        // 3. 수신 버퍼에 공간이 남아있다면 추가 등록
        //--------------------------------------------------------
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

    ///////////////////////////////////////////////////////////////////////////
    // 송신 등록
    ///////////////////////////////////////////////////////////////////////////
    bool LanClient::SendPost(void)
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

            PacketBuffer* pPacket;
            WSABUF	 Buffers[en_SEND_PACKET_MAX];
            int		 BufferCount = 0;

            //--------------------------------------------------------
            // SendQ 에 존재하는 패킷 Dequeue
            //--------------------------------------------------------
            while (m_SendBuffer.GetCapacity() != 0)
            {
                m_SendBuffer.Dequeue(&pPacket);
                m_SendPacketArray[BufferCount] = pPacket;
                Buffers[BufferCount].buf = pPacket->GetLanHeaderPtr();
                Buffers[BufferCount].len = pPacket->GetLanPacketSize();
                BufferCount++;

                //--------------------------------------------------------
                // 최대 사이즈에 도달했으면 다음 기회에 보낸다.
                //--------------------------------------------------------
                if (BufferCount == en_SEND_PACKET_MAX)
                {
                    break;
                }
            }

            m_SendPacketCount = BufferCount;

            ZeroMemory(&m_SendOverlapped.Overlapped, sizeof(OVERLAPPED));
            InterlockedIncrement16(&m_IOCount);

            if (SOCKET_ERROR == WSASend(m_Socket, Buffers, BufferCount, NULL, 0, &m_SendOverlapped.Overlapped, NULL))
            {
                errno_t err = WSAGetLastError();

                if (err != ERROR_IO_PENDING)
                {
                    if (err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAECONNRESET && err != WSAESHUTDOWN)
                    {
                        CrashDumper::Crash();
                        //LOG(L"Error", en_LOG_LEVEL::eLOG_LEVEL_ERROR, L"");
                    }

                    if (0 == InterlockedDecrement16(&m_IOCount))
                    {
                        Release();
                    }
                    return false;
                }
            }
            return true;

        } while (true);
    }

    ///////////////////////////////////////////////////////////////////////////
    // 수신 완료 프로시저
    ///////////////////////////////////////////////////////////////////////////
    void LanClient::RecvProc(const DWORD dwTransferred)
    {
        int iResult;
        m_RecvBuffer.MoveWritePos(dwTransferred);

        while (true)
        {
            PacketBuffer::st_PACKET_LAN Packet;

            //--------------------------------------------------------
            // 1. 수신 버퍼에 헤더는 왔는가?
            //--------------------------------------------------------
            int iUseSize = m_RecvBuffer.GetUseSize();
            if (iUseSize < sizeof(PacketBuffer::st_PACKET_LAN::Len))
            {
                break;
            }

            //--------------------------------------------------------
            // 2. 수신 버퍼에서 헤더 Peek
            //--------------------------------------------------------
            m_RecvBuffer.Peek((char*)&Packet, sizeof(PacketBuffer::st_PACKET_LAN::Len));

            //--------------------------------------------------------
            // 3. 수신 버퍼에 패킷이 완성됐는가?
            //--------------------------------------------------------
            if (iUseSize < (sizeof(PacketBuffer::st_PACKET_LAN::Len) + Packet.Len))
            {
                break;
            }

            //--------------------------------------------------------
            // 4. 완성된 메세지라면 수신 버퍼에서 헤더 삭제
            //--------------------------------------------------------
            m_RecvBuffer.MoveReadPos(sizeof(PacketBuffer::st_PACKET_LAN::Len));
            m_RecvPacketTPS++;

            //--------------------------------------------------------
            // 5. 페이로드를 수신 버퍼에서 직렬화 버퍼로 복사
            //--------------------------------------------------------
            PacketBuffer* pRecvPacket = PacketBuffer::Alloc();
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

    ///////////////////////////////////////////////////////////////////////////
    // 송신 완료 프로시저
    ///////////////////////////////////////////////////////////////////////////
    void LanClient::SendProc(const DWORD dwTransferred)
    {
        m_SendPacketTPS += m_SendPacketCount;
        OnSend(dwTransferred);

        int iCompletedCount = m_SendPacketCount;
        for (int i = 0; i < iCompletedCount; i++)
        {
            m_SendPacketArray[i]->SubRef();
            m_SendPacketCount--;
        }

        m_bSendFlag = false;
        SendPost();
    }

    ///////////////////////////////////////////////////////////////////////////
    // IOCP Worker Thread
    // 
    ///////////////////////////////////////////////////////////////////////////
    unsigned int __stdcall LanClient::IOCPWorkerThread(void* lpParam)
    {
        return ((LanClient*)lpParam)->IOCPWorkerThread_Procedure();
    }
    int LanClient::IOCPWorkerThread_Procedure(void)
    {
        DWORD		dwTransferred;
        OVERLAPPED* pOverlapped;
        LanClient* pLanClient;

        while (true)
        {
            dwTransferred   = 0;
            pOverlapped     = nullptr;
            pLanClient      = nullptr;

            GetQueuedCompletionStatus(m_hIOCP, &dwTransferred, (PULONG_PTR)&pLanClient, &pOverlapped, INFINITE);

            //--------------------------------------------------------
            // [예외] Thread 종료
            //--------------------------------------------------------
            if (dwTransferred == 0 && pOverlapped == nullptr && pLanClient == nullptr)
            {
                OnWorkerThreadEnd();
                PostQueuedCompletionStatus(m_hIOCP, 0, (ULONG_PTR)nullptr, nullptr);
                return 0;
            }

            OnWorkerThreadBegin();

            switch (dwTransferred)
            {
            //--------------------------------------------------------
            // 1. I/O 실패
            //--------------------------------------------------------
            case en_IO_FAILED:
                //--------------------------------------------------------
                // 지금 즉시 Release 유도
                //--------------------------------------------------------
                if (!m_bDisconnectFlag)
                {
                    Disconnect();
                }

                //--------------------------------------------------------
                // 이전 등록시 증가시킨 IOCount 차감
                //--------------------------------------------------------
                if (0 == InterlockedDecrement16(&m_IOCount))
                {
                    Release();
                }
                break;

            //--------------------------------------------------------
            // 2. 대신 SendPost 실행
            //--------------------------------------------------------
            case en_SEND_PACKET:
                SendPost();

                //--------------------------------------------------------
                // SendPacket 함수에서 증가시킨 IOCount 차감
                //--------------------------------------------------------
                if (0 == InterlockedDecrement16(&m_IOCount))
                {
                    Release();
                }
                break;

            //--------------------------------------------------------
            // 3. IOCP I/O 완료 처리
            //--------------------------------------------------------
            default:
                switch (((st_OVERLAPPED*)pOverlapped)->Type)
                {
                case st_OVERLAPPED::en_TYPE::en_RECV:
                    if (!m_bDisconnectFlag)
                    {
                        RecvProc(dwTransferred);
                    }
                    break;

                case st_OVERLAPPED::en_TYPE::en_SEND:
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

                //--------------------------------------------------------
                // 이전 등록시 증가시킨 IOCount 차감
                //--------------------------------------------------------
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