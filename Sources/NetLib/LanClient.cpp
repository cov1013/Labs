
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
    // ������
    ///////////////////////////////////////////////////////////////////////////
    LanClient::LanClient()
    {
        //-----------------------------------------------------------------------
        // 1. ��� ���� �ʱ�ȭ
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
        //// 2. ���� Ŭ���� �ʱ�ȭ
        ////-----------------------------------------------------------------------
        //CrashDumper::CrashDumper();

        ////-----------------------------------------------------------------------
        //// 3. �ý��� �ΰ� Ŭ���� �ʱ�ȭ
        ////-----------------------------------------------------------------------
        //Logger::Logger(L"../Logs_LanClient", eLOG_LEVEL_DEBUG);

        ////-----------------------------------------------------------------------
        //// 4. �������Ϸ� �ʱ�ȭ
        ////-----------------------------------------------------------------------
        //InitializeProfiler(L"../Profiling_LanClient", en_PROFILER_UNIT::eUNIT_NANO);

        //-----------------------------------------------------------------------
        // 5. WinSock �ʱ�ȭ
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
    // �Ҹ���
    ///////////////////////////////////////////////////////////////////////////
    LanClient::~LanClient()
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    // ����
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
        // ������ ������ ���� ��쿡�� ���� �����Ѵ�.
        // ���� ���ӿ� ������ ��� ������ �����س��� �������� ���� ��õ�.
        //-----------------------------------------------------------------------
        if (m_Socket == INVALID_SOCKET)
        {
            //-----------------------------------------------------------------------
            // ���� ����
            //-----------------------------------------------------------------------
            m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (INVALID_SOCKET == m_Socket)
            {
                LOG(L"System", eLOG_LEVEL_ERROR, L"Socket Create Failed (error:%ld)\n", WSAGetLastError());
                return false;
            }
            LOG(L"System", eLOG_LEVEL_SYSTEM, L"Socket Create Successful\n");

            //-----------------------------------------------------------------------
            // ���̱� �˰��� ����
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
            // �񵿱� ��� ����
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
            // RST �۽� ����
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
        // ���� ���� ����
        //-----------------------------------------------------------------------
        SOCKADDR_IN	Addr;
        int addrLen = sizeof(Addr);
        getpeername(m_Socket, (sockaddr*)&Addr, &addrLen);
        wcscpy_s(m_BindIP, 16, BindIP);
        wcscpy_s(m_ServerIP, 16, ServerIP);
        m_ServerPort = ServerPort;
        m_BindPort = htons(Addr.sin_port);

        //-----------------------------------------------------------------------
        // ���ӿ� �� ���̶� �����ߴٸ�, IOCP�� �������� �ʾƾ��Ѵ�.
        //-----------------------------------------------------------------------
        if (m_bFlag == false)
        {
            //-----------------------------------------------------------------------
            // IOCP ���� �� ���
            //-----------------------------------------------------------------------
            m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);

            //-----------------------------------------------------------------------
            // IOCP WorkerThread ����
            //-----------------------------------------------------------------------
            for (int i = 0; i < NumberOfIOCPWorkerThread; i++)
            {
                m_hIOCPWorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, this->IOCPWorkerThread, this, 0, nullptr);
            }
            m_NumberOfIOCPWorkerThread = NumberOfIOCPWorkerThread;

            m_bFlag = true;
        }

        //-----------------------------------------------------------------------
        // ���/����
        //-----------------------------------------------------------------------
        CreateIoCompletionPort((HANDLE)m_Socket, m_hIOCP, (ULONG_PTR)this, 0);

        InterlockedIncrement16(&m_IOCount);
        m_bDisconnectFlag = false;
        m_bReleaseFlag = false;

        //-----------------------------------------------------------------------
        // ���������� ���� ���� �Ϸ� �˸���
        //-----------------------------------------------------------------------
        OnEnterJoinServer();

        //-----------------------------------------------------------------------
        // ���� ���
        //-----------------------------------------------------------------------
        RecvPost();

        if (0 == InterlockedDecrement16(&m_IOCount))
        {
            Release();
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    // ���� ����
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
    // ��Ŷ �۽�
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
    // Ŭ���̾�Ʈ ����
    ///////////////////////////////////////////////////////////////////////////
    void LanClient::Stop(void)
    {
        Disconnect();
        while (m_IOCount != 0);

        //--------------------------------------------------------
        // IOCP Worker Thread ���� ���
        //--------------------------------------------------------
        PostQueuedCompletionStatus(m_hIOCP, 0, (ULONG_PTR)nullptr, nullptr);
        WaitForMultipleObjects(m_NumberOfIOCPWorkerThread, m_hIOCPWorkerThreads, TRUE, INFINITE);

        //--------------------------------------------------------
        // ���ҽ� ����
        //--------------------------------------------------------
        for (int i = 0; i < m_NumberOfIOCPWorkerThread; i++)
        {
            CloseHandle(m_hIOCPWorkerThreads[i]);
        }
        CloseHandle(m_hIOCP);
    }

    ///////////////////////////////////////////////////////////////////////////
    // ����
    ///////////////////////////////////////////////////////////////////////////
    bool LanClient::Release(void)
    {
        //--------------------------------------------------------
        // �̹� �ٸ� ������ Release ���̾����� ����
        //--------------------------------------------------------
        if (true == (bool)InterlockedCompareExchange((long*)&m_bReleaseFlag, 0x00000001, false))
        {
            return false;
        }

        //--------------------------------------------------------
        // 1. SendFlag�� ���� ������ ���� ��Ŷ ����
        //--------------------------------------------------------
        PacketBuffer* pPacket;
        while (m_SendBuffer.GetCapacity() != 0)
        {
            m_SendBuffer.Dequeue(&pPacket);
            pPacket->SubRef();
        }

        //--------------------------------------------------------
        // 2. WSASend ���з� ���� ������ ���� ��Ŷ ����
        //--------------------------------------------------------
        for (int i = 0; i < m_SendPacketCount; i++)
        {
            m_SendPacketArray[i]->SubRef();
        }

        //--------------------------------------------------------
        // 5. ���� ���ҽ� ��ȯ �� RST �۽�
        //--------------------------------------------------------
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
        OnLeaveServer();

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    // ���� ���
    ///////////////////////////////////////////////////////////////////////////
    void LanClient::RecvPost(void)
    {
        DWORD	dwFlags = 0;
        WSABUF	Buffers[2];
        int		iBufferCount;
        int		iLen1;
        int		iLen2;

        //--------------------------------------------------------
        // 1. ��� ���� ���� ���� ������ ���
        //--------------------------------------------------------
        iLen1 = m_RecvBuffer.GetNonBrokenPutSize();
        iLen2 = m_RecvBuffer.GetFreeSize();

        //--------------------------------------------------------
        // 2. �⺻ ���� ���� ���
        //--------------------------------------------------------
        Buffers[0].buf = m_RecvBuffer.GetWritePos();
        Buffers[0].len = iLen1;
        iBufferCount = 1;

        //--------------------------------------------------------
        // 3. ���� ���ۿ� ������ �����ִٸ� �߰� ���
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
    // �۽� ���
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
            // SendQ �� �����ϴ� ��Ŷ Dequeue
            //--------------------------------------------------------
            while (m_SendBuffer.GetCapacity() != 0)
            {
                m_SendBuffer.Dequeue(&pPacket);
                m_SendPacketArray[BufferCount] = pPacket;
                Buffers[BufferCount].buf = pPacket->GetLanHeaderPtr();
                Buffers[BufferCount].len = pPacket->GetLanPacketSize();
                BufferCount++;

                //--------------------------------------------------------
                // �ִ� ����� ���������� ���� ��ȸ�� ������.
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
    // ���� �Ϸ� ���ν���
    ///////////////////////////////////////////////////////////////////////////
    void LanClient::RecvProc(const DWORD dwTransferred)
    {
        int iResult;
        m_RecvBuffer.MoveWritePos(dwTransferred);

        while (true)
        {
            PacketBuffer::st_PACKET_LAN Packet;

            //--------------------------------------------------------
            // 1. ���� ���ۿ� ����� �Դ°�?
            //--------------------------------------------------------
            int iUseSize = m_RecvBuffer.GetUseSize();
            if (iUseSize < sizeof(PacketBuffer::st_PACKET_LAN::Len))
            {
                break;
            }

            //--------------------------------------------------------
            // 2. ���� ���ۿ��� ��� Peek
            //--------------------------------------------------------
            m_RecvBuffer.Peek((char*)&Packet, sizeof(PacketBuffer::st_PACKET_LAN::Len));

            //--------------------------------------------------------
            // 3. ���� ���ۿ� ��Ŷ�� �ϼ��ƴ°�?
            //--------------------------------------------------------
            if (iUseSize < (sizeof(PacketBuffer::st_PACKET_LAN::Len) + Packet.Len))
            {
                break;
            }

            //--------------------------------------------------------
            // 4. �ϼ��� �޼������ ���� ���ۿ��� ��� ����
            //--------------------------------------------------------
            m_RecvBuffer.MoveReadPos(sizeof(PacketBuffer::st_PACKET_LAN::Len));
            m_RecvPacketTPS++;

            //--------------------------------------------------------
            // 5. ���̷ε带 ���� ���ۿ��� ����ȭ ���۷� ����
            //--------------------------------------------------------
            PacketBuffer* pRecvPacket = PacketBuffer::Alloc();
            iResult = m_RecvBuffer.Get(pRecvPacket->GetWritePos(), Packet.Len);
            pRecvPacket->MoveWritePos(iResult);

            //--------------------------------------------------------
            // 6. �������� ����
            //--------------------------------------------------------
            OnRecv(pRecvPacket);
            pRecvPacket->SubRef();
        }

        RecvPost();
    }

    ///////////////////////////////////////////////////////////////////////////
    // �۽� �Ϸ� ���ν���
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
            // [����] Thread ����
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
            // 1. I/O ����
            //--------------------------------------------------------
            case en_IO_FAILED:
                //--------------------------------------------------------
                // ���� ��� Release ����
                //--------------------------------------------------------
                if (!m_bDisconnectFlag)
                {
                    Disconnect();
                }

                //--------------------------------------------------------
                // ���� ��Ͻ� ������Ų IOCount ����
                //--------------------------------------------------------
                if (0 == InterlockedDecrement16(&m_IOCount))
                {
                    Release();
                }
                break;

            //--------------------------------------------------------
            // 2. ��� SendPost ����
            //--------------------------------------------------------
            case en_SEND_PACKET:
                SendPost();

                //--------------------------------------------------------
                // SendPacket �Լ����� ������Ų IOCount ����
                //--------------------------------------------------------
                if (0 == InterlockedDecrement16(&m_IOCount))
                {
                    Release();
                }
                break;

            //--------------------------------------------------------
            // 3. IOCP I/O �Ϸ� ó��
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
                // Disconnect �ߴµ� �̹� ��ϵ� ��� ���� ����
                //--------------------------------------------------------
                if (m_bDisconnectFlag)
                {
                    CancelIoEx((HANDLE)m_Socket, NULL);
                }

                //--------------------------------------------------------
                // ���� ��Ͻ� ������Ų IOCount ����
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