#include <windows.h>
#include <stdio.h>
#include "CSpriteDib.h"

CSpriteDib::CSpriteDib(const int iMaxSprite, const DWORD dwColorKey)
{
	m_iMaxSprite = iMaxSprite;
	m_dwColorKey = dwColorKey;

	// ---------------------------------------------
	// 최대 읽어올 개수만큼 미리 할당 받는다.
	// ---------------------------------------------
	m_stpSprite = new st_SPRITE[iMaxSprite];
	memset(m_stpSprite, 0, sizeof(st_SPRITE) * iMaxSprite);
}

CSpriteDib::~CSpriteDib()
{
	//----------------------------------------------------------
	// 스프라이트 메모리 포인터 해제
	//----------------------------------------------------------
	int iCount;
	for (iCount = 0; iCount < m_iMaxSprite; iCount++)
	{
		ReleaseSprite(iCount);
	}

	//----------------------------------------------------------
	// 스프라이트 구조체 배열 포인터 해제
	//----------------------------------------------------------
	delete[] m_stpSprite;
}

BOOL CSpriteDib::LoadDibSprite(const int iSpriteIndex, const WCHAR* szFileName, const int iCenterPointX, const int iCenterPointY)
{
	FILE* pFile;
	int iPitch;
	int iImageSize;
	int iCount;
	BITMAPFILEHEADER stFileHeader;
	BITMAPINFOHEADER stInfoHeader;

	// ---------------------------------------------
	// 일단 파일을 열자
	// ---------------------------------------------
	_wfopen_s(&pFile, szFileName, L"rb");
	if (pFile == NULL)
	{
		return FALSE;
	}

	ReleaseSprite(iSpriteIndex);
	fread(&stFileHeader, sizeof(BITMAPFILEHEADER), 1, pFile);

	if (0x4d42 == stFileHeader.bfType)
	{
		//----------------------------------------------------------
		// 인포헤더를 읽어서 저장 & 32비트 확인
		//----------------------------------------------------------
		fread(&stInfoHeader, sizeof(BITMAPINFOHEADER), 1, pFile);
		if (32 == stInfoHeader.biBitCount)
		{
			//----------------------------------------------------------
			// 한 줄, 한 줄의 피치값을 구한다.
			//----------------------------------------------------------
			iPitch = ((stInfoHeader.biWidth * 4) + 3) & ~3;

			//----------------------------------------------------------
			// 스프라이트 구조체에 크기 저장
			//----------------------------------------------------------
			m_stpSprite[iSpriteIndex].iWidth = stInfoHeader.biWidth;
			m_stpSprite[iSpriteIndex].iHeight = stInfoHeader.biHeight;
			m_stpSprite[iSpriteIndex].iPitch = iPitch;

			//----------------------------------------------------------
			// 이미지에 대한 전체 크기를 구하고, 메모리 할당.
			//----------------------------------------------------------
			iImageSize = iPitch * stInfoHeader.biHeight;
			m_stpSprite[iSpriteIndex].bypImage = new BYTE[iImageSize];

			//----------------------------------------------------------
			// 이미지 부분은 스프라이트 버퍼로 읽어온다.
			// DIB는 뒤집어져 있으므로 이를 다시 뒤집자.
			// 임시 버퍼에 읽은 뒤에 뒤집으면서 복사한다.
			//----------------------------------------------------------
			BYTE* bypTempBuffer = new BYTE[iImageSize];
			BYTE* bypSpriteImage = m_stpSprite[iSpriteIndex].bypImage;
			BYTE* bypTurnTemp;

			fread(bypTempBuffer, iImageSize, 1, pFile);

			//----------------------------------------------------------
			// 한 줄, 한 줄 뒤집자.
			// 
			// 읽어온 스프라이트 메모리의 마지막 시작 포인터를 기준으로 뒤집으면서 메모리 복사.
			//----------------------------------------------------------
			bypTurnTemp = bypTempBuffer + (iPitch * (stInfoHeader.biHeight - 1));

			for (iCount = 0; iCount < stInfoHeader.biHeight; iCount++)
			{
				memcpy_s(bypSpriteImage, iPitch, bypTurnTemp, iPitch);

				bypSpriteImage = bypSpriteImage + iPitch;
				bypTurnTemp = bypTurnTemp - iPitch;
			}

			delete[] bypTempBuffer;

			m_stpSprite[iSpriteIndex].iCenterPointX = iCenterPointX;
			m_stpSprite[iSpriteIndex].iCenterPointY = iCenterPointY;
			fclose(pFile);

			++m_iCurSprite;
			return TRUE;
		}
	}

	fclose(pFile);
	return FALSE;
}

void CSpriteDib::ReleaseSprite(const int iSpriteIndex)
{
	//----------------------------------------------------------
	// 인덱스 기준이므로, 스프라이트 최대 사이즈와 같거나 크면 리턴
	//----------------------------------------------------------
	if (iSpriteIndex >= m_iMaxSprite)
	{
		return;
	}

	if (NULL != m_stpSprite[iSpriteIndex].bypImage)
	{
		//----------------------------------------------------------
		// 삭제 후 초기화
		//----------------------------------------------------------
		delete[] m_stpSprite[iSpriteIndex].bypImage;
		memset(&m_stpSprite[iSpriteIndex], 0, sizeof(st_SPRITE));
	}
}

void CSpriteDib::DrawSprite(const int iSpriteIndex, int iDrawX, int iDrawY, const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch, const int iDrawLen)
{
	//----------------------------------------------------------
	// 최대 스프라이트 개수를 초과하거나, 로드되지 않는 스프라이트라면 무시
	//----------------------------------------------------------
	if (iSpriteIndex >= m_iMaxSprite)
	{
		return;
	}

	if (NULL == m_stpSprite[iSpriteIndex].bypImage)
	{
		return;
	}

	st_SPRITE* stpSprite = &m_stpSprite[iSpriteIndex];

	//----------------------------------------------------------
	// 스프라이트 출력을 위해 사이즈 저장
	//----------------------------------------------------------
	int iSpriteWidth = stpSprite->iWidth;
	int iSpriteHeight = stpSprite->iHeight;
	int iCountX, iCountY;

	//----------------------------------------------------------
	// 출력 길이 설정
	//----------------------------------------------------------
	iSpriteWidth = iSpriteWidth * iDrawLen / 100;

	DWORD* dwpDest = (DWORD*)bypDest;
	DWORD* dwpSprite = (DWORD*)stpSprite->bypImage;

	//----------------------------------------------------------
	// 출력 중점 처리.
	//----------------------------------------------------------
	iDrawX = iDrawX - stpSprite->iCenterPointX;
	iDrawY = iDrawY - stpSprite->iCenterPointY;

	//----------------------------------------------------------
	// 상단에 대한 스프라이트 출력 위치 계산. (상단 클리핑)
	//----------------------------------------------------------
	if (0 > iDrawY)
	{
		iSpriteHeight = iSpriteHeight - abs(iDrawY);
		dwpSprite = (DWORD*)(stpSprite->bypImage + stpSprite->iPitch * abs(iDrawY));

		//----------------------------------------------------------
		// 윗쪽이 짤리는 경우이므로 스프라이트 시작 위치를 아래로 내려준다.
		//----------------------------------------------------------
		iDrawY = 0;
	}

	//----------------------------------------------------------
	// 하단에 대한 스프라이트 출력 위치 계산. (하단 클리핑)
	//----------------------------------------------------------
	if (iDestHeight < iDrawY + stpSprite->iHeight)
	{
		iSpriteHeight = iSpriteHeight - ((iDrawY + stpSprite->iHeight) - iDestHeight);
	}

	//----------------------------------------------------------
	// 왼쪽 출력 위치 계산 (좌측 클리핑)
	//----------------------------------------------------------
	if (0 > iDrawX)
	{
		iSpriteWidth -= abs(iDrawX);
		dwpSprite += abs(iDrawX);

		iDrawX = 0;
	}

	//----------------------------------------------------------
	// 오른쪽 출력 위치 계산 (우측 클리핑)
	//----------------------------------------------------------
	if (iDestWidth < iDrawX + stpSprite->iWidth)
	{
		iSpriteWidth = iSpriteWidth - ((iDrawX + stpSprite->iWidth) - iDestWidth);
	}

	//----------------------------------------------------------
	// 찍을 그림이 없다면 종료한다.
	//----------------------------------------------------------
	if (iSpriteWidth == 0 && iSpriteHeight == 0)
	{
		return;
	}

	//----------------------------------------------------------
	// 화면에 찍을 화면으로 이동한다.
	//----------------------------------------------------------
	dwpDest = (DWORD*)((BYTE*)(dwpDest + iDrawX) + (iDrawY * iDestPitch));

	//----------------------------------------------------------
	// 전체 크기를 돌면서 투명색 처리를 한다.
	//----------------------------------------------------------
	BYTE* bypDestOrigin = (BYTE*)dwpDest;
	BYTE* bypSpriteOrigin = (BYTE*)dwpSprite;

	for (iCountY = 0; iCountY < iSpriteHeight; iCountY++)
	{
		for (iCountX = 0; iCountX < iSpriteWidth; iCountX++)
		{
			if (m_dwColorKey != (*dwpSprite & 0x00ffffff))
			{
				*dwpDest = *dwpSprite;
			}

			++dwpDest;
			++dwpSprite;
		}

		// 개행
		bypDestOrigin += iDestPitch;
		bypSpriteOrigin += stpSprite->iPitch;

		dwpDest = (DWORD*)bypDestOrigin;
		dwpSprite = (DWORD*)bypSpriteOrigin;
	}
}

void CSpriteDib::DrawSpriteRed(const int iSpriteIndex, int iDrawX, int iDrawY, const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch)
{
	//----------------------------------------------------------
	// 최대 스프라이트 개수를 초과하거나, 로드되지 않는 스프라이트라면 무시
	//----------------------------------------------------------
	if (iSpriteIndex >= m_iMaxSprite)
	{
		return;
	}

	if (NULL == m_stpSprite[iSpriteIndex].bypImage)
	{
		return;
	}

	st_SPRITE* stpSprite = &m_stpSprite[iSpriteIndex];

	//----------------------------------------------------------
	// 스프라이트 출력을 위해 사이즈 저장
	//----------------------------------------------------------
	int iSpriteWidth = stpSprite->iWidth;
	int iSpriteHeight = stpSprite->iHeight;
	int iCountX, iCountY;

	//----------------------------------------------------------
	// 출력 길이 설정
	//----------------------------------------------------------
	DWORD* dwpDest = (DWORD*)bypDest;
	DWORD* dwpSprite = (DWORD*)stpSprite->bypImage;

	//----------------------------------------------------------
	// 출력 중점 처리.
	//----------------------------------------------------------
	iDrawX = iDrawX - stpSprite->iCenterPointX;
	iDrawY = iDrawY - stpSprite->iCenterPointY;

	//----------------------------------------------------------
	// 상단에 대한 스프라이트 출력 위치 계산. (상단 클리핑)
	//----------------------------------------------------------
	if (0 > iDrawY)
	{
		iSpriteHeight = iSpriteHeight - abs(iDrawY);
		dwpSprite = (DWORD*)(stpSprite->bypImage + stpSprite->iPitch * abs(iDrawY));
		//----------------------------------------------------------
		// 윗쪽이 짤리는 경우이므로 스프라이트 시작 위치를 아래로 내려준다.
		//----------------------------------------------------------

		iDrawY = 0;
	}

	//----------------------------------------------------------
	// 하단에 대한 스프라이트 출력 위치 계산. (하단 클리핑)
	//----------------------------------------------------------
	if (iDestHeight < iDrawY + stpSprite->iHeight)
	{
		iSpriteHeight = iSpriteHeight - ((iDrawY + stpSprite->iHeight) - iDestHeight);
	}

	//----------------------------------------------------------
	// 왼쪽 출력 위치 계산 (좌측 클리핑)
	//----------------------------------------------------------
	if (0 > iDrawX)
	{
		iSpriteWidth -= abs(iDrawX);
		dwpSprite += abs(iDrawX);

		iDrawX = 0;
	}

	//----------------------------------------------------------
	// 오른쪽 출력 위치 계산 (우측 클리핑)
	//----------------------------------------------------------
	if (iDestWidth < iDrawX + stpSprite->iWidth)
	{
		iSpriteWidth = iSpriteWidth - ((iDrawX + stpSprite->iWidth) - iDestWidth);
	}

	//----------------------------------------------------------
	// 찍을 그림이 없다면 종료한다.
	//----------------------------------------------------------
	if (iSpriteWidth == 0 || iSpriteHeight == 0)
	{
		return;
	}

	//----------------------------------------------------------
	// 화면에 찍을 화면으로 이동한다.
	//----------------------------------------------------------
	dwpDest = (DWORD*)((BYTE*)(dwpDest + iDrawX) + (iDrawY * iDestPitch));


	BYTE* bypDestOrigin = (BYTE*)dwpDest;
	BYTE* bypSpriteOrigin = (BYTE*)dwpSprite;
	//----------------------------------------------------------
	// 전체 크기를 돌면서 투명색 처리를 한다.
	//----------------------------------------------------------
	for (iCountY = 0; iCountY < iSpriteHeight; iCountY++)
	{
		for (iCountX = 0; iCountX < iSpriteWidth; iCountX++)
		{
			if (m_dwColorKey != (*dwpSprite & 0x00ffffff))
			{
				*dwpDest = (*dwpSprite & 0x00ff7070);
			}

			++dwpDest;
			++dwpSprite;
		}

		// 개행
		bypDestOrigin += iDestPitch;
		bypSpriteOrigin += stpSprite->iPitch;

		dwpDest = (DWORD*)bypDestOrigin;
		dwpSprite = (DWORD*)bypSpriteOrigin;
	}
}

void CSpriteDib::DrawImage(const int iSpriteIndex, int iDrawX, int iDrawY, const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch, const int iDrawLen)
{
	//----------------------------------------------------------
	// 최대 스프라이트 개수를 초과하거나, 로드되지 않는 스프라이트라면 무시
	//----------------------------------------------------------
	if (iSpriteIndex >= m_iMaxSprite)
	{
		return;
	}

	if (NULL == m_stpSprite[iSpriteIndex].bypImage)
	{
		return;
	}

	st_SPRITE* stpSprite = &m_stpSprite[iSpriteIndex];

	//----------------------------------------------------------
	// 스프라이트 출력을 위해 사이즈 저장
	//----------------------------------------------------------
	int iSpriteWidth = stpSprite->iWidth;
	int iSpriteHeight = stpSprite->iHeight;
	int iCountY;
	iSpriteWidth = iSpriteWidth * iDrawLen / 100;

	DWORD* dwpDest = (DWORD*)bypDest;
	DWORD* dwpSprite = (DWORD*)stpSprite->bypImage;

	//----------------------------------------------------------
	// 상단에 대한 스프라이트 출력 위치 계산. (상단 클리핑)
	//----------------------------------------------------------
	if (0 > iDrawY)
	{
		iSpriteHeight = iSpriteHeight - abs(iDrawY);
		dwpSprite = (DWORD*)(stpSprite->bypImage + stpSprite->iPitch * abs(iDrawY));

		//----------------------------------------------------------
		// 윗쪽이 짤리는 경우이므로 스프라이트 시작 위치를 아래로 내려준다.
		//----------------------------------------------------------
		iDrawY = 0;
	}

	//----------------------------------------------------------
	// 하단에 대한 스프라이트 출력 위치 계산. (하단 클리핑)
	//----------------------------------------------------------
	if (iDestHeight < iDrawY + stpSprite->iHeight)
	{
		iSpriteHeight = iSpriteHeight - ((iDrawY + stpSprite->iHeight) - iDestHeight);
	}

	//----------------------------------------------------------
	// 왼쪽 출력 위치 계산 (좌측 클리핑)
	//----------------------------------------------------------
	if (0 > iDrawX)
	{
		iSpriteWidth -= abs(iDrawX);
		dwpSprite += abs(iDrawX);

		iDrawX = 0;
	}

	//----------------------------------------------------------
	// 오른쪽 출력 위치 계산 (우측 클리핑)
	//----------------------------------------------------------
	if (iDestWidth < iDrawX + stpSprite->iWidth)
	{
		iSpriteWidth = iSpriteWidth - ((iDrawX + stpSprite->iWidth) - iDestWidth);
	}

	//----------------------------------------------------------
	// 찍을 그림이 없다면 종료한다.
	//----------------------------------------------------------
	if (iSpriteWidth == 0 && iSpriteHeight == 0)
	{
		return;
	}

	//----------------------------------------------------------
	// 화면에 찍을 화면으로 이동한다.
	//----------------------------------------------------------
	dwpDest = (DWORD*)((BYTE*)(dwpDest + iDrawX) + (iDrawY * iDestPitch));


	//----------------------------------------------------------
	// 전체 크기를 돌면서 찍는다.
	//----------------------------------------------------------
	for (iCountY = 0; iCountY < iSpriteHeight; iCountY++)
	{
		memcpy_s(dwpDest, iDestPitch, dwpSprite, iSpriteWidth * 4);

		dwpDest = (DWORD*)((BYTE*)dwpDest + iDestPitch);
		dwpSprite = (DWORD*)((BYTE*)dwpSprite + stpSprite->iPitch);
	}
}

int CSpriteDib::GetMaxSprite(void)
{
	return m_iMaxSprite;
}
