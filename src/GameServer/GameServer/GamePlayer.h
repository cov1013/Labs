#pragma once
#include "Protocol.h"
#include "MMOSession.h"
#include "PacketDispatcher.h"

namespace cov1013
{
	class GamePlayer : public MMOSession
	{
		friend class GameServer;

	public:
		GamePlayer();
		~GamePlayer();

	protected:
		///////////////////////////////////////////////////////////////////
		// AUTH ������ �ű� Ŭ���̾�Ʈ ����.
		// 
		// �ش� �÷��̾� �������� ���� ���ӻ��� ������ �Ҵ� �� �غ�
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_ClientJoin() override;

		///////////////////////////////////////////////////////////////////
		// AUTH ��忡�� Ŭ���̾�Ʈ ����
		// 
		// �������������� ����ߴ� �����Ͱ� �ִٸ� ����
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_ClientLeave() override;

		///////////////////////////////////////////////////////////////////
		// AUTH ��� ������ Ŭ���̾�Ʈ ��Ŷ ����
		// 
		// �α��� ��Ŷ Ȯ�� �� DB ������ �ε�
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_Packet(PacketBuffer* pPacket);

		///////////////////////////////////////////////////////////////////
		// GAME ������ �ű� Ŭ���̾�Ʈ ����
		// 
		// ���������� ������ ���� ������ �غ� �� ����
		// (���� �ʿ� ĳ���� �ڱ�, ����Ʈ �غ� ��)
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_ClientJoin(void);

		///////////////////////////////////////////////////////////////////
		// GAME ��忡�� Ŭ���̾�Ʈ ����
		// 
		// ���� ���������� �÷��̾� ����
		// (����ʿ��� ����, ��Ƽ���� ��)
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_ClientLeave(void);

		///////////////////////////////////////////////////////////////////
		// GAME ��忡�� Ŭ���̾�Ʈ ���� ����
		// 
		// �ش� Ŭ���̾�Ʈ�� ������ ���� ����, ����, ���� ��
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_ClientRelease(void);

		///////////////////////////////////////////////////////////////////
		// GAME ��� ������ Ŭ���̾�Ʈ ��Ŷ ����
		// 
		// ���� ���� ��Ŷ ó��
		///////////////////////////////////////////////////////////////////
		virtual void OnGame_Packet(PacketBuffer* pPacket);

	private:
		void netPacketProc_Login(PacketBuffer* pRecvPacket);
		void netPacketProc_Echo(PacketBuffer* pRecvPacket);
		void netPacketProc_HeartBeat(PacketBuffer* pRecvPacket);

		void mp_ResLogin(PacketBuffer* pSendPacket, BYTE Status, INT64 AccountNo);
		void mp_ResEcho(PacketBuffer* pSendPacket, INT64 AccountNo, LONGLONG SendTick);

	private:
		DWORD m_LastRecvTime = 0;
	};
}