#pragma once
#pragma comment(lib, "Winmm")
#pragma comment(lib, "lib/libmysql")
#pragma comment(lib, "lib/cpp_redis")
#pragma comment(lib, "lib/tacopie")

#include <time.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <unordered_map>
#include "Protocol.h"
#include "../../netlib/CRedis_TLS.h"
#include "../../netlib/CDBConnector_TLS.h"
#include "../../netlib/NetServer/CNetServer.h"
#include "../../netlib/LanClient/CLanClient.h"

using namespace std;

namespace cov1013
{
	class CLoginServer : public CNetServer
	{
	public:

		#define df_INVALID_ACCOUNT_NO	-1

		//==================================================================
		// 서버 기본 구성 정보
		//==================================================================
		enum en_CONFIG
		{
			en_IP_MAX = 16,
			en_ID_MAX = 20,
			en_NICK_NAME_MAX = 20,
			en_SESSION_KEY_MAX = 64,
		};

		//==================================================================
		// 로그인 서버에서 관리하는 클라이언트 구조체 (관리 단위)
		//==================================================================
		struct st_CLIENT
		{
			st_CLIENT()
			{
				InitializeSRWLock(&SRWLock);
			}
			void Lock(void)
			{
				AcquireSRWLockExclusive(&SRWLock);
			}
			void Unlock(void)
			{
				ReleaseSRWLockExclusive(&SRWLock);
			}

			bool		bLogin;
			BYTE		Status;
			DWORD		LastRecvTime;
			SESSION_ID	SessionID;
			INT64		AccountNo;
			WCHAR		ID[en_ID_MAX];
			WCHAR		NickName[en_NICK_NAME_MAX];
			char		SessionKey[en_SESSION_KEY_MAX];

			SRWLOCK		SRWLock;
			long		AuthElapsedTime;
		};

	private:

		//==================================================================
		// 모니터링 서버 접속을 위한 LanClient 상속 클래스
		//==================================================================
		class CMonitorClient : public CLanClient
		{
		public:
			CMonitorClient();
			~CMonitorClient();
			void SendPacket_UpdateData(const int DataValues[], const int DataCount);

		protected:
			virtual void OnEnterJoinServer(void);					// 서버와의 연결 성공 후
			virtual void OnLeaveServer(void);						// 서버와의 연결이 끊어졌을 때
			virtual void OnRecv(CPacket* pRecvPacket);				// 하나의 패킷 수신 완료 후
			virtual void OnSend(int sendsize);						// 패킷 송신 완료 후
			virtual void OnWorkerThreadBegin(void);
			virtual void OnWorkerThreadEnd(void);
			virtual void OnError(int errorcode, wchar_t* szMsg);

		private:
			void MakePacket_Login(CPacket* pSendPacket, const int ServerNo);
			void MakePacket_DataUpdate(CPacket* pSendPacket, const BYTE DataType, const int DataValue, int TimeStamp);

		public:
			int	 m_MonitorNo;
			bool m_bConnectedFlag;
		};

	public:
		CLoginServer();
		~CLoginServer();
		void Run(void);
		void Quit(void);
		void Controler(void);

	protected:
		virtual bool OnConnectionRequest(const WCHAR* ConnectIP, const WORD ConnectPORT);
		virtual void OnClientJoin(const SESSION_ID SessionID);
		virtual void OnClientLeave(const SESSION_ID SessionID);
		virtual void OnRecv(const SESSION_ID SessionID, CPacket* pRecvPacket);
		virtual void OnSend(const SESSION_ID SessionID, const DWORD dwSendLength);
		virtual void OnWorkerThreadBegin(void);
		virtual void OnWorkerThreadEnd(void);
		virtual void OnError(const en_ERROR_CODE eErrCode, const SESSION_ID SessionID = df_INVALID_SESSION_ID);

	private:
		st_CLIENT*						CreateClient(const SESSION_ID SessionID);
		st_CLIENT*						FindClient(const SESSION_ID SessionID);
		void							DeleteClient(const SESSION_ID SessionID, st_CLIENT* pClient);
		void							LockClients(void);
		void							UnlockClients(void);
		void							ClearClients(void);

		void							PacketProcedure(const SESSION_ID SessionID, CPacket* pRecvPacket);
		bool							PacketProcedure_Login(const SESSION_ID SessionID, CPacket* pRecvPacket);
		void							MakePacket_ResLogin(CPacket* pSendPacket, const INT64 AccountNo, const BYTE Status, const WCHAR* ID, const WCHAR* NickName, const WCHAR* GameServerIP, const USHORT	GameServerPort, const WCHAR* ChatServerIP, const USHORT ChatServerPort);

		static unsigned int __stdcall	MonitorThread(void* lpParam);
		int								MonitorThread_Procedure(void);

		static unsigned int __stdcall	HeartBeatThread(void* lpParam);
		int								HeartBeatThread_Procedure(void);

		static unsigned int __stdcall	MonitorServerConnectThread(void* lpParam);
		int								MonitorServerConnectThread_Procedure(void);

	public:
		int										m_TimeOutMax;

		alignas(en_CACHE_ALIGN) long			m_DBConnectorTPS;
		alignas(en_CACHE_ALIGN) long			m_RedisConnectorTPS;

		alignas(en_CACHE_ALIGN) long			m_AuthTPS;
		alignas(en_CACHE_ALIGN) long			m_AuthWaitCount;
		alignas(en_CACHE_ALIGN) __int64			m_AuthFailedTotal;
		alignas(en_CACHE_ALIGN) __int64			m_AuthCompletedTotal;
		alignas(en_CACHE_ALIGN) __int64			m_AuthElapsedTime;

	private:
		bool									m_bLoop;
		bool									m_bRunFlag;

		CParser									m_Parser;
		CMonitorClient							m_MonitorClient;

		unordered_map<SESSION_ID, st_CLIENT*>	m_Clients;
		CRITICAL_SECTION						m_Clients_cs;
		CMemoryPool<st_CLIENT>					m_ClientPool = CMemoryPool<st_CLIENT>(0);

		CDBConnector_TLS*						m_pDBConnector;
		CRedis_TLS*								m_pRedisConnector;

		HANDLE									m_hMonitorThread;
		HANDLE									m_hHeartBeatThread;
		HANDLE									m_hHeartBeatEvent;

		HANDLE									m_hMonitorServerConnectThread;
		HANDLE									m_hMonitorServerConnectThreadEvent;
	};
}