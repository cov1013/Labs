#include <windows.h>
#include "CScreenDib.h"

CScreenDib::CScreenDib(const int iWidth, const int iHeight, const int iColorBit)
{
	memset(&m_stDibInfo, 0x00, sizeof(BITMAPINFO));
	m_bypBuffer = NULL;
	m_iWidth = 0;
	m_iHeight = 0;
	m_iPitch = 0;
	m_iColorBit = 0;
	m_iBufferSize = 0;

	// ------------------------------------------------------------
	// ���ڸ� �������� ���� ����
	// ------------------------------------------------------------
	CreateDibBuffer(iWidth, iHeight, iColorBit);
}

CScreenDib::~CScreenDib()
{
	ReleaseDibBuffer();
}

void CScreenDib::CreateDibBuffer(const int iWidth, const int iHeight, const int iColorBit)
{
	m_iWidth = iWidth;
	m_iHeight = iHeight;
	m_iColorBit = iColorBit;

	// ------------------------------------------------------------
	// Picth ���� ����
	// 
	// 4�� ��Ʈ�� 100�̰� � ������ ������ �� �ڸ� ��Ʈ�� 100�̸� 4�� ������ ��������.
	// ~3 ������ ���� 011 -> 100���� �����ϰ�, �װ� &�����Ͽ� �� �ڸ��� 100���� �����.
	// ��� �� 2��Ʈ�� ���ư����� ������ 2��Ʈ�� +�Ͽ� �ڸ��� �ø���. (�Ѿ�� ���� ����)
	// ------------------------------------------------------------
	m_iPitch = ((iWidth * (iColorBit / 8)) + 3) & ~3;
	m_iBufferSize = m_iPitch * m_iHeight;

	// ------------------------------------------------------------
	// DibInfo ��� ����
	// DIB ��½� ����� ����ϱ� ���� ���̰��� - �� �Է��Ѵ�.
	// ------------------------------------------------------------
	m_stDibInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_stDibInfo.bmiHeader.biWidth = m_iWidth;
	m_stDibInfo.bmiHeader.biHeight = -m_iHeight;
	m_stDibInfo.bmiHeader.biPlanes = 1;
	m_stDibInfo.bmiHeader.biBitCount = m_iColorBit;
	m_stDibInfo.bmiHeader.biCompression = 0;
	m_stDibInfo.bmiHeader.biSizeImage = m_iBufferSize;
	m_stDibInfo.bmiHeader.biXPelsPerMeter = 0;
	m_stDibInfo.bmiHeader.biYPelsPerMeter = 0;
	m_stDibInfo.bmiHeader.biClrUsed = 0;
	m_stDibInfo.bmiHeader.biClrImportant = 0;

	// ------------------------------------------------------------
	// ���� ���� �Ҵ�
	// ------------------------------------------------------------
	m_bypBuffer = (BYTE*)malloc(sizeof(BYTE) * m_iBufferSize);
	if (NULL != m_bypBuffer)
	{
		memset(m_bypBuffer, 0, m_iBufferSize);
	}
}

void CScreenDib::ReleaseDibBuffer(void)
{
	memset(&m_stDibInfo, 0x00, sizeof(BITMAPINFO));
	m_iWidth = 0;
	m_iHeight = 0;
	m_iPitch = 0;
	m_iColorBit = 0;
	m_iBufferSize = 0;

	if (NULL != m_bypBuffer)
	{
		free(m_bypBuffer);
	}
	m_bypBuffer = NULL;
}

void CScreenDib::Flip(const HWND hWnd, const int iX, const int iY)
{
	if (m_bypBuffer == NULL)
	{
		return;
	}

	HDC hDC;
	// ------------------------------------
	// ���� ����� DC�� ��´�.
	// ------------------------------------
	hDC = GetDC(hWnd);

	// ------------------------------------
	// GDI �Լ��� ����Ͽ� DC�� ���
	// ------------------------------------
	int i = SetDIBitsToDevice(
		hDC,
		0, 0, 640, 480,					// ���� ����� 0(X) 0(Y)���� m_iWidth, m_iHeight������ ��������
		0, 0, 0, 480,					// iX, iY�� �������� ��ĵ�� �����ؼ� �� ������ m_iHeight
		m_bypBuffer,
		&m_stDibInfo,
		DIB_RGB_COLORS);

	ReleaseDC(hWnd, hDC);
}

BYTE* CScreenDib::GetDibBuffer(void)
{
	return m_bypBuffer;
}

int CScreenDib::GetWidth(void)
{
	return m_iWidth;
}

int CScreenDib::GetHeight(void)
{
	return m_iHeight;
}

int CScreenDib::GetPitch(void)
{
	return m_iPitch;
}
