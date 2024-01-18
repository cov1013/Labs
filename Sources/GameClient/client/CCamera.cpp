#include "framework.h"
#include "resource.h"

// COMMOM
#include "CList.h"
#include "Protocol.h"
#include "Define.h"
#include "Struct.h"

// SYSTEM
#include "CLogger.h"
#include "profiler.h"

// CONTENTS
#include "CBaseObject.h"		
#include "CPlayerObject.h"		
#include "CCamera.h"

CCamera* CCamera::m_pInstance = NULL;

CCamera::CCamera(CPlayerObject** pPlayer, const DWORD dwWidth, const DWORD dwHeight)
{
	m_pPlayer = pPlayer;
	m_dwWidth = dwWidth;
	m_dwHeight = dwHeight;

	m_iPosX = 0;
	m_iPosY = 0;

	m_pInstance = this;
}

CCamera::~CCamera()
{
}

DWORD CCamera::GetPosX(void)
{
	return m_iPosX;
}

DWORD CCamera::GetPosY(void)
{
	return m_iPosY;
}

DWORD CCamera::GetWidth(void)
{
	return m_dwWidth;
}

DWORD CCamera::GetHeight(void)
{
	return m_dwHeight;
}

CCamera* CCamera::GetInstance()
{
	return m_pInstance;
}

void CCamera::Update(void)
{
	if (NULL == *m_pPlayer)
	{
		return;
	}

	int iPosX = (*m_pPlayer)->GetCurX() - (m_dwWidth / 2);
	int iPosY = (*m_pPlayer)->GetCurY() - (m_dwHeight / 2);

	if (iPosX < dfRANGE_MOVE_LEFT)
	{
		iPosX = dfRANGE_MOVE_LEFT;
	}
	if ((iPosX + m_dwWidth) >= dfRANGE_MOVE_RIGHT)
	{
		iPosX = iPosX;
	}

	if (iPosY < dfRANGE_MOVE_TOP)
	{
		iPosY = dfRANGE_MOVE_TOP;
	}
	if ((iPosY + m_dwHeight) >= dfRANGE_MOVE_BOTTOM)
	{
		iPosY = iPosY;
	}

	m_iPosX = iPosX;
	m_iPosY = iPosY;
}
