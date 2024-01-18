#pragma once
class CCamera
{
public:
	CCamera(CPlayerObject** pPlayer, const DWORD dwWidth, const DWORD dwHeight);
	~CCamera();

	DWORD GetPosX(void);
	DWORD GetPosY(void);
	DWORD GetWidth(void);
	DWORD GetHeight(void);

	void Update(void);

	static CCamera* GetInstance();

private:
	static CCamera* m_pInstance;
	CPlayerObject** m_pPlayer;

	DWORD m_dwWidth;
	DWORD m_dwHeight;
	int m_iPosX;
	int m_iPosY;
};

