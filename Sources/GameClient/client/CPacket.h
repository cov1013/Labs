/*****************************************************************
*
* TODO : 버퍼 사이즈 초과시 동적 할당 후 로그 남기기
*
*
*
******************************************************************/
#ifndef __PACKET__
#define __PACKET__

class CPacket
{
public:
	enum
	{
		eBUFFER_DEFAULT = 3000
		//---------------------------------------------------------------
		// 기본 버퍼 사이즈
		// 
		// 
		//---------------------------------------------------------------
	};


public:
	CPacket();
	//---------------------------------------------------------------
	//
	// 
	// 
	//---------------------------------------------------------------


	virtual ~CPacket();
	//---------------------------------------------------------------
	//
	// 
	// 
	//---------------------------------------------------------------

public:
	void Release(void);
	//---------------------------------------------------------------
	// 버퍼 데이터 초기화
	// 
	// 
	//---------------------------------------------------------------


	void Clear(void);
	//---------------------------------------------------------------
	// Rear, Front 포인터 시작점으로 이동
	// 
	// 
	//---------------------------------------------------------------

	int MoveRear(int iSize);
	//---------------------------------------------------------------
	// Rear를 iSize 만큼 이동
	// 
	// 
	//---------------------------------------------------------------


	int MoveFront(int iSize);
	//---------------------------------------------------------------
	// Front를 iSize 만큼 이동
	// 
	// 
	//---------------------------------------------------------------


	int	GetData(char* chpDest, int iSize);
	//---------------------------------------------------------------
	// 버퍼에 있는 데이터를 chpDest에 iSize 만큼 데이터 복사
	// 
	// 
	//---------------------------------------------------------------


	int	PutData(char* chpSource, int iSrcSize);
	//---------------------------------------------------------------
	// chpSoruce에 있는 데이터를 버퍼에 iSize 만큼 데이터 복사
	// 
	// 
	//---------------------------------------------------------------

	char* GetBufferPtr(void);
	//---------------------------------------------------------------
	// 버퍼 시작 주소 리턴
	// 
	// 
	//---------------------------------------------------------------


	const int GetFreeSize(void);
	//---------------------------------------------------------------
	// 사용 가능한 버퍼 사이즈 리턴
	// 
	// 
	//---------------------------------------------------------------


	const int GetUseSize(void);
	//---------------------------------------------------------------
	// 사용 중인 버퍼 사이즈 리턴
	// 
	// 
	//---------------------------------------------------------------

public:
	CPacket& operator=(CPacket& pPayload);
	//---------------------------------------------------------------
	// 
	// 
	// 
	//---------------------------------------------------------------

	CPacket& operator << (const BYTE byValue);
	CPacket& operator << (const char chValue);

	CPacket& operator << (const WORD wValue);
	CPacket& operator << (const short shValue);

	CPacket& operator << (const UINT uiValue);
	CPacket& operator << (const int iValue);

	CPacket& operator << (const DWORD dwValue);
	CPacket& operator << (const long lValue);

	CPacket& operator << (const float fValue);
	CPacket& operator << (const double fValue);

	CPacket& operator << (const __int64 _i64Value);
	//---------------------------------------------------------------
	// 버퍼에 데이터 삽입
	// 
	// 
	//---------------------------------------------------------------

	CPacket& operator >> (BYTE& byValue);
	CPacket& operator >> (char& chValue);

	CPacket& operator >> (WORD& shValue);
	CPacket& operator >> (short& shValue);

	CPacket& operator >> (UINT& uiValue);
	CPacket& operator >> (int& iValue);

	CPacket& operator >> (DWORD& dwValue);
	CPacket& operator >> (long& lValue);

	CPacket& operator >> (float& fValue);
	CPacket& operator >> (double& dValue);

	CPacket& operator >> (__int64& iValue);
	//---------------------------------------------------------------
	// 인자 주소에 버퍼에 있는 데이터 출력
	// 
	// 
	//---------------------------------------------------------------

protected:
	char	m_bypBuffer[eBUFFER_DEFAULT];
	char* m_bypRear;
	char* m_bypFront;
	int		m_iBufferSize;
	int		m_iFreeSize;
	int		m_iUseSize;
};

#endif