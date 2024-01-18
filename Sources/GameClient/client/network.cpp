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
	// 윈속 2.2 초기화
	// ------------------------------
	WSAData wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	//---------------------------------------
	// 소켓 생성
	//---------------------------------------
	g_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_Socket == INVALID_SOCKET)
	{
		_LOG_FILE(dfLOG_LEVEL_ERROR, L"netStartUp() error %d\n", WSAGetLastError());
		g_bExitFlag = TRUE;
		return;
	}

	//---------------------------------------
	// 접속 IP, 포트 할당
	//---------------------------------------
	SOCKADDR_IN SockAddr;
	IN_ADDR		Addr;

//	DomainToIP(L"procademyserver.iptime.org", (IN_ADDR*)&Addr);
	//SockAddr.sin_addr = Addr;

	SockAddr.sin_family = AF_INET;
	InetPton(AF_INET, L"127.0.0.1", &SockAddr.sin_addr);	// TEST
	SockAddr.sin_port = htons(dfNETWORK_PORT);

	//---------------------------------------
	// AsyncSelect 등록
	//--------------------------------------
	iResult = WSAAsyncSelect(g_Socket, *hWnd, ASYCN_SOCKET, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);
	if (iResult != 0)
	{
		_LOG_FILE(dfLOG_LEVEL_ERROR, L"WSAAsyncSelect() error %d\n", WSAGetLastError());
		g_bExitFlag = TRUE;
		return;
	}

	//---------------------------------------
	// 접속, AsysnSelect 소켓으로 등록한 순간부터 
	// 논-블락 소켓으로 변경되었음.
	//---------------------------------------
	if (SOCKET_ERROR == connect(g_Socket, (SOCKADDR*)&SockAddr, sizeof(SOCKADDR_IN)))
	{
		errno_t err = WSAGetLastError();

		//---------------------------------------
		// 논블락 소켓에서 connect의 에러가 WOULDBLOCK이 아니라면 경우는 두 가지.
		// 
		// 1. 접속 요청 포트가 LISTENING 상태가 아니다.
		// 2. 서버쪽 BackLogQ가 꽉 찼다.
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
	// 오류 발생 여부 확인
	//--------------------------------
	if (0 != WSAGETSELECTERROR(lParam))
	{
		_LOG(dfLOG_LEVEL_ERROR, L"netIOProcess() error:%ld\n", WSAGETSELECTERROR(lParam));
		return FALSE;
	}

	//--------------------------------
	// 메세지 처리
	//--------------------------------
	switch (lParam)
	{
	case FD_CONNECT: // 연결 성공처리
		g_bConnected = TRUE;
		return TRUE;

	case FD_CLOSE:	// 연결 끊김 처리
		return FALSE;

	case FD_READ:	// Read 확인
		if (!netRecvProc())
		{
			return FALSE;
		}
		return TRUE;

	case FD_WRITE:	// Write 확인
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
	// 해당 소켓에는 무조건 recv를 해줘야하며, 만약 recv를 건너뛴다면 다시는 FD_READ 이벤트가 발생하지 않는다.
	// FD_READ 이벤트는 언제 발생할 지 모르므로 이번 프레임에서 처리할 수 있는 패킷은 다 처리해야한다.
	//----------------------------------------------------------
	st_PACKET_HEADER Header;

	int iResult;
	int iUseSize;

	//-----------------------------
	// 접속 체크
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
	// RecvQ에 받은걸로 한다.
	//----------------------------------------------------------
	g_RecvRQ.MoveRear(iResult);

	while (TRUE)
	{
		CPacket Packet;

		//-----------------------------
		// 패킷 사이즈 확인 (헤더)
		//-----------------------------
		iUseSize = g_RecvRQ.GetUseSize();

		//-----------------------------------
		// 1. 헤더는 왔는가?
		//-----------------------------------
		if (sizeof(st_PACKET_HEADER) > iUseSize)
		{
			break;
		}

		//-----------------------------
		// 1. 헤더 정보 뽑기
		//-----------------------------
		g_RecvRQ.Peek((char*)&Header, sizeof(st_PACKET_HEADER));

		//-----------------------------
		// 3. 패킷 코드가 같은가?
		//-----------------------------
		if (Header.byCode != dfNETWORK_PACKET_CODE)
		{
			g_RecvRQ.MoveFront(sizeof(st_PACKET_HEADER));
			break;
		}

		//-----------------------------------
		// 4. 페이로드 받기
		//-----------------------------------
		if (iUseSize < (int)(sizeof(st_PACKET_HEADER) + Header.bySize))
		{
			break;
		}

		// 4-1 헤더 삭제
		g_RecvRQ.MoveFront(sizeof(st_PACKET_HEADER));

		// 4-2 페이로드 출력
		iResult = g_RecvRQ.Get(Packet.GetBufferPtr(), (int)Header.bySize);

		// 직렬화버퍼 Rear 포인터 이동
		Packet.MoveRear(iResult);

		//-----------------------------------
		// 5. 패킷 분기
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
	// 보낼 수 없었다가 보낼 수 있게 됐다.
	// 
	// 1. 최초 연결 성공
	// 2. 송신 버퍼가 꽉 찼었다(상대방 윈도우가 0 / 네이글 사용)가 보낼 수 있게됐다.
	//    이번 프레임에 쌓아두었던 메시지를 전부 보내야한다.
	//----------------------------------------------------------
	int iResult;

	//-----------------------------
	// 접속 체크
	//-----------------------------
	if (!g_bConnected)
	{
		return FALSE;
	}

	//-----------------------------
	// 보낼 수 있는지 체크
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
			// 논블락 소켓에서의 Send에러는
			// 1. 송신측 송신 버퍼가 꽉 찾서 더 이상 보낼 수 없거나.
			// 2. 소켓에 등록된 수신측 정보가 더 이상 유효하지 않거나.
			// 
			// Q1. 그럼 Send에서 WSAEWOLDBLOCK이 뜨는 경우는?
			// A1. 송신측 송신 버퍼가 꽉 찾다.
			// ---------------------------------------------
			errno_t err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				g_bSendPossibility = FALSE;
				return FALSE;
			}
			// ---------------------------------------------
			// 그 외 에러
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
	// 패킷 삽입 불가
	//-----------------------------------------
	if (iSendQFreeSize < iMessageSize)
	{
		_LOG_FILE(dfLOG_LEVEL_ERROR, L"Send_Unicast() Info > SendQ Overflow [SendQFreeSize:%d] [MssageSize:%d]\n", iSendQFreeSize, iMessageSize);
		return FALSE;
	}
	//-----------------------------------------
	// 패킷 삽입 가능
	//-----------------------------------------
	else
	{
		//-----------------------------------------
		// 1. 헤더 넣기
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
		// 2. 페이로드 넣기
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
		// Send를 시도해본다.
		// ---------------------------------------------
		return netSendProc();
	}
}