#include "LoginServer.h"

namespace cov1013
{
	///////////////////////////////////////////////////////////////////////////
	// 생성자
	///////////////////////////////////////////////////////////////////////////
	LoginServer::LoginServer()
	{
		m_TimeOutMax	= INT_MAX;
		m_bLoop			= false;
		m_bRunFlag		= false;
		InitializeCriticalSection(&m_Clients_cs);
	}

	///////////////////////////////////////////////////////////////////////////
	// 소멸자
	///////////////////////////////////////////////////////////////////////////
	LoginServer::~LoginServer()
	{
		DeleteCriticalSection(&m_Clients_cs);

		//delete m_pDBConnector;
		//delete m_pRedisConnector;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 서버 실행
	//////////////////////////////////////////////////////////////////////////////////
	void LoginServer::Run(void)
	{
		//----------------------------------------------------------------------
		// 중복 실행 방지
		//----------------------------------------------------------------------
		if (m_bRunFlag)
		{
			wprintf(L"Overlapped Running");
			return;
		}
		m_bRunFlag = true;

		//----------------------------------------------------------------------
		// 서버 설정 정보 불러오기 & 세팅
		//----------------------------------------------------------------------
		if (!m_Parser.LoadFile(L"./Configs/LoginServer.ini"))
		{
			wprintf(L"Server Config Open Failed");
			m_bRunFlag = false;
			return;
		}

		CrashDumper::CrashDumper();
		Logger::Logger(L"./Logs/LoginServer", eLOG_LEVEL_DEBUG);
		InitializeProfiler(L"./Profiling/LoginServer", en_PROFILER_UNIT::eUNIT_NANO);

		//------------------------------------------------------
		// Listen IP / PORT
		//------------------------------------------------------
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
		// Server Setting
		//------------------------------------------------------
		bool SELF_SEND_FLAG;
		m_Parser.GetValue(L"SELF_SEND_FLAG", &SELF_SEND_FLAG);

		//------------------------------------------------------
		// SERVICE : 미응답 유저 타임아웃 처리
		//------------------------------------------------------
		m_Parser.GetValue(L"TIMEOUT_DISCONNECT", &m_TimeOutMax);

		//----------------------------------------------------------------------
		// DB 연결
		//----------------------------------------------------------------------
		//m_pDBConnector = new CDBConnector_TLS(L"127.0.0.1", L"root", L"0000", L"accountdb", 3306);
		//m_pRedisConnector = new CRedis_TLS();

		//----------------------------------------------------------------------
		// 네트워크 라이브러리 실행
		//----------------------------------------------------------------------
		if (!Start(BIND_IP, BIND_PORT, IOCP_WORKER_THREAD, IOCP_ACTIVE_THREAD, NAGLE_FLAG, ZERO_COPY_FLAG, CLIENT_MAX, PACKET_SIZE_MAX, SELF_SEND_FLAG, KEEP_ALIVE_FLAG))
		{
			wprintf(L"Network Library Start Failed\n");
			m_bRunFlag = false;
			return;
		}

		//----------------------------------------------------------
		// 콘솔창 초기화
		//----------------------------------------------------------
		EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_GRAYED);
		DrawMenuBar(GetConsoleWindow());
		system("mode con cols=70 lines=42");
		wprintf(L"Server Open...\n");
		m_bLoop = true;

		//----------------------------------------------------------
		// 스레드 생성
		//----------------------------------------------------------
		m_hMonitorThread = (HANDLE)_beginthreadex(nullptr, 0, MonitorThread, this, false, nullptr);
		m_hHeartBeatThread = (HANDLE)_beginthreadex(nullptr, 0, HeartBeatThread, this, false, nullptr);
		m_hHeartBeatEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
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

	///////////////////////////////////////////////////////////////////////////
	// 서버 정지
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::Quit(void)
	{
		if (m_bRunFlag == true)
		{
			Stop(en_STOP_TYPE::QUIT);
		}
		else
		{
			Stop(en_STOP_TYPE::RESOURCE_RELEASE);
		}

		//----------------------------------------------------------
		// 컨텐츠 파트의 모든 클라이언트가 삭제될 때 까지 대기
		//----------------------------------------------------------
		while (m_Clients.size() != 0) {};

		//----------------------------------------------------------
		// 컨텐츠 파트 스레드 종료
		//----------------------------------------------------------
		m_bLoop = false;
		SetEvent(m_hHeartBeatEvent);
		SetEvent(m_hMonitorServerConnectThreadEvent);
		WaitForSingleObject(m_hMonitorThread, INFINITE);
		WaitForSingleObject(m_hHeartBeatThread, INFINITE);
		WaitForSingleObject(m_hMonitorServerConnectThread, INFINITE);

		//----------------------------------------------------------
		// 리소스 정리
		//----------------------------------------------------------
		CloseHandle(m_hMonitorThread);
		CloseHandle(m_hHeartBeatThread);
		CloseHandle(m_hHeartBeatEvent);
		CloseHandle(m_hMonitorServerConnectThread);
		CloseHandle(m_hMonitorServerConnectThreadEvent);
		ClearClients();

		//----------------------------------------------------------
		// 모니터링 클라이언트가 연결 종료될 때 까지 대기
		//----------------------------------------------------------
		m_MonitorClient.Stop();
	}

	///////////////////////////////////////////////////////////////////////////
	// 서버 컨트롤러
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::Controler(void)
	{
		WCHAR ControlKey = _getwch();

		//----------------------------------------------------------
		// 서버 종료
		//----------------------------------------------------------
		if ('q' == ControlKey || 'Q' == ControlKey)
		{
			Quit();
			return;
		}

		//----------------------------------------------------------
		// 서버 정지/재시작
		//----------------------------------------------------------
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
				//while (m_Clients.size() != 0) {};
				m_bRunFlag = false;
			}
			else
			{
				Restart();
				m_bRunFlag = true;
			}
			return;
		}

		//----------------------------------------------------------
		// 프로파일러 출력
		//----------------------------------------------------------
		if ('p' == ControlKey || 'P' == ControlKey)
		{
			OutputProfilingData();
			return;
		}

		//----------------------------------------------------------
		// 로그 레벨 변경
		//----------------------------------------------------------
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

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	bool LoginServer::OnConnectionRequest(const WCHAR* ConnectIP, const WORD ConnectPORT)
	{
		return false;
	}

	///////////////////////////////////////////////////////////////////////////
	// 새로운 클라이언트가 접속했다.
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::OnClientJoin(const SESSION_ID SessionID)
	{
		CreateClient(SessionID);
	}

	///////////////////////////////////////////////////////////////////////////
	// 기존 클라이언트가 연결 종료됐다.
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::OnClientLeave(const SESSION_ID SessionID)
	{
		st_CLIENT* pClient = FindClient(SessionID);
		if (nullptr == pClient)
		{
			CrashDumper::Crash();
			return;
		}

		DeleteClient(SessionID, pClient);
	}

	///////////////////////////////////////////////////////////////////////////
	// 패킷을 수신받았다.
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::OnRecv(const SESSION_ID SessionID, PacketBuffer* pRecvPacket)
	{
		PacketProcedure(SessionID, pRecvPacket);
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::OnSend(const SESSION_ID SessionID, const DWORD dwSendLength)
	{
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::OnWorkerThreadBegin(void)
	{
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::OnWorkerThreadEnd(void)
	{
	}

	///////////////////////////////////////////////////////////////////////////
	// 에러 처리
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::OnError(const en_ERROR_CODE eErrCode, const SESSION_ID SessionID)
	{
		switch (eErrCode)
		{
		case en_ERROR_WINSOCK_INIT:
		case en_ERROR_WINSOCK_BIND:
		case en_ERROR_WINSOCK_LISTEN_SOCK:
		case en_ERROR_WINSOCK_SET_NAGLE:
		case en_ERROR_WINSOCK_SET_RST:
			CrashDumper::Crash();

			break;
		case en_ERROR_RING_BUF_RECV:
		case en_ERROR_RING_BUF_SEND:
			CrashDumper::Crash();

			break;
		case en_ERROR_CONFIG_OVER_THREAD:
		case en_ERROR_CONFIG_OVER_SESSION:
			CrashDumper::Crash();

			break;

		case en_ERROR_NET_PACKET:
			CrashDumper::Crash();
			break;

		case en_ERROR_WINSOCK_OVER_CONNECT:
			CrashDumper::Crash();

			break;
		case en_ERROR_DECODE_FAILD:
			CrashDumper::Crash();

			break;

		default:
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// 로그인 응답 메세지 만들기
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::MakePacket_ResLogin(PacketBuffer* pSendPacket, const INT64 AccountNo, const BYTE Status, const WCHAR* ID, const WCHAR* NickName, const WCHAR* GameServerIP, const USHORT GameServerPort, const WCHAR* ChatServerIP, const USHORT ChatServerPort)
	{
		*pSendPacket << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN;
		*pSendPacket << AccountNo;
		*pSendPacket << Status;
		pSendPacket->Put_UNI16(ID, en_ID_MAX);
		pSendPacket->Put_UNI16(NickName, en_NICK_NAME_MAX);
		pSendPacket->Put_UNI16(GameServerIP, en_IP_MAX);
		*pSendPacket << GameServerPort;
		pSendPacket->Put_UNI16(ChatServerIP, en_IP_MAX);
		*pSendPacket << ChatServerPort;
	}

	///////////////////////////////////////////////////////////////////////////
	// 패킷 처리
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::PacketProcedure(const SESSION_ID SessionID, PacketBuffer* pRecvPacket)
	{
		WORD Type;
		*pRecvPacket >> Type;

		switch (Type)
		{
		case en_PACKET_CS_LOGIN_REQ_LOGIN:
			InterlockedIncrement(&m_AuthWaitCount);
			PacketProcedure_Login(SessionID, pRecvPacket);
			break;

		default:
			CrashDumper::Crash();
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// 로그인 처리 프로시저
	///////////////////////////////////////////////////////////////////////////
	bool LoginServer::PacketProcedure_Login(const SESSION_ID SessionID, PacketBuffer* pRecvPacket)
	{
		INT64	RecvPacket_AccountNo;
		char	RecvPacket_SessionKey[en_SESSION_KEY_MAX];

		//--------------------------------------------------------------------------
		// DB 데이터 저장할 변수
		//--------------------------------------------------------------------------
		INT64	DB_AccountNo = -1;
		BYTE	DB_Status = -1;
		WCHAR	DB_ID[en_ID_MAX] = { 0 };
		WCHAR	DB_NickName[en_NICK_NAME_MAX] = { 0 };
		char	DB_SessionKey[en_SESSION_KEY_MAX] = { 0 };
		USHORT	DB_ChatServerPort = -1;
		USHORT	DB_GameServerPort = -1;
		WCHAR	DB_GameServerIP[16] = { 0 };
		WCHAR	DB_ChatServerIP[16] = { 0 };

		//--------------------------------------------------------------------------
		// Redis 데이터 저장할 변수
		//--------------------------------------------------------------------------
		char	Redis_Key[32] = { 0 };

		//--------------------------------------------------------------------------
		// 로그인 처리 시간 계산 변수
		//--------------------------------------------------------------------------
		DWORD	StartTime;
		DWORD	ElapsedTime;

		//--------------------------------------------------------------------------
		// [예외 1] 존재하는 세션인가?
		//          OnRecv를 타고왔다는 건 라이브러리측에서는 무조건 존재하는 세션이여야 한다.
		//--------------------------------------------------------------------------
		st_CLIENT* pClient = FindClient(SessionID);
		if (pClient == nullptr)
		{
			CrashDumper::Crash();
			return false;
		}

		StartTime = timeGetTime();

		//--------------------------------------------------------------------------
		// 클라이언트의 마지막 패킷 수신 시간 갱신
		//--------------------------------------------------------------------------
		pClient->LastRecvTime = StartTime;

		//--------------------------------------------------------------------------
		// 수신 패킷에서 데이터 뽑고
		//--------------------------------------------------------------------------
		*pRecvPacket >> RecvPacket_AccountNo;
		pRecvPacket->Get(RecvPacket_SessionKey, en_SESSION_KEY_MAX);

		//--------------------------------------------------------------------------
		// [예외 2] DB에 없는 AccountNo 였다.
		//--------------------------------------------------------------------------
		//if (!m_pDBConnector->Query_Save(L"SELECT `accountno`, `userid`, `usernick`, `sessionkey`, `status` FROM `accountdb`.`v_account` WHERE `accountno`=%d", RecvPacket_AccountNo))
		//{
		//	CrashDumper::Crash();

		//	//--------------------------------------------------------------------------
		//	// 여기까지 걸린 시간 계산 및 누적
		//	//--------------------------------------------------------------------------
		//	ElapsedTime = timeGetTime() - StartTime;
		//	pClient->AuthElapsedTime = ElapsedTime;
		//	InterlockedAdd64(&m_AuthElapsedTime, ElapsedTime);

		//	//--------------------------------------------------------------------------
		//	// 로그인 관련 모니터링 데이터 갱신
		//	//--------------------------------------------------------------------------
		//	InterlockedDecrement(&m_AuthWaitCount);
		//	InterlockedIncrement64(&m_AuthFailedTotal);

		//	return false;
		//}

		////--------------------------------------------------------------------------
		//// DB 접근 완료
		////--------------------------------------------------------------------------
		//MYSQL_ROW Row = m_pDBConnector->FetchRow();
		//if (Row != nullptr)
		//{
		//	//--------------------------------------------------------------------------
		//	// DB 에서 읽어온 값 복사
		//	//--------------------------------------------------------------------------
		//	DB_AccountNo = atoi(Row[0]);
		//	m_pDBConnector->UTF8ToUTF16(Row[1], DB_ID);
		//	m_pDBConnector->UTF8ToUTF16(Row[2], DB_NickName);
		//	memcpy_s(Row[3], en_SESSION_KEY_MAX, DB_SessionKey, en_SESSION_KEY_MAX);
		//	DB_Status = atoi(Row[4]);
		//}
		//else
		//{
		//	CrashDumper::Crash();
		//}
		//m_pDBConnector->FreeResult();
		//InterlockedIncrement(&m_DBConnectorTPS);

		////--------------------------------------------------------------------------
		//// Redis에 SessionKey 저장
		////--------------------------------------------------------------------------
		//_i64toa_s(DB_AccountNo, Redis_Key, 32, 10);
		//m_pRedisConnector->Set(Redis_Key, RecvPacket_SessionKey);
		//InterlockedIncrement(&m_RedisConnectorTPS);

		//--------------------------------------------------------------------------
		// 로그인 요청 결과에 대한 송신 패킷 세팅
		//--------------------------------------------------------------------------
		PacketBuffer* pSendPacket = PacketBuffer::Alloc();
		if (RecvPacket_AccountNo >= 0 && RecvPacket_AccountNo < 20000)
		{
			MakePacket_ResLogin(pSendPacket, DB_AccountNo, (BYTE)dfLOGIN_STATUS_OK, DB_ID, DB_NickName, L"127.0.0.1", 7000, L"127.0.0.1", 5000);
		}
		else if (RecvPacket_AccountNo >= 20000 && RecvPacket_AccountNo < 30000)
		{
			MakePacket_ResLogin(pSendPacket, DB_AccountNo, (BYTE)dfLOGIN_STATUS_OK, DB_ID, DB_NickName, L"10.0.1.1", 7000, L"10.0.1.1", 5000);
		}
		else if (RecvPacket_AccountNo >= 30000 && RecvPacket_AccountNo < 40000)
		{
			MakePacket_ResLogin(pSendPacket, DB_AccountNo, (BYTE)dfLOGIN_STATUS_OK, DB_ID, DB_NickName, L"10.0.2.1", 7000, L"10.0.2.1", 5000);
		}

		//--------------------------------------------------------------------------
		// 로그인 요청 결과 결과 송신
		//--------------------------------------------------------------------------
		SendPacket(SessionID, pSendPacket);
		pSendPacket->SubRef();

		//--------------------------------------------------------------------------
		// 여기까지 걸린 시간 갱신 및 누적
		//--------------------------------------------------------------------------
		ElapsedTime = timeGetTime() - StartTime;
		pClient->AuthElapsedTime = ElapsedTime;
		InterlockedAdd64(&m_AuthElapsedTime, ElapsedTime);

		//--------------------------------------------------------------------------
		// 모니터링 데이터 갱신
		//--------------------------------------------------------------------------
		InterlockedIncrement(&m_AuthTPS);
		InterlockedDecrement(&m_AuthWaitCount);
		InterlockedIncrement64(&m_AuthCompletedTotal);

		//--------------------------------------------------------------------------
		// [최종] 클라이언트 정보 세팅
		//--------------------------------------------------------------------------
		pClient->bLogin = true;
		pClient->Status = DB_Status;
		pClient->SessionID = SessionID;
		pClient->AccountNo = DB_AccountNo;
		wcscpy_s(pClient->ID, en_ID_MAX, DB_ID);
		wcscpy_s(pClient->NickName, en_NICK_NAME_MAX, DB_NickName);
		memcpy_s(pClient->SessionKey, en_SESSION_KEY_MAX, RecvPacket_SessionKey, en_SESSION_KEY_MAX);

		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// 클라이언트 생성
	///////////////////////////////////////////////////////////////////////////
	LoginServer::st_CLIENT* LoginServer::CreateClient(const SESSION_ID SessionID)
	{
		st_CLIENT* pNewClient = m_ClientPool.Alloc();
		pNewClient->bLogin = false;
		pNewClient->SessionID = SessionID;
		pNewClient->LastRecvTime = timeGetTime();

		LockClients();
		m_Clients.insert(pair<SESSION_ID, st_CLIENT*>(SessionID, pNewClient));
		UnlockClients();

		return pNewClient;
	}

	///////////////////////////////////////////////////////////////////////////
	// 클라이언트 찾기
	///////////////////////////////////////////////////////////////////////////
	LoginServer::st_CLIENT* LoginServer::FindClient(const SESSION_ID SessionID)
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

	///////////////////////////////////////////////////////////////////////////
	// 클라이언트 삭제
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::DeleteClient(const SESSION_ID SessionID, st_CLIENT* pClient)
	{
		LockClients();
		m_Clients.erase(m_Clients.find(pClient->SessionID));
		UnlockClients();

		m_ClientPool.Free(pClient);
	}

	///////////////////////////////////////////////////////////////////////////
	// 모든 클라이언트 정리
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::ClearClients(void)
	{
		unordered_map<SESSION_ID, st_CLIENT*>::iterator iter = m_Clients.begin();
		unordered_map<SESSION_ID, st_CLIENT*>::iterator end = m_Clients.end();

		for (iter; iter != end;)
		{
			iter = m_Clients.erase(iter);
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// 클라이언트 컨테이너 락 걸기
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::LockClients(void)
	{
		EnterCriticalSection(&m_Clients_cs);
	}

	///////////////////////////////////////////////////////////////////////////
	// 클라이언트 컨테이너 락 풀기
	///////////////////////////////////////////////////////////////////////////
	void LoginServer::UnlockClients(void)
	{
		LeaveCriticalSection(&m_Clients_cs);
	}

	///////////////////////////////////////////////////////////////////////////
	// Monitor Thread
	// 
	///////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall LoginServer::MonitorThread(void* lpParam)
	{
		return ((LoginServer*)lpParam)->MonitorThread_Procedure();
	}
	int LoginServer::MonitorThread_Procedure(void)
	{
		CPUMeter	CpuUsage;
		PerformaceDataHelper		PDH(L"LoginServer");
		WCHAR		szFileName[32];
		SYSTEMTIME	stNowTime;
		GetLocalTime(&stNowTime);
		wsprintf(szFileName, L"%04d.%02d.%02d %02d.%02d.%02d",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, 
			stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond
		);

		while (m_bLoop)
		{
			Sleep(999);

			//--------------------------------------------------------
			// 모니터링 데이터 갱신
			//--------------------------------------------------------
			CpuUsage.UpdateCpuTime();
			PDH.Collect();

			//--------------------------------------------------------
			// 서버 시작 시간 출력
			//--------------------------------------------------------
			wprintf(L"[LOGIN SERVER] StartTime : %s\n\n", szFileName);

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

			//--------------------------------------------------------
			// 모니터링 서버로 전송할 데이터 목록
			//--------------------------------------------------------
			int PacketPoolSize = PacketBuffer::GetPoolCapacity();
			int SessionCount = GetSessionCount();
			float ProcessCpuUsage = CpuUsage.ProcessTotal();

			//--------------------------------------------------------
			// Auth 평균 시간 계산을 위한 클라이언트 수
			//--------------------------------------------------------
			int AuthTotalCount = m_AuthCompletedTotal;
			if (AuthTotalCount == 0)
			{
				AuthTotalCount = 1;
			}

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
			CONSOLE(eLOG_LEVEL_DEBUG, L"Session                 : %d\n", SessionCount);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Index                   : %d\n", GetIndexesCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Accpet           TPS    : %u\n", m_AcceptTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Disconnect       TPS    : %u\n", m_DisconnectTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"RecvPacket       TPS    : %u\n", m_RecvPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"SendPacket       TPS    : %u\n", m_SendPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"MClient Send     TPS    : %d\n", m_MonitorClient.m_SendPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"ExpSendBytes            : %I64u\n", m_ExpSendBytes);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"DBConnector      TPS    : %u\n", m_DBConnectorTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"RedisConnector   TPS    : %u\n", m_RedisConnectorTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Accpet           Total  : %I64u\n", m_AcceptTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Disconnect       Total  : %I64u\n", m_DisconnectTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool       Size   : %d\n", PacketPoolSize);
			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool       Use    : %d\n", PacketBuffer::GetPoolUseSize());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"ClientPool       Size   : %u\n", m_ClientPool.GetCapacity());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ClientPool       Use    : %u\n", m_ClientPool.GetUseCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ClientMap        Size   : %u\n", m_Clients.size());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Auth             TPS    : %d\n", m_AuthTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Auth Wait        Count  : %u\n", m_AuthWaitCount);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Auth Failed      Total  : %I64u\n", m_AuthFailedTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Auth Compeleted  Total  : %I64u\n", m_AuthCompletedTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Auth ElapsedTime Avr    : %.2fms\n", (float)m_AuthElapsedTime / (float)AuthTotalCount);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"ProcessCommitMemory     : %I64u\n", PDH.GetProcessCommitMemory());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ProcessNonPagedMemory   : %I64u\n", PDH.GetProcessNonPagedMemory());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"CPU Usage [T:%.1f%% U:%.1f%% K:%.1f%%]\n",ProcessCpuUsage, CpuUsage.ProcessUser(), CpuUsage.ProcessKernel());

#ifdef __SAFE_MODE__
			unsigned __int64 overBytes = 1024 * 1024 * 1024 * 2;
			unsigned __int64 overBytes2 = 500;
			unsigned __int64 commitMemSize = PDH.GetProcessCommitMemory();
			unsigned __int64 nonPagedPoolSize = PDH.GetNonPagedMemory();
			if (commitMemSize > overBytes || commitMemSize < 0)
			{
				LOG(L"System", en_LOG_LEVEL_SYSTEM, L"Lack of User Memory (%d bytes)\n", commitMemSize);
				Stop(true);
				CrashDumper::Crash();
			}

			if (nonPagedPoolSize < overBytes2)
			{
				LOG(L"System", en_LOG_LEVEL_SYSTEM, L"Lack of NonPaged Memory (%d bytes)\n", nonPagedPoolSize);
				CrashDumper::Crash();
			}
#endif

			//-------------------------------------------------------
			// 모니터링 서버로 데이터 전송
			//-------------------------------------------------------
			int	DataValues[30] = { 0 };
			int	DataValuesIndex = 0;
			DataValues[DataValuesIndex++] = true;
			DataValues[DataValuesIndex++] = ProcessCpuUsage;
			DataValues[DataValuesIndex++] = PDH.GetProcessCommitMemory() / (1024 * 1024);	// MB
			DataValues[DataValuesIndex++] = SessionCount;
			DataValues[DataValuesIndex++] = m_AuthTPS;
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

			//-------------------------------------------------------
			// 모니터 클라이언트 데이터 갱신
			//-------------------------------------------------------
			m_MonitorClient.m_SendPacketTPS = 0;

			//-------------------------------------------------------
			// 컨텐츠 측 모니터링 데이터 갱신
			//-------------------------------------------------------
			m_DBConnectorTPS = 0;
			m_RedisConnectorTPS = 0;
			m_AuthTPS = 0;
		}

		return 0;
	}

	///////////////////////////////////////////////////////////////////////////
	// HeartBeat Thread
	// 
	///////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall LoginServer::HeartBeatThread(void* lpParam)
	{
		return ((LoginServer*)lpParam)->HeartBeatThread_Procedure();
	}
	int LoginServer::HeartBeatThread_Procedure(void)
	{
		DWORD NowTime;
		unordered_map<SESSION_ID, st_CLIENT*>::iterator ClientIter;
		unordered_map<SESSION_ID, st_CLIENT*>::iterator ClientEnd;

		while (m_bLoop)
		{
			if (WAIT_TIMEOUT != WaitForSingleObject(m_hHeartBeatEvent, INFINITE))
			{
				return 0;
			}

			if (m_Clients.size() != 0)
			{
				NowTime = timeGetTime();

				LockClients();

				ClientIter = m_Clients.begin();
				ClientEnd = m_Clients.end();
				for (ClientIter; ClientIter != ClientEnd; ++ClientIter)
				{
					st_CLIENT* pClient = (*ClientIter).second;

					if ((NowTime - pClient->LastRecvTime) >= m_TimeOutMax)
					{
						//-------------------------------------------------------
						// Disconnect를 타고 OnClientLeave가 호출되는 경우 거기서 또 Lock을 거는데
						// 재귀적인 Lock이므로 크리티컬 섹션 동기화 객체를 사용한다.
						//-------------------------------------------------------
						Disconnect(pClient->SessionID);
					}
				}

				UnlockClients();
			}
		}

		return 0;
	}

	unsigned int __stdcall LoginServer::MonitorServerConnectThread(void* lpParam)
	{
		return ((LoginServer*)lpParam)->MonitorServerConnectThread_Procedure();
	}

	int LoginServer::MonitorServerConnectThread_Procedure(void)
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
	LoginServer::CMonitorClient::CMonitorClient()
	{
		m_bConnectedFlag = false;
	}

	LoginServer::CMonitorClient::~CMonitorClient()
	{
	}

	void LoginServer::CMonitorClient::SendPacket_UpdateData(const int DataValues[], const int DataCount)
	{
		//--------------------------------------------------------------
		// [예외] 연결이 끊어져있다면 리턴
		//--------------------------------------------------------------
		if (!m_bConnectedFlag)
		{
			return;
		}

		PacketBuffer*	pPacket;
		time_t		NowTime;

		_time64(&NowTime);

		for (int i = 0; i < DataCount; i++)
		{
			pPacket = PacketBuffer::Alloc();
			MakePacket_DataUpdate(pPacket, dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN + i, DataValues[i], (int)NowTime);

			//--------------------------------------------------------------
			// 실제로는 연결이 끊어졌는데, bConnectedFlag를 true라고 판단하고 
			// 여기까지 들어왔어도 라이브러리측에서 SendPacket에 실패할 것.
			//--------------------------------------------------------------
			SendPacket(pPacket);
			pPacket->SubRef();
		}
	}

	void LoginServer::CMonitorClient::OnEnterJoinServer(void)
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

	void LoginServer::CMonitorClient::OnLeaveServer(void)
	{
		m_bConnectedFlag = false;
	}

	void LoginServer::CMonitorClient::OnRecv(PacketBuffer* pRecvPacket)
	{
	}

	void LoginServer::CMonitorClient::OnSend(int sendsize)
	{
	}

	void LoginServer::CMonitorClient::OnWorkerThreadBegin(void)
	{
	}

	void LoginServer::CMonitorClient::OnWorkerThreadEnd(void)
	{
	}

	void LoginServer::CMonitorClient::OnError(int errorcode, wchar_t* szMsg)
	{
	}

	void LoginServer::CMonitorClient::MakePacket_Login(PacketBuffer* pSendPacket, const int ServerNo)
	{
		*pSendPacket << (WORD)en_PACKET_SS_MONITOR_LOGIN;
		*pSendPacket << ServerNo;
	}

	void LoginServer::CMonitorClient::MakePacket_DataUpdate(PacketBuffer* pSendPacket, const BYTE DataType, const int DataValue, int TimeStamp)
	{
		*pSendPacket << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
		*pSendPacket << DataType;
		*pSendPacket << DataValue;
		*pSendPacket << TimeStamp;
	}
}