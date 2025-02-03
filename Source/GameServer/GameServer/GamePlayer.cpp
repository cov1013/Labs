#include "GamePlayer.h"

namespace cov1013
{
	GamePlayer::GamePlayer()
	{
	}

	GamePlayer::~GamePlayer()
	{
	}

	void GamePlayer::OnAuth_ClientJoin()
	{
	}

	void GamePlayer::OnAuth_ClientLeave()
	{
	}

	void GamePlayer::OnAuth_Packet(PacketBuffer* pPacket)
	{
		WORD Type;
		*pPacket >> Type;

		switch (Type)
		{
		case en_PACKET_CS_GAME_REQ_LOGIN:
			netPacketProc_Login(pPacket);
			break;

		default:
			CrashDumper::Crash();
			break;
		}
	}

	void GamePlayer::OnGame_ClientJoin(void)
	{
	}

	void GamePlayer::OnGame_ClientLeave(void)
	{
	}

	void GamePlayer::OnGame_ClientRelease(void)
	{
	}

	void GamePlayer::OnGame_Packet(PacketBuffer* pPacket)
	{
		WORD Type;
		*pPacket >> Type;

		switch (Type)
		{
		case en_PACKET_CS_GAME_REQ_ECHO:
			netPacketProc_Echo(pPacket);
			break;

		default:
			CrashDumper::Crash();
			break;
		}
	}

	void GamePlayer::netPacketProc_Login(PacketBuffer* pRecvPacket)
	{
		INT64	AccountNo;
		char	SessionKey[64];
		int		Version;

		*pRecvPacket >> AccountNo;
		pRecvPacket->Get(SessionKey, 64);
		*pRecvPacket >> Version;

		// 로그인 결과 송신.
		PacketBuffer* pSendPacket = PacketBuffer::Alloc();
		mp_ResLogin(pSendPacket, 1, AccountNo);
		SendPacket(pSendPacket);
		pSendPacket->SubRef();

		SetMode_ToGame();
	}

	void GamePlayer::netPacketProc_Echo(PacketBuffer* pRecvPacket)
	{
		INT64		AccountNo;
		LONGLONG	SendTick;

		*pRecvPacket >> AccountNo;
		*pRecvPacket >> SendTick;

		PacketBuffer* pSendPacket = PacketBuffer::Alloc();
		mp_ResEcho(pSendPacket, AccountNo, SendTick);
		SendPacket(pSendPacket);
		pSendPacket->SubRef();
	}

	void GamePlayer::netPacketProc_HeartBeat(PacketBuffer* pRecvPacket)
	{
	}

	void GamePlayer::mp_ResLogin(PacketBuffer* pSendPacket, BYTE Status, INT64 AccountNo)
	{
		*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN;
		*pSendPacket << Status;
		*pSendPacket << AccountNo;
	}

	void GamePlayer::mp_ResEcho(PacketBuffer* pSendPacket, INT64 AccountNo, LONGLONG SendTick)
	{
		*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_ECHO;
		*pSendPacket << AccountNo;
		*pSendPacket << SendTick;
	}
}

