#pragma once
#pragma once
#pragma comment(lib, "Winmm")

#include <time.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <unordered_map>
#include "Protocol.h"
#include "CLanClient.h"

namespace cov1013
{
	class CMonitorAgent : public CLanClient
	{
	public:
		CMonitorAgent();
		~CMonitorAgent();
		void Run(void);
		void Quit(void);
		void Controler(void);
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

		static unsigned int __stdcall	MonitorThread(void* lpParam);
		int								MonitorThread_Procedure(void);

	public:
		int		m_MonitorNo;

		bool	m_bLoop;
		bool	m_bRunFlag;
		bool	m_bConnectedFlag;

		HANDLE	m_hMnitorThread;
	};
}