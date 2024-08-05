#pragma once
#pragma comment(lib, "lib/libmysql")
#include <unordered_map>
#include "Protocol.h"
#include "Monitor_Common.h"
#include "CLanServer.h"	// 1
#include "CDBConnector_TLS.h"		// 2

namespace cov1013
{
	class CMonitorServer_Net;
	class CMonitorServer_Lan : public CLanServer
	{
		friend class CMonitorServer_Net;

	public:

		//==================================================================
		// DB 저장 데이터 세트
		//==================================================================
		struct st_DB_WRITE_DATA_SET
		{
			int		DataValue;
			int		TimeStamp;
			int		ServerNo;
			BYTE    Type;
		};

		//==================================================================
		// 모니터링 데이터 세트
		//==================================================================
		struct st_MONITERING_DATA_SET
		{
			st_MONITERING_DATA_SET()
			{
				DataValue = 0;
				TimeStamp = 0;
				Total = 0;
				Min[0] = INT_MAX;
				Min[1] = INT_MAX;
				Max[0] = 0;
				Max[1] = 0;
				Count = 0;
				DataType = -1;
			}

			int		DataValue;
			int		TimeStamp;
			int		Total;
			int		Min[2];
			int		Max[2];
			int		Count;
			BYTE    DataType;
		};

		//==================================================================
		// 모니터링 서버에서 관리할 서버 구조체 (관리 단위)
		//==================================================================
		struct st_SERVER
		{
			int								ServerNo;
			SESSION_ID						SessionID;
			DWORD							LastRecvTime;

			//===============================================
			// Default Data
			//===============================================
			st_MONITERING_DATA_SET			ServerRun;
			st_MONITERING_DATA_SET			ServerCpu;
			st_MONITERING_DATA_SET			ServerMem;
			st_MONITERING_DATA_SET			SessionCount;
			st_MONITERING_DATA_SET			PacketPoolSize;

			//===============================================
			// Login Server
			//===============================================
			st_MONITERING_DATA_SET			AuthTPS;

			//===============================================
			// Game Server
			//===============================================
			st_MONITERING_DATA_SET			AuthPlayerCount;
			st_MONITERING_DATA_SET			GamePlayerCount;
			st_MONITERING_DATA_SET			AcceptTPS;
			st_MONITERING_DATA_SET			RecvPacketTPS;
			st_MONITERING_DATA_SET			SendPacketTPS;
			st_MONITERING_DATA_SET			DBWriteTPS;
			st_MONITERING_DATA_SET			DBWriteMSG;
			st_MONITERING_DATA_SET			AuthThreadTPS;
			st_MONITERING_DATA_SET			GameThreadTPS;

			//===============================================
			// Chat Server
			//===============================================
			st_MONITERING_DATA_SET			PlayerCount;
			st_MONITERING_DATA_SET			UpdateTPS;
			st_MONITERING_DATA_SET			UpdateMsgPoolSize;
		};

	public:
		CMonitorServer_Lan();
		~CMonitorServer_Lan();
		bool Run(void);
		void Quit(void);

	protected:
		virtual bool OnConnectionRequest(const WCHAR* ConnectIP, const WORD ConnectPORT);
		virtual void OnClientJoin(const SESSION_ID SessionID);
		virtual void OnClientLeave(const SESSION_ID SessionID);
		virtual void OnRecv(const SESSION_ID SessionID, CPacket* pRecvPacket);
		virtual void OnSend(const SESSION_ID SessionID, const DWORD dwTransferred);
		virtual void OnWorkerThreadBegin(void);
		virtual void OnWorkerThreadEnd(void);
		virtual void OnError(const en_ERROR_CODE eErrCode, const SESSION_ID SessionID = df_INVALID_SESSION_ID);

	private:
		void							PacketProcedure(SESSION_ID SessionID, CPacket* pRecvPacket);
		void							PacketProcedure_Login(SESSION_ID SessionID, CPacket* pRecvPacket);
		void							PacketProcedure_DataUpdate(SESSION_ID SessionID, CPacket* pRecvPacket);

		st_SERVER*						CreateServer(const SESSION_ID SessionID);
		st_SERVER*						FindServer(const SESSION_ID SessionID);
		void							DeleteServer(const SESSION_ID SessionID, st_SERVER* pServer);
		void							ClearServers(void);
		void							LockServers(void);
		void							UnlockServers(void);

		static unsigned int __stdcall	DBWroteThread(void* lpParam);
		int								DBWroteThread_Procedure(void);

		static unsigned int __stdcall	HeartBeatThread(void* lpParam);
		int								HeartBeatThread_Procedure(void);

	public:
		long										m_DataUpdateTPS;
		long										m_DBWriteTPS;
		__int64										m_DBWriteElapsedTime;
		__int64										m_DBWriteCompletedTotal;

	private:
		bool										m_bLoop;
		bool										m_bRunFlag;
		int											m_TimeOutMax;

		unordered_map<SESSION_ID, st_SERVER*>		m_Servers;
		CRITICAL_SECTION							m_Servers_cs;
		CMemoryPool<st_SERVER>						m_ServerPool = CMemoryPool<st_SERVER>(0);

		CDBConnector_TLS*							m_pDBConnector;
		CLockFreeQueue<st_DB_WRITE_DATA_SET>		m_MonitoringDataQueue;

		HANDLE										m_hMonitorThread;
		HANDLE										m_hHeartBeatThread;
		HANDLE										m_hHeartBeatEvent;
		HANDLE										m_hDBWriteThread;
		HANDLE										m_hDBWriteEvent;

		st_MONITERING_DATA_SET						m_CpuTotal;
		st_MONITERING_DATA_SET						m_NonPagedMem;
		st_MONITERING_DATA_SET						m_NetworkRecvSize;
		st_MONITERING_DATA_SET						m_NetworkSendSize;
		st_MONITERING_DATA_SET						m_AvailabeMem;
	};
};