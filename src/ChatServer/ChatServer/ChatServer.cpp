
#pragma comment(lib, "Winmm")
#pragma comment(lib, "ws2_32")

#include <ws2tcpip.h>
#include <winsock.h>
#include <mstcpip.h>
#include <windows.h>
#include <time.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <unordered_map>

#include "CommonDefine.h"
#include "ErrorCode.h"

#include "Protocol.h"
#include "CDBConnector_TLS.h"
#include "CLanClient.h"
#include "CNetServer.h"

#include "ClientManager.h"

#include "ChatServer.h"

namespace cov1013
{
	ChatServer::ChatServer()
	{
		m_AuthWaitCount = 0;
		m_AuthFailedTotal = 0;
		m_AuthCompletedTotal = 0;
		m_AuthElapsedTime = 0.f;

		m_TimeOutMax = INT_MAX;
		m_bLoop = false;
		m_bRunFlag = false;

		//m_pDBConnector = nullptr;
		//m_pRedisConnector = nullptr;
	}

	ChatServer::~ChatServer()
	{
		if (nullptr != m_pDBConnector)
		{
			delete m_pDBConnector;
		}
	}

	void ChatServer::Run(void)
	{
		if (m_bRunFlag)
		{
			wprintf(L"Overlapped Running");
			return;
		}
		m_bRunFlag = true;

		if (!m_Parser.LoadFile(L"../Config.txt"))
		{
			wprintf(L"Server Config Open Failed");
			m_bRunFlag = false;
			return;
		}

		WCHAR	BIND_IP[16];
		WORD	BIND_PORT;
		m_Parser.GetValue(L"BIND_IP", BIND_IP, 16);
		m_Parser.GetValue(L"BIND_PORT", &BIND_PORT);

		DWORD	IOCP_WORKER_THREAD;
		DWORD	IOCP_ACTIVE_THREAD;
		DWORD	CLIENT_MAX;
		m_Parser.GetValue(L"IOCP_WORKER_THREAD", &IOCP_WORKER_THREAD);
		m_Parser.GetValue(L"IOCP_ACTIVE_THREAD", &IOCP_ACTIVE_THREAD);
		m_Parser.GetValue(L"CLIENT_MAX", &CLIENT_MAX);

		//------------------------------------------------------
		// Packet Encode / Decode Key
		//------------------------------------------------------
		BYTE PACKET_CODE;
		BYTE PACKET_KEY;
		int	 PACKET_SIZE_MAX;
		m_Parser.GetValue(L"PACKET_CODE", &PACKET_CODE);
		m_Parser.GetValue(L"PACKET_KEY", &PACKET_KEY);
		m_Parser.GetValue(L"PACKET_SIZE_MAX", &PACKET_SIZE_MAX);
		CPacket::SetPacketCode(PACKET_CODE);
		CPacket::SetPacketKey(PACKET_KEY);

		//------------------------------------------------------
		// Network Setting
		//------------------------------------------------------
		bool NAGLE_FLAG;
		bool ZERO_COPY_FLAG;
		bool KEEP_ALIVE_FLAG;
		m_Parser.GetValue(L"NAGLE_FLAG", &NAGLE_FLAG);
		m_Parser.GetValue(L"ZERO_COPY_FLAG", &ZERO_COPY_FLAG);
		m_Parser.GetValue(L"KEEP_ALIVE_FLAG", &KEEP_ALIVE_FLAG);

		//------------------------------------------------------
		// Server Setting
		//------------------------------------------------------
		bool SELF_SEND_FLAG;
		m_Parser.GetValue(L"SELF_SEND_FLAG", &SELF_SEND_FLAG);
		m_Parser.GetValue(L"REDIS_FALG", &m_bRedisFlag);
		if (m_bRedisFlag)
		{
			m_Parser.GetValue(L"REDIS_IOCP_WORKER_THREAD", &m_NumberOfRedisIOCPWorkerThread);
			m_Parser.GetValue(L"REDIS_IOCP_ACTIVE_THREAD", &m_NumberOfRedisIOCPActiveThread);
		}

		//------------------------------------------------------
		// SERVICE : 미응답 유저 타임아웃 처리
		//------------------------------------------------------
		m_Parser.GetValue(L"TIMEOUT_DISCONNECT", &m_TimeOutMax);

		//------------------------------------------------------
		// DB 연결
		//------------------------------------------------------
		//m_pDBConnector = new CDBConnector_TLS(L"127.0.0.1", L"root", L"0000", L"accountdb", 3306);
		//m_pRedisConnector = new CRedis_TLS();

		//------------------------------------------------------
		// 네트워크 라이브러리 실행
		//------------------------------------------------------
		if (!Start(BIND_IP, BIND_PORT, IOCP_WORKER_THREAD, IOCP_ACTIVE_THREAD, NAGLE_FLAG, ZERO_COPY_FLAG, CLIENT_MAX, PACKET_SIZE_MAX, SELF_SEND_FLAG, KEEP_ALIVE_FLAG))
		{
			wprintf(L"Network Library Start Failed\n");
			m_bRunFlag = false;
			return;
		}

		//----------------------------------------------------------
		// 콘솔창 초기화 및 컨트롤러 실행
		//----------------------------------------------------------
		EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_GRAYED);
		DrawMenuBar(GetConsoleWindow());
		if (m_bRedisFlag)
		{
			system("mode con cols=70 lines=53");
		}
		else
		{
			system("mode con cols=70 lines=42");
		}
		wprintf(L"Server Open...\n");

		m_bLoop = true;

		//----------------------------------------------------------
		// Monitor Thread 생성
		//----------------------------------------------------------
		m_hMonitorThread = (HANDLE)_beginthreadex(nullptr, 0, MonitorThread, this, false, nullptr);

		//----------------------------------------------------------
		// Update Thread 생성
		//----------------------------------------------------------
		m_hUpdateEvent = (HANDLE)CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hUpdateThread = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, this, false, nullptr);

		//----------------------------------------------------------
		// HeartBeat Thread 생성
		//----------------------------------------------------------
		m_hHeartBeatThread = (HANDLE)_beginthreadex(nullptr, 0, HeartBeatThread, this, false, nullptr);
		m_hHeartBeatEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		//----------------------------------------------------------
		// Redis Thread 생성
		//----------------------------------------------------------
		if (m_bRedisFlag)
		{
			/*m_hRedisIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, m_NumberOfRedisIOCPActiveThread);
			for (int i = 0; i < m_NumberOfRedisIOCPWorkerThread; i++)
			{
				m_hRedisIOCPWorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, RedisThread, this, false, nullptr);
			}*/
		}

		//----------------------------------------------------------
		// MonitorServer Connect Thread 생성
		//----------------------------------------------------------
		m_hMonitorServerConnectThread = (HANDLE)_beginthreadex(nullptr, 0, this->MonitorServerConnectThread, this, false, nullptr);
		m_hMonitorServerConnectThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		
		//----------------------------------------------------------
		// 컨트롤러 실행
		//----------------------------------------------------------
		while (m_bLoop)
		{
			Controler();
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 서버 종료
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::Quit(void)
	{
		//----------------------------------------------------------------------
		// 서버가 실행중이라면 완벽한 종료
		//----------------------------------------------------------------------
		if (m_bRunFlag == true)
		{
			Stop(en_STOP_TYPE::QUIT);
		}
		//----------------------------------------------------------------------
		// 서버가 정지중이라면 윈도우 리소스만 정리
		//----------------------------------------------------------------------
		else
		{
			Stop(en_STOP_TYPE::RESOURCE_RELEASE);
		}

		//----------------------------------------------------------------------
		// 컨텐츠 파트의 모든 클라이언트가 삭제될 때 까지 대기
		//----------------------------------------------------------------------
		while (m_Clients.size() != 0) {};

		//----------------------------------------------------------------------
		// 스레드 정지 및 리소스 정리
		//----------------------------------------------------------------------
		m_bLoop = false;

		//----------------------------------------------------------------------
		// Monitor Server Connect Thread 정지
		//----------------------------------------------------------------------
		SetEvent(m_hMonitorServerConnectThreadEvent);
		WaitForSingleObject(m_hMonitorServerConnectThread, INFINITE);
		CloseHandle(m_hMonitorServerConnectThread);
		CloseHandle(m_hMonitorServerConnectThreadEvent);

		//----------------------------------------------------------------------
		// HeartBeat Thread 정지
		//----------------------------------------------------------------------
		SetEvent(m_hHeartBeatEvent);
		WaitForSingleObject(m_hHeartBeatThread, INFINITE);
		CloseHandle(m_hHeartBeatThread);
		CloseHandle(m_hHeartBeatEvent);

		//----------------------------------------------------------------------
		// Monitor Thread 정지
		//----------------------------------------------------------------------
		WaitForSingleObject(m_hMonitorThread, INFINITE);
		CloseHandle(m_hMonitorThread);

		//----------------------------------------------------------------------
		// Update Thread 정지
		//----------------------------------------------------------------------
		st_UPDATE_JOB* pNewJob = m_UpdateJobPool.Alloc();
		pNewJob->Type = st_UPDATE_JOB::en_JOB_EXIT;
		m_UpdateJobQueue.Enqueue(pNewJob);
		SetEvent(m_hUpdateEvent);
		WaitForSingleObject(m_hUpdateThread, INFINITE);
		CloseHandle(m_hUpdateEvent);
		CloseHandle(m_hUpdateThread);

		//----------------------------------------------------------------------
		// Redis Thread 정지
		//----------------------------------------------------------------------
		if (m_bRedisFlag)
		{
			PostQueuedCompletionStatus(m_hRedisIOCP, 0, (ULONG_PTR)nullptr, nullptr);
			WaitForMultipleObjects(m_NumberOfRedisIOCPWorkerThread, m_hRedisIOCPWorkerThreads, TRUE, INFINITE);
			CloseHandle(m_hRedisIOCP);

			for (int i = 0; i < m_NumberOfRedisIOCPWorkerThread; i++)
			{
				CloseHandle(m_hRedisIOCPWorkerThreads[i]);
			}
		}

		//----------------------------------------------------------------------
		// 자료 관리 컨테이너 정리
		//----------------------------------------------------------------------
		ClearSector();
		ClearClients();

		//----------------------------------------------------------
		// 모니터링 클라이언트 연결 종료
		//----------------------------------------------------------
		m_MonitorClient.Stop();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 서버 컨트롤러
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::Controler(void)
	{
		WCHAR ControlKey = _getwch();

		//----------------------------------------------------------------------
		// 서버 종료
		//----------------------------------------------------------------------
		if ('q' == ControlKey || 'Q' == ControlKey)
		{
			Quit();
			return;
		}

		//----------------------------------------------------------------------
		// 서버 정지 및 재시작
		//----------------------------------------------------------------------
		if ('s' == ControlKey || 'S' == ControlKey)
		{
			if (m_bRunFlag)
			{
				//----------------------------------------------------------
				// 라이브러리 STOP
				//----------------------------------------------------------
				Stop(en_STOP_TYPE::STOP);

				//----------------------------------------------------------
				// 컨텐츠 파트의 모든 클라이언트가 삭제될 때 까지 대기
				//----------------------------------------------------------
				while (m_Clients.size() != 0) {};
				m_bRunFlag = false;
			}
			else
			{
				Restart();
				m_bRunFlag = true;
			}
			return;
		}

		//----------------------------------------------------------------------
		// 프로파일링 데이터 출력
		//----------------------------------------------------------------------
		if ('p' == ControlKey || 'P' == ControlKey)
		{
			OutputProfilingData();
			return;
		}

		//----------------------------------------------------------------------
		// 로그 레벨 변경
		//----------------------------------------------------------------------
		if ('1' == ControlKey)
		{
			SYSLOG_LEVEL(eLOG_LEVEL_DEBUG);
			return;
		}
		if ('2' == ControlKey)
		{
			SYSLOG_LEVEL(eLOG_LEVEL_ERROR);
			return;
		}
		if ('3' == ControlKey)
		{
			SYSLOG_LEVEL(eLOG_LEVEL_SYSTEM);
			return;
		}
		if ('0' == ControlKey)
		{
			SYSLOG_LEVEL(eLOG_LEVEL_OFF);
			return;
		}
	}

	int ChatServer::GetAuthCompletedPlayerCount(void)
	{
		int iResult = 0;
		unordered_map<SESSION_ID, st_CLIENT*>::iterator iter = m_Clients.begin();

		//-----------------------------------------------
		// 클라이언트 관리 컨테이너를 돌면서 노드 반환
		//-----------------------------------------------
		for (iter; iter != m_Clients.end(); ++iter)
		{
			if (iter->second->bLogin == true)
			{
				iResult += 1;
			}
		}

		return iResult;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::OnConnectionRequest(const WCHAR* ConnectIP, const WORD ConnectPORT)
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 새로운 클라이언트가 접속했다.
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::OnClientJoin(const SESSION_ID SessionID)
	{
		st_UPDATE_JOB* pNewJob = m_UpdateJobPool.Alloc();		
		pNewJob->Type = st_UPDATE_JOB::en_JOB_CONNECT;
		pNewJob->SessionID = SessionID;
		pNewJob->pMessage = nullptr;

		m_UpdateJobQueue.Enqueue(pNewJob);
		SetEvent(m_hUpdateEvent);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 기존 클라이언트가 연결 종료됐다.
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::OnClientLeave(const SESSION_ID SessionID)
	{
		st_UPDATE_JOB* pNewJob = m_UpdateJobPool.Alloc();
		pNewJob->Type = st_UPDATE_JOB::en_JOB_DISCONNECT;
		pNewJob->SessionID = SessionID;
		pNewJob->pMessage = nullptr;

		m_UpdateJobQueue.Enqueue(pNewJob);
		SetEvent(m_hUpdateEvent);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 패킷을 수신 받았다.
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::OnRecv(const SESSION_ID SessionID, CPacket* pRecvPacket)
	{
		// 패킷이 반환되지 않게 래퍼런스 카운트 증가
		// 해당 패킷은 UpdateThread 처리 완료 후 반환된다.

#ifdef __MULTI_THREAD_DEBUG_MODE__
		pRecvPacket->AddRef(CPacket::en_LOGIC::RECV_PROC + 20, SessionID);
#else
		pRecvPacket->AddRef();
#endif
		
		st_UPDATE_JOB* pNewJob = m_UpdateJobPool.Alloc();
		pNewJob->Type = st_UPDATE_JOB::en_JOB_RECV_MESSAGE;
		pNewJob->SessionID = SessionID;
		pNewJob->pMessage = pRecvPacket;

		m_UpdateJobQueue.Enqueue(pNewJob);
		SetEvent(m_hUpdateEvent);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 패킷을 송신했다.
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::OnSend(const SESSION_ID SessionID, const DWORD dwSendLength)
	{
	}

	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::OnWorkerThreadBegin(void)
	{
	}

	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::OnWorkerThreadEnd(void)
	{
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 라이브러리에서 에러가 발생했다.
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::OnError(const en_ERROR_CODE eErrCode, const SESSION_ID SessionID)
	{
		switch (eErrCode)
		{
		case en_ERROR_WINSOCK_INIT:
		case en_ERROR_WINSOCK_BIND:
		case en_ERROR_WINSOCK_LISTEN_SOCK:
		case en_ERROR_WINSOCK_SET_NAGLE:
		case en_ERROR_WINSOCK_SET_RST:
			CCrashDump::Crash();

			break;
		case en_ERROR_RING_BUF_RECV:
		case en_ERROR_RING_BUF_SEND:
			CCrashDump::Crash();

			break;
		case en_ERROR_CONFIG_OVER_THREAD:
		case en_ERROR_CONFIG_OVER_SESSION:
			CCrashDump::Crash();

			break;

		case en_ERROR_NET_PACKET:
			CCrashDump::Crash();
			break;

		case en_ERROR_WINSOCK_OVER_CONNECT:
			CCrashDump::Crash();

			break;
		case en_ERROR_DECODE_FAILD:
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Decode Error - (SessionID:%I64u)\n", SessionID);
			Disconnect(SessionID);
			break;

		default:
			break;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Update Thread Jod 처리 : 신규 접속
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::JobProcedure_Connect(const SESSION_ID SessionID)
	{
		if (nullptr != FindClient(SessionID))
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Miss Match SessionID - (Req:%I64u)\n", SessionID);
			Disconnect(SessionID);
			return false;
		}

		CreateClient(SessionID);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Update Thread Jod 처리 : 접속 종료
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::JobProcedure_Disconnect(const SESSION_ID SessionID)
	{
		return DeleteClient(SessionID);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Update Thread Jod 처리 : 요청 패킷
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::JobProcedure_RecvMessage(const SESSION_ID SessionID, CPacket* pRecvPacket)
	{
		bool bResult = false;
		WORD wPacketType;
		*pRecvPacket >> wPacketType;

		switch (wPacketType)
		{
		case en_PACKET_CS_CHAT_REQ_LOGIN:
			bResult = PacketProcedure_Login(SessionID, pRecvPacket);
			break;

		case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
			bResult = PacketProcedure_SectorMove(SessionID, pRecvPacket);
			break;

		case en_PACKET_CS_CHAT_REQ_MESSAGE:
			bResult = PacketProcedure_ChatMessage(SessionID, pRecvPacket);
			break;

		case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
			bResult = PacketProcedure_Heartbeat(SessionID);
			break;

		default:
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Miss Match Job Type - (SessionID:%I64u)\n", SessionID);
			Disconnect(SessionID);

			break;
		}

#ifdef __MULTI_THREAD_DEBUG_MODE__
		pRecvPacket->SubRef(JOB_PROC_CHAT_MSG + 30, SessionID);
#else
		pRecvPacket->SubRef();
#endif
		return bResult;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Update Thread Jod 처리 : 하트비트
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::JobProcedure_HeartBeat(void)
	{
		if (m_Clients.size() == 0)
		{
			return true;
		}

		DWORD CurTime = timeGetTime();
		unordered_map<SESSION_ID, st_CLIENT*>::iterator iter = m_Clients.begin();
		unordered_map<SESSION_ID, st_CLIENT*>::iterator end = m_Clients.end();

		for (iter; iter != end; ++iter)
		{
			if ((CurTime - iter->second->LastRecvTime) >= m_TimeOutMax)
			{
				Disconnect(iter->second->SessionID);
				//LOG(L"TimeOut", eLOG_LEVEL_DEBUG, L"### TimeOut - AccountNo:%lld | ID:%s\n", iter->second->AccountNo, iter->second->ID);
			}
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Update Thread Jod 처리 : Redis 확인 완료 -> 로그인 완료 처리
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::JobProcedure_RedisCompeleted(const SESSION_ID SessionID)
	{
		//------------------------------------------------------------------
		// [예외] 로그인 요청 패킷을 보내두고 Redis 확인하는 도중 삭제될수도 있음.
		// 반대로, 존재하는 세션이라고 판단된 경우, Delete 작업이 해당 작업보다 앞설 수 없으므로 
		// 무조건 존재하는 경우, 만약 라이브러리에서 이미 끊어졌다면 SendPacket이 실패할 것임.
		//------------------------------------------------------------------
		st_CLIENT* pClient = FindClient(SessionID);
		if (pClient == nullptr)
		{
			return false;
		}

		//------------------------------------------------------------------
		// 로그인 상태로 변경
		//------------------------------------------------------------------
		pClient->bLogin = true;

		//------------------------------------------------------------------
		// 로그인 응답 메세지 생성 후 송신
		//------------------------------------------------------------------
		CPacket* pSendMSG = CPacket::Alloc();
		MakePacket_Login(pSendMSG, 1, pClient->AccountNo);
		SendPacket(SessionID, pSendMSG);
		m_SendLoginMessage += 1;
		pSendMSG->SubRef();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 패킷 요청 처리 : 로그인 처리
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::PacketProcedure_Login(const SESSION_ID SessionID, CPacket* pMessage)
	{
		m_RecvLoginMessage += 1;

		//---------------------------------------------------------------
		// [예외] 존재하는 클라이언트인지 확인
		//---------------------------------------------------------------
		st_CLIENT* pClient = FindClient(SessionID);
		if (nullptr == pClient)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Miss Match SessionID - (Req:%I64u)\n", SessionID);
			Disconnect(SessionID);

			return false;
		}

		//---------------------------------------------------------------
		// 수신 시간 갱신
		//---------------------------------------------------------------
		pClient->LastRecvTime = timeGetTime();

		//---------------------------------------------------------------
		// [예외] 이미 로그인되어있는 클라이언트라면 실패 메세지 송신 후 리턴
		//---------------------------------------------------------------
		*pMessage >> pClient->AccountNo;
		if (pClient->bLogin)
		{
			CPacket* pSendMSG = CPacket::Alloc();
			MakePacket_Login(pSendMSG, 0, pClient->AccountNo);
			SendPacket(SessionID, pSendMSG);
			pSendMSG->SubRef();

			return true;
		}

		//---------------------------------------------------------------
		// 클라이언트가 준 정보로 클라이언트 세팅
		//---------------------------------------------------------------
		pMessage->Get_UNI16(pClient->ID, en_ID_MAX);
		pMessage->Get_UNI16(pClient->NickName, en_NICK_NAME_MAX);

		//---------------------------------------------------------------
		// 로그인 처리를 RedisThread에 넘긴다.
		//---------------------------------------------------------------
		if (m_bRedisFlag)
		{
			//---------------------------------------------------------------
			// 로그인 요청 정보를 RedisJob 구조체에 세팅해서 Enqeueue
			//---------------------------------------------------------------
			/*st_REDIS_JOB* pRedisJob = m_RedisJobPool.Alloc();
			pRedisJob->AccountNo = pClient->AccountNo;
			pRedisJob->SessionID = SessionID;
			pMessage->Get(pRedisJob->SessionKey, en_SESSION_KEY_MAX);

			PostQueuedCompletionStatus(m_hRedisIOCP, 0xFFFFFFFF, (ULONG_PTR)pRedisJob, nullptr);*/
		}
		//---------------------------------------------------------------
		// 로그인 처리를 UpdateThread에서 완료한다.
		//---------------------------------------------------------------
		else
		{
			//---------------------------------------------------------------
			// 로그인 상태로 변경
			//---------------------------------------------------------------
			pClient->bLogin = true;

			//---------------------------------------------------------------
			// 로그인 응답 메세지 생성 후 송신
			//---------------------------------------------------------------
			CPacket* pSendMSG = CPacket::Alloc();
			MakePacket_Login(pSendMSG, 1, pClient->AccountNo);
			SendPacket(SessionID, pSendMSG);
			m_SendLoginMessage += 1;
			pSendMSG->SubRef();
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 패킷 요청 처리 : 섹터 이동
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::PacketProcedure_SectorMove(const SESSION_ID SessionID, CPacket* pMessage)
	{
		m_RecvMoveSectorMessage += 1;

		//-----------------------------------------------
		// [예외] 존재하는 클라이언트인지 확인
		//-----------------------------------------------
		st_CLIENT* pClient = FindClient(SessionID);
		if (pClient == nullptr)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Miss Match SessionID - (Req:%I64u)\n", SessionID);
			Disconnect(SessionID);

			return false;
		}

		//-----------------------------------------------
		// [예외] 로그인 상태인지 확인
		//-----------------------------------------------
		if (!pClient->bLogin)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Not Login Client - (Req:%I64u)\n", SessionID);
			Disconnect(pClient->SessionID);

			return false;
		}

		//-----------------------------------------------
		// [예외] AccountNo가 같은지 확인
		//-----------------------------------------------
		INT64 AccountNo;
		*pMessage >> AccountNo;
		if (pClient->AccountNo != AccountNo)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Miss Match AccountNo - (Req:%I64u)\n", AccountNo);
			Disconnect(pClient->SessionID);

			return false;
		}

		pClient->LastRecvTime = timeGetTime();

		//-----------------------------------------------
		// 4. 페이로드 출력
		//-----------------------------------------------
		WORD wSectorX;
		WORD wSectorY;
		*pMessage >> wSectorX;
		*pMessage >> wSectorY;

		//-----------------------------------------------
		// [예외] 섹터 범위에서 벗어난 위치를 요청했는지 확인
		//-----------------------------------------------
		if (wSectorX >= en_SECTOR_MAX_X || wSectorY >= en_SECTOR_MAX_Y)
		{
			Disconnect(pClient->SessionID);
			return false;
		}

		//-----------------------------------------------
		// 5. 소속된 섹터에서 해당 클라이언트를 삭제한다.
		//-----------------------------------------------
		if (pClient->Sector.wX != (WORD)-1 && pClient->Sector.wY != (WORD)-1)
		{
			m_Sectors[pClient->Sector.wY][pClient->Sector.wX].remove(pClient->SessionID);
		}

		//-----------------------------------------------
		// 6. 새로운 섹터에 해당 클라이언트를 추가한다.
		//-----------------------------------------------
		pClient->Sector.wX = wSectorX;
		pClient->Sector.wY = wSectorY;
		m_Sectors[pClient->Sector.wY][pClient->Sector.wX].push_back(pClient->SessionID);

		//-----------------------------------------------
		// 7. 응답 메세지 생성 및 송신
		//-----------------------------------------------
		CPacket* pSendMSG = CPacket::Alloc();
		MakePacket_SectorMove(pSendMSG, pClient->AccountNo, pClient->Sector.wX, pClient->Sector.wY);
		SendPacket(SessionID, pSendMSG);
		m_SendMoveSectorMessage += 1;
		pSendMSG->SubRef();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 패킷 요청 처리 : 채팅 송신
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::PacketProcedure_ChatMessage(const SESSION_ID SessionID, CPacket* pMessage)
	{
		m_RecvChatMessage += 1; 

		//-----------------------------------------------
		// [예외] 존재하는 클라이언트인가?
		//-----------------------------------------------
		st_CLIENT* pClient = FindClient(SessionID);
		if (nullptr == pClient)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Miss Match SessionID - (Req:%I64u)\n", SessionID);
			Disconnect(SessionID);
			return false;
		}

		//-----------------------------------------------
		// [예외] 로그인되어있는 클라이언트인가?
		//-----------------------------------------------
		if (!pClient->bLogin)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Not Login Client - (Req:%I64u)\n", SessionID);
			Disconnect(pClient->SessionID);
			return false;
		}

		//-----------------------------------------------
		// [예외] AccountNo가 같은가?
		//-----------------------------------------------
		INT64 AccountNo;
		*pMessage >> AccountNo;
		if (pClient->AccountNo != AccountNo)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Miss Match AccountNo - (Req:%I64u)\n", AccountNo);
			Disconnect(pClient->SessionID);
			return false;
		}

		pClient->LastRecvTime = timeGetTime();

		//-----------------------------------------------
		// 페이로드 출력
		//-----------------------------------------------
		WORD wMessageLen;
		*pMessage >> wMessageLen;

		//-----------------------------------------------
		// 메세지 생성 및 송신
		//-----------------------------------------------
		CPacket* pSendMSG = CPacket::Alloc();
		MakePacket_ChatMessage(pSendMSG, pClient->AccountNo, pClient->ID, pClient->NickName, wMessageLen, (WCHAR*)pMessage->GetReadPos());
		SendPacket_Around(pClient, pSendMSG, true);
		pSendMSG->SubRef();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 패킷 요청 처리 : 하트비트 갱신
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::PacketProcedure_Heartbeat(const SESSION_ID SessionID)
	{
		st_CLIENT* pClient = FindClient(SessionID);
		pClient->LastRecvTime = timeGetTime();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 수신 패킷 만들기 : 로그인 응답
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::MakePacket_Login(CPacket* pSendMessage, const BYTE byStatus, const INT64 AccountNo)
	{
		*pSendMessage << (WORD)en_PACKET_CS_CHAT_RES_LOGIN;
		*pSendMessage << byStatus;
		*pSendMessage << AccountNo;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 수신 패킷 만들기 : 섹터 이동 응답
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::MakePacket_SectorMove(CPacket* pSendMessage, const INT64 AccountNo, const WORD shSectorX, const WORD shSectorY)
	{
		*pSendMessage << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
		*pSendMessage << AccountNo;
		*pSendMessage << shSectorX;
		*pSendMessage << shSectorY;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 수신 패킷 만들기 : 채팅 송신 응답
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::MakePacket_ChatMessage(CPacket* pSendMessage, const INT64 AccountNo, const WCHAR* szID, const WCHAR* szNickName, const WORD shChatMessageLen, const WCHAR* szChatMessage)
	{
		*pSendMessage << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE;
		*pSendMessage << AccountNo;
		pSendMessage->Put_UNI16(szID, en_ID_MAX);
		pSendMessage->Put_UNI16(szNickName, en_NICK_NAME_MAX);
		*pSendMessage << shChatMessageLen;
		pSendMessage->Put_UNI16(szChatMessage, shChatMessageLen);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 새로운 클라이언트 생성
	//////////////////////////////////////////////////////////////////////////////////
	ChatServer::st_CLIENT* ChatServer::CreateClient(const SESSION_ID SessionID)
	{
		//-----------------------------------------------
		// 1. 메모리풀에서 메모리 할당 후 세팅
		//-----------------------------------------------
		st_CLIENT* pNewClient = m_ClientPool.Alloc();
		pNewClient->bLogin = false;
		pNewClient->Sector.wX = INVALID_SECTOR_POS;
		pNewClient->Sector.wY = INVALID_SECTOR_POS;
		pNewClient->AccountNo = INVALID_ACCOUNT_NO;
		pNewClient->SessionID = SessionID;
		pNewClient->LastRecvTime = timeGetTime();

		//-----------------------------------------------
		// 2. 클라이언트 관리 컨테이너에 추가
		//-----------------------------------------------
		m_Clients.insert(pair<SESSION_ID, st_CLIENT*>(SessionID, pNewClient));

		return pNewClient;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 기존 클라이언트 찾기
	//////////////////////////////////////////////////////////////////////////////////
	ChatServer::st_CLIENT* ChatServer::FindClient(const SESSION_ID SessionID)
	{
		unordered_map<SESSION_ID, st_CLIENT*>::iterator ClientIter = m_Clients.find(SessionID);

		if (ClientIter == m_Clients.end())
		{
			return nullptr;
		}
		else
		{
			return m_Clients.find(SessionID)->second;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 기존 클라이언트 삭제
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::DeleteClient(const SESSION_ID SessionID)
	{
		st_CLIENT* pClient = FindClient(SessionID);
		unordered_map<SESSION_ID, st_CLIENT*>::iterator iter;

		if (pClient == nullptr)
		{
			Disconnect(SessionID);
			return false;
		}

		iter = m_Clients.find(pClient->SessionID);
		if (pClient->Sector.wX != (WORD)-1 && pClient->Sector.wY != (WORD)-1)
		{
			m_Sectors[pClient->Sector.wY][pClient->Sector.wX].remove(pClient->SessionID);
		}

		m_Clients.erase(iter);
		m_ClientPool.Free(pClient);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 클라이언트 관리 컨테이너 정리
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::ClearClients(void)
	{
		unordered_map<SESSION_ID, st_CLIENT*>::iterator iter = m_Clients.begin();

		//-----------------------------------------------
		// 클라이언트 관리 컨테이너를 돌면서 노드 반환
		//-----------------------------------------------
		for (iter; iter != m_Clients.end();)
		{
			iter = m_Clients.erase(iter);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 주변 섹터 정보 얻기
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND* pDestiation)
	{
		pDestiation->iCount = 0;

		// 현재 좌표의 섹터 Entry로 이동
		iSectorX = iSectorX - 1;
		iSectorY = iSectorY - 1;

		for (int y = 0; y < 3; y++)
		{
			// Clipping for Y
			if (((iSectorY + y) < 0) || ((iSectorY + y) >= en_SECTOR_MAX_Y))
			{
				continue;
			}

			for (int x = 0; x < 3; x++)
			{
				// Clipping for X
				if (((iSectorX + x) < 0) || ((iSectorX + x) >= en_SECTOR_MAX_X))
				{
					continue;
				}

				pDestiation->Around[pDestiation->iCount].wX = iSectorX + x;
				pDestiation->Around[pDestiation->iCount].wY = iSectorY + y;

				pDestiation->iCount++;
			}
		}
	}

	void ChatServer::ClearSector(void)
	{
		for (int y = 0; y < en_SECTOR_MAX_Y; y++)
		{
			for (int x = 0; x < en_SECTOR_MAX_X; x++)
			{
				list<SESSION_ID> Sector = m_Sectors[y][x];
				list<SESSION_ID>::iterator iter = Sector.begin();
				list<SESSION_ID>::iterator end = Sector.end();

				//-----------------------------------------------
				// 섹터에 존재하는 클라이언트 노드 삭제 (클라이언트를 삭제하는 게 아니다)
				//-----------------------------------------------
				for (iter; iter != end;)
				{
					iter = Sector.erase(iter);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 주변 섹터 전체 유저에게 패킷 송신
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::SendPacket_Around(st_CLIENT* pClient, CPacket* pSendMessage, const bool bSendMe)
	{
		st_SECTOR_AROUND SectorAround;

		//-----------------------------------------------
		// 1. 해당 캐릭터가 속한 섹터 영향권을 얻는다.
		//-----------------------------------------------
		GetSectorAround(pClient->Sector.wX, pClient->Sector.wY, &SectorAround);

		//-----------------------------------------------
		// 2. 섹터 한 곳 한 곳 마다 메세지를 뿌린다.
		//-----------------------------------------------
		for (int i = 0; i < SectorAround.iCount; i++)
		{
			SendPacket_SectorOne(SectorAround.Around[i].wX, SectorAround.Around[i].wY, pSendMessage);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 지정 섹터 하나에 속한 모든 유저에게 패킷 송신
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::SendPacket_SectorOne(const int iX, const int iY, CPacket* pSendMessage, const SESSION_ID ExceptSessionID)
	{
		//-----------------------------------------------
		// 1. 섹터를 찾는다
		//-----------------------------------------------
		list<SESSION_ID> Sector = m_Sectors[iY][iX];
		if (Sector.size() == 0)
		{
			return;
		}

		m_SectorPlayerCount += Sector.size();

		//-----------------------------------------------
		// 2. 섹터에 존재하는 모든 세션에게 패킷을 보낸다.
		//-----------------------------------------------
		list<SESSION_ID>::iterator iter = Sector.begin();
		list<SESSION_ID>::iterator end = Sector.end();

		for (iter; iter != end; ++iter)
		{
			++m_SendChatMessage;
			SendPacket(*iter, pSendMessage);
		}
	}

	unsigned int __stdcall ChatServer::UpdateThread(void* lpParam)
	{
		int iResult = ((ChatServer*)lpParam)->UpdateThread_Procedure();

		return iResult;
	}

	int ChatServer::UpdateThread_Procedure(void)
	{
		DWORD		dwTransferred;
		OVERLAPPED* pOverlapped;
		st_UPDATE_JOB*		pJob;

		while (true)
		{
			if (WAIT_OBJECT_0 != WaitForSingleObject(m_hUpdateEvent, INFINITE))
			{
				break;
			}

			while (m_UpdateJobQueue.GetCapacity() > 0)
			{
				st_UPDATE_JOB* pJob;
				m_UpdateJobQueue.Dequeue(&pJob);

				switch (pJob->Type)
				{
				case st_UPDATE_JOB::en_JOB_CONNECT:
					JobProcedure_Connect(pJob->SessionID);
					break;

				case st_UPDATE_JOB::en_JOB_DISCONNECT:
					JobProcedure_Disconnect(pJob->SessionID);
					break;

				case st_UPDATE_JOB::en_JOB_RECV_MESSAGE:
					JobProcedure_RecvMessage(pJob->SessionID, pJob->pMessage);
					break;

				case st_UPDATE_JOB::en_JOB_HEARTBEAT:
					JobProcedure_HeartBeat();
					break;

				case st_UPDATE_JOB::en_JOB_REDIS_COMPELETED:
					JobProcedure_RedisCompeleted(pJob->SessionID);
					break;

				case st_UPDATE_JOB::en_JOB_EXIT:
					m_UpdateJobPool.Free(pJob);
					return 0;

				default:
					CCrashDump::Crash();
					break;
				}

				m_UpdateJobPool.Free(pJob);
				m_UpdateTPS++;
			}
		}

		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Monitor Thread
	// 
	//////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall ChatServer::MonitorThread(void* lpParam)
	{
		return ((ChatServer*)lpParam)->MonitorThread_Procedure();
	}
	int ChatServer::MonitorThread_Procedure(void)
	{
		CCpuUsage	CpuUsage;
		CPDH		PDH(L"ChatServer");

		WCHAR szFileName[32];
		SYSTEMTIME stNowTime;
		GetLocalTime(&stNowTime);
		wsprintf(szFileName,
			L"%04d.%02d.%02d %02d.%02d.%02d",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond
		);

		while (m_bLoop)
		{

//#ifdef __PROFILING__
//			static bool bFlag = false;
//
//			if (!bFlag)
//			{
//				DWORD dwCurTime = timeGetTime();
//				if (dwCurTime - m_StartTime >= 3600000)
//				{
//					m_StartTime = timeGetTime();
//
//					if (m_Clients.size() > 0 && !bFlag)
//					{
//						OutputProfilingData();
//						bFlag = true;
//					}
//				}
//			}
//#endif
			//--------------------------------------------------------
			// 모니터링 데이터 갱신
			//--------------------------------------------------------
			CpuUsage.UpdateCpuTime();
			PDH.Collect();

			//--------------------------------------------------------
			// 서버 시작 시간 출력
			//--------------------------------------------------------
			wprintf(L"[CHAT SERVER] StartTime : %s\n\n", szFileName);

			//--------------------------------------------------------
			// 로그 선택 조작 방법 출력
			//--------------------------------------------------------
			en_LOG_LEVEL eLogLevel = CLogger::GetLogLevel();
			if (eLogLevel == eLOG_LEVEL_DEBUG)
			{
				wprintf(L"LOG LEVEL : DEBUG");
			}
			else if (eLogLevel == eLOG_LEVEL_ERROR)
			{
				wprintf(L"LOG LEVEL : ERROR");
			}
			else if (eLogLevel == eLOG_LEVEL_SYSTEM)
			{
				wprintf(L"LOG LEVEL : SYSTEM");
			}
			else if (eLogLevel == eLOG_LEVEL_OFF)
			{
				wprintf(L"LOG LEVEL : OFF");
			}

			wprintf(L" (1 : DEBUG | 2 : ERROR | 3 : SYSTEM | 0 : OFF)\n");

			if (m_bRunFlag)
			{
				wprintf(L"NOW MODE : PLAY");
			}
			else
			{
				wprintf(L"NOW MODE : STOP");
			}
			wprintf(L" (Q : QUIT  | S : STOP/PLAY)\n");

			//--------------------------------------------------------
			// 모니터링 서버로 전송할 데이터 목록
			//--------------------------------------------------------
			int PacketPoolSize = CPacket::GetPoolCapacity();
			int JobPoolSize = m_UpdateJobPool.GetCapacity();
			int SessionCount = GetSessionCount();
			int AuthComClientCount = GetAuthCompletedPlayerCount();
			float ProcessCpuUsage = CpuUsage.ProcessTotal();

			//-------------------------------------------------------
			// 모니터링 서버로 데이터 전송
			//-------------------------------------------------------
			int	DataValues[30] = { 0 };
			int	DataValuesIndex = 0;
			DataValues[DataValuesIndex++] = true;
			DataValues[DataValuesIndex++] = ProcessCpuUsage;
			DataValues[DataValuesIndex++] = PDH.GetProcessCommitMemory() / (1024 * 1024);
			DataValues[DataValuesIndex++] = SessionCount;
			DataValues[DataValuesIndex++] = AuthComClientCount;
			DataValues[DataValuesIndex++] = m_UpdateTPS;
			DataValues[DataValuesIndex++] = PacketPoolSize;
			DataValues[DataValuesIndex++] = m_UpdateJobPool.GetCapacity();
			m_MonitorClient.SendPacket_UpdateData(DataValues, DataValuesIndex);

			//--------------------------------------------------------
			// 로그 출력
			//--------------------------------------------------------
			CONSOLE(eLOG_LEVEL_DEBUG, L"==============================================================\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"[NETWORK LIBRARY] : Worker:%d | Active:%d | Nagle:%d | ZeroCopy:%d\n",
				m_NumberOfIOCPWorkerThread, m_NumberOfIOCPActiveThread, m_bNagleFlag, m_bZeroCopyFlag
			);
			if (m_bRedisFlag)
			{
				CONSOLE(eLOG_LEVEL_DEBUG, L"[REDIS THREAD]    : Worker:%d | Active:%d\n", m_NumberOfRedisIOCPWorkerThread, m_NumberOfRedisIOCPActiveThread);
			}
			if (m_MonitorClient.m_bConnectedFlag)
			{
				CONSOLE(eLOG_LEVEL_DEBUG, L"[MONITER SERVER]  : Connected\n");
			}
			else
			{
				CONSOLE(eLOG_LEVEL_DEBUG, L"[MONITER SERVER]  : Reconecting...\n");
			}
			CONSOLE(eLOG_LEVEL_DEBUG, L"==============================================================\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Session                 : %d\n", SessionCount);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Index                   : %d\n", GetIndexesCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Accpet        TPS       : %d\n", m_AcceptTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Disconnect    TPS       : %d\n", m_DisconnectTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"RecvPacket    TPS       : %d (L:%d | S:%d | C:%d)\n", m_RecvPacketTPS, m_RecvLoginMessage, m_RecvMoveSectorMessage, m_RecvChatMessage);
			CONSOLE(eLOG_LEVEL_DEBUG, L"SendPacket    TPS       : %d (L:%d | S:%d | C:%d)\n", m_SendPacketTPS, m_SendLoginMessage, m_SendMoveSectorMessage, m_SendChatMessage);
			CONSOLE(eLOG_LEVEL_DEBUG, L"ChatSendCom / Player    : %d:%d (Failed:%d)\n", 
				m_SendPacketTPS - (m_SendLoginMessage + m_SendMoveSectorMessage), 
				m_SectorPlayerCount,
				m_SectorPlayerCount - (m_SendPacketTPS - (m_SendLoginMessage + m_SendMoveSectorMessage))
			);

			CONSOLE(eLOG_LEVEL_DEBUG, L"MClient Send  TPS       : %d\n", m_MonitorClient.m_SendPacketTPS);

			if (m_RecvChatMessage == 0)
			{
				m_RecvChatMessage = 1;
			}
			CONSOLE(eLOG_LEVEL_DEBUG, L"Recv To Send Ratio      : 1:%d\n", m_SendChatMessage /  m_RecvChatMessage);
			//CONSOLE(eLOG_LEVEL_DEBUG, L"Recv To Send Ratio    : 1:%d\n", m_SectorPlayerCount / m_RecvChatMessage);

			CONSOLE(eLOG_LEVEL_DEBUG, L"ExpSendBytes            : %I64u\n", m_ExpSendBytes);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Accpet     Total        : %I64u\n", m_AcceptTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Disconnect Total        : %I64u\n", m_DisconnectTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool   Size       : %d\n", PacketPoolSize);
			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool   Use        : %d\n", CPacket::GetPoolUseSize());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"ClientPool   Size       : %d\n", m_ClientPool.GetCapacity());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ClientPool   Use        : %d\n", m_ClientPool.GetUseCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ClientMap    Use        : %d\n", m_Clients.size());
			CONSOLE(eLOG_LEVEL_DEBUG, L"AuthClient   Count      : %d\n", AuthComClientCount);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"JobPool      Size       : %d\n", JobPoolSize);
			CONSOLE(eLOG_LEVEL_DEBUG, L"JobPool      Use        : %d\n", m_UpdateJobPool.GetUseCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");

			if (m_bRedisFlag)
			{
				if (m_AuthCompletedTotal == 0)
				{
					m_AuthCompletedTotal = 1;
				}

				//CONSOLE(eLOG_LEVEL_DEBUG, L"RedisJobPool     Size   : %d\n", m_RedisJobPool.GetCapacity());
				//CONSOLE(eLOG_LEVEL_DEBUG, L"RedisJobPool     Use    : %d\n", m_RedisJobPool.GetUseCount());
				CONSOLE(eLOG_LEVEL_DEBUG, L"RedisConnector   TPS    : %u\n", m_RedisConnectorTPS);
				CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");

				CONSOLE(eLOG_LEVEL_DEBUG, L"Auth             TPS    : %d\n", m_AuthTPS);
				CONSOLE(eLOG_LEVEL_DEBUG, L"Auth Wait        Count  : %d\n", m_AuthWaitCount);
				CONSOLE(eLOG_LEVEL_DEBUG, L"Auth Failed      Total  : %I64u\n", m_AuthFailedTotal);
				CONSOLE(eLOG_LEVEL_DEBUG, L"Auth Compeleted  Total  : %I64u\n", m_AuthCompletedTotal);
				CONSOLE(eLOG_LEVEL_DEBUG, L"Auth ElapsedTime Avr    : %.2fms\n", (float)m_AuthElapsedTime / (float)m_AuthCompletedTotal);
				CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			}

			CONSOLE(eLOG_LEVEL_DEBUG, L"UpdateJobQueue   Use    : %d\n", m_UpdateJobQueue.GetCapacity());
			CONSOLE(eLOG_LEVEL_DEBUG, L"Update           TPS    : %lld\n", m_UpdateTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");

			CONSOLE(eLOG_LEVEL_DEBUG, L"ProcessCommitMemory     : %I64u\n", PDH.GetProcessCommitMemory());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ProcessNonPagedMemory   : %I64u\n", PDH.GetProcessNonPagedMemory());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"CPU Usage [%.1f%% U:%.1f%% K:%.1f%%]\n", ProcessCpuUsage, CpuUsage.ProcessUser(), CpuUsage.ProcessKernel());

#ifdef __SAFE_MODE__
			unsigned __int64 overBytes = 1024 * 1024 * 1024 * 2;
			unsigned __int64 overBytes2 = 500;
			unsigned __int64 commitMemSize = PDH.GetProcessCommitMemory();
			unsigned __int64 nonPagedPoolSize = PDH.GetNonPagedMemory();
			if (commitMemSize > overBytes || commitMemSize < 0)
			{
				LOG(L"System", eLOG_LEVEL_SYSTEM, L"Lack of User Memory (%d bytes)\n", commitMemSize);
				Stop(true);
				//CPacket::OutputDebugData();
			}

			//if (nonPagedPoolSize < overBytes2)
			//{
			//	LOG(L"System", eLOG_LEVEL_SYSTEM, L"Lack of NonPaged Memory (%d bytes)\n", nonPagedPoolSize);
			//	CCrashDump::Crash();
			//}
#endif


			//-------------------------------------------------------
			// 라이브러리 측 모니터링 데이터 갱신
			//-------------------------------------------------------
			m_AcceptTPS = 0;
			m_DisconnectTPS = 0;
			m_RecvPacketTPS = 0;
			m_SendPacketTPS = 0;
			m_ExpSendBytes = 0;

			//-------------------------------------------------------
			// 모니터 클라이언트 데이터 갱신
			//-------------------------------------------------------
			m_MonitorClient.m_SendPacketTPS = 0;

			//-------------------------------------------------------
			// 컨텐츠 측 모니터링 데이터 갱신
			//-------------------------------------------------------
			m_UpdateTPS = 0;
			m_RecvLoginMessage = 0;
			m_RecvMoveSectorMessage = 0;
			m_RecvChatMessage = 0;
			m_SendLoginMessage = 0;
			m_SendMoveSectorMessage = 0;
			m_SendChatMessage = 0;
			m_SectorPlayerCount = 0;
			m_AuthTPS = 0;
			m_RedisConnectorTPS = 0;

			Sleep(999);
		}

		return 0;
	}


	//////////////////////////////////////////////////////////////////////////////////
	// HeartBeat Thread
	// 
	//////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall ChatServer::HeartBeatThread(void* lpParam)
	{
		return ((ChatServer*)lpParam)->HeartBeatThread_Procedure();
	}
	int ChatServer::HeartBeatThread_Procedure(void)
	{
		while (m_bLoop)
		{
			if (WAIT_TIMEOUT != WaitForSingleObject(m_hHeartBeatEvent, INFINITE))
			{
				break;
			}

			st_UPDATE_JOB* pNewJob = m_UpdateJobPool.Alloc();
			pNewJob->Type = st_UPDATE_JOB::en_JOB_HEARTBEAT;

			m_UpdateJobQueue.Enqueue(pNewJob);
			SetEvent(m_hUpdateEvent);
		}

		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Redis Thread
	// 
	//////////////////////////////////////////////////////////////////////////////////
	//unsigned int __stdcall ChatServer::RedisThread(void* lpParam)
	//{
	//	return ((ChatServer*)lpParam)->RedisThread_Procedure();
	//}

	//int ChatServer::RedisThread_Procedure(void)
	//{
	//	DWORD			StartTime;
	//	DWORD			ElapsedTime;

	//	DWORD			dwTransferred;
	//	OVERLAPPED*		pOverlapped;
	//	st_REDIS_JOB*	pRedisJob;
	//	
	//	SESSION_ID		SessionID;
	//	//INT64			AccountNo;
	//	char			SessionKey[en_SESSION_KEY_MAX];

	//	while (m_bLoop)
	//	{
	//		dwTransferred	= 0;
	//		pRedisJob		= nullptr;
	//		pOverlapped		= nullptr;

	//		GetQueuedCompletionStatus(m_hRedisIOCP, &dwTransferred, (PULONG_PTR)&pRedisJob, &pOverlapped, INFINITE);

	//		//--------------------------------------------------------
	//		// [예외] Therad 종료
	//		//--------------------------------------------------------
	//		if (dwTransferred == 0 && pRedisJob == nullptr && pOverlapped == nullptr)
	//		{
	//			PostQueuedCompletionStatus(m_hRedisIOCP, 0, (ULONG_PTR)nullptr, nullptr);
	//			return 0;
	//		}

	//		InterlockedIncrement(&m_AuthWaitCount);

	//		StartTime = timeGetTime();

	//		//--------------------------------------------------------
	//		// UpdateThread에게 회신해줄 패킷 생성
	//		//--------------------------------------------------------
	//		st_UPDATE_JOB* pJob = m_UpdateJobPool.Alloc();
	//		pJob->SessionID = pRedisJob->SessionID;

	//		//-----------------------------------------------
	//		// [Redis 검색]
	//		//-----------------------------------------------
	//		_i64toa_s(pRedisJob->AccountNo, Redis_Key, 64, 10);

	//		m_pRedisConnector->Get(Redis_Key, &Redis_Reply);
	//		InterlockedIncrement(&m_RedisConnectorTPS);

	//		Redis_Value = Redis_Reply._Get_value().as_string();

	//		int iResult = memcmp(Redis_Value.c_str(), pRedisJob->SessionKey, en_SESSION_KEY_MAX);

	//		//-----------------------------------------------
	//		// [예외] SessionKey가 다르다.
	//		//-----------------------------------------------
	//		if (0 != iResult)
	//		{
	//			InterlockedIncrement64(&m_AuthFailedTotal);
	//			CCrashDump::Crash();
	//		}
	//		//-----------------------------------------------
	//		// [통과] SessionKey가 같다.
	//		//-----------------------------------------------
	//		else if (0 == iResult)
	//		{
	//			pJob->Type = st_UPDATE_JOB::en_JOB_REDIS_COMPELETED;
	//		}

	//		InterlockedDecrement(&m_AuthWaitCount);

	//		ElapsedTime = timeGetTime() - StartTime;
	//		InterlockedAdd64(&m_AuthElapsedTime, ElapsedTime);
	//		InterlockedIncrement64(&m_AuthCompletedTotal);
	//		InterlockedIncrement(&m_AuthTPS);

	//		//-----------------------------------------------
	//		// UpdateThread에게 Redis 인증 완료 처리 Job을 넘겨준다.
	//		//-----------------------------------------------
	//		m_UpdateJobQueue.Enqueue(pJob);
	//		SetEvent(m_hUpdateEvent);

	//		//-----------------------------------------------
	//		// 처리 완료한 Redis Job 반환
	//		//-----------------------------------------------
	//		m_RedisJobPool.Free(pRedisJob);
	//	}

	//	return 0;
	//}

	unsigned int __stdcall ChatServer::MonitorServerConnectThread(void* lpParam)
	{
		return ((ChatServer*)lpParam)->MonitorServerConnectThread_Procedure();
	}

	int ChatServer::MonitorServerConnectThread_Procedure(void)
	{
		WCHAR	BIND_IP[16];
		WCHAR	SERVER_IP[16];
		WORD	SERVER_PORT;
		DWORD	IOCP_WORKER_THREAD;
		bool	NAGLE_FLAG;
		bool	ZERO_COPY_FLAG;

		//==========================================================
		// [MONITER CLIENT] 모니터링 서버 연결
		//==========================================================
		m_Parser.GetValue(L"MONITOR_NO", &m_MonitorClient.m_MonitorNo);

		//------------------------------------------------------
		// Bind IP / PORT
		//------------------------------------------------------
		m_Parser.GetValue(L"BIND_IP", BIND_IP, 16);

		//------------------------------------------------------
		// Moniter Server IP / PORT
		//------------------------------------------------------
		m_Parser.GetValue(L"SERVER_IP", SERVER_IP, 16);
		m_Parser.GetValue(L"SERVER_PORT", &SERVER_PORT);

		//------------------------------------------------------
		// Library Setting
		//------------------------------------------------------
		m_Parser.GetValue(L"IOCP_WORKER_THREAD", &IOCP_WORKER_THREAD);

		//------------------------------------------------------
		// Network Setting
		//------------------------------------------------------
		m_Parser.GetValue(L"NAGLE_FLAG", &NAGLE_FLAG);
		m_Parser.GetValue(L"ZERO_COPY_FLAG", &ZERO_COPY_FLAG);

		while (true)
		{
			if (WAIT_TIMEOUT != WaitForSingleObject(m_hMonitorServerConnectThreadEvent, 9999))
			{
				return 0;
			}

			if (m_MonitorClient.m_bConnectedFlag == false)
			{
				if (!m_MonitorClient.Connect(BIND_IP, SERVER_IP, SERVER_PORT, IOCP_WORKER_THREAD, NAGLE_FLAG, ZERO_COPY_FLAG))
				{
					LOG(L"System", en_LOG_LEVEL::eLOG_LEVEL_SYSTEM, L"Monitor Server Connect Failed!\n");
				}
			}
		}

		return 0;
	}

	//=================================================================================
	// Monitor Server Client 관련
	//=================================================================================
	ChatServer::CMonitorClient::CMonitorClient()
	{
	}

	ChatServer::CMonitorClient::~CMonitorClient()
	{
	}

	void ChatServer::CMonitorClient::SendPacket_UpdateData(const int DataValues[], const int DataCount)
	{
		if (!m_bConnectedFlag)
		{
			return;
		}

		CPacket*	pPacket;
		time_t		Time;

		_time64(&Time);

		for (int i = 0; i < DataCount; i++)
		{
			pPacket = CPacket::Alloc();
			MakePacket_DataUpdate(pPacket, dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN + i, DataValues[i], (int)Time);
			SendPacket(pPacket);
			pPacket->SubRef();
		}
	}

	void ChatServer::CMonitorClient::OnEnterJoinServer(void)
	{
		//-------------------------------------------------------
		// 로그인 요청 패킷 보내기
		//-------------------------------------------------------
		CPacket* pSendPacket = CPacket::Alloc();
		MakePacket_Login(pSendPacket, m_MonitorNo);
		SendPacket(pSendPacket);
		pSendPacket->SubRef();

		m_bConnectedFlag = true;
	}

	void ChatServer::CMonitorClient::OnLeaveServer(void)
	{
		m_bConnectedFlag = false;
	}

	void ChatServer::CMonitorClient::OnRecv(CPacket* pRecvPacket)
	{
	}

	void ChatServer::CMonitorClient::OnSend(int sendsize)
	{
	}

	void ChatServer::CMonitorClient::OnWorkerThreadBegin(void)
	{
	}

	void ChatServer::CMonitorClient::OnWorkerThreadEnd(void)
	{
	}

	void ChatServer::CMonitorClient::OnError(int errorcode, wchar_t* szMsg)
	{
	}
	void ChatServer::CMonitorClient::MakePacket_Login(CPacket* pSendPacket, const int ServerNo)
	{
		*pSendPacket << (WORD)en_PACKET_SS_MONITOR_LOGIN;
		*pSendPacket << ServerNo;
	}

	void ChatServer::CMonitorClient::MakePacket_DataUpdate(CPacket* pSendPacket, const BYTE DataType, const int DataValue, int TimeStamp)
	{
		*pSendPacket << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
		*pSendPacket << DataType;
		*pSendPacket << DataValue;
		*pSendPacket << TimeStamp;
	}
}