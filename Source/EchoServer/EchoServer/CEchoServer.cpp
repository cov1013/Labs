
#include "CEchoServer.h"

namespace cov1013
{
	CEchoServer::CEchoServer()
	{
		m_bLoop = false;
		m_bRunFlag = false;
		m_hMoniterThread = nullptr;
	}

	CEchoServer::~CEchoServer()
	{
	}

	void CEchoServer::Run(void)
	{
		//----------------------------------------------------------
		// 중복 실행 방지
		//----------------------------------------------------------
		if (m_bRunFlag)
		{
			return;
		}
		m_bRunFlag = true;

		//----------------------------------------------------------
		// 서버 구성 불러오기
		//----------------------------------------------------------
		CParser Parser;
		if (!Parser.LoadFile(L"../Config.txt"))
		{
			wprintf(L"Server Config Open Failed");
			return;
		}

		WCHAR BIND_IP[16];
		WORD  BIND_PORT;
		DWORD IOCP_WORKER_THREAD;
		DWORD IOCP_ACTIVE_THREAD;
		DWORD CLIENT_MAX;
		bool IOCP_NAGLE_FLAG;
		bool IOCP_ZERO_COPY;
		bool SELF_SENDPOST_FLAG;
		bool KEEP_ALIVE_FLAG;

		Parser.GetValue(L"BIND_IP", BIND_IP, 16);
		Parser.GetValue(L"BIND_PORT", &BIND_PORT);
		Parser.GetValue(L"IOCP_WORKER_THREAD", &IOCP_WORKER_THREAD);
		Parser.GetValue(L"IOCP_ACTIVE_THREAD", &IOCP_ACTIVE_THREAD);
		Parser.GetValue(L"CLIENT_MAX", &CLIENT_MAX);
		Parser.GetValue(L"IOCP_NAGLE_FLAG", &IOCP_NAGLE_FLAG);
		Parser.GetValue(L"IOCP_ZERO_COPY", &IOCP_ZERO_COPY);
		Parser.GetValue(L"KEEP_ALIVE_FLAG", &KEEP_ALIVE_FLAG);
		Parser.GetValue(L"SELF_SENDPOST_FLAG", &SELF_SENDPOST_FLAG);

		//----------------------------------------------------------
		// 네트워크 라이브러리 실행
		//----------------------------------------------------------
		if (!Start(BIND_IP, BIND_PORT, IOCP_WORKER_THREAD, IOCP_ACTIVE_THREAD, IOCP_NAGLE_FLAG, IOCP_ZERO_COPY, CLIENT_MAX, SELF_SENDPOST_FLAG, KEEP_ALIVE_FLAG))
		{
			LOG(L"System", eLOG_LEVEL_SYSTEM, L"Network Library Run Failed...\n");
			return;
		}

		m_hMoniterThread = (HANDLE)_beginthreadex(nullptr, 0, this->MoniterThread, this, false, nullptr);

		m_bLoop = true;

		while (m_bLoop)
		{
			EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_GRAYED);
			DrawMenuBar(GetConsoleWindow());
			system("mode con cols=70 lines=45");
			wprintf(L"Server Open...\n");
			Controler();
		}
	}

	void CEchoServer::Quit(void)
	{
		// 이미 Stop 상태라면 무시
		if (m_bRunFlag == true)
		{
			Stop(true);
		}

		m_bLoop = false;
		WaitForSingleObject(m_hMoniterThread, INFINITE);

		//-------------------------------------------------
		// 스레드 리소스 정리
		//-------------------------------------------------
		CloseHandle(m_hMoniterThread);
	}

	void CEchoServer::Controler(void)
	{
		WCHAR ControlKey = _getwch();

		//----------------------------------------------------------
		// 서버 종료
		//----------------------------------------------------------
		if ('q' == ControlKey || 'Q' == ControlKey)
		{
			Quit();
		}

		//----------------------------------------------------------
		// 서버 정지/재시작
		//----------------------------------------------------------
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
		}

		//----------------------------------------------------------
		// 프로파일러 데이터 출력
		//----------------------------------------------------------
		if ('p' == ControlKey || 'P' == ControlKey)
		{
			OutputProfilingData();
		}

		//----------------------------------------------------------
		// 로그 레벨 변경
		//----------------------------------------------------------
		if ('1' == ControlKey)
		{
			SYSLOG_LEVEL(eLOG_LEVEL_DEBUG);
		}
		if ('2' == ControlKey)
		{
			SYSLOG_LEVEL(eLOG_LEVEL_ERROR);
		}
		if ('3' == ControlKey)
		{
			SYSLOG_LEVEL(eLOG_LEVEL_SYSTEM);
		}
		if ('0' == ControlKey)
		{
			SYSLOG_LEVEL(eLOG_LEVEL_OFF);
		}
	}

	bool CEchoServer::OnConnectionRequest(const WCHAR* ConnectIP, const WORD ConnectPORT)
	{
		return false;
	}

	void CEchoServer::OnClientJoin(const SESSION_ID SessionID)
	{
		CPacket* pPacket = CPacket::Alloc();
		__int64 Payload = 0x7fffffffffffffff;

		pPacket->Put((CHAR*)&Payload, sizeof(Payload));
		SendPacket(SessionID, pPacket);
		pPacket->SubRef();
	}

	void CEchoServer::OnRecv(const SESSION_ID SessionID, CPacket* pRecvPacket)
	{
		netPacketProc(SessionID, pRecvPacket);
	}

	void CEchoServer::OnSend(const SESSION_ID sessionID, const DWORD dwSendLength)
	{
	}

	void CEchoServer::OnClientLeave(const SESSION_ID SessionID)
	{
	}

	void CEchoServer::OnWorkerThreadBegin(void)
	{
	}

	void CEchoServer::OnWorkerThreadEnd(void)
	{
	}

	void CEchoServer::OnError(const en_ERROR_CODE eErrCode, const SESSION_ID SessionID)
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
			CCrashDump::Crash();

			break;

		default:
			break;
		}
	}

	void CEchoServer::netPacketProc(const SESSION_ID SessionID, CPacket* pRecvPacket)
	{
		WORD PacketType = en_PACKET_CS_ECHO_REQ_ECHO;	// 테스트용
		//*pRecvPacket >> PacketType;

		switch (PacketType)
		{
		case en_PACKET_CS_ECHO_REQ_ECHO:
			netPacketProc_Echo(SessionID, pRecvPacket);
			break;
		}
	}

	void CEchoServer::netPacketProc_Echo(const SESSION_ID SessionID, CPacket* pRecvPacket)
	{
		CPacket* pSendPacket = CPacket::Alloc();

		pSendPacket->Put(pRecvPacket->GetReadPos(), pRecvPacket->GetUseSize());
		SendPacket(SessionID, pSendPacket);

		pSendPacket->SubRef();
	}

	unsigned int __stdcall CEchoServer::MoniterThread(void* lpParam)
	{
		return ((CEchoServer*)lpParam)->MoniterThread_Procedure();
	}

	int CEchoServer::MoniterThread_Procedure(void)
	{
		CCpuUsage	CpuUsage;
		CPDH		PDH(L"ChatServer");

		WCHAR szFileName[32];
		SYSTEMTIME stNowTime;
		GetLocalTime(&stNowTime);
		wsprintf(szFileName, L"%04d.%02d.%02d %02d.%02d.%02d\n",
			stNowTime.wYear,
			stNowTime.wMonth,
			stNowTime.wDay,
			stNowTime.wHour,
			stNowTime.wMinute,
			stNowTime.wSecond
		);

		while (m_bLoop)
		{
#ifdef __PROFILING__
			//static bool bFlag = false;

			//if (!bFlag)
			//{
			//	DWORD dwCurTime = timeGetTime();
			//	if (dwCurTime - m_StartTime >= 3600000)
			//	{
			//		m_StartTime = timeGetTime();

			//		if (m_Clients.size() > 0 && !bFlag)
			//		{
			//			OutputProfilingData();
			//			bFlag = true;
			//		}
			//	}
			//}
#endif

			//--------------------------------------------------------
			// 모니터링 데이터 갱신
			//--------------------------------------------------------
			CpuUsage.UpdateCpuTime();
			PDH.Collect();

			//--------------------------------------------------------
			// 서버 시작 시간 출력
			//--------------------------------------------------------
			wprintf(L"StartTime : %s\n", szFileName);

			//--------------------------------------------------------
			// 로그 선택 조작 방법 출력
			//--------------------------------------------------------
			en_LOG_LEVEL eLogLevel = CLogger::GetLogLevel();
			if (eLogLevel == eLOG_LEVEL_DEBUG)
			{
				wprintf(L"LOG LEVEL : DEBUG\n");
			}
			else if (eLogLevel == eLOG_LEVEL_ERROR)
			{
				wprintf(L"LOG LEVEL : ERROR\n");
			}
			else if (eLogLevel == eLOG_LEVEL_SYSTEM)
			{
				wprintf(L"LOG LEVEL : SYSTEM\n");
			}
			else if (eLogLevel == eLOG_LEVEL_OFF)
			{
				wprintf(L"LOG LEVEL : OFF\n");
			}

			wprintf(L"1 : DEBUG | 2 : ERROR | 3 : SYSTEM | 0 : OFF\n\n");

			if (m_bRunFlag)
			{
				wprintf(L"NOW MODE : PLAY\n");
			}
			else
			{
				wprintf(L"NOW MODE : STOP\n");
			}
			wprintf(L"Q : QUIT  | S : STOP/PLAY\n\n");

			//--------------------------------------------------------
			// 로그 출력
			//--------------------------------------------------------
			CONSOLE(eLOG_LEVEL_DEBUG, L"=============================================\n");
			CONSOLE(eLOG_LEVEL_DEBUG,
				L"Worker:%d | Active:%d | Nagle:%d | ZeroCopy:%d\n",
				m_NumberOfIOCPWorkerThread,
				m_NumberOfIOCPActiveThread,
				m_bNagleFlag,
				m_bZeroCopyFlag
			);
			CONSOLE(eLOG_LEVEL_DEBUG, L"=============================================\n\n");

			CONSOLE(eLOG_LEVEL_DEBUG, L"Session               : %d\n", GetSessionCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"Index                 : %d\n", GetIndexesCount());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n\n");

			CONSOLE(eLOG_LEVEL_DEBUG, L"Accpet        TPS     : %d\n", m_AcceptTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Disconnect    TPS     : %d\n", m_DisconnectTPS);

			CONSOLE(eLOG_LEVEL_DEBUG, L"RecvPacket    TPS     : %d\n", m_RecvPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"SendPacket    TPS     : %d\n", m_SendPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"ExpSendBytes          : %I64u\n", m_ExpSendBytes);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n\n");

			CONSOLE(eLOG_LEVEL_DEBUG, L"Accpet     Total      : %I64u\n", m_AcceptTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Disconnect Total      : %I64u\n", m_DisconnectTotal);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n\n");

			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool Size       : %d\n", CPacket::GetPoolCapacity());
			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool Use        : %d\n", CPacket::GetPoolUseSize());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n\n");

			CONSOLE(eLOG_LEVEL_DEBUG, L"ProcessCommitMemory   : %I64u\n", PDH.GetProcessCommitMemory());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ProcessNonPagedMemory : %I64u\n", PDH.GetProcessNonPagedMemory());
			CONSOLE(eLOG_LEVEL_DEBUG, L"AvailableMemory       : %I64u\n", PDH.GetAvailableMemory());
			CONSOLE(eLOG_LEVEL_DEBUG, L"NonPagedMemory        : %I64u\n", PDH.GetNonPagedMemory());
			CONSOLE(eLOG_LEVEL_DEBUG, L"Page  Fault           : %.4lf\n", PDH.GetPageFaults());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n\n");

			CONSOLE(eLOG_LEVEL_DEBUG, L"TCP Retransmitted     : %I64u\n", PDH.GetTCPv4Retransmitted());
			CONSOLE(eLOG_LEVEL_DEBUG, L"Ethernet RecvBytes    : %I64u\n", PDH.GetRecvBytes());
			CONSOLE(eLOG_LEVEL_DEBUG, L"Ethernet SendBytes    : %I64u\n", PDH.GetSendBytes());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n\n");

			CONSOLE(eLOG_LEVEL_DEBUG, L"CPU usage [T:%.1f%% U:%.1f%% K:%.1f%%] [Server:%.1f%% U:%.1f%% K:%.1f%%]\n",
				CpuUsage.ProcessorTotal(), CpuUsage.ProcessorUser(), CpuUsage.ProcessorKernel(),
				CpuUsage.ProcessTotal(),   CpuUsage.ProcessUser(),   CpuUsage.ProcessKernel()
			);

#ifdef __SAFE_MODE__
			unsigned __int64 overBytes = 1024 * 1024 * 1024;
			unsigned __int64 overBytes2 = 500;
			unsigned __int64 commitMemSize = PDH.GetProcessCommitMemory();
			unsigned __int64 nonPagedPoolSize = PDH.GetNonPagedMemory();
			if (commitMemSize > overBytes || commitMemSize < 0)
			{
				LOG(L"System", eLOG_LEVEL_SYSTEM, L"Lack of User Memory (%d bytes)\n", commitMemSize);
				Stop(true);
				CCrashDump::Crash();
			}

			if (nonPagedPoolSize < overBytes2)
			{
				LOG(L"System", eLOG_LEVEL_SYSTEM, L"Lack of NonPaged Memory (%d bytes)\n", nonPagedPoolSize);
				CCrashDump::Crash();
			}
#endif

			// TPS 갱신
			m_AcceptTPS = 0;
			m_DisconnectTPS = 0;
			m_RecvPacketTPS = 0;
			m_SendPacketTPS = 0;
			m_ExpSendBytes = 0;
			//m_SendCompeltedTPS = 0;

			Sleep(999);
		}
		return 0;
	}

}

