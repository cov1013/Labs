#pragma once

using namespace std;

namespace cov1013
{
	class CDBConnector_TLS;
	class ChatServer : public CNetServer
	{
	public:

		enum en_CONFIG
		{
			en_ID_MAX = 20,
			en_NICK_NAME_MAX = 20,
			en_SESSION_KEY_MAX = 64,

			en_SECTOR_MAX_X = 50,
			en_SECTOR_MAX_Y = 50,
		};

		struct st_UPDATE_JOB
		{
			enum en_TYPE
			{
				en_JOB_CONNECT,
				en_JOB_DISCONNECT,
				en_JOB_RECV_MESSAGE,
				en_JOB_HEARTBEAT,
				en_JOB_REDIS_COMPELETED,
				en_JOB_EXIT
			};

			char		Type;
			SESSION_ID	SessionID;
			CPacket*	pMessage;
		};

		//==================================================================
		// 섹터 구조체
		//==================================================================
		struct st_SECTOR_POS
		{
			WORD			wX;
			WORD			wY;
		};

		//==================================================================
		// 섹터 주변 정보 구조체
		//==================================================================
		struct st_SECTOR_AROUND
		{
			int				iCount;
			st_SECTOR_POS	Around[9];
		};

		//==================================================================
		// 채팅 서버에서 관리하는 클라이언트 구조체 (관리 단위)
		//==================================================================
		struct st_CLIENT
		{
			bool			bLogin;								
			char			Status;
			DWORD			LastRecvTime;							
			st_SECTOR_POS	Sector;								
			INT64			AccountNo;							
			SESSION_ID		SessionID;							
			WCHAR			ID[en_ID_MAX];										
			WCHAR			NickName[en_NICK_NAME_MAX];			
		};

		//==================================================================
		// RedisThread Job 구조체
		//==================================================================
	/*	struct st_REDIS_JOB
		{
			SESSION_ID		SessionID;
			INT64			AccountNo;
			char			SessionKey[en_SESSION_KEY_MAX];

		};*/

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

		private :
			void MakePacket_Login(CPacket* pSendPacket, const int ServerNo);
			void MakePacket_DataUpdate(CPacket* pSendPacket, const BYTE DataType, const int DataValue, int TimeStamp);

		public:
			int	 m_MonitorNo;
			bool m_bConnectedFlag;
		};

	public:
		ChatServer();

		~ChatServer();

		void Run();

		void Quit();

		void Controler();

		int GetAuthCompletedPlayerCount();



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
		bool PacketProcedure_Login(const SESSION_ID SessionID, CPacket* pRecvPacket);
		bool PacketProcedure_SectorMove(const SESSION_ID SessionID, CPacket* pRecvPacket);
		bool PacketProcedure_ChatMessage(const SESSION_ID SessionID, CPacket* pRecvPacket);
		bool PacketProcedure_Heartbeat(const SESSION_ID SessionID);

		void MakePacket_Login(CPacket* pSendPacket, const BYTE byStatus, const INT64 AccountNo);
		void MakePacket_SectorMove(CPacket* pSendPacket, const INT64 AccountNo, const WORD shSectorX, const WORD shSectorY);
		void MakePacket_ChatMessage(CPacket* pSendPacket, const INT64 AccountNo, const WCHAR* szID, const WCHAR* szNickName, const WORD shChatMessageLen, const WCHAR* szChatMessage);

		bool JobProcedure_Connect(const SESSION_ID SessionID);
		bool JobProcedure_Disconnect(const SESSION_ID SessionID);
		bool JobProcedure_RecvMessage(const SESSION_ID SessionID, CPacket* pRecvPacket);
		bool JobProcedure_HeartBeat(void);
		bool JobProcedure_RedisCompeleted(const SESSION_ID SessionID);

		st_CLIENT* CreateClient(const SESSION_ID SessionID);
		st_CLIENT* FindClient(const SESSION_ID SessionID);
		bool DeleteClient(const SESSION_ID SessionID);
		void ClearClients(void);

		void GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND* pDestiation);
		void ClearSector(void);
		void SendPacket_Around(st_CLIENT* pClient, CPacket* pSendMessage, const bool bSendMe = false);
		void SendPacket_SectorOne(const int iX, const int iY, CPacket* pSendMessage, const SESSION_ID ExceptSessionID = df_INVALID_SESSION_ID);

		static unsigned int __stdcall	UpdateThread(void* lpParam);
		int								UpdateThread_Procedure(void);

		static unsigned int __stdcall	MonitorThread(void* lpParam);
		int								MonitorThread_Procedure(void);

		static unsigned int __stdcall	HeartBeatThread(void* lpParam);
		int								HeartBeatThread_Procedure(void);

		/*static unsigned int __stdcall	RedisThread(void* lpParam);
		int								RedisThread_Procedure(void);*/

		static unsigned int __stdcall	MonitorServerConnectThread(void* lpParam);
		int								MonitorServerConnectThread_Procedure(void);

	public:
		int										m_TimeOutMax;

		//--------------------------------------------------------------------
		// 모니터링 데이터
		//--------------------------------------------------------------------
		int										m_UpdateTPS;
		alignas(en_CACHE_ALIGN) long			m_RedisConnectorTPS;
		alignas(en_CACHE_ALIGN) long			m_AuthTPS;
		alignas(en_CACHE_ALIGN) long			m_AuthWaitCount;
		alignas(en_CACHE_ALIGN) __int64			m_AuthFailedTotal;
		alignas(en_CACHE_ALIGN) __int64			m_AuthCompletedTotal;
		alignas(en_CACHE_ALIGN) __int64			m_AuthElapsedTime;

		//--------------------------------------------------------------------
		// SendPacket 비율 계산용
		//--------------------------------------------------------------------
		int										m_RecvLoginMessage;
		int										m_RecvMoveSectorMessage;
		int										m_RecvChatMessage;
		int										m_SendLoginMessage;
		int										m_SendMoveSectorMessage;
		int										m_SendChatMessage;
		int										m_SectorPlayerCount;

		//--------------------------------------------------------------------
		// 서버 설정 정보
		//--------------------------------------------------------------------
		bool									m_bRedisFlag;
		int										m_NumberOfRedisIOCPWorkerThread;
		int										m_NumberOfRedisIOCPActiveThread;

	private:
		bool									m_bLoop;
		bool									m_bRunFlag;

		CParser									m_Parser;
		CMonitorClient							m_MonitorClient;

		unordered_map<SESSION_ID, st_CLIENT*>	m_Clients;
		list<SESSION_ID>						m_Sectors[en_SECTOR_MAX_Y][en_SECTOR_MAX_X];

		CMemoryPool<st_CLIENT>					m_ClientPool = CMemoryPool<st_CLIENT>(0);
		CMemoryPool<st_UPDATE_JOB>				m_UpdateJobPool = CMemoryPool<st_UPDATE_JOB>(0);
		//CMemoryPool<st_REDIS_JOB>				m_RedisJoFbPool = CMemoryPool<st_REDIS_JOB>(0);

		CDBConnector_TLS*						m_pDBConnector;

		CLockFreeQueue<st_UPDATE_JOB*>			m_UpdateJobQueue;
		HANDLE									m_hUpdateEvent;
		HANDLE									m_hUpdateThread;

		HANDLE									m_hRedisIOCP;
		HANDLE									m_hRedisIOCPWorkerThreads[100];

		HANDLE									m_hMonitorThread;
		HANDLE									m_hHeartBeatThread;
		HANDLE									m_hHeartBeatEvent;

		HANDLE									m_hMonitorServerConnectThread;
		HANDLE									m_hMonitorServerConnectThreadEvent;
	};
}