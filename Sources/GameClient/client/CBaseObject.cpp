#include <windows.h>

// COMMOM
#include "Define.h"

#include "CBaseObject.h"

CBaseObject::CBaseObject()
{
	m_bDestroyFlag = FALSE;
	m_bEndFrame = FALSE;
	m_dwActionInput = dfACTION_STAND;
	m_iCurX = 0;
	m_iCurY = 0;
	m_iDelayCount = 0;
	m_iFrameDelay = 0;
	m_iObjectID = -1;
	m_iObjectType = -1;
	m_iSpriteEnd = 0;
	m_iSpriteNow = ePLAYER_STAND_L01;
	m_iSpriteStart = 0;
}

CBaseObject::~CBaseObject()
{
}

BOOL CBaseObject::IsEndFrame(void)
{
	return m_bEndFrame;
}

BOOL CBaseObject::IsDestroy(void)
{
	return m_bDestroyFlag;
}

void CBaseObject::ActionInput(const DWORD dwAction)
{
	if (dwAction < 0 || dwAction >= dfACTION_END)
	{
		return;
	}

	m_dwActionInput = dwAction;
}

const int CBaseObject::GetCurX() const
{
	return m_iCurX;
}

const int CBaseObject::GetCurY() const
{
	return m_iCurY;
}

const int CBaseObject::GetObjectID() const
{
	return m_iObjectID;
}

const int CBaseObject::GetObjectType() const
{
	return m_iObjectType;
}

const int CBaseObject::GetSprite() const
{
	return m_iSpriteNow;
}

void CBaseObject::NextFrame(void)
{
	if (0 > m_iSpriteStart)
	{
		return;
	}

	m_iDelayCount++;

	// -----------------------------------------------------
	// 프레임 딜레이 값을 넘어야 다음 프레임으로 넘어간다.
	// -----------------------------------------------------
	if (m_iDelayCount >= m_iFrameDelay)
	{
		m_iDelayCount = 0;
		m_iSpriteNow++;

		// -----------------------------------------------------
		// 애니메이션 끝으로 이동되면 처음으로 돌린다.
		// -----------------------------------------------------
		if (m_iSpriteNow > m_iSpriteEnd)
		{
			m_iSpriteNow = m_iSpriteStart;
			m_bEndFrame = TRUE;
		}
	}
}

void CBaseObject::SetObjectID(const int iID)
{
	m_iObjectID = iID;
}

void CBaseObject::SetPosition(const int iX, const int iY)
{
	m_iCurX = iX;
	m_iCurY = iY;
}

void CBaseObject::SetAnimation(const int iStart, const int iEnd, const int iFramDelay)
{
	m_iSpriteStart = iStart;	// 스프라이트 시작 인덱스
	m_iSpriteEnd = iEnd;		// 스프라이트 종료 인덱스
	m_iSpriteNow = iStart;		// 현재 스프라이트 인덱스

	m_iFrameDelay = iFramDelay;	// 해당 스프라이트 애니메이션의 프레임 딜레이 값
	m_iDelayCount = 0;			// 해당 스프라이트 애니메이션의 진행 시간
	m_bEndFrame = FALSE;		// 해당 스프라이트가 종료됐는지 확인하는 플래그
}