#include "CMMOSession.h"

namespace cov1013
{
	///////////////////////////////////////////////////////////////////
	// 생성자
	///////////////////////////////////////////////////////////////////
	CMMOSession::CMMOSession()
	{
		m_eThreadMode = en_THREAD_MODE_NONE;
		m_IOCount = 0;

		m_bExitSocketFlag = FALSE;
		m_bSendFlag = FALSE;
		m_bDisconnectFlag = FALSE;
		m_bToGameFlag = FALSE;

		m_SendPacketCount = 0;

		m_SessionID = df_INVALID_SESSION_ID;
		m_SessionIndex = df_INVALID_SESSION_INDEX;

		m_RecvOverlapped.Type = st_OVERLAPPED::en_TYPE::en_RECV;
		m_SendOverlapped.Type = st_OVERLAPPED::en_TYPE::en_SEND;

#ifdef __MULTI_THREAD_DEBUG_MODE__
		m_LogIndex = -1;
#endif
	}

	///////////////////////////////////////////////////////////////////
	// 소멸자
	// 
	///////////////////////////////////////////////////////////////////
	CMMOSession::~CMMOSession()
	{
	}

	///////////////////////////////////////////////////////////////////
	// 연결 끊기
	// 
	///////////////////////////////////////////////////////////////////
	bool CMMOSession::Disconnect(void)
	{
		m_bDisconnectFlag = TRUE;
		CancelIoEx((HANDLE)m_Socket, NULL);

		return TRUE;
	}

	///////////////////////////////////////////////////////////////////
	// 패킷 보내기
	// 
	///////////////////////////////////////////////////////////////////
	bool CMMOSession::SendPacket(CPacket* pPacket)
	{
		pPacket->AddRef();
		m_SendBuffer.Enqueue(pPacket);

		return TRUE;
	}

	///////////////////////////////////////////////////////////////////
	// 게임 모드로 전환
	// 
	///////////////////////////////////////////////////////////////////
	void CMMOSession::SetMode_ToGame(void)
	{
		if (m_eThreadMode != en_THREAD_MODE_AUTH_IN)
		{
			return;
		}
		if (m_bToGameFlag != FALSE)
		{
			return;
		}

		m_bToGameFlag = TRUE;
	}

#ifdef __MULTI_THREAD_DEBUG_MODE__

	void CMMOSession::SetLog(const int Logic)
	{
		WORD index = InterlockedIncrement16(&m_LogIndex);
		index %= st_LOG::en_LOG_MAX;

		m_Logs[index].ID = GetCurrentThreadId();
		m_Logs[index].Logic = Logic;
		m_Logs[index].Time = timeGetTime();

		m_Logs[index].ThreadMode = m_eThreadMode;
		m_Logs[index].SendPacketCount = m_SendPacketCount;
		m_Logs[index].SendBufferSize = m_SendBuffer.GetCapacity();
		m_Logs[index].CompleleteRecvBufferSize = m_CompleteRecvPacket.GetCapacity();
		m_Logs[index].IOCount = m_IOCount;
		m_Logs[index].bExitSocketFlag = m_bExitSocketFlag;
		m_Logs[index].bSendFlag = m_bSendFlag;
		m_Logs[index].bDisconnectFlag = m_bDisconnectFlag;
		m_Logs[index].SessionID = m_SessionID;
		m_Logs[index].SessionIndex = m_SessionIndex;
		m_Logs[index].Socket = m_Socket;
	}

#endif
}