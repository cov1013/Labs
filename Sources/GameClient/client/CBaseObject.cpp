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
	// ������ ������ ���� �Ѿ�� ���� ���������� �Ѿ��.
	// -----------------------------------------------------
	if (m_iDelayCount >= m_iFrameDelay)
	{
		m_iDelayCount = 0;
		m_iSpriteNow++;

		// -----------------------------------------------------
		// �ִϸ��̼� ������ �̵��Ǹ� ó������ ������.
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
	m_iSpriteStart = iStart;	// ��������Ʈ ���� �ε���
	m_iSpriteEnd = iEnd;		// ��������Ʈ ���� �ε���
	m_iSpriteNow = iStart;		// ���� ��������Ʈ �ε���

	m_iFrameDelay = iFramDelay;	// �ش� ��������Ʈ �ִϸ��̼��� ������ ������ ��
	m_iDelayCount = 0;			// �ش� ��������Ʈ �ִϸ��̼��� ���� �ð�
	m_bEndFrame = FALSE;		// �ش� ��������Ʈ�� ����ƴ��� Ȯ���ϴ� �÷���
}