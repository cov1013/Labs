#include "GamePlayer.h"
#include "GameServer.h"

namespace cov1013
{
	GameServer::GameServer()
	{
		m_TimeOutMax = INT_MAX;
		m_bLoop = false;
		m_bRunFlag = false;

		InitializeSRWLock(&m_Players_srw);
	}

	GameServer::~GameServer()
	{
		vector<GamePlayer*>::iterator iter = m_Players.begin();
		for (iter; iter != m_Players.end();)
		{
			(*iter)->~GamePlayer();
			_aligned_free(*iter);
			iter = m_Players.erase(iter);
		}
	}

	void GameServer::Run(void)
	{
		if (m_bRunFlag)
		{
			wprintf(L"Overlapped Running");
			return;
		}
		m_bRunFlag = true;

		// 서버 설정 정보 불러오기 & 세팅
		if (!m_Parser.LoadFile(L"./Configs/GameServerConfig.ini"))
		{
			wprintf(L"Server Config Open Failed");
			m_bRunFlag = false;
			return;
		}

		// Listen IP / PORT
		WCHAR	BIND_IP[16];
		WORD	BIND_PORT;
		m_Parser.GetValue(L"BIND_IP", BIND_IP, 16);
		m_Parser.GetValue(L"BIND_PORT", &BIND_PORT);

		//------------------------------------------------------
		// Library Setting
		//------------------------------------------------------
		DWORD	IOCP_WORKER_THREAD;
		DWORD	IOCP_ACTIVE_THREAD;
		DWORD	CLIENT_MAX;
		m_Parser.GetValue(L"IOCP_WORKER_THREAD", &IOCP_WORKER_THREAD);
		m_Parser.GetValue(L"IOCP_ACTIVE_THREAD", &IOCP_ACTIVE_THREAD);
		m_Parser.GetValue(L"CLIENT_MAX", &CLIENT_MAX);

		//------------------------------------------------------
		// Packet Encode / Decode Key
		//------------------------------------------------------
		BYTE	PACKET_CODE;
		BYTE	PACKET_KEY;
		int		PACKET_SIZE_MAX;
		m_Parser.GetValue(L"PACKET_CODE", &PACKET_CODE);
		m_Parser.GetValue(L"PACKET_KEY", &PACKET_KEY);
		m_Parser.GetValue(L"PACKET_SIZE_MAX", &PACKET_SIZE_MAX);
		PacketBuffer::SetPacketCode(PACKET_CODE);
		PacketBuffer::SetPacketKey(PACKET_KEY);

		//------------------------------------------------------
		// Network Setting
		//------------------------------------------------------
		bool	NAGLE_FLAG;
		bool	ZERO_COPY_FLAG;
		bool	KEEP_ALIVE_FLAG;
		m_Parser.GetValue(L"NAGLE_FLAG", &NAGLE_FLAG);
		m_Parser.GetValue(L"ZERO_COPY_FLAG", &ZERO_COPY_FLAG);
		m_Parser.GetValue(L"KEEP_ALIVE_FLAG", &KEEP_ALIVE_FLAG);

		//------------------------------------------------------
		// SERVICE : 미응답 유저 타임아웃 처리
		//------------------------------------------------------
		m_Parser.GetValue(L"TIMEOUT_DISCONNECT", &m_TimeOutMax);

		//----------------------------------------------------------------------
		// 세션 등록
		//----------------------------------------------------------------------
		GamePlayer* pPlayer;
		for (int i = 0; i < CLIENT_MAX; i++)
		{
			pPlayer = (GamePlayer*)_aligned_malloc(sizeof(GamePlayer), en_CACHE_ALIGN);
			new (pPlayer) GamePlayer();
			m_Players.push_back(pPlayer);
			SetSession(i, (MMOSession*)pPlayer);
		}

		//----------------------------------------------------------------------
		// 네트워크 라이브러리 실행
		//----------------------------------------------------------------------
		if (!Start(BIND_IP, BIND_PORT, IOCP_WORKER_THREAD, IOCP_ACTIVE_THREAD, CLIENT_MAX, PACKET_SIZE_MAX, NAGLE_FLAG, ZERO_COPY_FLAG, KEEP_ALIVE_FLAG))
		{
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Network Library Run Failed...\n");
			return;
		}

		//----------------------------------------------------------
		// 콘솔창 초기화 및 컨트롤러 실행
		//----------------------------------------------------------
		EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_GRAYED);
		DrawMenuBar(GetConsoleWindow());
		system("mode con cols=70 lines=37");
		wprintf(L"Server Open...\n");

		m_bLoop = true;

		//----------------------------------------------------------
		// 스레드 생성
		//----------------------------------------------------------
		m_hMonitorThread = (HANDLE)_beginthreadex(nullptr, 0, this->MonitorThread, this, false, nullptr);
		m_hMonitorServerConnectThread = (HANDLE)_beginthreadex(nullptr, 0, this->MonitorServerConnectThread, this, false, nullptr);
		m_hMonitorServerConnectThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		while (m_bLoop)
		{
			Controler();
		}
	}

	void GameServer::Quit(void)
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
		// 스레드 정지 및 리소스 정리
		//----------------------------------------------------------------------
		m_bLoop = false;
		SetEvent(m_hMonitorServerConnectThreadEvent);
		WaitForSingleObject(m_hMonitorServerConnectThread, INFINITE);
		WaitForSingleObject(m_hMonitorThread, INFINITE);
		CloseHandle(m_hMonitorThread);
		CloseHandle(m_hMonitorServerConnectThread);
		CloseHandle(m_hMonitorServerConnectThreadEvent);

		//----------------------------------------------------------
		// 모니터링 클라이언트가 연결 종료될 때 까지 대기
		//----------------------------------------------------------
		m_MonitorClient.Stop();
	}

	///////////////////////////////////////////////////////////////////////////////////
	// 서버 컨트롤러
	///////////////////////////////////////////////////////////////////////////////////
	void GameServer::Controler(void)
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
				Stop(en_STOP_TYPE::STOP);
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

	///////////////////////////////////////////////////////////////////////////////////
	// Monitor Thread
	// 
	///////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall GameServer::MonitorThread(void* lpParam)
	{
		return ((GameServer*)lpParam)->MonitorThread_Procedure();
	}
	int GameServer::MonitorThread_Procedure(void)
	{
		CPUMeter CpuUsage;
		PerformaceDataHelper PDH(L"GameServer");

		WCHAR szFileName[32];
		SYSTEMTIME stNowTime;
		GetLocalTime(&stNowTime);
		wsprintf(szFileName,
			L"%04d.%02d.%02d %02d.%02d.%02d\n",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond
		);

		while (m_bLoop)
		{
			//--------------------------------------------------------
			// 모니터링 데이터 갱신
			//--------------------------------------------------------
			CpuUsage.UpdateCpuTime();
			PDH.Collect();

			//--------------------------------------------------------
			// 서버 시작 시간 출력
			//--------------------------------------------------------
			wprintf(L"[GAME SERVER] StartTime : %s\n", szFileName);

			//--------------------------------------------------------
			// 로그 선택 조작 방법 출력
			//--------------------------------------------------------
			en_LOG_LEVEL eLogLevel = Logger::GetLogLevel();
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

			GetPlayerStatusCount();

			//--------------------------------------------------------
			// 모니터링 서버로 전송할 데이터 목록
			//--------------------------------------------------------
			int PacketPoolSize = PacketBuffer::GetPoolCapacity();
			int SessionCount = GetSessionCount();
			float ProcessCpuUsage = CpuUsage.ProcessTotal();
			LONGLONG ProcessCommitMemeory = PDH.GetProcessCommitMemory() / (1024 * 1024);

			//--------------------------------------------------------
			// 로그 출력
			//--------------------------------------------------------
			CONSOLE(eLOG_LEVEL_DEBUG, L"==============================================================\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"[NETWORK LIBRARY] : Worker:%d | Active:%d | Nagle:%d | ZeroCopy:%d\n",
				m_NumberOfIOCPWorkerThread, m_NumberOfIOCPActiveThread, m_bNagleFlag, m_bZeroCopyFlag
			);
			if (m_MonitorClient.m_bConnectedFlag)
			{
				CONSOLE(eLOG_LEVEL_DEBUG, L"[MONITER SERVER]  : Connected\n");
			}
			else
			{
				CONSOLE(eLOG_LEVEL_DEBUG, L"[MONITER SERVER]  : Reconecting...\n");
			}
			CONSOLE(eLOG_LEVEL_DEBUG, L"==============================================================\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Session               : %d\n", GetSessionCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"Index                 : %d\n", GetIndexesCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"AccpetQueue   Size    : %d\n", m_AcceptSockets.GetCapacity());
			CONSOLE(eLOG_LEVEL_DEBUG, L"Accpet        TPS     : %d\n", m_AcceptTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Disconnect    TPS     : %d\n", m_DisconnectTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"RecvPacket    TPS     : %d\n", m_RecvPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"SendPacket    TPS     : %d\n", m_SendPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"MClient Send  TPS     : %d\n", m_MonitorClient.m_SendPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"ExpSendBytes          : %d\n", m_ExpSendBytes);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Accpet     Total      : %I64u\n", m_AcceptTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Disconnect Total      : %I64u\n", m_DisconnectTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"AuthThread   FPS      : %d\n", m_AuthFPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"GameThread   FPS      : %d\n", m_GameFPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"SendThread   FPS      : %d\n", m_SendFPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"AuthPlayer   Count    : %d\n", m_AuthPlayer);
			CONSOLE(eLOG_LEVEL_DEBUG, L"GamePlayer   Count    : %d\n", m_GamePlayer);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool Size       : %d\n", PacketPoolSize);
			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool Use        : %d\n", PacketBuffer::GetPoolUseSize());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"ProcessCommitMemory   : %I64u MB\n", ProcessCommitMemeory);
			CONSOLE(eLOG_LEVEL_DEBUG, L"ProcessNonPagedMemory : %I64u MB\n", PDH.GetProcessNonPagedMemory() / (1024 * 1024));
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"CPU Usage [T:%.1f%% U:%.1f%% K:%.1f%%]\n", ProcessCpuUsage, CpuUsage.ProcessUser(), CpuUsage.ProcessKernel());

//#ifdef __SAFE_MODE__
//			unsigned __int64 overBytes = 1024 * 1024 * 1024 * 2;
//			unsigned __int64 overBytes2 = 500;
//			unsigned __int64 commitMemSize = PDH.GetProcessCommitMemory();
//			unsigned __int64 nonPagedPoolSize = PDH.GetNonPagedMemory();
//			if (commitMemSize > overBytes || commitMemSize < 0)
//			{
//				LOG(L"System", en_LOG_LEVEL_SYSTEM, L"Lack of User Memory (%d bytes)\n", commitMemSize);
//				Stop(true);
//				CrashDumper::Crash();
//			}
//
//			if (nonPagedPoolSize < overBytes2)
//			{
//				LOG(L"System", en_LOG_LEVEL_SYSTEM, L"Lack of NonPaged Memory (%d bytes)\n", nonPagedPoolSize);
//				CrashDumper::Crash();
//			}
//#endif
			int	DataValues[30] = { 0 };
			int	DataValuesIndex = 0;
			DataValues[DataValuesIndex++] = true;
			DataValues[DataValuesIndex++] = ProcessCpuUsage;
			DataValues[DataValuesIndex++] = ProcessCommitMemeory;
			DataValues[DataValuesIndex++] = SessionCount;
			DataValues[DataValuesIndex++] = m_AuthPlayer;
			DataValues[DataValuesIndex++] = m_GamePlayer;
			DataValues[DataValuesIndex++] = m_AcceptTPS;
			DataValues[DataValuesIndex++] = m_RecvPacketTPS;
			DataValues[DataValuesIndex++] = m_SendPacketTPS;
			DataValues[DataValuesIndex++] = 0;
			DataValues[DataValuesIndex++] = 0;
			DataValues[DataValuesIndex++] = m_AuthFPS;
			DataValues[DataValuesIndex++] = m_GameFPS;
			DataValues[DataValuesIndex++] = PacketPoolSize;
			m_MonitorClient.SendPacket_UpdateData(DataValues, DataValuesIndex);

			//-------------------------------------------------------
			// 라이브러리 측 모니터링 데이터 갱신
			//-------------------------------------------------------
			m_AcceptTPS = 0;
			m_DisconnectTPS = 0;
			m_RecvPacketTPS = 0;
			m_SendPacketTPS = 0;
			m_ExpSendBytes = 0;
			m_AuthFPS = 0;
			m_GameFPS = 0;
			m_SendFPS = 0;
			m_MonitorClient.m_SendPacketTPS = 0;
			m_AuthPlayer = 0;
			m_GamePlayer = 0;

			Sleep(999);
		}

		return 0;
	}
	unsigned int __stdcall GameServer::HeartBeatThread(void* lpParam)
	{
		return ((GameServer*)lpParam)->HeartBeatThread_Procedure();
	}
	int GameServer::HeartBeatThread_Procedure(void)
	{
		while (m_bLoop)
		{
			if (WAIT_TIMEOUT != WaitForSingleObject(m_hHeartBeatEvent, INFINITE))
			{
				return 0;
			}

			if (m_Players.size() != 0)
			{
				DWORD dwCurTime = timeGetTime();

				vector<GamePlayer*>::iterator iter = m_Players.begin();
				for (iter; iter != m_Players.end(); ++iter)
				{
					GamePlayer* m_Players = *iter;

					if ((dwCurTime - m_Players->m_LastRecvTime) >= m_TimeOutMax)
					{
						m_Players->Disconnect();
					}
				}
			}
		}

		return 0;
	}

	unsigned int __stdcall GameServer::MonitorServerConnectThread(void* lpParam)
	{
		return ((GameServer*)lpParam)->MonitorServerConnectThread_Procedure();
	}

	int GameServer::MonitorServerConnectThread_Procedure(void)
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

		while (m_bLoop)
		{
			if (WAIT_TIMEOUT != WaitForSingleObject(m_hMonitorServerConnectThreadEvent, 9999))
			{
				return 0;
			}

			if (m_MonitorClient.m_bConnectedFlag == false)
			{
				m_MonitorClient.Connect(BIND_IP, SERVER_IP, SERVER_PORT, IOCP_WORKER_THREAD, NAGLE_FLAG, ZERO_COPY_FLAG);
			}
		}

		return 0;
	}

	void GameServer::LockPlayers(void)
	{
		AcquireSRWLockExclusive(&m_Players_srw);
	}

	void GameServer::UnlockPlayers(void)
	{
		ReleaseSRWLockExclusive(&m_Players_srw);
	}

	//=================================================================================
	// Monitor Server Client 관련
	//=================================================================================
	GameServer::CMonitorClient::CMonitorClient()
	{
	}
	GameServer::CMonitorClient::~CMonitorClient()
	{
	}

	void GameServer::CMonitorClient::SendPacket_UpdateData(const int DataValues[], const int DataCount)
	{
		if (!m_bConnectedFlag)
		{
			return;
		}

		PacketBuffer* pPacket;
		time_t		Time;

		_time64(&Time);

		for (int i = 0; i < DataCount; i++)
		{
			pPacket = PacketBuffer::Alloc();
			MakePacket_DataUpdate(pPacket, dfMONITOR_DATA_TYPE_GAME_SERVER_RUN + i, DataValues[i], (int)Time);
			SendPacket(pPacket);
			pPacket->SubRef();
		}
	}

	void GameServer::CMonitorClient::OnEnterJoinServer(void)
	{
		//-------------------------------------------------------
		// 로그인 요청 패킷 보내기
		//-------------------------------------------------------
		PacketBuffer* pSendPacket = PacketBuffer::Alloc();
		MakePacket_Login(pSendPacket, m_MonitorNo);
		SendPacket(pSendPacket);
		pSendPacket->SubRef();

		m_bConnectedFlag = true;
	}
	void GameServer::CMonitorClient::OnLeaveServer(void)
	{
		m_bConnectedFlag = false;
	}

	void GameServer::CMonitorClient::OnRecv(PacketBuffer* pRecvPacket)
	{
	}

	void GameServer::CMonitorClient::OnSend(int sendsize)
	{
	}

	void GameServer::CMonitorClient::OnWorkerThreadBegin(void)
	{
	}

	void GameServer::CMonitorClient::OnWorkerThreadEnd(void)
	{
	}

	void GameServer::CMonitorClient::OnError(int errorcode, wchar_t* szMsg)
	{
	}

	void GameServer::CMonitorClient::MakePacket_Login(PacketBuffer* pSendPacket, const int ServerNo)
	{
		*pSendPacket << (WORD)en_PACKET_SS_MONITOR_LOGIN;
		*pSendPacket << ServerNo;
	}

	void GameServer::CMonitorClient::MakePacket_DataUpdate(PacketBuffer* pSendPacket, const BYTE DataType, const int DataValue, int TimeStamp)
	{
		*pSendPacket << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
		*pSendPacket << DataType;
		*pSendPacket << DataValue;
		*pSendPacket << TimeStamp;
	}
}