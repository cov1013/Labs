#pragma once
class CSpriteDib
{
public:
	struct st_SPRITE
	{
		BYTE*	bypImage;			// ��������Ʈ �̹��� ������
		int		iWidth;				// Width
		int		iHeight;			// Height
		int		iPitch;				// Picth
		int		iCenterPointX;		// ���� X
		int		iCenterPointY;		// ���� Y
	};

	CSpriteDib(const int iMaxSprite, const DWORD dwColorKey);
	//----------------------------------------
	// ������ - ��ü ��������Ʈ ������ �����(Į��Ű) �Է�
	// 
	// 
	//----------------------------------------


	virtual ~CSpriteDib();
	//----------------------------------------
	// �Ҹ��� - ��ü ��������Ʈ ���� ���� �� ������Ƽ �ʱ�ȭ
	// 
	// 
	//----------------------------------------


	BOOL LoadDibSprite(const int iSpriteIndex, const WCHAR* szFileName, const int iCenterPointX, const int iCenterPointY);
	//----------------------------------------
	// Ư�� BMP ������ ���� Index ��ȣ�� ��������Ʈ �ε�
	// 
	// 
	//----------------------------------------


	void ReleaseSprite(const int iSpriteIndex);
	//----------------------------------------
	// ���� Index ��ȣ�� ��������Ʈ ����
	// 
	// 
	//----------------------------------------


	void DrawSprite(const int iSpriteIndex, int iDrawX, int iDrawY,
		const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch, const int iDrawLen = 100);
	//----------------------------------------
	// Index�� ��������Ʈ�� Ư�� �޸� ���� X, Y ��ǥ�� ���.(Į��Ű ó��)
	// 
	// 
	//----------------------------------------


	void DrawSpriteRed(const int iSpriteIndex, int iDrawX, int iDrawY,
		const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch);
	//----------------------------------------
	// DrawSprite�� ������ ���� ���� �迭�� ���
	// 
	// 
	//----------------------------------------


	void DrawImage(const int iSpriteIndex, int iDrawX, int iDrawY,
		const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch, const int iDrawLen = 100);
	//----------------------------------------
	// DrawSprite�� ������ Į��Ű ó���� ���� (�̹��� ���� ���)
	// 
	// 
	//----------------------------------------

	int GetMaxSprite(void);


protected:
	int			m_iMaxSprite;	// ��������Ʈ ��ü ����
	int			m_iCurSprite;	// ���� ��������Ʈ ����
	st_SPRITE* m_stpSprite;	// st_SPRITE ����ü ���� �Ҵ� �迭�� ������
	DWORD		m_dwColorKey;	// ��������Ʈ ó�� �� ���Ǵ� ����� (�÷�Ű)
};