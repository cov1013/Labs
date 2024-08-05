#include "CMonitorServer_Net.h"

namespace cov1013
{
	//////////////////////////////////////////////////////////////////////////////////
	// 생성자
	//////////////////////////////////////////////////////////////////////////////////
	CMonitorServer_Net::CMonitorServer_Net()
	{
		m_bLoop = false;
		m_bRunFlag = false;
		m_TimeOutMax = INT_MAX;
		memcpy_s(m_LoginKey, 32, "ajfw@!cv980dSZ[fje#@fdj123948djf", 32);

		InitializeCriticalSection(&m_Clients_cs);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 소멸자
	//////////////////////////////////////////////////////////////////////////////////
	CMonitorServer_Net::~CMonitorServer_Net()
	{
		DeleteCriticalSection(&m_Clients_cs);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 서버 실행
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::Run(void)
	{
		//----------------------------------------------------------------------
		// LanServer 실행
		//----------------------------------------------------------------------
		if (!m_MonitorServer_Lan.Run())
		{
			wprintf(L"LanServer Run Failed\n");
			return;
		}

		//----------------------------------------------------------------------
		// 중복 실행 방지
		//----------------------------------------------------------------------
		if (true == m_bRunFlag)
		{
			wprintf(L"Overlapped Running\n");
			return;
		}
		m_bRunFlag = true;

		//----------------------------------------------------------------------
		// 서버 설정 정보 불러오기 & 세팅
		//----------------------------------------------------------------------
		CParser Parser;
		if (!Parser.LoadFile(L"../Config.txt"))
		{
			wprintf(L"Server Config Open Failed\n");
			m_bRunFlag = false;
			return;
		}

		//------------------------------------------------------
		// Listen IP / PORT
		//------------------------------------------------------
		WCHAR	BIND_IP[16];
		WORD	BIND_PORT;
		Parser.GetValue(L"BIND_IP", BIND_IP, 16);
		Parser.GetValue(L"BIND_PORT", &BIND_PORT);

		//------------------------------------------------------
		// Library Setting
		//------------------------------------------------------
		DWORD	IOCP_WORKER_THREAD;
		DWORD	IOCP_ACTIVE_THREAD;
		DWORD	CLIENT_MAX;
		Parser.GetValue(L"IOCP_WORKER_THREAD", &IOCP_WORKER_THREAD);
		Parser.GetValue(L"IOCP_ACTIVE_THREAD", &IOCP_ACTIVE_THREAD);
		Parser.GetValue(L"CLIENT_MAX", &CLIENT_MAX);

		//------------------------------------------------------
		// Packet Encode / Decode Key
		//------------------------------------------------------
		BYTE	PACKET_CODE;
		BYTE	PACKET_KEY;
		int		PACKET_SIZE_MAX;
		Parser.GetValue(L"PACKET_CODE", &PACKET_CODE);
		Parser.GetValue(L"PACKET_KEY", &PACKET_KEY);
		Parser.GetValue(L"PACKET_SIZE_MAX", &PACKET_SIZE_MAX);
		CPacket::SetPacketCode(PACKET_CODE);
		CPacket::SetPacketKey(PACKET_KEY);

		//------------------------------------------------------
		// Network Setting
		//------------------------------------------------------
		bool	NAGLE_FLAG;
		bool	ZERO_COPY_FLAG;
		bool	KEEP_ALIVE_FLAG;
		Parser.GetValue(L"NAGLE_FLAG", &NAGLE_FLAG);
		Parser.GetValue(L"ZERO_COPY_FLAG", &ZERO_COPY_FLAG);
		Parser.GetValue(L"KEEP_ALIVE_FLAG", &KEEP_ALIVE_FLAG);

		//------------------------------------------------------
		// SERVICE : 미응답 유저 타임아웃 처리
		//------------------------------------------------------
		Parser.GetValue(L"TIMEOUT_DISCONNECT", &m_TimeOutMax);

		//----------------------------------------------------------------------
		// 네트워크 라이브러리 실행
		//----------------------------------------------------------------------
		if (!Start(BIND_IP, BIND_PORT, IOCP_WORKER_THREAD, IOCP_ACTIVE_THREAD, NAGLE_FLAG, ZERO_COPY_FLAG, CLIENT_MAX, PACKET_SIZE_MAX, KEEP_ALIVE_FLAG))
		{
			wprintf(L"Network Library Start Failed\n");
			m_bRunFlag = false;
			return;
		}

		//----------------------------------------------------------------------
		// 콘솔창 초기화
		//----------------------------------------------------------------------
		EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_GRAYED);
		DrawMenuBar(GetConsoleWindow());
		system("mode con cols=70 lines=46");
		wprintf(L"Server Open...\n");

		//----------------------------------------------------------------------
		// 스레드 생성
		//----------------------------------------------------------------------
		m_bLoop = true;
		m_hSendThread = (HANDLE)_beginthreadex(nullptr, 0, SendThread, this, 0, nullptr);
		m_hMonitorThread = (HANDLE)_beginthreadex(nullptr, 0, MonitorThread, this, 0, nullptr);
		m_hHeartBeatThread = (HANDLE)_beginthreadex(nullptr, 0, HeartBeatThread, this, 0, nullptr);
		m_hHeartBeatEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		//----------------------------------------------------------------------
		// 컨트롤러 실행
		//----------------------------------------------------------------------
		while (m_bLoop)
		{
			Controler();
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 서버 종료
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::Quit(void)
	{
		//----------------------------------------------------------------------
		// 서버가 이미 Stop 상태인가?
		//----------------------------------------------------------------------
		if (m_bRunFlag == false)
		{
			Stop(en_STOP_TYPE::RESOURCE_RELEASE);
		}
		else
		{
			Stop(en_STOP_TYPE::QUIT);
		}

		//----------------------------------------------------------------------
		// 모든 클라이언트가 삭제될 때 까지 무한 대기
		//----------------------------------------------------------------------
		while (m_Clients.size() != 0) {};

		//----------------------------------------------------------------------
		// 스레드 정지
		//----------------------------------------------------------------------
		m_bLoop = false;
		SetEvent(m_hHeartBeatEvent);
		WaitForSingleObject(m_hHeartBeatThread, INFINITE);
		WaitForSingleObject(m_hMonitorThread, INFINITE);
		WaitForSingleObject(m_hSendThread, INFINITE);

		//----------------------------------------------------------------------
		// 리소스 정리
		//----------------------------------------------------------------------
		CloseHandle(m_hMonitorThread);
		CloseHandle(m_hHeartBeatThread);
		CloseHandle(m_hHeartBeatEvent);
		CloseHandle(m_hSendThread);
		ClearClients();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 서버 컨트롤러
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::Controler(void)
	{
		WCHAR ControlKey = _getwch();

		//----------------------------------------------------------------------
		// 서버 종료
		//----------------------------------------------------------------------
		if ('q' == ControlKey || 'Q' == ControlKey)
		{
			m_MonitorServer_Lan.Quit();
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
				m_MonitorServer_Lan.Stop(en_STOP_TYPE::STOP);
				m_MonitorServer_Lan.m_bRunFlag = false;

				Stop(en_STOP_TYPE::STOP);
				m_bRunFlag = false;
			}
			else
			{
				m_MonitorServer_Lan.Restart();
				m_MonitorServer_Lan.m_bRunFlag = true;

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

	//////////////////////////////////////////////////////////////////////////////////
	// 
	//////////////////////////////////////////////////////////////////////////////////
	bool CMonitorServer_Net::OnConnectionRequest(const WCHAR* ConnectIP, const WORD ConnectPORT)
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 새로운 클라이언트가 접속했다.
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::OnClientJoin(const SESSION_ID SessionID)
	{
		CreateClient(SessionID);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 기존 클라이언트가 연결 종료됐다.
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::OnClientLeave(const SESSION_ID SessionID)
	{
		//----------------------------------------------------------------------
		// [예외] 존재하는 세션인가?
		//		  이미 삭제되어있는 경우가 있어서는 안된다.
		//----------------------------------------------------------------------
		st_CLIENT* pClient = FindClient(SessionID);
		if (nullptr == pClient)
		{
			CCrashDump::Crash();
			return;
		}

		DeleteClient(SessionID, pClient);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 패킷을 수신 받았다.
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::OnRecv(const SESSION_ID SessionID, CPacket* pRecvPacket)
	{
		PacketProcedure(SessionID, pRecvPacket);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 패킷을 송신했다.
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::OnSend(const SESSION_ID SessionID, const DWORD dwSendLength)
	{
	}

	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::OnWorkerThreadBegin(void)
	{
	}

	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::OnWorkerThreadEnd(void)
	{
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 라이브러리에서 에러가 발생했다.
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::OnError(const en_ERROR_CODE eErrCode, const SESSION_ID SessionID)
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
	// 패킷 처리
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::PacketProcedure(SESSION_ID SessionID, CPacket* pRecvPacket)
	{
		WORD Type;
		*pRecvPacket >> Type;

		switch (Type)
		{
		case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
			PacketProcedure_Login(SessionID, pRecvPacket);
			break;

		default:
			CCrashDump::Crash();
			break;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 로그인 요청 패킷 처리
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::PacketProcedure_Login(SESSION_ID SessionID, CPacket* pRecvPacket)
	{
		st_CLIENT* pClient = FindClient(SessionID);
		if (nullptr == pClient)
		{
			CCrashDump::Crash();
			return;
		}

		//-------------------------------------------------------------------------
		// 로그인 키 체크
		//-------------------------------------------------------------------------
		char RecvPacket_LoginSessionKey[32] = { 0 };
		pRecvPacket->Get(RecvPacket_LoginSessionKey, 32);

		CPacket* pSendPacket = CPacket::Alloc();
		if (memcmp(m_LoginKey, RecvPacket_LoginSessionKey, 32) == 0)
		{
			MakePacket_ResLogin(pSendPacket, dfMONITOR_TOOL_LOGIN_OK);
		}
		else
		{
			MakePacket_ResLogin(pSendPacket, dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY);
		}

		//-------------------------------------------------------------------------
		// 결과 송신
		//-------------------------------------------------------------------------
		SendPacket(pClient->SessionID, pSendPacket);
		pSendPacket->SubRef();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 로그인 응답 패킷 만들기
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::MakePacket_ResLogin(CPacket* pSendPacket, const BYTE Status)
	{
		*pSendPacket << (WORD)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
		*pSendPacket << Status;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 툴에 송신해줄 데이터 패킷 만들기
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::MakePacket_UpdateData(CPacket* pSendPacket, const BYTE ServerNo, const BYTE DataType, const int DataValue, const int TimeStamp)
	{
		*pSendPacket << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
		*pSendPacket << ServerNo;
		*pSendPacket << DataType;
		*pSendPacket << DataValue;
		*pSendPacket << TimeStamp;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 새로운 클라이언트 생성
	//////////////////////////////////////////////////////////////////////////////////
	CMonitorServer_Net::st_CLIENT* CMonitorServer_Net::CreateClient(const SESSION_ID SessionID)
	{
		st_CLIENT* pNewClient = m_ClientPool.Alloc();
		pNewClient->SessionID = SessionID;

		LockClients();
		m_Clients.insert(pair<SESSION_ID, st_CLIENT*>(SessionID, pNewClient));
		UnlockClients();

		return pNewClient;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 기존 클라이언트 찾기
	//////////////////////////////////////////////////////////////////////////////////
	CMonitorServer_Net::st_CLIENT* CMonitorServer_Net::FindClient(const SESSION_ID SessionID)
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
	void CMonitorServer_Net::DeleteClient(const SESSION_ID SessionID, st_CLIENT* pClient)
	{
		LockClients();
		m_Clients.erase(m_Clients.find(pClient->SessionID));
		UnlockClients();

		m_ClientPool.Free(pClient);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 클라이언트 관리 컨테이너 정리
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::ClearClients(void)
	{
		unordered_map<SESSION_ID, st_CLIENT*>::iterator iter = m_Clients.begin();
		unordered_map<SESSION_ID, st_CLIENT*>::iterator end = m_Clients.end();

		for (iter; iter != end;)
		{
			iter = m_Clients.erase(iter);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 클라이언트 관리 컨테이너 락걸기
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::LockClients(void)
	{
		EnterCriticalSection(&m_Clients_cs);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 클라이언트 관리 컨테이너 락걸기
	//////////////////////////////////////////////////////////////////////////////////
	void CMonitorServer_Net::UnlockClients(void)
	{
		LeaveCriticalSection(&m_Clients_cs);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Monitor Thread
	// 
	//////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall CMonitorServer_Net::MonitorThread(void* lpParam)
	{
		return ((CMonitorServer_Net*)lpParam)->MonitorThread_Procedure();
	}
	int CMonitorServer_Net::MonitorThread_Procedure(void)
	{
		CCpuUsage	CpuUsage;
		CPDH		PDH(L"MonitorServer_Lan");
		WCHAR		szFileName[32];
		SYSTEMTIME	stNowTime;
		GetLocalTime(&stNowTime);
		wsprintf(szFileName,
			L"%04d.%02d.%02d %02d.%02d.%02d",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay,
			stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond
		);

		while (m_bLoop)
		{
			int DBWriteCompeleteCount = m_MonitorServer_Lan.m_DBWriteCompletedTotal;
			if (DBWriteCompeleteCount == 0)
			{
				DBWriteCompeleteCount = 1;
			}

			//--------------------------------------------------------
			// 모니터링 데이터 갱신
			//--------------------------------------------------------
			CpuUsage.UpdateCpuTime();
			PDH.Collect();

			//--------------------------------------------------------
			// 서버 시작 시간 출력
			//--------------------------------------------------------
			wprintf(L"[MONITOR SERVER] StartTime : %s\n\n", szFileName);

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
				wprintf(L"NOW MODE  : PLAY");
			}
			else
			{
				wprintf(L"NOW MODE  : STOP");
			}
			wprintf(L" (Q : QUIT  | S : STOP/PLAY)\n");

			//--------------------------------------------------------
			// 로그 출력
			//--------------------------------------------------------
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool    Size    : %d\n", CPacket::GetPoolCapacity());
			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool    Use     : %d\n", CPacket::GetPoolUseSize());
			CONSOLE(eLOG_LEVEL_DEBUG, L"==============================================================\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"[LAN SERVER] : Worker:%d | Active:%d | Nagle:%d | ZeroCopy:%d\n",
				m_MonitorServer_Lan.m_NumberOfIOCPWorkerThread, 
				m_MonitorServer_Lan.m_NumberOfIOCPActiveThread,
				m_MonitorServer_Lan.m_bNagleFlag, 
				m_MonitorServer_Lan.m_bZeroCopyFlag
			);
			CONSOLE(eLOG_LEVEL_DEBUG, L"==============================================================\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Session               : %d\n", m_MonitorServer_Lan.GetSessionCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"Index                 : %d\n", m_MonitorServer_Lan.GetIndexesCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"RecvPacket    TPS     : %d\n", m_MonitorServer_Lan.m_RecvPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Accpet        Total   : %I64u\n", m_MonitorServer_Lan.m_AcceptTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Disconnect    Total   : %I64u\n", m_MonitorServer_Lan.m_DisconnectTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"ServerPool    Size    : %d\n", m_MonitorServer_Lan.m_ServerPool.GetCapacity());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ServerPool    Use     : %d\n", m_MonitorServer_Lan.m_ServerPool.GetUseCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ServerMap     Use     : %d\n", m_MonitorServer_Lan.m_Servers.size());
			CONSOLE(eLOG_LEVEL_DEBUG, L"DataUpdate    TPS     : %d\n", m_MonitorServer_Lan.m_DataUpdateTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"DBWrite       TPS     : %d\n", m_MonitorServer_Lan.m_DBWriteTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"DBWrite       Avr     : %.2fms\n", (float)m_MonitorServer_Lan.m_DBWriteCompletedTotal / (float)DBWriteCompeleteCount);
			CONSOLE(eLOG_LEVEL_DEBUG, L"DBWrite Total Count   : %d\n", m_MonitorServer_Lan.m_DBWriteCompletedTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"\n==============================================================\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"[NET SERVER] : Worker:%d | Active:%d | Nagle:%d | ZeroCopy:%d\n",
				m_NumberOfIOCPWorkerThread, m_NumberOfIOCPActiveThread, m_bNagleFlag, m_bZeroCopyFlag
			);
			CONSOLE(eLOG_LEVEL_DEBUG, L"==============================================================\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Session               : %d\n", GetSessionCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"Index                 : %d\n", GetIndexesCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"RecvPacket    TPS     : %d\n", m_RecvPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"SendPacket    TPS     : %d\n", m_SendPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"ExpSendBytes          : %I64u\n", m_ExpSendBytes);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Accpet        Total   : %I64u\n", m_AcceptTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Disconnect    Total   : %I64u\n", m_DisconnectTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"ClientPool    Size    : %d\n", m_ClientPool.GetCapacity());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ClientPool    Use     : %d\n", m_ClientPool.GetUseCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ClientMap     Use     : %d\n", m_Clients.size());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"CPU usage [T:%.1f%% U:%.1f%% K:%.1f%%]\n", CpuUsage.ProcessTotal(), CpuUsage.ProcessUser(), CpuUsage.ProcessKernel());

			m_MonitorServer_Lan.m_AcceptTPS = 0;
			m_MonitorServer_Lan.m_DisconnectTPS = 0;
			m_MonitorServer_Lan.m_RecvPacketTPS = 0;
			m_MonitorServer_Lan.m_SendPacketTPS = 0;
			m_MonitorServer_Lan.m_DataUpdateTPS = 0;
			m_MonitorServer_Lan.m_DBWriteTPS = 0;

			m_AcceptTPS = 0;
			m_DisconnectTPS = 0;
			m_RecvPacketTPS = 0;
			m_SendPacketTPS = 0;
			m_ExpSendBytes = 0;

			Sleep(999);
		}

		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// HeartBeat Thread
	// 
	//////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall CMonitorServer_Net::HeartBeatThread(void* lpParam)
	{
		return ((CMonitorServer_Net*)lpParam)->HeartBeatThread_Procedure();
	}

	int CMonitorServer_Net::HeartBeatThread_Procedure(void)
	{
		DWORD NowTime;
		st_CLIENT* pClient;
		unordered_map<SESSION_ID, st_CLIENT*>::iterator ClientIter;
		unordered_map<SESSION_ID, st_CLIENT*>::iterator ClientEnd;

		while (m_bLoop)
		{
			if (WAIT_TIMEOUT != WaitForSingleObject(m_hHeartBeatEvent, INFINITE))
			{
				return 0;
			}

			if (m_Clients.size() == 0)
			{
				continue;
			}

			NowTime = timeGetTime();

			LockClients();
			ClientIter = m_Clients.begin();
			ClientEnd = m_Clients.end();

			for (ClientIter; ClientIter != ClientEnd; ++ClientIter)
			{
				pClient = ClientIter->second;
				if ((NowTime - pClient->LastRecvTime) >= m_TimeOutMax)
				{
					Disconnect(pClient->SessionID);
				}
			}

			UnlockClients();
		}

		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Send Thread
	// 
	//////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall CMonitorServer_Net::SendThread(void* lpParam)
	{
		return ((CMonitorServer_Net*)lpParam)->SendThread_Procedure();
	}
	int CMonitorServer_Net::SendThread_Procedure(void)
	{
		unordered_map<SESSION_ID, CMonitorServer_Lan::st_SERVER*>::iterator ServerIter;
		unordered_map<SESSION_ID, CMonitorServer_Lan::st_SERVER*>::iterator ServerEnd;
		unordered_map<SESSION_ID, st_CLIENT*>::iterator ClientIter;
		unordered_map<SESSION_ID, st_CLIENT*>::iterator ClientEnd;
		CMonitorServer_Lan::st_SERVER* pServer = nullptr;
		st_CLIENT* pClient = nullptr;
		CPacket* pSendPacket = nullptr;

		while (m_bLoop)
		{
			Sleep(999);

			//-------------------------------------------------------------------------
			// 접속한 서버가 없다면 스킵
			//-------------------------------------------------------------------------
			if (m_MonitorServer_Lan.m_Servers.size() == 0)
			{
				continue;
			}

			//-------------------------------------------------------------------------
			// 접속한 클라가 없다면 스킵
			//-------------------------------------------------------------------------
			if (m_Clients.size() == 0)
			{
				continue;
			}

			ServerIter = m_MonitorServer_Lan.m_Servers.begin();
			ServerEnd = m_MonitorServer_Lan.m_Servers.end();

			m_MonitorServer_Lan.LockServers();

			//-------------------------------------------------------------------------
			// 접속해있는 모든 서버 데이터를 읽는다.
			//-------------------------------------------------------------------------
			for (ServerIter; ServerIter != ServerEnd; ++ServerIter)
			{
				pServer = ServerIter->second;

				LockClients();

				ClientIter = m_Clients.begin();
				ClientEnd = m_Clients.end();

				//-------------------------------------------------------------------------
				// n : m 비율로 서버데이터를 클라이언트에게 뿌려준다.
				//-------------------------------------------------------------------------
				for (ClientIter; ClientIter != ClientEnd; ++ClientIter)
				{
					pClient = ClientIter->second;

					switch (pServer->ServerNo)
					{
					case en_SERVER_TYPE::MASTER:
						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 0, dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, m_MonitorServer_Lan.m_CpuTotal.DataValue, m_MonitorServer_Lan.m_CpuTotal.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 0, dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, m_MonitorServer_Lan.m_NonPagedMem.DataValue, m_MonitorServer_Lan.m_NonPagedMem.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 0, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, m_MonitorServer_Lan.m_NetworkRecvSize.DataValue, m_MonitorServer_Lan.m_NetworkRecvSize.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 0, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, m_MonitorServer_Lan.m_NetworkSendSize.DataValue, m_MonitorServer_Lan.m_NetworkSendSize.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 0, dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, m_MonitorServer_Lan.m_AvailabeMem.DataValue, m_MonitorServer_Lan.m_AvailabeMem.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						break;

					case en_SERVER_TYPE::LOGIN:
						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 1, dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, pServer->ServerRun.DataValue, pServer->ServerRun.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 1, dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, pServer->ServerCpu.DataValue, pServer->ServerCpu.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 1, dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, pServer->ServerMem.DataValue, pServer->ServerMem.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 1, dfMONITOR_DATA_TYPE_LOGIN_SESSION, pServer->SessionCount.DataValue, pServer->SessionCount.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 1, dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, pServer->AuthTPS.DataValue, pServer->AuthTPS.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 1, dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, pServer->PacketPoolSize.DataValue, pServer->PacketPoolSize.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						break;

					case en_SERVER_TYPE::GAME:
						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, pServer->ServerRun.DataValue, pServer->ServerRun.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, pServer->ServerCpu.DataValue, pServer->ServerCpu.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, pServer->ServerMem.DataValue, pServer->ServerMem.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_SESSION, pServer->SessionCount.DataValue, pServer->SessionCount.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, pServer->AuthPlayerCount.DataValue, pServer->AuthPlayerCount.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, pServer->GamePlayerCount.DataValue, pServer->GamePlayerCount.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, pServer->AcceptTPS.DataValue, pServer->AcceptTPS.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, pServer->RecvPacketTPS.DataValue, pServer->RecvPacketTPS.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, pServer->SendPacketTPS.DataValue, pServer->SendPacketTPS.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, pServer->DBWriteTPS.DataValue, pServer->DBWriteTPS.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG, pServer->DBWriteMSG.DataValue, pServer->DBWriteMSG.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, pServer->AuthThreadTPS.DataValue, pServer->AuthThreadTPS.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, pServer->GameThreadTPS.DataValue, pServer->GameThreadTPS.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 2, dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, pServer->PacketPoolSize.DataValue, pServer->PacketPoolSize.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						break;

					case en_SERVER_TYPE::CHAT:
						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 3, dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, pServer->ServerRun.DataValue, pServer->ServerRun.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 3, dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, pServer->ServerCpu.DataValue, pServer->ServerCpu.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 3, dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, pServer->ServerMem.DataValue, pServer->ServerMem.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 3, dfMONITOR_DATA_TYPE_CHAT_SESSION, pServer->SessionCount.DataValue, pServer->SessionCount.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 3, dfMONITOR_DATA_TYPE_CHAT_PLAYER, pServer->PlayerCount.DataValue, pServer->PlayerCount.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 3, dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, pServer->UpdateTPS.DataValue, pServer->UpdateTPS.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 3, dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, pServer->PacketPoolSize.DataValue, pServer->PacketPoolSize.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						pSendPacket = CPacket::Alloc();
						MakePacket_UpdateData(pSendPacket, 3, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, pServer->UpdateMsgPoolSize.DataValue, pServer->UpdateMsgPoolSize.TimeStamp);
						SendPacket(pClient->SessionID, pSendPacket);
						pSendPacket->SubRef();

						break;

					default:
						CCrashDump::Crash();
						break;
					}
				}
				UnlockClients();
			}
			m_MonitorServer_Lan.UnlockServers();
		}

		return 0;
	}
}