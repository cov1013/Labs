#include "framework.h"
#include "resource.h"

// COMMOM
#include "Protocol.h"
#include "Define.h"
#include "Struct.h"

// SYSTEM
#include "CLogger.h"
#include "profiler.h"

// NETWORK
#include "CStreamQ.h"
#include "CPacket.h"
#include "makePacket.h"
#include "packetProc.h"
#include "network.h"

extern BOOL			g_bConnected;
extern BOOL			g_bExitFlag;

extern SOCKET		g_Socket;
extern CStreamQ		g_RecvRQ;
extern CStreamQ		g_SendRQ;

BOOL g_bSendPossibility;

BOOL DomainToIP(const WCHAR* szDomain, IN_ADDR* pAddr)
{
	ADDRINFOW*   pAddrInfo;
	SOCKADDR_IN* pSockAddr;
	if (GetAddrInfo(szDomain, L"0", NULL, &pAddrInfo) != 0)
	{
		return FALSE;
	}
	pSockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
	*pAddr = pSockAddr->sin_addr;
	FreeAddrInfo(pAddrInfo);
	return TRUE;
}

void netStartUp(HWND* hWnd)
{
	int iResult;
	WCHAR IP[16];

	// TEST
	//char IP[16] = "127.0.0.1";

	// ------------------------------
	// ���� 2.2 �ʱ�ȭ
	// ------------------------------
	WSAData wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	//---------------------------------------
	// ���� ����
	//---------------------------------------
	g_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_Socket == INVALID_SOCKET)
	{
		_LOG_FILE(dfLOG_LEVEL_ERROR, L"netStartUp() error %d\n", WSAGetLastError());
		g_bExitFlag = TRUE;
		return;
	}

	//---------------------------------------
	// ���� IP, ��Ʈ �Ҵ�
	//---------------------------------------
	SOCKADDR_IN SockAddr;
	IN_ADDR		Addr;

//	DomainToIP(L"procademyserver.iptime.org", (IN_ADDR*)&Addr);
	//SockAddr.sin_addr = Addr;

	SockAddr.sin_family = AF_INET;
	InetPton(AF_INET, L"127.0.0.1", &SockAddr.sin_addr);	// TEST
	SockAddr.sin_port = htons(dfNETWORK_PORT);

	//---------------------------------------
	// AsyncSelect ���
	//--------------------------------------
	iResult = WSAAsyncSelect(g_Socket, *hWnd, ASYCN_SOCKET, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);
	if (iResult != 0)
	{
		_LOG_FILE(dfLOG_LEVEL_ERROR, L"WSAAsyncSelect() error %d\n", WSAGetLastError());
		g_bExitFlag = TRUE;
		return;
	}

	//---------------------------------------
	// ����, AsysnSelect �������� ����� �������� 
	// ��-��� �������� ����Ǿ���.
	//---------------------------------------
	if (SOCKET_ERROR == connect(g_Socket, (SOCKADDR*)&SockAddr, sizeof(SOCKADDR_IN)))
	{
		errno_t err = WSAGetLastError();

		//---------------------------------------
		// ���� ���Ͽ��� connect�� ������ WOULDBLOCK�� �ƴ϶�� ���� �� ����.
		// 
		// 1. ���� ��û ��Ʈ�� LISTENING ���°� �ƴϴ�.
		// 2. ������ BackLogQ�� �� á��.
		//---------------------------------------
		if (err != WSAEWOULDBLOCK)
		{
			_LOG_FILE(dfLOG_LEVEL_ERROR, L"connect() error %d\n", err);
			g_bExitFlag = TRUE;

			return;
		}
	}
	else
	{
		//wprintf(L"127.0.0.1:%d connect completed\n", dfNETWORK_PORT);
		InetNtop(AF_INET, (const void*)&Addr, IP, 16);
		wprintf(L"%s:%d connect completed\n", IP, dfNETWORK_PORT);
	}
}

void netCleanUp(void)
{
	WSACleanup();
}

BOOL netIOProcess(WPARAM wParam, LPARAM lParam)
{
	//--------------------------------
	// ���� �߻� ���� Ȯ��
	//--------------------------------
	if (0 != WSAGETSELECTERROR(lParam))
	{
		_LOG(dfLOG_LEVEL_ERROR, L"netIOProcess() error:%ld\n", WSAGETSELECTERROR(lParam));
		return FALSE;
	}

	//--------------------------------
	// �޼��� ó��
	//--------------------------------
	switch (lParam)
	{
	case FD_CONNECT: // ���� ����ó��
		g_bConnected = TRUE;
		return TRUE;

	case FD_CLOSE:	// ���� ���� ó��
		return FALSE;

	case FD_READ:	// Read Ȯ��
		if (!netRecvProc())
		{
			return FALSE;
		}
		return TRUE;

	case FD_WRITE:	// Write Ȯ��
		g_bSendPossibility = TRUE;
		if (!netSendProc())
		{
			return FALSE;
		}
		return TRUE;
	}

	return TRUE;
}

BOOL netPacketProc(BYTE byPacketType, CPacket* pPacket)
{
	try
	{
		switch (byPacketType)
		{
		case dfPACKET_SC_CREATE_MY_CHARACTER:
			netPacketProc_CreateMyCharacter(pPacket);
			break;

		case dfPACKET_SC_CREATE_OTHER_CHARACTER:
			netPacketProc_CreateOtherCharacter(pPacket);
			break;

		case dfPACKET_SC_DELETE_CHARACTER:
			netPacketProc_DeleteCharacter(pPacket);
			break;

		case dfPACKET_SC_MOVE_START:
			netPacketProc_MoveStart(pPacket);
			break;

		case dfPACKET_SC_MOVE_STOP:
			netPacketProc_MoveStop(pPacket);
			break;

		case dfPACKET_SC_ATTACK1:
			netPacketProc_Attack1(pPacket);
			break;

		case dfPACKET_SC_ATTACK2:
			netPacketProc_Attack2(pPacket);
			break;

		case dfPACKET_SC_ATTACK3:
			netPacketProc_Attack3(pPacket);
			break;

		case dfPACKET_SC_DAMAGE:
			netPacketProc_Damage(pPacket);
			break;

		case dfPACKET_SC_SYNC:
			netPacketProc_Sync(pPacket);
			break;

		default:
			break;
		}
	}
	catch (BOOL bFailed)
	{
		return bFailed;
	}

	return TRUE;
}

BOOL netRecvProc(void)
{
	//----------------------------------------------------------
	// �ش� ���Ͽ��� ������ recv�� ������ϸ�, ���� recv�� �ǳʶڴٸ� �ٽô� FD_READ �̺�Ʈ�� �߻����� �ʴ´�.
	// FD_READ �̺�Ʈ�� ���� �߻��� �� �𸣹Ƿ� �̹� �����ӿ��� ó���� �� �ִ� ��Ŷ�� �� ó���ؾ��Ѵ�.
	//----------------------------------------------------------
	st_PACKET_HEADER Header;

	int iResult;
	int iUseSize;

	//-----------------------------
	// ���� üũ
	//-----------------------------
	if (!g_bConnected)
	{
		return FALSE;
	}

	iResult = recv(g_Socket, g_RecvRQ.GetRearBufferPtr(), g_RecvRQ.DirectEnqueueSize(), 0);
	if (iResult == SOCKET_ERROR || iResult == 0)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"RecvProc() error:%d\n", WSAGetLastError());
		g_bExitFlag = TRUE;

		return FALSE;
	}

	//----------------------------------------------------------
	// RecvQ�� �����ɷ� �Ѵ�.
	//----------------------------------------------------------
	g_RecvRQ.MoveRear(iResult);

	while (TRUE)
	{
		CPacket Packet;

		//-----------------------------
		// ��Ŷ ������ Ȯ�� (���)
		//-----------------------------
		iUseSize = g_RecvRQ.GetUseSize();

		//-----------------------------------
		// 1. ����� �Դ°�?
		//-----------------------------------
		if (sizeof(st_PACKET_HEADER) > iUseSize)
		{
			break;
		}

		//-----------------------------
		// 1. ��� ���� �̱�
		//-----------------------------
		g_RecvRQ.Peek((char*)&Header, sizeof(st_PACKET_HEADER));

		//-----------------------------
		// 3. ��Ŷ �ڵ尡 ������?
		//-----------------------------
		if (Header.byCode != dfNETWORK_PACKET_CODE)
		{
			g_RecvRQ.MoveFront(sizeof(st_PACKET_HEADER));
			break;
		}

		//-----------------------------------
		// 4. ���̷ε� �ޱ�
		//-----------------------------------
		if (iUseSize < (int)(sizeof(st_PACKET_HEADER) + Header.bySize))
		{
			break;
		}

		// 4-1 ��� ����
		g_RecvRQ.MoveFront(sizeof(st_PACKET_HEADER));

		// 4-2 ���̷ε� ���
		iResult = g_RecvRQ.Get(Packet.GetBufferPtr(), (int)Header.bySize);

		// ����ȭ���� Rear ������ �̵�
		Packet.MoveRear(iResult);

		//-----------------------------------
		// 5. ��Ŷ �б�
		//-----------------------------------
		if (!netPacketProc(Header.byType, &Packet))
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL netSendProc(void)
{
	//----------------------------------------------------------
	// ���� �� �����ٰ� ���� �� �ְ� �ƴ�.
	// 
	// 1. ���� ���� ����
	// 2. �۽� ���۰� �� á����(���� �����찡 0 / ���̱� ���)�� ���� �� �ְԵƴ�.
	//    �̹� �����ӿ� �׾Ƶξ��� �޽����� ���� �������Ѵ�.
	//----------------------------------------------------------
	int iResult;

	//-----------------------------
	// ���� üũ
	//-----------------------------
	if (!g_bConnected)
	{
		return FALSE;
	}

	//-----------------------------
	// ���� �� �ִ��� üũ
	//-----------------------------
	if (!g_bSendPossibility)
	{
		return FALSE;
	}

	while (TRUE)
	{
		if (g_SendRQ.GetUseSize() == 0)
		{
			break;
		}

		iResult = send(g_Socket, g_SendRQ.GetFrontBufferPtr(), g_SendRQ.DirectDequeueSize(), 0);
		if (iResult == SOCKET_ERROR)
		{
			// ---------------------------------------------
			// ���� ���Ͽ����� Send������
			// 1. �۽��� �۽� ���۰� �� ã�� �� �̻� ���� �� ���ų�.
			// 2. ���Ͽ� ��ϵ� ������ ������ �� �̻� ��ȿ���� �ʰų�.
			// 
			// Q1. �׷� Send���� WSAEWOLDBLOCK�� �ߴ� ����?
			// A1. �۽��� �۽� ���۰� �� ã��.
			// ---------------------------------------------
			errno_t err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				g_bSendPossibility = FALSE;
				return FALSE;
			}
			// ---------------------------------------------
			// �� �� ����
			// ---------------------------------------------
			else
			{
				_LOG_FILE(dfLOG_LEVEL_ERROR, L"SendProc error:%d\n", err);
				return FALSE;
			}
		}
		g_SendRQ.MoveFront(iResult);
	}

	return TRUE;
}

BOOL Send_Unicast(st_PACKET_HEADER* pHeader, CPacket* pPayload)
{
	int iResult;
	int	iSendQFreeSize;
	int iMessageSize;

	if (!g_bConnected)
	{
		return FALSE;
	}

	iMessageSize = sizeof(st_PACKET_HEADER) + pPayload->GetUseSize();
	iSendQFreeSize = g_SendRQ.DirectEnqueueSize();

	//-----------------------------------------
	// ��Ŷ ���� �Ұ�
	//-----------------------------------------
	if (iSendQFreeSize < iMessageSize)
	{
		_LOG_FILE(dfLOG_LEVEL_ERROR, L"Send_Unicast() Info > SendQ Overflow [SendQFreeSize:%d] [MssageSize:%d]\n", iSendQFreeSize, iMessageSize);
		return FALSE;
	}
	//-----------------------------------------
	// ��Ŷ ���� ����
	//-----------------------------------------
	else
	{
		//-----------------------------------------
		// 1. ��� �ֱ�
		//-----------------------------------------
		iResult = g_SendRQ.Put((const char*)pHeader, sizeof(st_PACKET_HEADER));
		if (iResult != sizeof(st_PACKET_HEADER))
		{
			_LOG_FILE(dfLOG_LEVEL_WARN,
				L"Send_Unicast() SendQ error > FreeSize :%d / ReqSize :%d\n",
				iSendQFreeSize, sizeof(st_PACKET_HEADER)
			);
		}

		//-----------------------------------------
		// 2. ���̷ε� �ֱ�
		//-----------------------------------------
		iResult = g_SendRQ.Put((const char*)pPayload->GetBufferPtr(), pPayload->GetUseSize());
		if (iResult != pPayload->GetUseSize())
		{
			_LOG_FILE(dfLOG_LEVEL_WARN,
				L"Send_Unicast() SendQ error > FreeSize :%d / ReqSize :%d\n",
				iSendQFreeSize, sizeof(st_PACKET_HEADER)
			);
		}

		// ---------------------------------------------
		// Send�� �õ��غ���.
		// ---------------------------------------------
		return netSendProc();
	}
}