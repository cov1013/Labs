#include <windows.h>
#include "CPacket.h"

CPacket::CPacket()
{
	memset(m_bypBuffer, 0, eBUFFER_DEFAULT);
	m_bypRear = m_bypBuffer;
	m_bypFront = m_bypBuffer;
	m_iBufferSize = eBUFFER_DEFAULT;
	m_iFreeSize = eBUFFER_DEFAULT;
	m_iUseSize = 0;
}

CPacket::~CPacket()
{
}

void CPacket::Release(void)
{
	memset(m_bypBuffer, 0, m_iBufferSize);
}

void CPacket::Clear(void)
{
	m_bypRear = m_bypBuffer;
	m_bypFront = m_bypBuffer;

	m_iUseSize = 0;
	m_iFreeSize = eBUFFER_DEFAULT;
}

const int CPacket::GetFreeSize(void)
{
	return m_iFreeSize;
}

const int CPacket::GetUseSize(void)
{
	return m_iUseSize;
}

char* CPacket::GetBufferPtr(void)
{
	return m_bypBuffer;
}

int CPacket::MoveRear(int iSize)
{
	//------------------------------
	// 유효성 체크
	//------------------------------
	if (iSize < 0)
	{
		return -1;
	}

	//------------------------------
	// 요구한 사이즈가 이동할 수 있는 사이즈보다 크다면 
	// 이동할 수 있는 사이즈만큼만 이동.
	//------------------------------
	if (((int)(m_bypRear - m_bypBuffer) + iSize) > m_iBufferSize)
	{
		iSize = m_iBufferSize - (int)(m_bypRear - m_bypBuffer);
	}

	//------------------------------
	// Rear 이동
	//------------------------------
	m_bypRear += iSize;

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return iSize;
}

int CPacket::MoveFront(int iSize)
{
	//------------------------------
	// 유효성 체크
	//------------------------------
	if (iSize < 0)
	{
		return -1;
	}

	//------------------------------
	// 요구한 사이즈가 뽑을 수 있는 사이즈보다 크다면 
	// 뽑을 수 있는 사이즈만큼만 복사
	//------------------------------
	if (((int)(m_bypFront - m_bypBuffer) + iSize) > m_iBufferSize)
	{
		iSize = m_iBufferSize - (int)(m_bypFront - m_bypBuffer);
	}

	//------------------------------
	// Front 이동
	//------------------------------
	m_bypFront += iSize;

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return iSize;
}

CPacket& CPacket::operator=(CPacket& pPacket)
{
	return *this;
}

CPacket& CPacket::operator<<(const BYTE byValue)
{
	int iFreeSize = m_iBufferSize - m_iUseSize;

	if (iFreeSize < sizeof(char))
	{
		return *this;
	}

	*m_bypRear = byValue;

	m_bypRear += sizeof(BYTE);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator<<(const char chValue)
{
	int iFreeSize = m_iBufferSize - m_iUseSize;

	if (iFreeSize < sizeof(char))
	{
		return *this;
	}

	*m_bypRear = chValue;

	m_bypRear += sizeof(char);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator<<(const WORD wValue)
{

	int iFreeSize = m_iBufferSize - (m_bypRear - m_bypBuffer);

	if (iFreeSize < sizeof(short))
	{
		return *this;
	}

	*((WORD*)m_bypRear) = wValue;

	m_bypRear += sizeof(WORD);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator<<(const short shValue)
{
	int iFreeSize = m_iBufferSize - (m_bypRear - m_bypBuffer);

	if (iFreeSize < sizeof(short))
	{
		return *this;
	}

	*((short*)m_bypRear) = shValue;

	m_bypRear += sizeof(short);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator<<(const DWORD dwValue)
{
	int iFreeSize = m_iBufferSize - (m_bypRear - m_bypBuffer);

	if (iFreeSize < sizeof(DWORD))
	{
		return *this;
	}

	*((DWORD*)m_bypRear) = dwValue;

	m_bypRear += sizeof(DWORD);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator<<(const UINT uiValue)
{
	int iFreeSize = m_iBufferSize - (m_bypRear - m_bypBuffer);

	if (iFreeSize < sizeof(DWORD))
	{
		return *this;
	}

	*((UINT*)m_bypRear) = uiValue;

	m_bypRear += sizeof(UINT);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator<<(const int iValue)
{
	int iFreeSize = m_iBufferSize - (m_bypRear - m_bypBuffer);

	if (iFreeSize < sizeof(int))
	{
		return *this;
	}

	*((int*)m_bypRear) = iValue;

	m_bypRear += sizeof(int);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator<<(const long lValue)
{
	int iFreeSize = m_iBufferSize - (m_bypRear - m_bypBuffer);

	if (iFreeSize < sizeof(long))
	{
		return *this;
	}

	*((long*)m_bypRear) = lValue;

	m_bypRear += sizeof(long);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator<<(const float fValue)
{
	int iFreeSize = m_iBufferSize - (m_bypRear - m_bypBuffer);

	if (iFreeSize < sizeof(float))
	{
		return *this;
	}

	*((float*)m_bypRear) = fValue;

	m_bypRear += sizeof(float);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator<<(const __int64 _i64Value)
{
	int iFreeSize = m_iBufferSize - (m_bypRear - m_bypBuffer);

	if (iFreeSize < sizeof(__int64))
	{
		return *this;
	}

	*((__int64*)m_bypRear) = _i64Value;

	m_bypRear += sizeof(__int64);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator<<(const double fValue)
{
	int iFreeSize = m_iBufferSize - (m_bypRear - m_bypBuffer);

	if (iFreeSize < sizeof(double))
	{
		return *this;
	}

	*((double*)m_bypRear) = fValue;

	m_bypRear += sizeof(double);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator>>(BYTE& byValue)
{
	int iUseSize = m_bypRear - m_bypFront;

	if (iUseSize < sizeof(BYTE))
	{
		return *this;
	}

	byValue = *((BYTE*)m_bypFront);

	m_bypFront += sizeof(BYTE);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator>>(char& chValue)
{
	int iUseSize = m_bypRear - m_bypFront;

	if (iUseSize < sizeof(char))
	{
		return *this;
	}

	chValue = *((char*)m_bypFront);

	m_bypFront += sizeof(char);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator>>(short& shValue)
{
	int iUseSize = m_bypRear - m_bypFront;

	if (iUseSize < sizeof(short))
	{
		return *this;
	}

	shValue = *((short*)m_bypFront);

	m_bypFront += sizeof(short);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator>>(WORD& wValue)
{
	int iUseSize = m_bypRear - m_bypFront;

	if (iUseSize < sizeof(WORD))
	{
		return *this;
	}

	wValue = *((WORD*)m_bypFront);

	m_bypFront += sizeof(WORD);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator>>(UINT& uiValue)
{
	int iUseSize = m_bypRear - m_bypFront;

	if (iUseSize < sizeof(UINT))
	{
		throw uiValue;
	}

	uiValue = *((int*)m_bypFront);

	m_bypFront += sizeof(UINT);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator>>(int& iValue)
{
	int iUseSize = m_bypRear - m_bypFront;

	if (iUseSize < sizeof(int))
	{
		throw iValue;
	}

	iValue = *((int*)m_bypFront);

	m_bypFront += sizeof(int);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator>>(DWORD& dwValue)
{
	int iUseSize = m_bypRear - m_bypFront;

	if (iUseSize < sizeof(DWORD))
	{
		return *this;
	}

	dwValue = *((DWORD*)m_bypFront);

	m_bypFront += sizeof(DWORD);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator>>(long& lValue)
{
	int iUseSize = m_bypRear - m_bypFront;

	if (iUseSize < sizeof(long))
	{
		return *this;
	}

	lValue = *((long*)m_bypFront);

	m_bypFront += sizeof(long);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator>>(float& fValue)
{
	int iUseSize = m_bypRear - m_bypFront;

	if (iUseSize < sizeof(float))
	{
		return *this;
	}

	fValue = *((float*)m_bypFront);

	m_bypFront += sizeof(float);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator>>(double& dValue)
{
	int iUseSize = m_bypRear - m_bypFront;

	if (iUseSize < sizeof(double))
	{
		return *this;
	}

	dValue = *((double*)m_bypFront);

	m_bypFront += sizeof(double);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

CPacket& CPacket::operator>>(__int64& iValue)
{
	int iUseSize = m_bypRear - m_bypFront;

	if (iUseSize < sizeof(__int64))
	{
		return *this;
	}

	iValue = *((__int64*)m_bypFront);

	m_bypFront += sizeof(__int64);

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return *this;
}

int CPacket::GetData(char* chpDest, int iSize)
{
	int iUseSize;

	if (nullptr == chpDest)
	{
		return -1;
	}
	if (0 > iSize)
	{
		return -1;
	}

	iUseSize = (int)(m_bypRear - m_bypFront);

	if (iSize > iUseSize)
	{
		iSize = iUseSize;
	}

	for (int iCnt = 0; iCnt < iSize; iCnt++)
	{
		chpDest[iCnt] = m_bypFront[iCnt];
	}

	m_bypFront += iSize;

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return iSize;
}

int CPacket::PutData(char* chpSrc, int iSize)
{
	int iFreeSize;

	if (nullptr == chpSrc)
	{
		return -1;
	}
	if (0 > iSize)
	{
		return -1;
	}

	iFreeSize = m_iBufferSize - m_iUseSize;

	if (iFreeSize < iSize)
	{
		iSize = iFreeSize;
	}

	for (int iCnt = 0; iCnt < iSize; iCnt++)
	{
		m_bypRear[iCnt] = chpSrc[iCnt];
	}

	m_bypRear += iSize;

	//------------------------------
	// Free, Use Size 갱신
	//------------------------------
	m_iUseSize = (int)(m_bypRear - m_bypFront);
	m_iFreeSize = m_iBufferSize - m_iUseSize;

	return iSize;
}
