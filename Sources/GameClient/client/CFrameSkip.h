#pragma once
class CFrameSkip
{

public:
	CFrameSkip(int iMaxFPS);
	~CFrameSkip();

	BOOL FrameSkip(void);
	void PrintFPS(const HWND hWnd);

private:
	BOOL	m_bSkipFlag;		
	int		m_iMaxFPS;			
	int		m_iOneFrameTime;	
	int		m_iOldTime;			
	int		m_iCurTime;			
	int		m_iDeltaTime;		
	int		m_iExcTime;			
	int		m_iAccTime;			
};