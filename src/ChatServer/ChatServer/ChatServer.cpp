
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
		// SERVICE : ������ ���� Ÿ�Ӿƿ� ó��
		//------------------------------------------------------
		m_Parser.GetValue(L"TIMEOUT_DISCONNECT", &m_TimeOutMax);

		//------------------------------------------------------
		// DB ����
		//------------------------------------------------------
		//m_pDBConnector = new CDBConnector_TLS(L"127.0.0.1", L"root", L"0000", L"accountdb", 3306);
		//m_pRedisConnector = new CRedis_TLS();

		//------------------------------------------------------
		// ��Ʈ��ũ ���̺귯�� ����
		//------------------------------------------------------
		if (!Start(BIND_IP, BIND_PORT, IOCP_WORKER_THREAD, IOCP_ACTIVE_THREAD, NAGLE_FLAG, ZERO_COPY_FLAG, CLIENT_MAX, PACKET_SIZE_MAX, SELF_SEND_FLAG, KEEP_ALIVE_FLAG))
		{
			wprintf(L"Network Library Start Failed\n");
			m_bRunFlag = false;
			return;
		}

		//----------------------------------------------------------
		// �ܼ�â �ʱ�ȭ �� ��Ʈ�ѷ� ����
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
		// Monitor Thread ����
		//----------------------------------------------------------
		m_hMonitorThread = (HANDLE)_beginthreadex(nullptr, 0, MonitorThread, this, false, nullptr);

		//----------------------------------------------------------
		// Update Thread ����
		//----------------------------------------------------------
		m_hUpdateEvent = (HANDLE)CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hUpdateThread = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, this, false, nullptr);

		//----------------------------------------------------------
		// HeartBeat Thread ����
		//----------------------------------------------------------
		m_hHeartBeatThread = (HANDLE)_beginthreadex(nullptr, 0, HeartBeatThread, this, false, nullptr);
		m_hHeartBeatEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		//----------------------------------------------------------
		// Redis Thread ����
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
		// MonitorServer Connect Thread ����
		//----------------------------------------------------------
		m_hMonitorServerConnectThread = (HANDLE)_beginthreadex(nullptr, 0, this->MonitorServerConnectThread, this, false, nullptr);
		m_hMonitorServerConnectThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		
		//----------------------------------------------------------
		// ��Ʈ�ѷ� ����
		//----------------------------------------------------------
		while (m_bLoop)
		{
			Controler();
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// ���� ����
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::Quit(void)
	{
		//----------------------------------------------------------------------
		// ������ �������̶�� �Ϻ��� ����
		//----------------------------------------------------------------------
		if (m_bRunFlag == true)
		{
			Stop(en_STOP_TYPE::QUIT);
		}
		//----------------------------------------------------------------------
		// ������ �������̶�� ������ ���ҽ��� ����
		//----------------------------------------------------------------------
		else
		{
			Stop(en_STOP_TYPE::RESOURCE_RELEASE);
		}

		//----------------------------------------------------------------------
		// ������ ��Ʈ�� ��� Ŭ���̾�Ʈ�� ������ �� ���� ���
		//----------------------------------------------------------------------
		while (m_Clients.size() != 0) {};

		//----------------------------------------------------------------------
		// ������ ���� �� ���ҽ� ����
		//----------------------------------------------------------------------
		m_bLoop = false;

		//----------------------------------------------------------------------
		// Monitor Server Connect Thread ����
		//----------------------------------------------------------------------
		SetEvent(m_hMonitorServerConnectThreadEvent);
		WaitForSingleObject(m_hMonitorServerConnectThread, INFINITE);
		CloseHandle(m_hMonitorServerConnectThread);
		CloseHandle(m_hMonitorServerConnectThreadEvent);

		//----------------------------------------------------------------------
		// HeartBeat Thread ����
		//----------------------------------------------------------------------
		SetEvent(m_hHeartBeatEvent);
		WaitForSingleObject(m_hHeartBeatThread, INFINITE);
		CloseHandle(m_hHeartBeatThread);
		CloseHandle(m_hHeartBeatEvent);

		//----------------------------------------------------------------------
		// Monitor Thread ����
		//----------------------------------------------------------------------
		WaitForSingleObject(m_hMonitorThread, INFINITE);
		CloseHandle(m_hMonitorThread);

		//----------------------------------------------------------------------
		// Update Thread ����
		//----------------------------------------------------------------------
		st_UPDATE_JOB* pNewJob = m_UpdateJobPool.Alloc();
		pNewJob->Type = st_UPDATE_JOB::en_JOB_EXIT;
		m_UpdateJobQueue.Enqueue(pNewJob);
		SetEvent(m_hUpdateEvent);
		WaitForSingleObject(m_hUpdateThread, INFINITE);
		CloseHandle(m_hUpdateEvent);
		CloseHandle(m_hUpdateThread);

		//----------------------------------------------------------------------
		// Redis Thread ����
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
		// �ڷ� ���� �����̳� ����
		//----------------------------------------------------------------------
		ClearSector();
		ClearClients();

		//----------------------------------------------------------
		// ����͸� Ŭ���̾�Ʈ ���� ����
		//----------------------------------------------------------
		m_MonitorClient.Stop();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// ���� ��Ʈ�ѷ�
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::Controler(void)
	{
		WCHAR ControlKey = _getwch();

		//----------------------------------------------------------------------
		// ���� ����
		//----------------------------------------------------------------------
		if ('q' == ControlKey || 'Q' == ControlKey)
		{
			Quit();
			return;
		}

		//----------------------------------------------------------------------
		// ���� ���� �� �����
		//----------------------------------------------------------------------
		if ('s' == ControlKey || 'S' == ControlKey)
		{
			if (m_bRunFlag)
			{
				//----------------------------------------------------------
				// ���̺귯�� STOP
				//----------------------------------------------------------
				Stop(en_STOP_TYPE::STOP);

				//----------------------------------------------------------
				// ������ ��Ʈ�� ��� Ŭ���̾�Ʈ�� ������ �� ���� ���
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
		// �������ϸ� ������ ���
		//----------------------------------------------------------------------
		if ('p' == ControlKey || 'P' == ControlKey)
		{
			OutputProfilingData();
			return;
		}

		//----------------------------------------------------------------------
		// �α� ���� ����
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
		// Ŭ���̾�Ʈ ���� �����̳ʸ� ���鼭 ��� ��ȯ
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
	// ���ο� Ŭ���̾�Ʈ�� �����ߴ�.
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
	// ���� Ŭ���̾�Ʈ�� ���� ����ƴ�.
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
	// ��Ŷ�� ���� �޾Ҵ�.
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::OnRecv(const SESSION_ID SessionID, CPacket* pRecvPacket)
	{
		// ��Ŷ�� ��ȯ���� �ʰ� ���۷��� ī��Ʈ ����
		// �ش� ��Ŷ�� UpdateThread ó�� �Ϸ� �� ��ȯ�ȴ�.

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
	// ��Ŷ�� �۽��ߴ�.
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
	// ���̺귯������ ������ �߻��ߴ�.
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
	// Update Thread Jod ó�� : �ű� ����
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
	// Update Thread Jod ó�� : ���� ����
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::JobProcedure_Disconnect(const SESSION_ID SessionID)
	{
		return DeleteClient(SessionID);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Update Thread Jod ó�� : ��û ��Ŷ
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
	// Update Thread Jod ó�� : ��Ʈ��Ʈ
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
	// Update Thread Jod ó�� : Redis Ȯ�� �Ϸ� -> �α��� �Ϸ� ó��
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::JobProcedure_RedisCompeleted(const SESSION_ID SessionID)
	{
		//------------------------------------------------------------------
		// [����] �α��� ��û ��Ŷ�� �����ΰ� Redis Ȯ���ϴ� ���� �����ɼ��� ����.
		// �ݴ��, �����ϴ� �����̶�� �Ǵܵ� ���, Delete �۾��� �ش� �۾����� �ռ� �� �����Ƿ� 
		// ������ �����ϴ� ���, ���� ���̺귯������ �̹� �������ٸ� SendPacket�� ������ ����.
		//------------------------------------------------------------------
		st_CLIENT* pClient = FindClient(SessionID);
		if (pClient == nullptr)
		{
			return false;
		}

		//------------------------------------------------------------------
		// �α��� ���·� ����
		//------------------------------------------------------------------
		pClient->bLogin = true;

		//------------------------------------------------------------------
		// �α��� ���� �޼��� ���� �� �۽�
		//------------------------------------------------------------------
		CPacket* pSendMSG = CPacket::Alloc();
		MakePacket_Login(pSendMSG, 1, pClient->AccountNo);
		SendPacket(SessionID, pSendMSG);
		m_SendLoginMessage += 1;
		pSendMSG->SubRef();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// ��Ŷ ��û ó�� : �α��� ó��
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::PacketProcedure_Login(const SESSION_ID SessionID, CPacket* pMessage)
	{
		m_RecvLoginMessage += 1;

		//---------------------------------------------------------------
		// [����] �����ϴ� Ŭ���̾�Ʈ���� Ȯ��
		//---------------------------------------------------------------
		st_CLIENT* pClient = FindClient(SessionID);
		if (nullptr == pClient)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Miss Match SessionID - (Req:%I64u)\n", SessionID);
			Disconnect(SessionID);

			return false;
		}

		//---------------------------------------------------------------
		// ���� �ð� ����
		//---------------------------------------------------------------
		pClient->LastRecvTime = timeGetTime();

		//---------------------------------------------------------------
		// [����] �̹� �α��εǾ��ִ� Ŭ���̾�Ʈ��� ���� �޼��� �۽� �� ����
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
		// Ŭ���̾�Ʈ�� �� ������ Ŭ���̾�Ʈ ����
		//---------------------------------------------------------------
		pMessage->Get_UNI16(pClient->ID, en_ID_MAX);
		pMessage->Get_UNI16(pClient->NickName, en_NICK_NAME_MAX);

		//---------------------------------------------------------------
		// �α��� ó���� RedisThread�� �ѱ��.
		//---------------------------------------------------------------
		if (m_bRedisFlag)
		{
			//---------------------------------------------------------------
			// �α��� ��û ������ RedisJob ����ü�� �����ؼ� Enqeueue
			//---------------------------------------------------------------
			/*st_REDIS_JOB* pRedisJob = m_RedisJobPool.Alloc();
			pRedisJob->AccountNo = pClient->AccountNo;
			pRedisJob->SessionID = SessionID;
			pMessage->Get(pRedisJob->SessionKey, en_SESSION_KEY_MAX);

			PostQueuedCompletionStatus(m_hRedisIOCP, 0xFFFFFFFF, (ULONG_PTR)pRedisJob, nullptr);*/
		}
		//---------------------------------------------------------------
		// �α��� ó���� UpdateThread���� �Ϸ��Ѵ�.
		//---------------------------------------------------------------
		else
		{
			//---------------------------------------------------------------
			// �α��� ���·� ����
			//---------------------------------------------------------------
			pClient->bLogin = true;

			//---------------------------------------------------------------
			// �α��� ���� �޼��� ���� �� �۽�
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
	// ��Ŷ ��û ó�� : ���� �̵�
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::PacketProcedure_SectorMove(const SESSION_ID SessionID, CPacket* pMessage)
	{
		m_RecvMoveSectorMessage += 1;

		//-----------------------------------------------
		// [����] �����ϴ� Ŭ���̾�Ʈ���� Ȯ��
		//-----------------------------------------------
		st_CLIENT* pClient = FindClient(SessionID);
		if (pClient == nullptr)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Miss Match SessionID - (Req:%I64u)\n", SessionID);
			Disconnect(SessionID);

			return false;
		}

		//-----------------------------------------------
		// [����] �α��� �������� Ȯ��
		//-----------------------------------------------
		if (!pClient->bLogin)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Not Login Client - (Req:%I64u)\n", SessionID);
			Disconnect(pClient->SessionID);

			return false;
		}

		//-----------------------------------------------
		// [����] AccountNo�� ������ Ȯ��
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
		// 4. ���̷ε� ���
		//-----------------------------------------------
		WORD wSectorX;
		WORD wSectorY;
		*pMessage >> wSectorX;
		*pMessage >> wSectorY;

		//-----------------------------------------------
		// [����] ���� �������� ��� ��ġ�� ��û�ߴ��� Ȯ��
		//-----------------------------------------------
		if (wSectorX >= en_SECTOR_MAX_X || wSectorY >= en_SECTOR_MAX_Y)
		{
			Disconnect(pClient->SessionID);
			return false;
		}

		//-----------------------------------------------
		// 5. �Ҽӵ� ���Ϳ��� �ش� Ŭ���̾�Ʈ�� �����Ѵ�.
		//-----------------------------------------------
		if (pClient->Sector.wX != (WORD)-1 && pClient->Sector.wY != (WORD)-1)
		{
			m_Sectors[pClient->Sector.wY][pClient->Sector.wX].remove(pClient->SessionID);
		}

		//-----------------------------------------------
		// 6. ���ο� ���Ϳ� �ش� Ŭ���̾�Ʈ�� �߰��Ѵ�.
		//-----------------------------------------------
		pClient->Sector.wX = wSectorX;
		pClient->Sector.wY = wSectorY;
		m_Sectors[pClient->Sector.wY][pClient->Sector.wX].push_back(pClient->SessionID);

		//-----------------------------------------------
		// 7. ���� �޼��� ���� �� �۽�
		//-----------------------------------------------
		CPacket* pSendMSG = CPacket::Alloc();
		MakePacket_SectorMove(pSendMSG, pClient->AccountNo, pClient->Sector.wX, pClient->Sector.wY);
		SendPacket(SessionID, pSendMSG);
		m_SendMoveSectorMessage += 1;
		pSendMSG->SubRef();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// ��Ŷ ��û ó�� : ä�� �۽�
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::PacketProcedure_ChatMessage(const SESSION_ID SessionID, CPacket* pMessage)
	{
		m_RecvChatMessage += 1; 

		//-----------------------------------------------
		// [����] �����ϴ� Ŭ���̾�Ʈ�ΰ�?
		//-----------------------------------------------
		st_CLIENT* pClient = FindClient(SessionID);
		if (nullptr == pClient)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Miss Match SessionID - (Req:%I64u)\n", SessionID);
			Disconnect(SessionID);
			return false;
		}

		//-----------------------------------------------
		// [����] �α��εǾ��ִ� Ŭ���̾�Ʈ�ΰ�?
		//-----------------------------------------------
		if (!pClient->bLogin)
		{
			//LOG(L"Debug", eLOG_LEVEL_DEBUG, L"### Not Login Client - (Req:%I64u)\n", SessionID);
			Disconnect(pClient->SessionID);
			return false;
		}

		//-----------------------------------------------
		// [����] AccountNo�� ������?
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
		// ���̷ε� ���
		//-----------------------------------------------
		WORD wMessageLen;
		*pMessage >> wMessageLen;

		//-----------------------------------------------
		// �޼��� ���� �� �۽�
		//-----------------------------------------------
		CPacket* pSendMSG = CPacket::Alloc();
		MakePacket_ChatMessage(pSendMSG, pClient->AccountNo, pClient->ID, pClient->NickName, wMessageLen, (WCHAR*)pMessage->GetReadPos());
		SendPacket_Around(pClient, pSendMSG, true);
		pSendMSG->SubRef();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// ��Ŷ ��û ó�� : ��Ʈ��Ʈ ����
	//////////////////////////////////////////////////////////////////////////////////
	bool ChatServer::PacketProcedure_Heartbeat(const SESSION_ID SessionID)
	{
		st_CLIENT* pClient = FindClient(SessionID);
		pClient->LastRecvTime = timeGetTime();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// ���� ��Ŷ ����� : �α��� ����
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::MakePacket_Login(CPacket* pSendMessage, const BYTE byStatus, const INT64 AccountNo)
	{
		*pSendMessage << (WORD)en_PACKET_CS_CHAT_RES_LOGIN;
		*pSendMessage << byStatus;
		*pSendMessage << AccountNo;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// ���� ��Ŷ ����� : ���� �̵� ����
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::MakePacket_SectorMove(CPacket* pSendMessage, const INT64 AccountNo, const WORD shSectorX, const WORD shSectorY)
	{
		*pSendMessage << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
		*pSendMessage << AccountNo;
		*pSendMessage << shSectorX;
		*pSendMessage << shSectorY;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// ���� ��Ŷ ����� : ä�� �۽� ����
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
	// ���ο� Ŭ���̾�Ʈ ����
	//////////////////////////////////////////////////////////////////////////////////
	ChatServer::st_CLIENT* ChatServer::CreateClient(const SESSION_ID SessionID)
	{
		//-----------------------------------------------
		// 1. �޸�Ǯ���� �޸� �Ҵ� �� ����
		//-----------------------------------------------
		st_CLIENT* pNewClient = m_ClientPool.Alloc();
		pNewClient->bLogin = false;
		pNewClient->Sector.wX = INVALID_SECTOR_POS;
		pNewClient->Sector.wY = INVALID_SECTOR_POS;
		pNewClient->AccountNo = INVALID_ACCOUNT_NO;
		pNewClient->SessionID = SessionID;
		pNewClient->LastRecvTime = timeGetTime();

		//-----------------------------------------------
		// 2. Ŭ���̾�Ʈ ���� �����̳ʿ� �߰�
		//-----------------------------------------------
		m_Clients.insert(pair<SESSION_ID, st_CLIENT*>(SessionID, pNewClient));

		return pNewClient;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// ���� Ŭ���̾�Ʈ ã��
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
	// ���� Ŭ���̾�Ʈ ����
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
	// Ŭ���̾�Ʈ ���� �����̳� ����
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::ClearClients(void)
	{
		unordered_map<SESSION_ID, st_CLIENT*>::iterator iter = m_Clients.begin();

		//-----------------------------------------------
		// Ŭ���̾�Ʈ ���� �����̳ʸ� ���鼭 ��� ��ȯ
		//-----------------------------------------------
		for (iter; iter != m_Clients.end();)
		{
			iter = m_Clients.erase(iter);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// �ֺ� ���� ���� ���
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND* pDestiation)
	{
		pDestiation->iCount = 0;

		// ���� ��ǥ�� ���� Entry�� �̵�
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
				// ���Ϳ� �����ϴ� Ŭ���̾�Ʈ ��� ���� (Ŭ���̾�Ʈ�� �����ϴ� �� �ƴϴ�)
				//-----------------------------------------------
				for (iter; iter != end;)
				{
					iter = Sector.erase(iter);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// �ֺ� ���� ��ü �������� ��Ŷ �۽�
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::SendPacket_Around(st_CLIENT* pClient, CPacket* pSendMessage, const bool bSendMe)
	{
		st_SECTOR_AROUND SectorAround;

		//-----------------------------------------------
		// 1. �ش� ĳ���Ͱ� ���� ���� ������� ��´�.
		//-----------------------------------------------
		GetSectorAround(pClient->Sector.wX, pClient->Sector.wY, &SectorAround);

		//-----------------------------------------------
		// 2. ���� �� �� �� �� ���� �޼����� �Ѹ���.
		//-----------------------------------------------
		for (int i = 0; i < SectorAround.iCount; i++)
		{
			SendPacket_SectorOne(SectorAround.Around[i].wX, SectorAround.Around[i].wY, pSendMessage);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// ���� ���� �ϳ��� ���� ��� �������� ��Ŷ �۽�
	//////////////////////////////////////////////////////////////////////////////////
	void ChatServer::SendPacket_SectorOne(const int iX, const int iY, CPacket* pSendMessage, const SESSION_ID ExceptSessionID)
	{
		//-----------------------------------------------
		// 1. ���͸� ã�´�
		//-----------------------------------------------
		list<SESSION_ID> Sector = m_Sectors[iY][iX];
		if (Sector.size() == 0)
		{
			return;
		}

		m_SectorPlayerCount += Sector.size();

		//-----------------------------------------------
		// 2. ���Ϳ� �����ϴ� ��� ���ǿ��� ��Ŷ�� ������.
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
			// ����͸� ������ ����
			//--------------------------------------------------------
			CpuUsage.UpdateCpuTime();
			PDH.Collect();

			//--------------------------------------------------------
			// ���� ���� �ð� ���
			//--------------------------------------------------------
			wprintf(L"[CHAT SERVER] StartTime : %s\n\n", szFileName);

			//--------------------------------------------------------
			// �α� ���� ���� ��� ���
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
			// ����͸� ������ ������ ������ ���
			//--------------------------------------------------------
			int PacketPoolSize = CPacket::GetPoolCapacity();
			int JobPoolSize = m_UpdateJobPool.GetCapacity();
			int SessionCount = GetSessionCount();
			int AuthComClientCount = GetAuthCompletedPlayerCount();
			float ProcessCpuUsage = CpuUsage.ProcessTotal();

			//-------------------------------------------------------
			// ����͸� ������ ������ ����
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
			// �α� ���
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
			// ���̺귯�� �� ����͸� ������ ����
			//-------------------------------------------------------
			m_AcceptTPS = 0;
			m_DisconnectTPS = 0;
			m_RecvPacketTPS = 0;
			m_SendPacketTPS = 0;
			m_ExpSendBytes = 0;

			//-------------------------------------------------------
			// ����� Ŭ���̾�Ʈ ������ ����
			//-------------------------------------------------------
			m_MonitorClient.m_SendPacketTPS = 0;

			//-------------------------------------------------------
			// ������ �� ����͸� ������ ����
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
	//		// [����] Therad ����
	//		//--------------------------------------------------------
	//		if (dwTransferred == 0 && pRedisJob == nullptr && pOverlapped == nullptr)
	//		{
	//			PostQueuedCompletionStatus(m_hRedisIOCP, 0, (ULONG_PTR)nullptr, nullptr);
	//			return 0;
	//		}

	//		InterlockedIncrement(&m_AuthWaitCount);

	//		StartTime = timeGetTime();

	//		//--------------------------------------------------------
	//		// UpdateThread���� ȸ������ ��Ŷ ����
	//		//--------------------------------------------------------
	//		st_UPDATE_JOB* pJob = m_UpdateJobPool.Alloc();
	//		pJob->SessionID = pRedisJob->SessionID;

	//		//-----------------------------------------------
	//		// [Redis �˻�]
	//		//-----------------------------------------------
	//		_i64toa_s(pRedisJob->AccountNo, Redis_Key, 64, 10);

	//		m_pRedisConnector->Get(Redis_Key, &Redis_Reply);
	//		InterlockedIncrement(&m_RedisConnectorTPS);

	//		Redis_Value = Redis_Reply._Get_value().as_string();

	//		int iResult = memcmp(Redis_Value.c_str(), pRedisJob->SessionKey, en_SESSION_KEY_MAX);

	//		//-----------------------------------------------
	//		// [����] SessionKey�� �ٸ���.
	//		//-----------------------------------------------
	//		if (0 != iResult)
	//		{
	//			InterlockedIncrement64(&m_AuthFailedTotal);
	//			CCrashDump::Crash();
	//		}
	//		//-----------------------------------------------
	//		// [���] SessionKey�� ����.
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
	//		// UpdateThread���� Redis ���� �Ϸ� ó�� Job�� �Ѱ��ش�.
	//		//-----------------------------------------------
	//		m_UpdateJobQueue.Enqueue(pJob);
	//		SetEvent(m_hUpdateEvent);

	//		//-----------------------------------------------
	//		// ó�� �Ϸ��� Redis Job ��ȯ
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
		// [MONITER CLIENT] ����͸� ���� ����
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
	// Monitor Server Client ����
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
		// �α��� ��û ��Ŷ ������
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