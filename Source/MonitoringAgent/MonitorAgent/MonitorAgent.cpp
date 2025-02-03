#include "CMonitorAgent.h"

namespace cov1013
{
	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	CMonitorAgent::CMonitorAgent()
	{
		m_bLoop = false;
		m_bRunFlag = false;
		m_bConnectedFlag = false;
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	CMonitorAgent::~CMonitorAgent()
	{
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::Run(void)
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
		// 에이전트 설정 정보 불러오기 & 세팅
		//----------------------------------------------------------------------
		CParser Parser;
		if (!Parser.LoadFile(L"../Config.txt"))
		{
			wprintf(L"Server Config Open Failed");
			m_bRunFlag = false;
			return;
		}

		WCHAR	BIND_IP[16];
		WCHAR	SERVER_IP[16];
		WORD	SERVER_PORT;
		DWORD	IOCP_WORKER_THREAD;
		DWORD	IOCP_ACTIVE_THREAD;
		bool	NAGLE_FLAG;
		bool	ZERO_COPY_FLAG;

		//----------------------------------------------------------
		// 모니터링 서버 연결
		//----------------------------------------------------------
		Parser.GetValue(L"MONITOR_NO", &m_MonitorNo);
		Parser.GetValue(L"BIND_IP", BIND_IP, 16);
		Parser.GetValue(L"SERVER_IP", SERVER_IP, 16);
		Parser.GetValue(L"SERVER_PORT", &SERVER_PORT);
		Parser.GetValue(L"IOCP_WORKER_THREAD", &IOCP_WORKER_THREAD);
		Parser.GetValue(L"NAGLE_FLAG", &NAGLE_FLAG);
		Parser.GetValue(L"ZERO_COPY_FLAG", &ZERO_COPY_FLAG);
		wprintf(L"Connecting Moniter Server...\n");

		if (!Connect(BIND_IP, SERVER_IP, SERVER_PORT, IOCP_WORKER_THREAD, NAGLE_FLAG, ZERO_COPY_FLAG))
		{
			wprintf(L"Moniter Server Connect Failed!\n");
			m_bRunFlag = false;
			return;
		}

		//----------------------------------------------------------
		// 콘솔창 초기화
		//----------------------------------------------------------
		EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_GRAYED);
		DrawMenuBar(GetConsoleWindow());
		system("mode con cols=70 lines=28");
		wprintf(L"Agent Run...\n");
		m_bLoop = true;

		//----------------------------------------------------------
		// 스레드 생성
		//----------------------------------------------------------
		m_hMnitorThread = (HANDLE)_beginthreadex(nullptr, 0, MonitorThread, this, false, nullptr);

		//----------------------------------------------------------
		// 컨트롤러 실행
		//----------------------------------------------------------
		while (m_bLoop)
		{
			Controler();
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::Quit(void)
	{
		Stop();

		m_bLoop = false;

		WaitForSingleObject(m_hMnitorThread, INFINITE);
		CloseHandle(m_hMnitorThread);
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::Controler(void)
	{
		WCHAR ControlKey = _getwch();

		//----------------------------------------------------------
		// 에이전트 종료
		//----------------------------------------------------------
		if ('q' == ControlKey || 'Q' == ControlKey)
		{
			Quit();
			return;
		}

		//----------------------------------------------------------
		// 프로파일링 데이터 출력
		//----------------------------------------------------------
		if ('p' == ControlKey || 'P' == ControlKey)
		{
			OutputProfilingData();
			return;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::SendPacket_UpdateData(const int DataValues[], const int DataCount)
	{
		if (!m_bConnectedFlag)
		{
			return;
		}

		CPacket* pPacket;
		time_t NowTime;

		_time64(&NowTime);

		for (int i = 0; i < DataCount; i++)
		{
			pPacket = CPacket::Alloc();
			MakePacket_DataUpdate(pPacket, dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL + i, DataValues[i], (int)NowTime);
			SendPacket(pPacket);
			pPacket->SubRef();
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::OnEnterJoinServer(void)
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

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::OnLeaveServer(void)
	{
		m_bConnectedFlag = false;

		//-------------------------------------------------------
		// 모니터링 서버와 연결이 끊어졌다면 재연결
		//-------------------------------------------------------
		Reconnect();
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::OnRecv(CPacket* pRecvPacket)
	{
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::OnSend(int sendsize)
	{
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::OnWorkerThreadBegin(void)
	{
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::OnWorkerThreadEnd(void)
	{
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::OnError(int errorcode, wchar_t* szMsg)
	{
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::MakePacket_Login(CPacket* pSendPacket, const int ServerNo)
	{
		*pSendPacket << (WORD)en_PACKET_SS_MONITOR_LOGIN;
		*pSendPacket << ServerNo;
	}

	///////////////////////////////////////////////////////////////////////////
	// 
	///////////////////////////////////////////////////////////////////////////
	void CMonitorAgent::MakePacket_DataUpdate(CPacket* pSendPacket, const BYTE DataType, const int DataValue, int TimeStamp)
	{
		*pSendPacket << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
		*pSendPacket << DataType;
		*pSendPacket << DataValue;
		*pSendPacket << TimeStamp;
	}

	unsigned int __stdcall CMonitorAgent::MonitorThread(void* lpParam)
	{
		return ((CMonitorAgent*)lpParam)->MonitorThread_Procedure();
	}

	int CMonitorAgent::MonitorThread_Procedure(void)
	{
		CCpuUsage	CpuUsage;
		CPDH		PDH(L"MonitorServer Agent");
		WCHAR		szFileName[32];
		SYSTEMTIME	stNowTime;
		GetLocalTime(&stNowTime);
		wsprintf(szFileName, L"%04d.%02d.%02d %02d.%02d.%02d\n",
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
			wprintf(L"[MONITOR AGENT] StartTime : %s\n", szFileName);

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

			float TotalCpuUsage = CpuUsage.ProcessorTotal();
			LONGLONG TotalNonPagedMem = PDH.GetNonPagedMemory() / (1024 * 1024);
			LONGLONG TotalAvailableMem = PDH.GetAvailableMemory() / (1024 * 1024);
			LONGLONG EthernetRecvSize = PDH.GetRecvBytes() / (1024);
			LONGLONG EthernetSendSize = PDH.GetSendBytes() / (1024);

			CONSOLE(eLOG_LEVEL_DEBUG, L"==============================================================\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"[NETWORK LIBRARY] : Worker:%d | Nagle:%d | ZeroCopy:%d\n",
				m_NumberOfIOCPWorkerThread, m_bNagleFlag, m_bZeroCopyFlag
			);
			if (m_bConnectedFlag)
			{
				CONSOLE(eLOG_LEVEL_DEBUG, L"[MONITOR SERVER]  : Connected\n");
			}
			else
			{
				CONSOLE(eLOG_LEVEL_DEBUG, L"[MONITOR SERVER]  : Reconecting...\n");
			}
			CONSOLE(eLOG_LEVEL_DEBUG, L"==============================================================\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"SendPacket       TPS    : %d\n", m_SendPacketTPS);
			CONSOLE(eLOG_LEVEL_DEBUG, L"ExpSendBytes            : %I64u\n", m_ExpSendBytes);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool       Size   : %d\n", CPacket::GetPoolCapacity());
			CONSOLE(eLOG_LEVEL_DEBUG, L"PacketPool       Use    : %d\n", CPacket::GetPoolUseSize());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Total CPU Usage         : %.1f%%\n", TotalCpuUsage);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Total NonPagedMemory    : %I64u MB\n", TotalNonPagedMem);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Total AvailableMemory   : %I64u MB\n", TotalAvailableMem);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Ethernet Recv Size      : %I64u KB\n", EthernetRecvSize);
			CONSOLE(eLOG_LEVEL_DEBUG, L"Ethernet Send Size      : %I64u KB\n", EthernetSendSize);
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"ProcessCommitMemory     : %I64u\n", PDH.GetProcessCommitMemory());
			CONSOLE(eLOG_LEVEL_DEBUG, L"ProcessNonPagedMemory   : %I64u\n", PDH.GetProcessNonPagedMemory());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"Page  Fault             : %.4lf\n", PDH.GetPageFaults());
			CONSOLE(eLOG_LEVEL_DEBUG, L"TCP Retransmitted       : %I64u\n", PDH.GetTCPv4Retransmitted());
			CONSOLE(eLOG_LEVEL_DEBUG, L"---------------------------------------------\n");
			CONSOLE(eLOG_LEVEL_DEBUG, L"CPU Usage [T:%.1f%% U:%.1f%% K:%.1f%%]\n", CpuUsage.ProcessTotal(), CpuUsage.ProcessUser(), CpuUsage.ProcessKernel());

			int	DataValues[30] = { 0 };
			int	DataValuesIndex = 0;

			//-------------------------------------------------------
			// 모니터링 서버로 데이터 전송
			//-------------------------------------------------------
			DataValues[DataValuesIndex++] = TotalCpuUsage;
			DataValues[DataValuesIndex++] = TotalNonPagedMem;
			DataValues[DataValuesIndex++] = EthernetRecvSize;
			DataValues[DataValuesIndex++] = EthernetSendSize;
			DataValues[DataValuesIndex++] = TotalAvailableMem;
			SendPacket_UpdateData(DataValues, DataValuesIndex);

			m_SendPacketTPS = 0;
			m_ExpSendBytes = 0;
		}

		return 0;
	}
};