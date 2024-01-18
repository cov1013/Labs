#include "framework.h"
#include "resource.h"

// COMMOM
#include "CList.h"
#include "Protocol.h"
#include "Define.h"
#include "Struct.h"

// SYSTEM
#include "CLogger.h"
#include "CSpriteDib.h"
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
#include "CCamera.h"	

//////////////////////////////////////////////////////////////////////////

extern CSpriteDib g_cSpriteDib;

//////////////////////////////////////////////////////////////////////////

CPlayerObject::CPlayerObject()
{
	m_bPlayerCharacter = FALSE;
	m_bEffectFlag = FALSE;
	m_iObjectType = ePLAYER;
	m_dwActionCur = dfACTION_STAND;
	m_dwActionOld = dfACTION_MOVE_LL;
	m_iDirCur = 0;
	m_iDirOld = 0;
	m_chHP = 0;
}

//////////////////////////////////////////////////////////////////////////

CPlayerObject::CPlayerObject(const BOOL bPlayerCharacter, const int iSessionID, const int iDir, const int iX, const int iY, const BYTE chHP)
{
	m_iCurX = iX;
	m_iCurY = iY;
	m_iObjectID = iSessionID;
	m_bPlayerCharacter = bPlayerCharacter;
	m_bEffectFlag = FALSE;
	m_iObjectType = ePLAYER;
	m_dwActionCur = dfACTION_STAND;
	m_dwActionOld = dfACTION_MOVE_LL;
	m_iDirCur = iDir;
	m_iDirOld = iDir;
	m_chHP = chHP;
}

//////////////////////////////////////////////////////////////////////////

CPlayerObject::~CPlayerObject()
{
}

//////////////////////////////////////////////////////////////////////////

void CPlayerObject::ActionProc(void)
{
	//----------------------------------------
	// 내 캐릭터 액션 처리
	//----------------------------------------
	if (m_bPlayerCharacter)
	{
		switch (m_dwActionCur)
		{
		//----------------------------------------
		// 현재 실행중인 액션이 공격 액션이다.
		//----------------------------------------
		case dfACTION_ATTACK1:
		case dfACTION_ATTACK2:
		case dfACTION_ATTACK3:
			if (IsEndFrame())
			{
				SetActionStand();

				//------------------------------------------------
				// 공격이 끝났더라면, 액션을 바꿔줘서 연속으로 공격할 때 재 전송이 가능하도록 한다.
				//------------------------------------------------
				m_dwActionInput = dfACTION_STAND;
				m_dwActionCur = dfACTION_STAND;
				m_dwActionOld = dfACTION_STAND;
			}
			break;

		default:
			//------------------------------------------------
			// 이외의 경우에는 사용자 입력 처리
			//------------------------------------------------
			InputActionProc();

			break;
		}

		return;
	}

	//----------------------------------------
	// 다른 플레이어 액션 처리
	//----------------------------------------
	else
	{
		switch (m_dwActionCur)
		{
			//----------------------------------------
			// 현재 실행중인 액션이 공격 액션이다.
			//----------------------------------------
		case dfACTION_ATTACK1:
		case dfACTION_ATTACK2:
		case dfACTION_ATTACK3:
			if (IsEndFrame())
			{
				SetActionStand();

				//------------------------------------------------
				// 공격이 끝났더라면, 액션을 바꿔줘서 연속으로 공격할 때 재 전송이 가능하도록 한다.
				//------------------------------------------------
				m_dwActionCur = dfACTION_STAND;
				m_dwActionOld = dfACTION_STAND;
				m_dwActionInput = dfACTION_STAND;
			}
			break;
		default:
			break;
		}

		//----------------------------------------
		// 서버에서 받은 정보는 공격을 끝마치고 움직이는 게 아니라
		// 오는 즉시 바로 처리해야하므로, 공격중이라고 해서 스킵하지 않는다.
		//----------------------------------------
		InputActionProc();
	}
}

//////////////////////////////////////////////////////////////////////////

void CPlayerObject::InputActionProc(void)
{
	st_PACKET_HEADER Header;
	CPacket			 cPacket;

	//--------------------------------------------------------------------
	// 이전 방향을 기준으로 현재 방향을 설정한다.
	// STAND, ATTACK, UU, DD에서 캐릭터의 이전 방향을 판단하여 방향에 맞는 애니메이션을 설정하기 위해 사용한다.
	// 
	// 방향을 알 수 있는 액션은 해당 함수에 대한 세팅을 무시하고 현재 프레임 방향에 맞는 애니메이션을 설정한다.
	//--------------------------------------------------------------------
	SetDirection();

	//====================================================
	// 액션 변경 부분
	//====================================================
	if (m_dwActionOld != m_dwActionInput)
	{
		switch (m_dwActionInput)
		{
		case dfACTION_STAND:
		{
			if (m_bPlayerCharacter)
			{
				mpMoveStop(&Header, &cPacket, m_iDirCur, m_iCurX, m_iCurY);
				Send_Unicast(&Header, &cPacket);

				_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] MOVE STOP  # > SessionID:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, m_iDirCur, m_iCurX, m_iCurY);
			}
			SetActionStand();
		}
		break;
		case dfACTION_MOVE_UU:
		{
			if (m_bPlayerCharacter)
			{
				if ((m_iCurY - dfSPEED_PLAYER_Y) > dfRANGE_MOVE_TOP)
				{
					mpMoveStart(&Header, &cPacket, dfPACKET_MOVE_DIR_UU, m_iCurX, m_iCurY);
					Send_Unicast(&Header, &cPacket);

					_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] MOVE START # > SessionID:%d / MoveDirection:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, dfPACKET_MOVE_DIR_UU, m_iDirCur, m_iCurX, m_iCurY);
				}
			}
			SetActionMove();
		}
		break;
		case dfACTION_MOVE_DD:
		{
			if (m_bPlayerCharacter)
			{
				if ((m_iCurY + dfSPEED_PLAYER_Y) < dfRANGE_MOVE_BOTTOM)
				{
					mpMoveStart(&Header, &cPacket, dfPACKET_MOVE_DIR_DD, m_iCurX, m_iCurY);
					Send_Unicast(&Header, &cPacket);

					_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] MOVE START # > SessionID:%d / MoveDirection:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, dfPACKET_MOVE_DIR_DD, m_iDirCur, m_iCurX, m_iCurY);
				}
			}
			SetActionMove();
		}

		break;
		case dfACTION_MOVE_LL:
		{
			m_iDirCur = dfACTION_MOVE_LL;

			if (m_bPlayerCharacter)
			{
				if ((m_iCurX - dfSPEED_PLAYER_X) > dfRANGE_MOVE_LEFT)
				{
					mpMoveStart(&Header, &cPacket, dfPACKET_MOVE_DIR_LL, m_iCurX, m_iCurY);
					Send_Unicast(&Header, &cPacket);

					_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] MOVE START # > SessionID:%d / MoveDirection:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, dfPACKET_MOVE_DIR_LL, m_iDirCur, m_iCurX, m_iCurY);
				}
			}
			SetActionMove();
		}

		break;
		case dfACTION_MOVE_RR:
		{
			m_iDirCur = dfACTION_MOVE_RR;

			if (m_bPlayerCharacter)
			{
				if ((m_iCurX + dfSPEED_PLAYER_X) < dfRANGE_MOVE_RIGHT)
				{
					mpMoveStart(&Header, &cPacket, dfPACKET_MOVE_DIR_RR, m_iCurX, m_iCurY);
					Send_Unicast(&Header, &cPacket);

					_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] MOVE START # > SessionID:%d / MoveDirection:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, dfPACKET_MOVE_DIR_RR, m_iDirCur, m_iCurX, m_iCurY);
				}
			}
			SetActionMove();
		}
		break;
		case dfACTION_MOVE_LU:
		{
			m_iDirCur = dfACTION_MOVE_LL;

			if (m_bPlayerCharacter)
			{
				if ((m_iCurX - dfSPEED_PLAYER_X) > dfRANGE_MOVE_LEFT && (m_iCurY - dfSPEED_PLAYER_Y) > dfRANGE_MOVE_TOP)
				{
					mpMoveStart(&Header, &cPacket, dfPACKET_MOVE_DIR_LU, m_iCurX, m_iCurY);
					Send_Unicast(&Header, &cPacket);

					_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] MOVE START # > SessionID:%d / MoveDirection:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, dfPACKET_MOVE_DIR_LU, m_iDirCur, m_iCurX, m_iCurY);
				}
			}
			SetActionMove();
		}
		break;
		case dfACTION_MOVE_LD:
		{
			m_iDirCur = dfACTION_MOVE_LL;

			if (m_bPlayerCharacter)
			{
				if ((m_iCurX - dfSPEED_PLAYER_X) > dfRANGE_MOVE_LEFT && (m_iCurY + dfSPEED_PLAYER_Y) < dfRANGE_MOVE_BOTTOM)
				{
					mpMoveStart(&Header, &cPacket, dfPACKET_MOVE_DIR_LD, m_iCurX, m_iCurY);
					Send_Unicast(&Header, &cPacket);

					_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] MOVE START # > SessionID:%d / MoveDirection:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, dfPACKET_MOVE_DIR_LD, m_iDirCur, m_iCurX, m_iCurY);
				}
			}
			SetActionMove();
		}
		break;
		case dfACTION_MOVE_RU:
		{
			m_iDirCur = dfACTION_MOVE_RR;

			if (m_bPlayerCharacter)
			{
				if ((m_iCurX + dfSPEED_PLAYER_X) < dfRANGE_MOVE_RIGHT && (m_iCurY - dfSPEED_PLAYER_Y) > dfRANGE_MOVE_TOP)
				{
					mpMoveStart(&Header, &cPacket, dfPACKET_MOVE_DIR_RU, m_iCurX, m_iCurY);
					Send_Unicast(&Header, &cPacket);

					_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] MOVE START # > SessionID:%d / MoveDirection:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, dfPACKET_MOVE_DIR_RU, m_iDirCur, m_iCurX, m_iCurY);
				}
			}
			SetActionMove();
		}
		break;
		case dfACTION_MOVE_RD:
		{
			m_iDirCur = dfACTION_MOVE_RR;

			if (m_bPlayerCharacter)
			{
				if ((m_iCurX + dfSPEED_PLAYER_X) < dfRANGE_MOVE_RIGHT && (m_iCurY + dfSPEED_PLAYER_Y) < dfRANGE_MOVE_BOTTOM)
				{
					mpMoveStart(&Header, &cPacket, dfPACKET_MOVE_DIR_RD, m_iCurX, m_iCurY);
					Send_Unicast(&Header, &cPacket);

					_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] MOVE START # > SessionID:%d / MoveDirection:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, dfPACKET_MOVE_DIR_RD, m_iDirCur, m_iCurX, m_iCurY);
				}
			}
			SetActionMove();
		}
		break;
		case dfACTION_ATTACK1:
		{
			if (m_bPlayerCharacter)
			{
				// 움직이다가 공격했으면 STOP SEND
				if (m_dwActionOld < dfACTION_ATTACK1)
				{
					mpMoveStop(&Header, &cPacket, m_iDirCur, m_iCurX, m_iCurY);
					Send_Unicast(&Header, &cPacket);
				}

				mpAttack1(&Header, &cPacket, m_iDirCur, m_iCurX, m_iCurY);
				Send_Unicast(&Header, &cPacket);

				_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] ATTACK1 # > SessionID:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, m_iDirCur, m_iCurX, m_iCurY);
			}

			SetActionAttack1();
		}

		break;

		case dfACTION_ATTACK2:
		{
			if (m_bPlayerCharacter)
			{
				// 움직이다가 공격했으면 STOP SEND
				if (m_dwActionOld < dfACTION_ATTACK1)
				{
					mpMoveStop(&Header, &cPacket, m_iDirCur, m_iCurX, m_iCurY);
					Send_Unicast(&Header, &cPacket);
				}

				mpAttack2(&Header, &cPacket, m_iDirCur, m_iCurX, m_iCurY);
				Send_Unicast(&Header, &cPacket);

				_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] ATTACK2 # > SessionID:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, m_iDirCur, m_iCurX, m_iCurY);
			}

			SetActionAttack2();
		}

			break;

		case dfACTION_ATTACK3:
		{
			if (m_bPlayerCharacter)
			{
				// 움직이다가 공격했으면 STOP SEND
				if (m_dwActionOld < dfACTION_ATTACK1)
				{
					mpMoveStop(&Header, &cPacket, m_iDirCur, m_iCurX, m_iCurY);
					Send_Unicast(&Header, &cPacket);
				}

				mpAttack3(&Header, &cPacket, m_iDirCur, m_iCurX, m_iCurY);
				Send_Unicast(&Header, &cPacket);

				_LOG_NCON(dfLOG_LEVEL_DEBUG, L"# [SEND] ATTACK3 # > SessionID:%d / Direction:%d / X:%d / Y:%d\n", m_iObjectID, m_iDirCur, m_iCurX, m_iCurY);
			}

			SetActionAttack3();
		}
			break;

		default:
			break;
		}

		// 이전 액션 갱신
		m_dwActionOld = m_dwActionInput;

		// 현재 진행중인 액션 갱신
		m_dwActionCur = m_dwActionInput;
	}

	//====================================================
	// 실제 이동 처리
	//====================================================
	switch (m_dwActionInput)
	{
	case dfACTION_MOVE_UU:
		if ((m_iCurY - dfSPEED_PLAYER_Y) > dfRANGE_MOVE_TOP)
		{
			m_iCurY -= dfSPEED_PLAYER_Y;
		}
		break;
	case dfACTION_MOVE_DD:
		if ((m_iCurY + dfSPEED_PLAYER_Y) < dfRANGE_MOVE_BOTTOM)
		{
			m_iCurY += dfSPEED_PLAYER_Y;
		}
		break;
	case dfACTION_MOVE_LL:
		if ((m_iCurX - dfSPEED_PLAYER_X) > dfRANGE_MOVE_LEFT)
		{
			m_iCurX -= dfSPEED_PLAYER_X;
		}
		break;
	case dfACTION_MOVE_RR:
		if ((m_iCurX + dfSPEED_PLAYER_X) < dfRANGE_MOVE_RIGHT)
		{
			m_iCurX += dfSPEED_PLAYER_X;
		}
		break;
	case dfACTION_MOVE_LU:
		if ((m_iCurX - dfSPEED_PLAYER_X) > dfRANGE_MOVE_LEFT && (m_iCurY - dfSPEED_PLAYER_Y) > dfRANGE_MOVE_TOP)
		{
			m_iCurX -= dfSPEED_PLAYER_X;
			m_iCurY -= dfSPEED_PLAYER_Y;
		}
		break;
	case dfACTION_MOVE_LD:
		if ((m_iCurX - dfSPEED_PLAYER_X) > dfRANGE_MOVE_LEFT && (m_iCurY + dfSPEED_PLAYER_Y) < dfRANGE_MOVE_BOTTOM)
		{
			m_iCurX -= dfSPEED_PLAYER_X;
			m_iCurY += dfSPEED_PLAYER_Y;
		}
		break;
	case dfACTION_MOVE_RU:
		if ((m_iCurX + dfSPEED_PLAYER_X) < dfRANGE_MOVE_RIGHT && (m_iCurY - dfSPEED_PLAYER_Y) > dfRANGE_MOVE_TOP)
		{
			m_iCurX += dfSPEED_PLAYER_X;
			m_iCurY -= dfSPEED_PLAYER_Y;
		}
		break;
	case dfACTION_MOVE_RD:
		if ((m_iCurX + dfSPEED_PLAYER_X) < dfRANGE_MOVE_RIGHT && (m_iCurY + dfSPEED_PLAYER_Y) < dfRANGE_MOVE_BOTTOM)
		{
			m_iCurX += dfSPEED_PLAYER_X;
			m_iCurY += dfSPEED_PLAYER_Y;
		}
		break;
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////

void CPlayerObject::Update(void)
{
	NextFrame();
	ActionProc();
}

//////////////////////////////////////////////////////////////////////////

void CPlayerObject::Render(BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch)
{
	int iX = m_iCurX - CCamera::GetInstance()->GetPosX();
	int iY = m_iCurY - CCamera::GetInstance()->GetPosY();

	// 그림자
	g_cSpriteDib.DrawSprite(eSHADOW, iX, iY, bypDest, iDestWidth, iDestHeight, iDestPitch);

	// 캐릭터
	if (m_bPlayerCharacter)
	{
		g_cSpriteDib.DrawSpriteRed(m_iSpriteNow, iX, iY, bypDest, iDestWidth, iDestHeight, iDestPitch);
	}
	else
	{
		g_cSpriteDib.DrawSprite(m_iSpriteNow, iX, iY, bypDest, iDestWidth, iDestHeight, iDestPitch);
	}

	// HP 게이지
	g_cSpriteDib.DrawSprite(eGUAGE_HP, iX, iY, bypDest, iDestWidth, iDestHeight, iDestPitch, m_chHP);
}

//////////////////////////////////////////////////////////////////////////

void CPlayerObject::SetDirection(void)
{
	m_iDirOld = m_iDirCur;

	switch (m_iDirOld)
	{
	case dfACTION_MOVE_LL:
	case dfACTION_MOVE_LU:
	case dfACTION_MOVE_LD:
		m_iDirCur = dfACTION_MOVE_LL;
		break;
	case dfACTION_MOVE_RR:
	case dfACTION_MOVE_RU:
	case dfACTION_MOVE_RD:
		m_iDirCur = dfACTION_MOVE_RR;
		break;
	}
}

//////////////////////////////////////////////////////////////////////////

void CPlayerObject::SetCurDirection(int iDirection)
{
	m_iDirCur = iDirection;
}

void CPlayerObject::SetActionMove(void)
{
	switch (m_iDirCur)
	{
	case dfACTION_MOVE_LL:
	case dfACTION_MOVE_LU:
	case dfACTION_MOVE_LD:
		SetAnimation(ePLAYER_MOVE_L01, ePLAYER_MOVE_L12, dfDELAY_MOVE);
		break;

	case dfACTION_MOVE_RR:
	case dfACTION_MOVE_RU:
	case dfACTION_MOVE_RD:
		SetAnimation(ePLAYER_MOVE_R01, ePLAYER_MOVE_R12, dfDELAY_MOVE);
		break;
	}
}

void CPlayerObject::SetActionStand(void)
{
	switch (m_iDirCur)
	{
	case dfACTION_MOVE_LL:
	case dfACTION_MOVE_LU:
	case dfACTION_MOVE_LD:
		SetAnimation(ePLAYER_STAND_L01, ePLAYER_STAND_L03, dfDELAY_STAND);
		break;

	case dfACTION_MOVE_RR:
	case dfACTION_MOVE_RU:
	case dfACTION_MOVE_RD:
		SetAnimation(ePLAYER_STAND_R01, ePLAYER_STAND_R03, dfDELAY_STAND);
		break;
	}
}
 
void CPlayerObject::SetActionAttack1(void)
{
	switch (m_iDirCur)
	{
	case dfACTION_MOVE_LL:
	case dfACTION_MOVE_LU:
	case dfACTION_MOVE_LD:
		SetAnimation(ePLAYER_ATTACK1_L01, ePLAYER_ATTACK1_L04, dfDELAY_ATTACK1);
		break;

	case dfACTION_MOVE_RR:
	case dfACTION_MOVE_RU:
	case dfACTION_MOVE_RD:
		SetAnimation(ePLAYER_ATTACK1_R01, ePLAYER_ATTACK1_R04, dfDELAY_ATTACK1);
		break;
	}
}

void CPlayerObject::SetActionAttack2(void)
{
	switch (m_iDirCur)
	{
	case dfACTION_MOVE_LL:
	case dfACTION_MOVE_LU:
	case dfACTION_MOVE_LD:
		SetAnimation(ePLAYER_ATTACK2_L01, ePLAYER_ATTACK2_L04, dfDELAY_ATTACK2);
		break;

	case dfACTION_MOVE_RR:
	case dfACTION_MOVE_RU:
	case dfACTION_MOVE_RD:
		SetAnimation(ePLAYER_ATTACK2_R01, ePLAYER_ATTACK2_R04, dfDELAY_ATTACK2);
		break;
	}
}

void CPlayerObject::SetActionAttack3(void)
{
	switch (m_iDirCur)
	{
	case dfACTION_MOVE_LL:
	case dfACTION_MOVE_LU:
	case dfACTION_MOVE_LD:
		SetAnimation(ePLAYER_ATTACK3_L01, ePLAYER_ATTACK3_L06, dfDELAY_ATTACK3);
		break;

	case dfACTION_MOVE_RR:
	case dfACTION_MOVE_RU:
	case dfACTION_MOVE_RD:
		SetAnimation(ePLAYER_ATTACK3_R01, ePLAYER_ATTACK3_R06, dfDELAY_ATTACK3);
		break;
	}
}

BOOL CPlayerObject::IsPlayer(void)
{
	return m_bPlayerCharacter;
}

BOOL CPlayerObject::IsEffect()
{
	return m_bEffectFlag;
}

void CPlayerObject::OnEffectFlag()
{
	m_bEffectFlag = TRUE;
}

void CPlayerObject::OffEffectFlag()
{
	m_bEffectFlag = FALSE;
}

const DWORD CPlayerObject::GetActionCur(void) const
{
	return m_dwActionCur;
}

const int CPlayerObject::GetDirection(void) const
{
	return m_iDirCur;
}

const char CPlayerObject::GetHP(void) const
{
	return m_chHP;
}

void CPlayerObject::SetHP(const BYTE chHP)
{
	m_chHP = chHP;
}