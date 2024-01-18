#pragma once
//#pragma comment(lib, "lib/libmysql")
//#include "CDBConnector_TLS.h"		// 2
#include <unordered_map>
#include "Protocol.h"
#include "LanServer.h"

namespace cov1013
{
	class MonitorServer;
	class MonitorServer_Internal : public LanServer
	{
		friend class MonitorServer;

	public:

	public:
		MonitorServer_Internal();
		~MonitorServer_Internal();
		bool Run(void);
		void Quit(void);

	protected:
		virtual bool OnConnectionRequest(const WCHAR* ConnectIP, const WORD ConnectPORT);
		virtual void OnClientJoin(const SESSION_ID SessionID);
		virtual void OnClientLeave(const SESSION_ID SessionID);
		virtual void OnRecv(const SESSION_ID SessionID, PacketBuffer* pRecvPacket);
		virtual void OnSend(const SESSION_ID SessionID, const DWORD dwTransferred);
		virtual void OnWorkerThreadBegin(void);
		virtual void OnWorkerThreadEnd(void);
		virtual void OnError(const en_ERROR_CODE eErrCode, const SESSION_ID SessionID = df_INVALID_SESSION_ID);

	private:
		void							PacketProcedure(SESSION_ID SessionID, PacketBuffer* pRecvPacket);
		void							PacketProcedure_Login(SESSION_ID SessionID, PacketBuffer* pRecvPacket);
		void							PacketProcedure_DataUpdate(SESSION_ID SessionID, PacketBuffer* pRecvPacket);

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
		MemoryPool<st_SERVER>						m_ServerPool = MemoryPool<st_SERVER>(0);

		/*DBConnector_TLS*							m_pDBConnector;*/
		LockFreeQueue<st_DB_WRITE_DATA_SET>		m_MonitoringDataQueue;

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