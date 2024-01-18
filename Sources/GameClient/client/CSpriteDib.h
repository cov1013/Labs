#pragma once
class CSpriteDib
{
public:
	struct st_SPRITE
	{
		BYTE*	bypImage;			// 스프라이트 이미지 포인터
		int		iWidth;				// Width
		int		iHeight;			// Height
		int		iPitch;				// Picth
		int		iCenterPointX;		// 중점 X
		int		iCenterPointY;		// 중점 Y
	};

	CSpriteDib(const int iMaxSprite, const DWORD dwColorKey);
	//----------------------------------------
	// 생성자 - 전체 스프라이트 개수와 투명색(칼라키) 입력
	// 
	// 
	//----------------------------------------


	virtual ~CSpriteDib();
	//----------------------------------------
	// 소멸자 - 전체 스프라이트 버퍼 해제 및 프로퍼티 초기화
	// 
	// 
	//----------------------------------------


	BOOL LoadDibSprite(const int iSpriteIndex, const WCHAR* szFileName, const int iCenterPointX, const int iCenterPointY);
	//----------------------------------------
	// 특정 BMP 파일을 지정 Index 번호의 스프라이트 로드
	// 
	// 
	//----------------------------------------


	void ReleaseSprite(const int iSpriteIndex);
	//----------------------------------------
	// 지정 Index 번호의 스프라이트 삭제
	// 
	// 
	//----------------------------------------


	void DrawSprite(const int iSpriteIndex, int iDrawX, int iDrawY,
		const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch, const int iDrawLen = 100);
	//----------------------------------------
	// Index의 스프아이트를 특정 메모리 버퍼 X, Y 좌표에 출력.(칼라키 처리)
	// 
	// 
	//----------------------------------------


	void DrawSpriteRed(const int iSpriteIndex, int iDrawX, int iDrawY,
		const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch);
	//----------------------------------------
	// DrawSprite와 같으나 붉은 색상 계열로 출력
	// 
	// 
	//----------------------------------------


	void DrawImage(const int iSpriteIndex, int iDrawX, int iDrawY,
		const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch, const int iDrawLen = 100);
	//----------------------------------------
	// DrawSprite와 같으나 칼라키 처리가 없음 (이미지 원본 출력)
	// 
	// 
	//----------------------------------------

	int GetMaxSprite(void);


protected:
	int			m_iMaxSprite;	// 스프라이트 전체 개수
	int			m_iCurSprite;	// 현재 스프라이트 개수
	st_SPRITE* m_stpSprite;	// st_SPRITE 구조체 동적 할당 배열용 포인터
	DWORD		m_dwColorKey;	// 스프라이트 처리 시 사용되는 투명색 (컬러키)
};