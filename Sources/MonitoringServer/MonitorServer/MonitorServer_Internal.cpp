#include "MonitorServer_Internal.h"
#include "CommonDefine.h"

namespace cov1013
{
	//////////////////////////////////////////////////////////////////////////////////
	// 생성자
	//////////////////////////////////////////////////////////////////////////////////
	MonitorServer_Internal::MonitorServer_Internal()
	{
		m_bLoop = false;
		m_bRunFlag = false;
		m_TimeOutMax = INT_MAX;
		//m_pDBConnector = nullptr;

		InitializeCriticalSection(&m_Servers_cs);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 소멸자
	//////////////////////////////////////////////////////////////////////////////////
	MonitorServer_Internal::~MonitorServer_Internal()
	{
		//if (nullptr != m_pDBConnector)
		//{
		//	delete m_pDBConnector;
		//}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 서버 실행
	//////////////////////////////////////////////////////////////////////////////////
	bool MonitorServer_Internal::Run(void)
	{
		//----------------------------------------------------------------------
		// 중복 실행 방지
		//----------------------------------------------------------------------
		if (true == m_bRunFlag)
		{
			wprintf(L"Overlapped Running");
			return false;
		}
		m_bRunFlag = true;

		//----------------------------------------------------------------------
		// 서버 설정 정보 불러오기 & 세팅
		//----------------------------------------------------------------------
		ConfigLoader Parser;
		if (!Parser.LoadFile(L"../Config_Lan.txt"))
		{
			wprintf(L"Server Config Open Failed");
			m_bRunFlag = false;
			return false;
		}

		//------------------------------------------------------
		// Listen IP / PORT
		//------------------------------------------------------
		WCHAR BIND_IP[16];
		WORD  BIND_PORT;
		Parser.GetValue(L"BIND_IP", BIND_IP, 16);
		Parser.GetValue(L"BIND_PORT", &BIND_PORT);

		//------------------------------------------------------
		// Library Setting
		//------------------------------------------------------
		DWORD IOCP_WORKER_THREAD;
		DWORD IOCP_ACTIVE_THREAD;
		DWORD CLIENT_MAX;
		Parser.GetValue(L"IOCP_WORKER_THREAD", &IOCP_WORKER_THREAD);
		Parser.GetValue(L"IOCP_ACTIVE_THREAD", &IOCP_ACTIVE_THREAD);
		Parser.GetValue(L"CLIENT_MAX", &CLIENT_MAX);

		//------------------------------------------------------
		// Network Setting
		//------------------------------------------------------
		bool NAGLE_FLAG;
		bool ZERO_COPY_FLAG;
		bool KEEP_ALIVE_FLAG;
		Parser.GetValue(L"NAGLE_FLAG", &NAGLE_FLAG);
		Parser.GetValue(L"ZERO_COPY_FLAG", &ZERO_COPY_FLAG);
		Parser.GetValue(L"KEEP_ALIVE_FLAG", &KEEP_ALIVE_FLAG);

		//------------------------------------------------------
		// SERVICE : 미응답 유저 타임아웃 처리
		//------------------------------------------------------
		Parser.GetValue(L"TIMEOUT_DISCONNECT", &m_TimeOutMax);

		//----------------------------------------------------------------------
		// DB 연결
		//----------------------------------------------------------------------
		//m_pDBConnector = new CDBConnector_TLS(L"127.0.0.1", L"root", L"0000", L"logdb", 3306);

		//----------------------------------------------------------------------
		// 네트워크 라이브러리 실행
		//----------------------------------------------------------------------
		if (!Start(BIND_IP, BIND_PORT, IOCP_WORKER_THREAD, IOCP_ACTIVE_THREAD, NAGLE_FLAG, ZERO_COPY_FLAG, CLIENT_MAX))
		{
			wprintf(L"Network Library Start Failed\n");
			m_bRunFlag = false;
			return false;
		}

		//----------------------------------------------------------
		// 스레드 생성
		//----------------------------------------------------------
		m_bLoop = true;
		m_hDBWriteThread = (HANDLE)_beginthreadex(nullptr, 0, DBWroteThread, this, 0, nullptr);
		m_hDBWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hHeartBeatThread = (HANDLE)_beginthreadex(nullptr, 0, HeartBeatThread, this, 0, nullptr);
		m_hHeartBeatEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 서버 종료
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::Quit(void)
	{
		//----------------------------------------------------------------------
		// 서버가 실행중이라면 완벽한 종료
		//----------------------------------------------------------------------
		if (m_bRunFlag == false)
		{
			Stop(en_STOP_TYPE::RESOURCE_RELEASE);
		}
		//----------------------------------------------------------------------
		// 서버가 정지중이라면 윈도우 리소스만 정리
		//----------------------------------------------------------------------
		else
		{
			Stop(en_STOP_TYPE::QUIT);
		}

		if (m_Servers.size() != 0)
		{
			CrashDumper::Crash();
		}

		//----------------------------------------------------------------------
		// 모든 스레드 정지
		//----------------------------------------------------------------------
		m_bLoop = false;
		SetEvent(m_hDBWriteEvent);
		WaitForSingleObject(m_hDBWriteThread, INFINITE);
		SetEvent(m_hHeartBeatEvent);
		WaitForSingleObject(m_hHeartBeatThread, INFINITE);

		//----------------------------------------------------------------------
		// 리소스 정리
		//----------------------------------------------------------------------
		CloseHandle(m_hHeartBeatThread);
		CloseHandle(m_hHeartBeatEvent);
		CloseHandle(m_hDBWriteThread);
		CloseHandle(m_hDBWriteEvent);
		ClearServers();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 
	//////////////////////////////////////////////////////////////////////////////////
	bool MonitorServer_Internal::OnConnectionRequest(const WCHAR* ConnectIP, const WORD ConnectPORT)
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 새로운 클라이언트가 접속했다.
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::OnClientJoin(const SESSION_ID SessionID)
	{
		CreateServer(SessionID);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 기존 클라이언트가 연결 종료됐다.
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::OnClientLeave(const SESSION_ID SessionID)
	{
		st_SERVER* pServer = FindServer(SessionID);
		if (nullptr == pServer)
		{
			CrashDumper::Crash();
		}

		DeleteServer(SessionID, pServer);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 패킷을 수신 받았다.
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::OnRecv(const SESSION_ID SessionID, PacketBuffer* pRecvPacket)
	{
		PacketProcedure(SessionID, pRecvPacket);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 패킷을 송신했다.
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::OnSend(const SESSION_ID SessionID, const DWORD dwTransferred)
	{
	}

	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::OnWorkerThreadBegin(void)
	{
	}

	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::OnWorkerThreadEnd(void)
	{
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 라이브러리에서 에러가 발생했다.
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::OnError(const en_ERROR_CODE eErrCode, const SESSION_ID SessionID)
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
	void MonitorServer_Internal::PacketProcedure(SESSION_ID SessionID, PacketBuffer* pRecvPacket)
	{
		WORD Type;
		*pRecvPacket >> Type;

		switch (Type)
		{
		case en_PACKET_SS_MONITOR_LOGIN:
			PacketProcedure_Login(SessionID, pRecvPacket);
			break;

		case en_PACKET_SS_MONITOR_DATA_UPDATE:
			PacketProcedure_DataUpdate(SessionID, pRecvPacket);
			break;

		default:
			CrashDumper::Crash();
			break;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 로그인 요청 패킷 처리
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::PacketProcedure_Login(SESSION_ID SessionID, PacketBuffer* pRecvPacket)
	{
		//----------------------------------------------------------------------
		// [예외] 존재하는 세션인가?
		//----------------------------------------------------------------------
		st_SERVER* pServer = FindServer(SessionID);
		if (nullptr == pServer)
		{
			CrashDumper::Crash();
			return;
		}

		pServer->LastRecvTime = timeGetTime();

		int ServerNo;
		*pRecvPacket >> ServerNo;
		pServer->ServerNo = ServerNo;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 데이터 갱신 요청 패킷 처리
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::PacketProcedure_DataUpdate(SESSION_ID SessionID, PacketBuffer* pRecvPacket)
	{
		//----------------------------------------------------------------------
		// [예외] 존재하는 세션인가?
		//----------------------------------------------------------------------
		st_SERVER* pServer = FindServer(SessionID);
		if (nullptr == pServer)
		{
			CrashDumper::Crash();
			return;
		}

		pServer->LastRecvTime = timeGetTime();

		BYTE DataType;
		int  DataValue;
		int  TimeStamp;
		st_MONITERING_DATA_SET* pDataSet = nullptr;

		*pRecvPacket >> DataType;
		*pRecvPacket >> DataValue;
		*pRecvPacket >> TimeStamp;

		//----------------------------------------------------------------------
		// 데이터 갱신
		//----------------------------------------------------------------------
		switch (DataType)
		{
		//==========================================================
		// LOGIN SERVER
		//==========================================================
		case dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN:
			pDataSet = &pServer->ServerRun;

			break;

		case dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU:
			pDataSet = &pServer->ServerCpu;
			break;

		case dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM:
			pDataSet = &pServer->ServerMem;
			break;

		case dfMONITOR_DATA_TYPE_LOGIN_SESSION:
			pDataSet = &pServer->SessionCount;
			break;

		case dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS:
			pDataSet = &pServer->AuthTPS;
			break;

		case dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL:
			pDataSet = &pServer->PacketPoolSize;
			break;

		//==========================================================
		// GAME SERVER
		//==========================================================
		case dfMONITOR_DATA_TYPE_GAME_SERVER_RUN:
			pDataSet = &pServer->ServerRun;
			break;

		case dfMONITOR_DATA_TYPE_GAME_SERVER_CPU:
			pDataSet = &pServer->ServerCpu;
			break;

		case dfMONITOR_DATA_TYPE_GAME_SERVER_MEM:
			pDataSet = &pServer->ServerMem;
			break;

		case dfMONITOR_DATA_TYPE_GAME_SESSION:
			pDataSet = &pServer->SessionCount;
			break;

		case dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER:
			pDataSet = &pServer->AuthPlayerCount;
			break;

		case dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER:
			pDataSet = &pServer->GamePlayerCount;
			break;

		case dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS:
			pDataSet = &pServer->AcceptTPS;
			break;

		case dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS:
			pDataSet = &pServer->RecvPacketTPS;
			break;

		case dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS:
			pDataSet = &pServer->SendPacketTPS;
			break;

		case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS:
			pDataSet = &pServer->DBWriteTPS;
			break;

		case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG:
			pDataSet = &pServer->DBWriteMSG;
			break;

		case dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS:
			pDataSet = &pServer->AuthThreadTPS;
			break;

		case dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS:
			pDataSet = &pServer->GameThreadTPS;
			break;

		case dfMONITOR_DATA_TYPE_GAME_PACKET_POOL:
			pDataSet = &pServer->PacketPoolSize;
			break;

		//==========================================================
		// CHAT SERVER
		//==========================================================
		case dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN:
			pDataSet = &pServer->ServerRun;
			break;

		case dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU:
			pDataSet = &pServer->ServerCpu;
			break;

		case dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM:
			pDataSet = &pServer->ServerMem;
			break;

		case dfMONITOR_DATA_TYPE_CHAT_SESSION:
			pDataSet = &pServer->SessionCount;
			break;

		case dfMONITOR_DATA_TYPE_CHAT_PLAYER:
			pDataSet = &pServer->PlayerCount;
			break;

		case dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS:
			pDataSet = &pServer->UpdateTPS;
			break;

		case dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL:
			pDataSet = &pServer->PacketPoolSize;
			break;

		case dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL:
			pDataSet = &pServer->UpdateMsgPoolSize;
			break;

		//==========================================================
		// TOTAL SERVER Info
		//==========================================================
		case dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL:
			pDataSet = &m_CpuTotal;
			break;

		case dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY:
			pDataSet = &m_NonPagedMem;
			break;

		case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV:
			pDataSet = &m_NetworkRecvSize;
			break;

		case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND:
			pDataSet = &m_NetworkSendSize;
			break;

		case dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY:
			pDataSet = &m_AvailabeMem;
			break;

		default:
			CrashDumper::Crash();
			break;
		}

		//-------------------------------------------------------------
		// 데이터 갱신
		//-------------------------------------------------------------
		pDataSet->DataValue = DataValue;
		pDataSet->TimeStamp = TimeStamp;
		pDataSet->Total += DataValue;
		if (pDataSet->Max[1] < DataValue)
		{
			pDataSet->Max[0] = pDataSet->Max[1];
			pDataSet->Max[1] = DataValue;
		}
		if (pDataSet->Min[1] > DataValue)
		{
			pDataSet->Min[0] = pDataSet->Min[1];
			pDataSet->Min[1] = DataValue;
		}
		pDataSet->Count += 1;
		pDataSet->DataType = DataType;

		InterlockedIncrement(&m_DataUpdateTPS);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 새로운 서버 생성
	//////////////////////////////////////////////////////////////////////////////////
	MonitorServer_Internal::st_SERVER* MonitorServer_Internal::CreateServer(const SESSION_ID SessionID)
	{
		st_SERVER* pNewServer = m_ServerPool.Alloc();
		pNewServer->SessionID = SessionID;

		LockServers();
		m_Servers.insert(pair<SESSION_ID, st_SERVER*>(SessionID, pNewServer));
		UnlockServers();

		return pNewServer;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 기존 서버 찾기
	//////////////////////////////////////////////////////////////////////////////////
	MonitorServer_Internal::st_SERVER* MonitorServer_Internal::FindServer(const SESSION_ID SessionID)
	{
		unordered_map<SESSION_ID, st_SERVER*>::iterator ServerIter = m_Servers.find(SessionID);

		if (ServerIter == m_Servers.end())
		{
			return nullptr;
		}
		else
		{
			return m_Servers.find(SessionID)->second;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 기존 서버 삭제
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::DeleteServer(const SESSION_ID SessionID, st_SERVER* pServer)
	{
		LockServers();
		m_Servers.erase(m_Servers.find(SessionID));
		UnlockServers();

		m_ServerPool.Free(pServer);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 서버 관리 컨테이너 정리
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::ClearServers(void)
	{
		unordered_map<SESSION_ID, st_SERVER*>::iterator ServerIter = m_Servers.begin();
		unordered_map<SESSION_ID, st_SERVER*>::iterator ServerEnd = m_Servers.end();

		for (ServerIter; ServerIter != ServerEnd;)
		{
			ServerIter = m_Servers.erase(ServerIter);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 서버 관리 컨테이너 락걸기
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::LockServers(void)
	{
		EnterCriticalSection(&m_Servers_cs);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// 서버 관리 컨테이너 락풀기
	//////////////////////////////////////////////////////////////////////////////////
	void MonitorServer_Internal::UnlockServers(void)
	{
		LeaveCriticalSection(&m_Servers_cs);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// HeartBeat Thread
	// 
	//////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall MonitorServer_Internal::DBWroteThread(void* lpParam)
	{
		return ((MonitorServer_Internal*)lpParam)->DBWroteThread_Procedure();
	}
	int MonitorServer_Internal::DBWroteThread_Procedure(void)
	{
		DWORD StartTime;
		DWORD ElapsedTime;
		DWORD SleepTime = 60000 * 5;
		st_MONITERING_DATA_SET* pDataSet[100];

		st_SERVER* pServer;
		unordered_map<SESSION_ID, st_SERVER*>::iterator ServerIter;
		unordered_map<SESSION_ID, st_SERVER*>::iterator ServerEnd;

		while (m_bLoop)
		{
			if (WAIT_TIMEOUT != WaitForSingleObject(m_hDBWriteEvent, SleepTime))
			{
				return 0;
			}

			//--------------------------------------------------------------
			// 접속한 서버가 없다면 스킵
			//--------------------------------------------------------------
			if (m_Servers.size() == 0)
			{
				continue;
			}

			LockServers();

			ServerIter = m_Servers.begin();
			ServerEnd = m_Servers.end();

			//--------------------------------------------------------------
			// 서버별 모니터링 데이터 저장
			//--------------------------------------------------------------
			for (ServerIter; ServerIter != ServerEnd; ++ServerIter)
			{
				pServer = ServerIter->second;

				int DataSetIndex = 0;
				int ServerNo = pServer->ServerNo;
				WCHAR ServerName[32] = { 0 };

				//--------------------------------------------------------------
				// 각 서버별 공통 정보에 대한 포인터 세팅
				//--------------------------------------------------------------
				pDataSet[DataSetIndex++] = &pServer->ServerRun;
				pDataSet[DataSetIndex++] = &pServer->ServerCpu;
				pDataSet[DataSetIndex++] = &pServer->ServerMem;
				pDataSet[DataSetIndex++] = &pServer->SessionCount;
				pDataSet[DataSetIndex++] = &pServer->PacketPoolSize;

				//--------------------------------------------------------------
				// 각 서버별 고유 정보에 대한 포인터 추가 세팅
				//--------------------------------------------------------------
				switch (ServerNo)
				{
				case en_SERVER_TYPE::MASTER:

					wcscpy_s(ServerName, 32, L"'MASTER'");

					//--------------------------------------------------------------
					// MASTER 서버의 경우 공통 정보를 보내지 않으므로 무시한다.
					//--------------------------------------------------------------
					DataSetIndex = 0;
					pDataSet[DataSetIndex++] = &m_CpuTotal;
					pDataSet[DataSetIndex++] = &m_NonPagedMem;
					pDataSet[DataSetIndex++] = &m_NetworkRecvSize;
					pDataSet[DataSetIndex++] = &m_NetworkSendSize;
					pDataSet[DataSetIndex++] = &m_AvailabeMem;
					break;

				case en_SERVER_TYPE::LOGIN:
					wcscpy_s(ServerName, 32, L"'LOGIN'");

					pDataSet[DataSetIndex++] = &pServer->AuthTPS;
					break;

				case en_SERVER_TYPE::GAME:
					wcscpy_s(ServerName, 32, L"'GAME'");

					pDataSet[DataSetIndex++] = &pServer->AuthPlayerCount;
					pDataSet[DataSetIndex++] = &pServer->GamePlayerCount;
					pDataSet[DataSetIndex++] = &pServer->AcceptTPS;
					pDataSet[DataSetIndex++] = &pServer->RecvPacketTPS;
					pDataSet[DataSetIndex++] = &pServer->SendPacketTPS;
					pDataSet[DataSetIndex++] = &pServer->DBWriteTPS;
					pDataSet[DataSetIndex++] = &pServer->DBWriteMSG;
					pDataSet[DataSetIndex++] = &pServer->AuthThreadTPS;
					pDataSet[DataSetIndex++] = &pServer->GameThreadTPS;
					break;

				case en_SERVER_TYPE::CHAT:
					wcscpy_s(ServerName, 32, L"'CHAT'");

					pDataSet[DataSetIndex++] = &pServer->PlayerCount;
					pDataSet[DataSetIndex++] = &pServer->UpdateTPS;
					pDataSet[DataSetIndex++] = &pServer->UpdateMsgPoolSize;
					break;
				}

				//--------------------------------------------------------------
				// 각 서버별 데이터 개수만큼 반복문 돌면서 DB에 저장
				//--------------------------------------------------------------
				for (int i = 0; i < DataSetIndex; i++)
				{
					//--------------------------------------------------------------
					// 예외처리
					//--------------------------------------------------------------
					if (pDataSet[i]->DataType > dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY)
					{
						continue;
					}

					if (pDataSet[i]->Count < 100)
					{
						continue;
					}

					if (pDataSet[i]->Min[1] == INT_MAX)
					{
						continue;
					}

					if (pDataSet[i]->Max[1] == 0)
					{
						continue;
					}

					//--------------------------------------------------------------
					// 각 데이터 세팅
					//--------------------------------------------------------------
					int Type = pDataSet[i]->DataType;
					int Min	= pDataSet[i]->Min[1];
					int Max	= pDataSet[i]->Max[1];
					int Avr = (pDataSet[i]->Total - pDataSet[i]->Max[0] - Max) / (pDataSet[i]->Count - 2);
					time_t Time = pDataSet[i]->TimeStamp;
					tm	Today;
					localtime_s(&Today, &Time);
					WCHAR LogTime[32];
					wsprintf(LogTime, L"'%04d-%02d-%02d %02d:%02d:%02d'", Today.tm_year + 1900, Today.tm_mon + 1, Today.tm_mday, Today.tm_hour, Today.tm_min, Today.tm_sec);

					//------------------------------------------------------------------------
					// DB INSERT
					//------------------------------------------------------------------------
					StartTime = timeGetTime();
					//if (!m_pDBConnector->Query(L"INSERT INTO `logdb`.`monitorlog` (`serverno`, `logtime`, `servername`, `type`, `avr`, `min`, `max`) VALUES(%d, %s, %s, %d, %d, %d, %d)", ServerNo, LogTime, ServerName, Type, Avr, Min, Max))
					//{
					//	CrashDumper::Crash();
					//}
					ElapsedTime = timeGetTime() - StartTime;

					//------------------------------------------------------------------------
					// 해당 작업에 대한 경과 시간 및 모니터링 정보를 갱신
					//------------------------------------------------------------------------
					m_DBWriteElapsedTime += ElapsedTime;
					m_DBWriteTPS += 1;
					m_DBWriteCompletedTotal += 1;
				}
			}

			UnlockServers();
		}

		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// HeartBeat Thread
	// 
	//////////////////////////////////////////////////////////////////////////////////
	unsigned int __stdcall MonitorServer_Internal::HeartBeatThread(void* lpParam)
	{
		return ((MonitorServer_Internal*)lpParam)->HeartBeatThread_Procedure();
	}
	int MonitorServer_Internal::HeartBeatThread_Procedure(void)
	{
		DWORD NowTime;
		st_SERVER* pServer;
		unordered_map<SESSION_ID, st_SERVER*>::iterator ServerIter;
		unordered_map<SESSION_ID, st_SERVER*>::iterator ServerEnd;

		while (m_bLoop)
		{
			if (WAIT_TIMEOUT != WaitForSingleObject(m_hHeartBeatEvent, 59999))
			{
				return 0;
			}

			if (m_Servers.size() == 0)
			{
				continue;
			}

			NowTime = timeGetTime();

			LockServers();
			ServerIter = m_Servers.begin();
			ServerEnd = m_Servers.end();

			for (ServerIter; ServerIter != ServerEnd; ++ServerIter)
			{
				pServer = ServerIter->second;
				if ((NowTime - pServer->LastRecvTime) >= m_TimeOutMax)
				{
					Disconnect(pServer->SessionID);
				}
			}

			UnlockServers();
		}

		return 0;
	}
}