#include <windows.h>
#include <stdio.h>
#include "CFrameSkip.h"

extern DWORD g_dwSessionID;
int g_iUpdateFPS;
int g_iRenderFPS;

CFrameSkip::CFrameSkip(int iMaxFPS)
{
	m_iMaxFPS = iMaxFPS;
	m_iOneFrameTime = 1000 / iMaxFPS;

	m_iOldTime = timeGetTime();
	m_iCurTime = 0;
	m_iDeltaTime = 0;
	m_iExcTime = 0;

	m_bSkipFlag = FALSE;

	m_iAccTime = 0;
}

CFrameSkip::~CFrameSkip()
{
}

BOOL CFrameSkip::FrameSkip(void)
{
	//-------------------------------------
	// ���� �ð��� ���Ѵ�.
	//-------------------------------------
	m_iCurTime = timeGetTime();

	//-------------------------------------
	// ���� �����ӿ��� ������� ���� �� �ɸ� �ð�
	// OR
	// ���� �����ӿ��� �ʰ��� ���� �ð� �� ���� �ð�
	//-------------------------------------
	m_iDeltaTime = m_iCurTime - m_iOldTime;

	//-------------------------------------
	// ���� �����ӿ��� ��ŵ�ߴ°�?
	//-------------------------------------
	if (m_bSkipFlag)
	{
		//-------------------------------------
		// �������� ��ŵ�ߴٰ� 20ms�� �� �� �ƴϱ� ������, 
		// ��ŵ�� �������� ��������� �Ҹ� �ð��� m_iOneFrameTime���� �� ����
		// ������ �Ҹ� �ð����� ���ش�.
		//-------------------------------------
		m_iExcTime -= (m_iOneFrameTime - m_iDeltaTime);
		m_bSkipFlag = FALSE;

		//-------------------------------------
		// FPS ����� ���� �ð� ����
		//-------------------------------------
		m_iAccTime += m_iDeltaTime;
	}

	//-------------------------------------
	// �Ҹ�ð��� �� ������ ���� �ð� �̻��ΰ�?
	//-------------------------------------
	if (m_iDeltaTime >= m_iOneFrameTime)
	{
		m_iExcTime += m_iDeltaTime;

		//-------------------------------------
		// FPS ����� ���� �ð� ����
		//-------------------------------------
		m_iAccTime += m_iDeltaTime;
	}

	//-------------------------------------
	// �����ð��� �� ������ ���� �ð� �ʰ��ؼ� ���Ҵ°�?
	//-------------------------------------
	if (m_iExcTime >= m_iOneFrameTime)
	{
		//-------------------------------------
		// ��ŵ�� ����� �ð� ����
		//-------------------------------------
		m_iOldTime = m_iCurTime;
		m_bSkipFlag = TRUE;

		//-------------------------------------
		// FPS ����� ���� ī��Ʈ ����
		//-------------------------------------
		++g_iUpdateFPS;

#ifdef _DEBUG
		printf("Update FPS : %d Render FPS : %d ExcTime : %d\n", g_iUpdateFPS, g_iRenderFPS, m_iExcTime);
#endif

		return FALSE;
	}

	//-------------------------------------
	// �Ҹ� �ð��� �� ������ ���� �ð� ���϶��
	// [�� ������ ���� �ð� - �Ҹ�ð�]��ŭ Sleep
	//-------------------------------------
	Sleep(m_iOneFrameTime - m_iDeltaTime);

	//-------------------------------------
	// ���� �ð��� Sleep �ð� ��ŭ�� ����
	// ���� �ð����� ����
	//-------------------------------------
	m_iOldTime = m_iCurTime + (m_iOneFrameTime - m_iDeltaTime);

	//-------------------------------------
	// FPS ����� ���� �ð� ���� �� ī��Ʈ ����
	//-------------------------------------
	m_iAccTime += m_iOneFrameTime;
	++g_iUpdateFPS;
	++g_iRenderFPS;

	return TRUE;
}

void CFrameSkip::PrintFPS(const HWND hWnd)
{
	WCHAR szBuffer[128];

	if (m_iAccTime >= 1000)
	{
		swprintf_s(szBuffer, 128, L"SessionID: %d Update: %d Render: %d\n", g_dwSessionID, g_iUpdateFPS, g_iRenderFPS);
		SetWindowText(hWnd, szBuffer);
		memset(szBuffer, 0, 128);

		g_iUpdateFPS = 0;
		g_iRenderFPS = 0;
		m_iAccTime = 0;
	}
}
