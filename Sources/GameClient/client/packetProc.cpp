#include "framework.h"
#include "resource.h"

// COMMOM
#include "Clist.h"
#include "Protocol.h"
#include "Define.h"
#include "Struct.h"

// SYSTEM
#include "CLogger.h"
#include "profiler.h"

// NETWORK
#include "CStreamQ.h"
#include "CPacket.h"
#include "makePacket.h"
#include "network.h"

// CONTENTS
#include "CBaseObject.h"		
#include "CPlayerObject.h"		
#include "CEffectObject.h"		
#include "CObjectManager.h"	

#include "packetProc.h"

//////////////////////////////////////////////////////////////////////////

extern DWORD			g_dwSessionID;
extern CObjectManager	g_cObjectManager;
extern CPlayerObject*	g_cPlayerCharacter;

//////////////////////////////////////////////////////////////////////////

void netPacketProc_CreateMyCharacter(CPacket* pPacket)
{
	DWORD dwSessionID;	
	BYTE byDirection;	
	WORD shX;			
	WORD shY;			
	BYTE byHP;			

	*pPacket >> dwSessionID;
	*pPacket >> byDirection;
	*pPacket >> shX;
	*pPacket >> shY;
	*pPacket >> byHP;

	//_LOG_CON(dfLOG_LEVEL_DEBUG, 
	//	L"# [SERVER] CREATE MY CHARACTER # SessionID:%d Direction:%d X:%d Y:%d\n", 
	//	dwSessionID, byDirection, shX, shY
	//);

	if (byDirection < dfACTION_MOVE_LL || byDirection > dfACTION_STAND)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc CreateMyChararcter error > Not Direction:%d\n\n", byDirection);
		return;
	}
	if (shX < dfRANGE_MOVE_LEFT || shX > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc CreateMyChararcter error > Over PosX:%d\n\n", shX);
		return;
	}
	if (shY < dfRANGE_MOVE_TOP || shY > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc CreateMyChararcter error > Over PosY:%d\n\n", shY);
		return;
	}

	g_dwSessionID = dwSessionID;

	CPlayerObject* pPlayer = new CPlayerObject(TRUE, dwSessionID, byDirection, shX, shY, byHP);
	g_cObjectManager.AddObject(pPlayer);
	g_cPlayerCharacter = &(*pPlayer);
}

//////////////////////////////////////////////////////////////////////////

void netPacketProc_CreateOtherCharacter(CPacket* pPacket)
{
	DWORD	dwSessionID;
	BYTE	byDirection;
	WORD	shX;
	WORD	shY;
	BYTE	byHP;

	*pPacket >> dwSessionID;
	*pPacket >> byDirection;
	*pPacket >> shX;
	*pPacket >> shY;
	*pPacket >> byHP;

	//_LOG_CON(dfLOG_LEVEL_DEBUG,
	//	L"# [SERVER] CREATE OHTER CHARACTER # SessionID:%d Direction:%d X:%d Y:%d\n",
	//	dwSessionID, byDirection, shX, shY
	//);

	if (byDirection < dfACTION_MOVE_LL || byDirection > dfACTION_STAND)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc CreateOtherChararcter error > Not Direction:%d\n\n", byDirection);
		return;
	}
	if (shX < dfRANGE_MOVE_LEFT || shX > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc CreateOtherChararcter error > Over PosX:%d\n\n", shX);
		return;
	}
	if (shY < dfRANGE_MOVE_TOP || shY > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc CreateOtherChararcter error > Over PosY:%d\n\n", shY);
		return;
	}

	CPlayerObject* pPlayer = new CPlayerObject(FALSE, dwSessionID, byDirection, shX, shY, byHP);
	g_cObjectManager.AddObject(pPlayer);

	// 만약 다른 플레이어가 걷고있다가 생성됐다면,
	// 서버에서는 별도의 MOVE_START 패킷을 보내고 있기 때문에
	// Update 부분에서 걷는 애니메이션으로 변경될 것이다.
	pPlayer->SetActionMove();
}

//////////////////////////////////////////////////////////////////////////

void netPacketProc_DeleteCharacter(CPacket* pPacket)
{
	CPlayerObject* pPlayer;
	DWORD dwSessionID;

	*pPacket >> dwSessionID;

	//_LOG_CON(dfLOG_LEVEL_DEBUG, L"# [SERVER] DELETE OHTER CHARACTER # SessionID:%d\n", dwSessionID);

	pPlayer = (CPlayerObject*)g_cObjectManager.FindObject(dwSessionID);
	if (NULL == pPlayer)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc DeleteCharacter error > SessionID:%d Not Found\n\n", dwSessionID);
		return;
	}

	g_cObjectManager.DeleteObject(dwSessionID);
}

//////////////////////////////////////////////////////////////////////////

void netPacketProc_MoveStart(CPacket* pPacket)
{
	CPlayerObject* pPlayer;

	DWORD dwSessionID;
	BYTE byMoveDirection;
	WORD shX;
	WORD shY;

	*pPacket >> dwSessionID;
	*pPacket >> byMoveDirection;
	*pPacket >> shX;
	*pPacket >> shY;

	//_LOG_CON(dfLOG_LEVEL_DEBUG, L"# [SERVER] MOVE START # > SessionID:%d / MoveDirection:%d / X:%d / Y:%d\n", dwSessionID, byMoveDirection, shX, shY);

	if (byMoveDirection < dfACTION_MOVE_LL || byMoveDirection > dfACTION_MOVE_LD)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc MoveStart error > Not Direction:%d\n\n", byMoveDirection);
		return;
	}
	if (shX < dfRANGE_MOVE_LEFT || shX > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc MoveStart error > Over PosX:%d\n\n", shX);
		return;
	}
	if (shY < dfRANGE_MOVE_TOP || shY > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc MoveStart error > Over PosY:%d\n\n", shY);
		return;
	}

	pPlayer = (CPlayerObject*)g_cObjectManager.FindObject(dwSessionID);
	if (NULL == pPlayer)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc MoveStart error > SessionID:%d Not Found\n\n", dwSessionID);
		return;
	}

	// Update 부분에서 실행될 액션 코드 설정
	pPlayer->ActionInput(byMoveDirection);
}

//////////////////////////////////////////////////////////////////////////

void netPacketProc_MoveStop(CPacket* pPacket)
{
	CPlayerObject* pPlayer;
	DWORD dwSessionID;
	BYTE byDirection;
	WORD shX;
	WORD shY;

	*pPacket >> dwSessionID;
	*pPacket >> byDirection;
	*pPacket >> shX;
	*pPacket >> shY;

	//_LOG_CON(dfLOG_LEVEL_DEBUG, L"# [SERVER] MOVE STOP # > SessionID:%d / Direction:%d / X:%d / Y:%d\n", dwSessionID, byDirection, shX, shY);

	if (byDirection < dfACTION_MOVE_LL || byDirection > dfACTION_MOVE_LD)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc MoveStop error > Not Direction:%d\n\n", byDirection);
		return;
	}
	if (shX < dfRANGE_MOVE_LEFT || shX > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc MoveStop error > Over PosX:%d\n\n", shX);
		return;
	}
	if (shY < dfRANGE_MOVE_TOP || shY > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc MoveStop error > Over PosY:%d\n\n", shY);
		return;
	}

	pPlayer = (CPlayerObject*)g_cObjectManager.FindObject(dwSessionID);
	if (NULL == pPlayer)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc MoveStop error > SessionID:%d Not Found\n\n", dwSessionID);
		return;
	}

	// Update 부분에서 실행될 액션 코드 설정
	pPlayer->ActionInput(dfACTION_STAND);
}

//////////////////////////////////////////////////////////////////////////

void netPacketProc_Attack1(CPacket* pPacket)
{
	CPlayerObject* pPlayer;

	DWORD dwSessionID;
	BYTE byDirection;
	WORD shX;
	WORD shY;

	*pPacket >> dwSessionID;
	*pPacket >> byDirection;
	*pPacket >> shX;
	*pPacket >> shY;

	//_LOG_CON(dfLOG_LEVEL_DEBUG, L"# [SERVER] ATTACK1 # > SessionID:%d / Direction:%d / X:%d / Y:%d\n", dwSessionID, byDirection, shX, shY);

	if (byDirection < dfACTION_MOVE_LL || byDirection > dfACTION_MOVE_LD)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Attack1 error > Not Direction:%d\n\n", byDirection);
		return;
	}
	if (shX < dfRANGE_MOVE_LEFT || shX > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Attack1 error > Over PosX:%d\n\n", shX);
		return;
	}
	if (shY < dfRANGE_MOVE_TOP || shY > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Attack1 error > Over PosY:%d\n\n", shY);
		return;
	}

	pPlayer = (CPlayerObject*)g_cObjectManager.FindObject(dwSessionID);
	if (NULL != pPlayer)
	{
		// Update 부분에서 실행될 액션 코드 설정
		pPlayer->ActionInput(dfACTION_ATTACK1);
	}
}

//////////////////////////////////////////////////////////////////////////

void netPacketProc_Attack2(CPacket* pPacket)
{
	CPlayerObject* pPlayer;

	DWORD dwSessionID;
	BYTE byDirection;
	WORD shX;
	WORD shY;

	*pPacket >> dwSessionID;
	*pPacket >> byDirection;
	*pPacket >> shX;
	*pPacket >> shY;

	//_LOG_CON(dfLOG_LEVEL_DEBUG, L"# [SERVER] ATTACK2 # > SessionID:%d / Direction:%d / X:%d / Y:%d\n", dwSessionID, byDirection, shX, shY);

	if (byDirection < dfACTION_MOVE_LL || byDirection > dfACTION_MOVE_LD)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Attck2 error > Not Direction:%d\n\n", byDirection);
		return;
	}
	if (shX < dfRANGE_MOVE_LEFT || shX > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Attck2 error > Over PosX:%d\n\n", shX);
		return;
	}
	if (shY < dfRANGE_MOVE_TOP || shY > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Attck2 error > Over PosY:%d\n\n", shY);
		return;
	}

	pPlayer = (CPlayerObject*)g_cObjectManager.FindObject(dwSessionID);
	if (NULL != pPlayer)
	{
		// Update 부분에서 실행될 액션 코드 설정
		pPlayer->ActionInput(dfACTION_ATTACK2);
	}
}

//////////////////////////////////////////////////////////////////////////

void netPacketProc_Attack3(CPacket* pPacket)
{
	CPlayerObject* pPlayer;

	DWORD dwSessionID;
	BYTE byDirection;
	WORD shX;
	WORD shY;

	*pPacket >> dwSessionID;
	*pPacket >> byDirection;
	*pPacket >> shX;
	*pPacket >> shY;

	//_LOG_CON(dfLOG_LEVEL_DEBUG, L"# [SERVER] ATTACK3 # > SessionID:%d / Direction:%d / X:%d / Y:%d\n", dwSessionID, byDirection, shX, shY);

	if (byDirection < dfACTION_MOVE_LL || byDirection > dfACTION_MOVE_LD)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Attack3 error > Not Direction:%d\n\n", byDirection);
		return;
	}
	if (shX < dfRANGE_MOVE_LEFT || shX > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Attack3 error > Over PosX:%d\n\n", shX);
		return;
	}
	if (shY < dfRANGE_MOVE_TOP || shY > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Attack3 error > Over PosY:%d\n\n", shY);
		return;
	}

	pPlayer = (CPlayerObject*)g_cObjectManager.FindObject(dwSessionID);
	if (NULL != pPlayer)
	{
		// Update 부분에서 실행될 액션 코드 설정
		pPlayer->ActionInput(dfACTION_ATTACK3);
	}
}

//////////////////////////////////////////////////////////////////////////

void netPacketProc_Damage(CPacket* pPacket)
{
	CPlayerObject* pAttackPlayer;
	CPlayerObject* pDamagePlayer;

	DWORD	dwAttackID;
	DWORD	dwDamageID;
	BYTE	byDamageHP;

	*pPacket >> dwAttackID;
	*pPacket >> dwDamageID;
	*pPacket >> byDamageHP;

	
	// ------------------------------
	// 때린애 찾기
	// 
	// 섹터에서 벗어난 애가 내 섹터에 있는 애를 때릴 수 있으므로,
	// 클라이언트 기준으로는 때린애를 찾지 못할수도 있다.
	// ------------------------------
	pAttackPlayer = (CPlayerObject*)g_cObjectManager.FindObject(dwAttackID);

	if (NULL != pAttackPlayer)
	{
		if (pAttackPlayer == g_cPlayerCharacter)
		{
			wprintf(L"이펙트!\n");
			_LOG_CON(dfLOG_LEVEL_DEBUG, L"# [SERVER] DEMAGE # > AttackID:%d / DemageID:%d / DemageHP:%d\n", dwAttackID, dwDamageID, byDamageHP);
		}

		// ------------------------------
		// 이펙트 객체 생성 후 등록
		// ------------------------------
		CEffectObject* cNewEffect = new CEffectObject(pAttackPlayer);
		g_cObjectManager.AddObject(cNewEffect);
		pAttackPlayer->OnEffectFlag();
	}

	// ------------------------------
	// 맞은애 찾기
	// ------------------------------
	pDamagePlayer = (CPlayerObject*)g_cObjectManager.FindObject(dwDamageID);
	if (NULL != pDamagePlayer)
	{
		pDamagePlayer->SetHP(byDamageHP);
	}
}

//////////////////////////////////////////////////////////////////////////

void netPacketProc_Sync(CPacket* pPacket)
{
	CPlayerObject* pCharacter;

	DWORD	dwSessionID;
	WORD	shX;
	WORD	shY;

	*pPacket >> dwSessionID;
	*pPacket >> shX;
	*pPacket >> shY;

	//_LOG_CON(dfLOG_LEVEL_DEBUG, L"# [SERVER] SYNC # > SessionID:%d / X:%d / Y:%d\n", dwSessionID, shX, shY);

	if (shX < dfRANGE_MOVE_LEFT || shX > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Sync error > Over PosX:%d\n\n", shX);
		return;
	}
	if (shY < dfRANGE_MOVE_TOP || shY > dfRANGE_MOVE_RIGHT)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Sync error > Over PosY:%d\n\n", shY);
		return;
	}

	pCharacter = (CPlayerObject*)g_cObjectManager.FindObject(dwSessionID);

	if (NULL == pCharacter)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"PacketProc Sync error > SessionID:%d Not Found\n\n", dwSessionID);
		return;
	}

	pCharacter->SetPosition(shX, shY);
}

//////////////////////////////////////////////////////////////////////////