#pragma once
#include "Protocol.h"
#include "../netlib/MMOServer/CNetSession.h"

namespace cov1013
{
	class CPlayer : public CNetSession
	{
		friend class CGameServer;

	public:
		CPlayer();
		~CPlayer();

	protected:
		///////////////////////////////////////////////////////////////////
		// AUTH ������ �ű� Ŭ���̾�Ʈ ����.
		// 
		// �ش� �÷��̾� �������� ���� ���ӻ��� ������ �Ҵ� �� �غ�
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_ClientJoin(void);

		///////////////////////////////////////////////////////////////////
		// AUTH ��忡�� Ŭ���̾�Ʈ ����
		// 
		// �������������� ����ߴ� �����Ͱ� �ִٸ� ����
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_ClientLeave(void);

		///////////////////////////////////////////////////////////////////
		// AUTH ��� ������ Ŭ���̾�Ʈ ��Ŷ ����
		// 
		// �α��� ��Ŷ Ȯ�� �� DB ������ �ε�
		///////////////////////////////////////////////////////////////////
		virtual void OnAuth_Packet(CPacket* pPacket);

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
		virtual void OnGame_Packet(CPacket* pPacket);

	private:
		void netPacketProc_Login(CPacket* pRecvPacket);
		void netPacketProc_Echo(CPacket* pRecvPacket);
		void netPacketProc_HeartBeat(CPacket* pRecvPacket);

		void mp_ResLogin(CPacket* pSendPacket, BYTE Status, INT64 AccountNo);
		void mp_ResEcho(CPacket* pSendPacket, INT64 AccountNo, LONGLONG SendTick);

	private:
		DWORD m_LastRecvTime;
	};
}