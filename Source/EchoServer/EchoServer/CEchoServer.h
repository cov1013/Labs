#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "Protocol.h"
#include "CLanServer.h"

namespace cov1013
{
	class CEchoServer : public CLanServer
	{
	public:
		CEchoServer();
		~CEchoServer();
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
		void							netPacketProc(const SESSION_ID SessionID, CPacket* pRecvPacket);
		void							netPacketProc_Echo(const SESSION_ID SessionID, CPacket* pRecvPacket);

		static unsigned int __stdcall	MoniterThread(void* lpParam);
		int								MoniterThread_Procedure(void);

	private:
		bool	m_bLoop;
		bool	m_bRunFlag;

		HANDLE	m_hMoniterThread;
	};
}