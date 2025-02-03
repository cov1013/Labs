#pragma once
//#pragma comment(lib, "DBConnector.lib")
#pragma comment(lib, "NetLib.lib")

#include <vector>
#include <time.h>
#include "Protocol.h"
#include "MMOServer.h"
#include "LanClient.h"

using namespace std;

namespace cov1013
{
	class GameServer : public MMOServer
	{
		//==================================================================
		// ����͸� ���� ������ ���� LanClient ��� Ŭ����
		//==================================================================
		class CMonitorClient : public LanClient
		{
		public:
			CMonitorClient();
			~CMonitorClient();
			void SendPacket_UpdateData(const int DataValues[], const int DataCount);

		protected:
			virtual void OnEnterJoinServer(void);					// �������� ���� ���� ��
			virtual void OnLeaveServer(void);						// �������� ������ �������� ��
			virtual void OnRecv(PacketBuffer* pRecvPacket);				// �ϳ��� ��Ŷ ���� �Ϸ� ��
			virtual void OnSend(int sendsize);						// ��Ŷ �۽� �Ϸ� ��
			virtual void OnWorkerThreadBegin(void);
			virtual void OnWorkerThreadEnd(void);
			virtual void OnError(int errorcode, wchar_t* szMsg);

		private:
			void MakePacket_Login(PacketBuffer* pSendPacket, const int ServerNo);
			void MakePacket_DataUpdate(PacketBuffer* pSendPacket, const BYTE DataType, const int DataValue, int TimeStamp);

		public:
			int		m_MonitorNo;
			bool	m_bConnectedFlag;
		};

	public:
		GameServer();
		~GameServer();
		void Run(void);
		void Quit(void);
		void Controler(void);

	private:
		static unsigned int __stdcall	MonitorThread(void* lpParam);
		int								MonitorThread_Procedure(void);

		static unsigned int __stdcall	HeartBeatThread(void* lpParam);
		int								HeartBeatThread_Procedure(void);

		static unsigned int __stdcall	MonitorServerConnectThread(void* lpParam);
		int								MonitorServerConnectThread_Procedure(void);

		void							LockPlayers(void);
		void							UnlockPlayers(void);

	public:
		int								m_TimeOutMax;

	private:
		bool							m_bLoop;
		bool							m_bRunFlag;

		ConfigLoader					m_Parser;
		CMonitorClient					m_MonitorClient;

		vector<GamePlayer*>				m_Players;			
		SRWLOCK							m_Players_srw;

		HANDLE							m_hMonitorThread;
		HANDLE							m_hHeartBeatThread;
		HANDLE							m_hHeartBeatEvent;

		HANDLE							m_hMonitorServerConnectThread;
		HANDLE							m_hMonitorServerConnectThreadEvent;
	};
}