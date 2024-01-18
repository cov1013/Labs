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

// CONTENTS
#include "CBaseObject.h"
#include "CPlayerObject.h"	
#include "CCamera.h"	
#include "CEffectObject.h"

//////////////////////////////////////////////////////////////////////////

extern CSpriteDib g_cSpriteDib;

//////////////////////////////////////////////////////////////////////////

CEffectObject::CEffectObject()
{
	m_iObjectType = eEFFECT;

	m_bActionFlag = FALSE;
	m_dwAttackID = dfACTION_END;
	m_pPlayer = NULL;
}

//////////////////////////////////////////////////////////////////////////

CEffectObject::CEffectObject(CPlayerObject* pPlayer)
{
	m_iObjectType = eEFFECT;
	m_bActionFlag = FALSE;
	m_dwAttackID = dfACTION_END;
	m_pPlayer = pPlayer;
}

//////////////////////////////////////////////////////////////////////////

CEffectObject::~CEffectObject()
{
}

//////////////////////////////////////////////////////////////////////////

void CEffectObject::Update(void)
{
	if (m_pPlayer->GetObjectID() == -1)
	{
		m_bDestroyFlag = TRUE;
		return;
	}

	//-----------------------------------
	// 액션 플래그가 켜져있는가?
	//-----------------------------------
	if (m_bActionFlag)
	{
		NextFrame();

		//-----------------------------------
		// 이펙트가 끝났다면 삭제 예정
		//-----------------------------------
		if (IsEndFrame())
		{
			m_bActionFlag  = FALSE;
			m_bDestroyFlag = TRUE;
			return;
		}
	}
	
	// ---------------------------------------
	// 이펙트를 터트릴 스프라이트 이미지를 등록한다.
	// ---------------------------------------
	switch (m_pPlayer->GetSprite())
	{
	case ePLAYER_ATTACK1_R02:
	case ePLAYER_ATTACK1_L02:
	case ePLAYER_ATTACK2_R02:
	case ePLAYER_ATTACK2_L02:
	case ePLAYER_ATTACK3_R04:
	case ePLAYER_ATTACK3_L04:

		// 이펙트 액션 플래그 ON
		m_bActionFlag = TRUE;

		// 이펙트 그려질 위치 계산
		m_iCurX	= m_pPlayer->GetCurX();
		m_iCurY	= m_pPlayer->GetCurY();

		switch (m_pPlayer->GetDirection())
		{
		case dfACTION_MOVE_RR:
		case dfACTION_MOVE_RD:
		case dfACTION_MOVE_RU:
			m_iCurY -= 130;
			break;
		case dfACTION_MOVE_LL:
		case dfACTION_MOVE_LD:
		case dfACTION_MOVE_LU:
			m_iCurX -= 150;
			m_iCurY -= 130;
			break;
		}

		SetAnimation(eEFFECT_SPARK_01, eEFFECT_SPARK_04, dfDELAY_EFFECT);
		break;

	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////

void CEffectObject::Render(BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch)
{
	if (m_bActionFlag)
	{
		int iX = m_iCurX - CCamera::GetInstance()->GetPosX();
		int iY = m_iCurY - CCamera::GetInstance()->GetPosY();

		g_cSpriteDib.DrawSprite(m_iSpriteNow, iX, iY, bypDest, iDestWidth, iDestHeight, iDestPitch);
	}
}

//////////////////////////////////////////////////////////////////////////