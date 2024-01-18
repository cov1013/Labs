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
	// 인자를 기준으로 버퍼 생성
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
	// Picth 연산 설명
	// 
	// 4는 비트로 100이고 어떤 정수든 마지막 세 자리 비트가 100이면 4로 나누어 떨어진다.
	// ~3 연산을 통해 011 -> 100으로 변경하고, 그걸 &연산하여 뒷 자리를 100으로 만든다.
	// 대신 뒤 2비트가 날아감으로 기존의 2비트를 +하여 자리를 올린다. (넘어가는 수는 무시)
	// ------------------------------------------------------------
	m_iPitch = ((iWidth * (iColorBit / 8)) + 3) & ~3;
	m_iBufferSize = m_iPitch * m_iHeight;

	// ------------------------------------------------------------
	// DibInfo 헤더 생성
	// DIB 출력시 뒤집어서 출력하기 위해 높이값을 - 로 입력한다.
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
	// 버퍼 동적 할당
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
	// 찍을 대상의 DC를 얻는다.
	// ------------------------------------
	hDC = GetDC(hWnd);

	// ------------------------------------
	// GDI 함수를 사용하여 DC에 출력
	// ------------------------------------
	int i = SetDIBitsToDevice(
		hDC,
		0, 0, 640, 480,					// 찍을 대상의 0(X) 0(Y)부터 m_iWidth, m_iHeight까지의 범위까지
		0, 0, 0, 480,					// iX, iY을 기준으로 스캔을 시작해서 총 라인은 m_iHeight
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
