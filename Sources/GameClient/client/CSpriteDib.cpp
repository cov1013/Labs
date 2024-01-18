#include <windows.h>
#include <stdio.h>
#include "CSpriteDib.h"

CSpriteDib::CSpriteDib(const int iMaxSprite, const DWORD dwColorKey)
{
	m_iMaxSprite = iMaxSprite;
	m_dwColorKey = dwColorKey;

	// ---------------------------------------------
	// �ִ� �о�� ������ŭ �̸� �Ҵ� �޴´�.
	// ---------------------------------------------
	m_stpSprite = new st_SPRITE[iMaxSprite];
	memset(m_stpSprite, 0, sizeof(st_SPRITE) * iMaxSprite);
}

CSpriteDib::~CSpriteDib()
{
	//----------------------------------------------------------
	// ��������Ʈ �޸� ������ ����
	//----------------------------------------------------------
	int iCount;
	for (iCount = 0; iCount < m_iMaxSprite; iCount++)
	{
		ReleaseSprite(iCount);
	}

	//----------------------------------------------------------
	// ��������Ʈ ����ü �迭 ������ ����
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
	// �ϴ� ������ ����
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
		// ��������� �о ���� & 32��Ʈ Ȯ��
		//----------------------------------------------------------
		fread(&stInfoHeader, sizeof(BITMAPINFOHEADER), 1, pFile);
		if (32 == stInfoHeader.biBitCount)
		{
			//----------------------------------------------------------
			// �� ��, �� ���� ��ġ���� ���Ѵ�.
			//----------------------------------------------------------
			iPitch = ((stInfoHeader.biWidth * 4) + 3) & ~3;

			//----------------------------------------------------------
			// ��������Ʈ ����ü�� ũ�� ����
			//----------------------------------------------------------
			m_stpSprite[iSpriteIndex].iWidth = stInfoHeader.biWidth;
			m_stpSprite[iSpriteIndex].iHeight = stInfoHeader.biHeight;
			m_stpSprite[iSpriteIndex].iPitch = iPitch;

			//----------------------------------------------------------
			// �̹����� ���� ��ü ũ�⸦ ���ϰ�, �޸� �Ҵ�.
			//----------------------------------------------------------
			iImageSize = iPitch * stInfoHeader.biHeight;
			m_stpSprite[iSpriteIndex].bypImage = new BYTE[iImageSize];

			//----------------------------------------------------------
			// �̹��� �κ��� ��������Ʈ ���۷� �о�´�.
			// DIB�� �������� �����Ƿ� �̸� �ٽ� ������.
			// �ӽ� ���ۿ� ���� �ڿ� �������鼭 �����Ѵ�.
			//----------------------------------------------------------
			BYTE* bypTempBuffer = new BYTE[iImageSize];
			BYTE* bypSpriteImage = m_stpSprite[iSpriteIndex].bypImage;
			BYTE* bypTurnTemp;

			fread(bypTempBuffer, iImageSize, 1, pFile);

			//----------------------------------------------------------
			// �� ��, �� �� ������.
			// 
			// �о�� ��������Ʈ �޸��� ������ ���� �����͸� �������� �������鼭 �޸� ����.
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
	// �ε��� �����̹Ƿ�, ��������Ʈ �ִ� ������� ���ų� ũ�� ����
	//----------------------------------------------------------
	if (iSpriteIndex >= m_iMaxSprite)
	{
		return;
	}

	if (NULL != m_stpSprite[iSpriteIndex].bypImage)
	{
		//----------------------------------------------------------
		// ���� �� �ʱ�ȭ
		//----------------------------------------------------------
		delete[] m_stpSprite[iSpriteIndex].bypImage;
		memset(&m_stpSprite[iSpriteIndex], 0, sizeof(st_SPRITE));
	}
}

void CSpriteDib::DrawSprite(const int iSpriteIndex, int iDrawX, int iDrawY, const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch, const int iDrawLen)
{
	//----------------------------------------------------------
	// �ִ� ��������Ʈ ������ �ʰ��ϰų�, �ε���� �ʴ� ��������Ʈ��� ����
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
	// ��������Ʈ ����� ���� ������ ����
	//----------------------------------------------------------
	int iSpriteWidth = stpSprite->iWidth;
	int iSpriteHeight = stpSprite->iHeight;
	int iCountX, iCountY;

	//----------------------------------------------------------
	// ��� ���� ����
	//----------------------------------------------------------
	iSpriteWidth = iSpriteWidth * iDrawLen / 100;

	DWORD* dwpDest = (DWORD*)bypDest;
	DWORD* dwpSprite = (DWORD*)stpSprite->bypImage;

	//----------------------------------------------------------
	// ��� ���� ó��.
	//----------------------------------------------------------
	iDrawX = iDrawX - stpSprite->iCenterPointX;
	iDrawY = iDrawY - stpSprite->iCenterPointY;

	//----------------------------------------------------------
	// ��ܿ� ���� ��������Ʈ ��� ��ġ ���. (��� Ŭ����)
	//----------------------------------------------------------
	if (0 > iDrawY)
	{
		iSpriteHeight = iSpriteHeight - abs(iDrawY);
		dwpSprite = (DWORD*)(stpSprite->bypImage + stpSprite->iPitch * abs(iDrawY));

		//----------------------------------------------------------
		// ������ ©���� ����̹Ƿ� ��������Ʈ ���� ��ġ�� �Ʒ��� �����ش�.
		//----------------------------------------------------------
		iDrawY = 0;
	}

	//----------------------------------------------------------
	// �ϴܿ� ���� ��������Ʈ ��� ��ġ ���. (�ϴ� Ŭ����)
	//----------------------------------------------------------
	if (iDestHeight < iDrawY + stpSprite->iHeight)
	{
		iSpriteHeight = iSpriteHeight - ((iDrawY + stpSprite->iHeight) - iDestHeight);
	}

	//----------------------------------------------------------
	// ���� ��� ��ġ ��� (���� Ŭ����)
	//----------------------------------------------------------
	if (0 > iDrawX)
	{
		iSpriteWidth -= abs(iDrawX);
		dwpSprite += abs(iDrawX);

		iDrawX = 0;
	}

	//----------------------------------------------------------
	// ������ ��� ��ġ ��� (���� Ŭ����)
	//----------------------------------------------------------
	if (iDestWidth < iDrawX + stpSprite->iWidth)
	{
		iSpriteWidth = iSpriteWidth - ((iDrawX + stpSprite->iWidth) - iDestWidth);
	}

	//----------------------------------------------------------
	// ���� �׸��� ���ٸ� �����Ѵ�.
	//----------------------------------------------------------
	if (iSpriteWidth == 0 && iSpriteHeight == 0)
	{
		return;
	}

	//----------------------------------------------------------
	// ȭ�鿡 ���� ȭ������ �̵��Ѵ�.
	//----------------------------------------------------------
	dwpDest = (DWORD*)((BYTE*)(dwpDest + iDrawX) + (iDrawY * iDestPitch));

	//----------------------------------------------------------
	// ��ü ũ�⸦ ���鼭 ����� ó���� �Ѵ�.
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

		// ����
		bypDestOrigin += iDestPitch;
		bypSpriteOrigin += stpSprite->iPitch;

		dwpDest = (DWORD*)bypDestOrigin;
		dwpSprite = (DWORD*)bypSpriteOrigin;
	}
}

void CSpriteDib::DrawSpriteRed(const int iSpriteIndex, int iDrawX, int iDrawY, const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch)
{
	//----------------------------------------------------------
	// �ִ� ��������Ʈ ������ �ʰ��ϰų�, �ε���� �ʴ� ��������Ʈ��� ����
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
	// ��������Ʈ ����� ���� ������ ����
	//----------------------------------------------------------
	int iSpriteWidth = stpSprite->iWidth;
	int iSpriteHeight = stpSprite->iHeight;
	int iCountX, iCountY;

	//----------------------------------------------------------
	// ��� ���� ����
	//----------------------------------------------------------
	DWORD* dwpDest = (DWORD*)bypDest;
	DWORD* dwpSprite = (DWORD*)stpSprite->bypImage;

	//----------------------------------------------------------
	// ��� ���� ó��.
	//----------------------------------------------------------
	iDrawX = iDrawX - stpSprite->iCenterPointX;
	iDrawY = iDrawY - stpSprite->iCenterPointY;

	//----------------------------------------------------------
	// ��ܿ� ���� ��������Ʈ ��� ��ġ ���. (��� Ŭ����)
	//----------------------------------------------------------
	if (0 > iDrawY)
	{
		iSpriteHeight = iSpriteHeight - abs(iDrawY);
		dwpSprite = (DWORD*)(stpSprite->bypImage + stpSprite->iPitch * abs(iDrawY));
		//----------------------------------------------------------
		// ������ ©���� ����̹Ƿ� ��������Ʈ ���� ��ġ�� �Ʒ��� �����ش�.
		//----------------------------------------------------------

		iDrawY = 0;
	}

	//----------------------------------------------------------
	// �ϴܿ� ���� ��������Ʈ ��� ��ġ ���. (�ϴ� Ŭ����)
	//----------------------------------------------------------
	if (iDestHeight < iDrawY + stpSprite->iHeight)
	{
		iSpriteHeight = iSpriteHeight - ((iDrawY + stpSprite->iHeight) - iDestHeight);
	}

	//----------------------------------------------------------
	// ���� ��� ��ġ ��� (���� Ŭ����)
	//----------------------------------------------------------
	if (0 > iDrawX)
	{
		iSpriteWidth -= abs(iDrawX);
		dwpSprite += abs(iDrawX);

		iDrawX = 0;
	}

	//----------------------------------------------------------
	// ������ ��� ��ġ ��� (���� Ŭ����)
	//----------------------------------------------------------
	if (iDestWidth < iDrawX + stpSprite->iWidth)
	{
		iSpriteWidth = iSpriteWidth - ((iDrawX + stpSprite->iWidth) - iDestWidth);
	}

	//----------------------------------------------------------
	// ���� �׸��� ���ٸ� �����Ѵ�.
	//----------------------------------------------------------
	if (iSpriteWidth == 0 || iSpriteHeight == 0)
	{
		return;
	}

	//----------------------------------------------------------
	// ȭ�鿡 ���� ȭ������ �̵��Ѵ�.
	//----------------------------------------------------------
	dwpDest = (DWORD*)((BYTE*)(dwpDest + iDrawX) + (iDrawY * iDestPitch));


	BYTE* bypDestOrigin = (BYTE*)dwpDest;
	BYTE* bypSpriteOrigin = (BYTE*)dwpSprite;
	//----------------------------------------------------------
	// ��ü ũ�⸦ ���鼭 ����� ó���� �Ѵ�.
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

		// ����
		bypDestOrigin += iDestPitch;
		bypSpriteOrigin += stpSprite->iPitch;

		dwpDest = (DWORD*)bypDestOrigin;
		dwpSprite = (DWORD*)bypSpriteOrigin;
	}
}

void CSpriteDib::DrawImage(const int iSpriteIndex, int iDrawX, int iDrawY, const BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch, const int iDrawLen)
{
	//----------------------------------------------------------
	// �ִ� ��������Ʈ ������ �ʰ��ϰų�, �ε���� �ʴ� ��������Ʈ��� ����
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
	// ��������Ʈ ����� ���� ������ ����
	//----------------------------------------------------------
	int iSpriteWidth = stpSprite->iWidth;
	int iSpriteHeight = stpSprite->iHeight;
	int iCountY;
	iSpriteWidth = iSpriteWidth * iDrawLen / 100;

	DWORD* dwpDest = (DWORD*)bypDest;
	DWORD* dwpSprite = (DWORD*)stpSprite->bypImage;

	//----------------------------------------------------------
	// ��ܿ� ���� ��������Ʈ ��� ��ġ ���. (��� Ŭ����)
	//----------------------------------------------------------
	if (0 > iDrawY)
	{
		iSpriteHeight = iSpriteHeight - abs(iDrawY);
		dwpSprite = (DWORD*)(stpSprite->bypImage + stpSprite->iPitch * abs(iDrawY));

		//----------------------------------------------------------
		// ������ ©���� ����̹Ƿ� ��������Ʈ ���� ��ġ�� �Ʒ��� �����ش�.
		//----------------------------------------------------------
		iDrawY = 0;
	}

	//----------------------------------------------------------
	// �ϴܿ� ���� ��������Ʈ ��� ��ġ ���. (�ϴ� Ŭ����)
	//----------------------------------------------------------
	if (iDestHeight < iDrawY + stpSprite->iHeight)
	{
		iSpriteHeight = iSpriteHeight - ((iDrawY + stpSprite->iHeight) - iDestHeight);
	}

	//----------------------------------------------------------
	// ���� ��� ��ġ ��� (���� Ŭ����)
	//----------------------------------------------------------
	if (0 > iDrawX)
	{
		iSpriteWidth -= abs(iDrawX);
		dwpSprite += abs(iDrawX);

		iDrawX = 0;
	}

	//----------------------------------------------------------
	// ������ ��� ��ġ ��� (���� Ŭ����)
	//----------------------------------------------------------
	if (iDestWidth < iDrawX + stpSprite->iWidth)
	{
		iSpriteWidth = iSpriteWidth - ((iDrawX + stpSprite->iWidth) - iDestWidth);
	}

	//----------------------------------------------------------
	// ���� �׸��� ���ٸ� �����Ѵ�.
	//----------------------------------------------------------
	if (iSpriteWidth == 0 && iSpriteHeight == 0)
	{
		return;
	}

	//----------------------------------------------------------
	// ȭ�鿡 ���� ȭ������ �̵��Ѵ�.
	//----------------------------------------------------------
	dwpDest = (DWORD*)((BYTE*)(dwpDest + iDrawX) + (iDrawY * iDestPitch));


	//----------------------------------------------------------
	// ��ü ũ�⸦ ���鼭 ��´�.
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
