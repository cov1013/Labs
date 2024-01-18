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
	// 현재 시간을 구한다.
	//-------------------------------------
	m_iCurTime = timeGetTime();

	//-------------------------------------
	// 이전 프레임에서 여기까지 오는 데 걸린 시간
	// OR
	// 이전 프레임에서 초과한 진행 시간 중 남은 시간
	//-------------------------------------
	m_iDeltaTime = m_iCurTime - m_iOldTime;

	//-------------------------------------
	// 이전 프레임에서 스킵했는가?
	//-------------------------------------
	if (m_bSkipFlag)
	{
		//-------------------------------------
		// 랜더링을 스킵했다고 20ms를 번 건 아니기 때문에, 
		// 스킵한 시점부터 여기까지의 소모 시간을 m_iOneFrameTime에서 뺀 값을
		// 누적된 소모 시간에서 빼준다.
		//-------------------------------------
		m_iExcTime -= (m_iOneFrameTime - m_iDeltaTime);
		m_bSkipFlag = FALSE;

		//-------------------------------------
		// FPS 출력을 위한 시간 누적
		//-------------------------------------
		m_iAccTime += m_iDeltaTime;
	}

	//-------------------------------------
	// 소모시간이 한 프레임 진행 시간 이상인가?
	//-------------------------------------
	if (m_iDeltaTime >= m_iOneFrameTime)
	{
		m_iExcTime += m_iDeltaTime;

		//-------------------------------------
		// FPS 출력을 위한 시간 누적
		//-------------------------------------
		m_iAccTime += m_iDeltaTime;
	}

	//-------------------------------------
	// 누적시간이 한 프레임 진행 시간 초과해서 남았는가?
	//-------------------------------------
	if (m_iExcTime >= m_iOneFrameTime)
	{
		//-------------------------------------
		// 스킵한 당시의 시간 저장
		//-------------------------------------
		m_iOldTime = m_iCurTime;
		m_bSkipFlag = TRUE;

		//-------------------------------------
		// FPS 출력을 위한 카운트 갱신
		//-------------------------------------
		++g_iUpdateFPS;

#ifdef _DEBUG
		printf("Update FPS : %d Render FPS : %d ExcTime : %d\n", g_iUpdateFPS, g_iRenderFPS, m_iExcTime);
#endif

		return FALSE;
	}

	//-------------------------------------
	// 소모 시간이 한 프레임 진행 시간 이하라면
	// [한 프레임 진행 시간 - 소모시간]만큼 Sleep
	//-------------------------------------
	Sleep(m_iOneFrameTime - m_iDeltaTime);

	//-------------------------------------
	// 현재 시간과 Sleep 시간 만큼의 값을
	// 이전 시간으로 갱신
	//-------------------------------------
	m_iOldTime = m_iCurTime + (m_iOneFrameTime - m_iDeltaTime);

	//-------------------------------------
	// FPS 출력을 위한 시간 누적 및 카운트 갱신
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
