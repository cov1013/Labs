#include "CPlayer.h"

namespace cov1013
{
	CPlayer::CPlayer()
	{
	}

	CPlayer::~CPlayer()
	{
	}

	void CPlayer::OnAuth_ClientJoin(void)
	{
	}

	void CPlayer::OnAuth_ClientLeave(void)
	{
	}

	void CPlayer::OnAuth_Packet(CPacket* pPacket)
	{
		WORD Type;
		*pPacket >> Type;

		switch (Type)
		{
		case en_PACKET_CS_GAME_REQ_LOGIN:
			netPacketProc_Login(pPacket);
			break;

		default:
			CCrashDump::Crash();
			break;
		}
	}

	void CPlayer::OnGame_ClientJoin(void)
	{
	}

	void CPlayer::OnGame_ClientLeave(void)
	{
	}

	void CPlayer::OnGame_ClientRelease(void)
	{
	}

	void CPlayer::OnGame_Packet(CPacket* pPacket)
	{
		WORD Type;
		*pPacket >> Type;

		switch (Type)
		{
		case en_PACKET_CS_GAME_REQ_ECHO:
			netPacketProc_Echo(pPacket);
			break;

		default:
			CCrashDump::Crash();
			break;
		}
	}

	void CPlayer::netPacketProc_Login(CPacket* pRecvPacket)
	{
		INT64	AccountNo;
		char	SessionKey[64];
		int		Version;

		*pRecvPacket >> AccountNo;
		pRecvPacket->Get(SessionKey, 64);
		*pRecvPacket >> Version;

		// 로그인 결과 송신.
		CPacket* pSendPacket = CPacket::Alloc();
		mp_ResLogin(pSendPacket, 1, AccountNo);
		SendPacket(pSendPacket);
		pSendPacket->SubRef();

		SetMode_ToGame();
	}

	void CPlayer::netPacketProc_Echo(CPacket* pRecvPacket)
	{
		INT64		AccountNo;
		LONGLONG	SendTick;

		*pRecvPacket >> AccountNo;
		*pRecvPacket >> SendTick;

		CPacket* pSendPacket = CPacket::Alloc();
		mp_ResEcho(pSendPacket, AccountNo, SendTick);
		SendPacket(pSendPacket);
		pSendPacket->SubRef();
	}

	void CPlayer::netPacketProc_HeartBeat(CPacket* pRecvPacket)
	{
	}

	void CPlayer::mp_ResLogin(CPacket* pSendPacket, BYTE Status, INT64 AccountNo)
	{
		*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN;
		*pSendPacket << Status;
		*pSendPacket << AccountNo;
	}

	void CPlayer::mp_ResEcho(CPacket* pSendPacket, INT64 AccountNo, LONGLONG SendTick)
	{
		*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_ECHO;
		*pSendPacket << AccountNo;
		*pSendPacket << SendTick;
	}
}

