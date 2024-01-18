#include <Windows.h>
#include "CStreamQ.h"

CStreamQ::CStreamQ()
{
	memset(m_bypBuffer, 0, dfMAX);	// 이걸 굳이?
	m_bypRear = m_bypBuffer;
	m_bypFront = m_bypBuffer;
	m_iBufferSize = dfMAX;
	m_bCycleFlag = FALSE;
}

//CStreamQ::CStreamQ(const int bufferSize)
//{
//	////-----------------------------------------------
//	//// 메모리 할당 요청
//	////-----------------------------------------------
//	//mBuffer = new char[bufferSize];
//
//	////-----------------------------------------------
//	//// 프로퍼티 초기화
//	////-----------------------------------------------
//	//if (mBuffer != nullptr)
//	//{
//	//	memset(mBuffer, 0, bufferSize);
//	//	mRear = mBuffer;
//	//	mFront = mBuffer;
//	//	mBufferSize = bufferSize;
//	//}
//}

CStreamQ::~CStreamQ()
{
	//-----------------------------------------------
	// 메모리 해제 요청
	//-----------------------------------------------
	//delete[] mBuffer;
}

void CStreamQ::Resize(const int size)
{
	////-----------------------------------------------
	//// 요구한 사이즈가 기존 사이즈보다 작다면 리턴
	////-----------------------------------------------
	//if (size < m_iBufferSize)
	//{
	//	return;
	//}

	////-----------------------------------------------
	//// 메모리 할당 요청
	////-----------------------------------------------
	//char* newBuffer = new char[size];

	////-----------------------------------------------
	//// 프로퍼티 갱신
	////-----------------------------------------------
	//if (newBuffer != nullptr)
	//{
	//	//-----------------------------------------------
	//	// 기존 데이터 복사
	//	//-----------------------------------------------
	//	memset(newBuffer, 0, size);
	//	memcpy_s(newBuffer, size, mBuffer, m_iBufferSize);


	//	//-----------------------------------------------
	//	// Rear, Front 위치 저장
	//	//-----------------------------------------------
	//	int rearPos = abs(mBuffer - mRear);
	//	int frontPos = abs(mBuffer - mFront);

	//	//-----------------------------------------------
	//	// 기존 버퍼 해제 요청
	//	//-----------------------------------------------
	//	delete[] mBuffer;

	//	//-----------------------------------------------
	//	// 기존 데이터의 위치를 기반으로 새로운 데이터 설정
	//	//-----------------------------------------------
	//	mBuffer = newBuffer;
	//	mRear = &mBuffer[rearPos];
	//	mFront = &mBuffer[frontPos];
	//	mBufferSize = size;
	//}
}

unsigned int CStreamQ::GetBufferSize(void)
{
	return m_iBufferSize;
}

unsigned int CStreamQ::GetUseSize(void)
{
	if (m_bCycleFlag == TRUE)
	{
		//  0								   MAX
		//  -------------------------------------
		//  |##### R            F ############# |
		//  -------------------------------------
		return m_iBufferSize - (unsigned int)(m_bypFront - m_bypRear);
	}
	else
	{
		// 0								  MAX
		// -------------------------------------
		// |	 F ########### R			   |
		// -------------------------------------
		return m_bypRear - m_bypFront;
	}
}

unsigned int CStreamQ::GetFreeSize(void)
{
	if (m_bCycleFlag == true)
	{
		//  0								   MAX
		//  -------------------------------------
		//  |     R ########### F               |
		//  -------------------------------------
		return (unsigned int)(m_bypFront - m_bypRear);
	}
	else
	{
		// 0								  MAX
		// -------------------------------------
		// | ### F             R ############# |
		// -------------------------------------
		return m_iBufferSize - (unsigned int)(m_bypRear - m_bypFront);
	}


}

int CStreamQ::DirectEnqueueSize(void)
{
	//---------------------------------------
	// Rear가 Front보다 뒤에 있다.
	//---------------------------------------
	if (TRUE == m_bCycleFlag)
	{
		// 0								  MAX
		// -------------------------------------
		// |     R ########### F               |
		// -------------------------------------
		return (m_bypFront - m_bypRear);
	}
	//---------------------------------------
	// Rear가 Front보다 앞에 있다.
	//---------------------------------------
	else
	{
		// 0								  MAX
		// -------------------------------------
		// | F      R ######################## |
		// -------------------------------------
		return (int)((&m_bypBuffer[m_iBufferSize - 1] - m_bypRear) + 1);
	}
}

int CStreamQ::DirectDequeueSize(void)
{
	if (TRUE == m_bCycleFlag)
	{
		// 0								  MAX
		// -------------------------------------
		// |    R   F ######################## |
		// -------------------------------------
		return (int)((&m_bypBuffer[m_iBufferSize - 1] - m_bypFront) + 1);
	}
	else
	{
		// 0								  MAX
		// -------------------------------------
		// |     F ########### R               |
		// -------------------------------------
		return (int)(m_bypRear - m_bypFront);
	}
}

// 
int CStreamQ::Put(const char* bypSource, int iSize)
{
	if (iSize < 0)
	{
		return -1;
	}
	if (nullptr == bypSource)
	{
		return -1;
	}

	// ----------------------------------------
	// 버퍼에 새로운 데이터 저장 가능 공간 수 얻기
	// ----------------------------------------
	int iFreeSize = GetFreeSize();

	// ----------------------------------------
	// 버퍼에 공간이 없으면 리턴
	// ----------------------------------------
	if (iFreeSize == 0)
	{
		return 0;
	}

	// ----------------------------------------
	// 데이터의 크기가 버퍼의 빈 공간보다 크다면
	// 버퍼의 빈 공간만큼만 push하게끔 size 조정
	// ----------------------------------------
	if (iFreeSize < iSize)
	{
		iSize = iFreeSize;
	}

	// ----------------------------------------
	// Enqueue 작업 시작
	// ----------------------------------------
	int result = 0;

	for (int iCount = 0; iCount < iSize; iCount++)
	{
		// ----------------------------------------
		// 1. 데이터 삽입
		// ----------------------------------------
		*m_bypRear = bypSource[iCount];

		// ----------------------------------------
		// 2. Rear의 현재 위치에서 다음 위치 값이
		//	  Entry를 기준으로 BufferSize만큼 차이가 난다면
		//	  할당된 버퍼의 용량을 초과하므로 Entry로 갱신한다.
		// ----------------------------------------
		char* bypNext = m_bypRear + 1;
		char* bypEntry = m_bypBuffer;
		int iPeriod = abs(bypEntry - bypNext);

		if (iPeriod == m_iBufferSize)
		{
			m_bypRear = bypEntry;

			// ----------------------------------------
			// Rear가 Entry라는 것은 버퍼를 순회했다는 뜻이므로
			// CycleFlag를 ON한다.
			// ----------------------------------------
			m_bCycleFlag = true;
		}
		else
		{
			m_bypRear = bypNext;

			// ----------------------------------------
			// CycleFlag가 ON인 상태에서 Rear의 위치가
			// Front 보다 앞섰다면, CycleFlag를 OFF한다.
			// ----------------------------------------
			if (m_bCycleFlag == true && m_bypRear > m_bypFront)
			{
				m_bCycleFlag = false;
			}
		}

		result++;
	}

	return result;
}

int CStreamQ::Get(char* bypDest, int iSize)
{
	if (iSize < 0)
	{
		return -1;
	}
	if (nullptr == bypDest)
	{
		return -1;
	}

	// ----------------------------------------
	// 출력 가능 데이터 수 얻기
	// ----------------------------------------
	int useSize = GetUseSize();

	// ----------------------------------------
	// 버퍼에 데이터가 없으면 리턴
	// ----------------------------------------
	if (useSize == 0)
	{
		return 0;
	}

	// ----------------------------------------
	// 요구한 사이즈보다 버퍼에 존재하는 데이터의 크기가
	// 작을 경우 존재하는 크기만큼만 pop한다.
	// ----------------------------------------
	if (useSize < iSize)
	{
		iSize = useSize;
	}

	// ----------------------------------------
	// Dequeue 작업 시작
	// ----------------------------------------
	int result = 0;
	for (int iCount = 0; iCount < iSize; iCount++)
	{
		// ----------------------------------------
		// 1. 데이터 출력
		// ----------------------------------------
		bypDest[iCount] = *m_bypFront;

		// ----------------------------------------
		// Front의 현재 위치에서 다음 위치 값이
		// Entry를 기준으로 BufferSize만큼 차이가 난다면
		// 할당된 버퍼의 용량을 초과하므로 Entry로 갱신한다.
		// ----------------------------------------
		char* bypNext = m_bypFront + 1;
		char* bypEntry = m_bypBuffer;
		int iPeriod = abs(bypEntry - bypNext);

		if (iPeriod == m_iBufferSize)
		{
			m_bypFront = bypEntry;
			// ----------------------------------------
			// Front가 Entry로 왔다는 것은 Rear와 위치가
			// 같거나, Rear보다 뒤에 있는 것이므로, m_bCycleFlag를 OFF한다.
			// ----------------------------------------
			m_bCycleFlag = false;
		}
		else
		{
			m_bypFront = bypNext;
		}

		result++;
	}

	return result;
}

int CStreamQ::Peek(char* bypDest, int iSize)
{
	if (iSize < 0)
	{
		return -1;
	}
	if (nullptr == bypDest)
	{
		return -1;
	}

	int iUseSize = GetUseSize();

	// ----------------------------------------
	// 버퍼에 데이터가 없으면 리턴
	// ----------------------------------------
	if (iUseSize == 0)
	{
		return 0;
	}

	// ----------------------------------------
	// 요구한 사이즈보다 버퍼에 존재하는 데이터의 크기가
	// 작을 경우 존재하는 크기만큼만 pop한다.
	// ----------------------------------------
	if (iUseSize < iSize)
	{
		iSize = iUseSize;
	}

	// ----------------------------------------
	// Peek 작업 시작
	// ----------------------------------------
	int iResult = 0;
	char* bypTemp = m_bypFront;
	for (int iCount = 0; iCount < iSize; iCount++)
	{
		// ----------------------------------------
		// 1. 데이터 출력
		// ----------------------------------------
		bypDest[iCount] = *m_bypFront;

		// ----------------------------------------
		// 2. Front의 현재 위치에서 다음 위치 값이
		//    Entry를 기준으로 BufferSize만큼 차이가 난다면
		//    할당된 버퍼의 용량을 초과하므로 Entry로 갱신한다.
		// ----------------------------------------
		char* bypNext = m_bypFront + 1;
		char* bypEntry = m_bypBuffer;

		int iPeriod = abs(bypEntry - bypNext);
		if (iPeriod == m_iBufferSize)
		{
			m_bypFront = bypEntry;
		}
		else
		{
			m_bypFront = bypNext;
		}

		iResult++;
	}

	// ----------------------------------------
	// 기존 Front 위치 복구
	// ----------------------------------------
	m_bypFront = bypTemp;

	return iResult;
}

void CStreamQ::MoveRear(const int iSize)
{
	char* bypEntry;
	char* bypNext;
	int iCount, iPeriod;

	if (iSize < 0)
	{
		return;
	}
	if (iSize >= m_iBufferSize)
	{
		return;
	}

	bypEntry = m_bypBuffer;

	for (iCount = 0; iCount < iSize; iCount++)
	{
		// ----------------------------------------
		// 1. Rear의 현재 위치에서 다음 위치 값이
		//    Entry를 기준으로 BufferSize만큼 차이가 난다면
		//    할당된 버퍼의 용량을 초과하므로 Entry로 갱신한다.
		// ----------------------------------------
		bypNext = m_bypRear + 1;
		iPeriod = abs(bypEntry - bypNext);

		if (iPeriod == m_iBufferSize)
		{
			m_bypRear = bypEntry;

			// ----------------------------------------
			// Rear가 Entry라는 것은 버퍼를 순회했다는 뜻이므로
			// CycleFlag를 ON한다.
			// ----------------------------------------
			m_bCycleFlag = true;
		}
		else
		{
			m_bypRear = bypNext;

			// ----------------------------------------
			// CycleFlag가 ON인 상태에서 Rear의 위치가
			// Front 보다 앞섰다면, CycleFlag를 OFF한다.
			// ----------------------------------------
			if (m_bCycleFlag == true && m_bypRear > m_bypFront)
			{
				m_bCycleFlag = false;
			}
		}
	}
}

void CStreamQ::MoveFront(const int iSize)
{
	char* bypEntry;
	char* bypNext;
	int iCount, iPeriod;

	if (iSize < 0)
	{
		return;
	}
	if (iSize >= m_iBufferSize)
	{
		return;
	}

	bypEntry = m_bypBuffer;
	for (iCount = 0; iCount < iSize; iCount++)
	{
		// ----------------------------------------
		// Front의 현재 위치에서 다음 위치 값이
		// Entry를 기준으로 BufferSize만큼 차이가 난다면
		// 할당된 버퍼의 용량을 초과하므로 Entry로 갱신한다.
		// ----------------------------------------
		bypNext = m_bypFront + 1;
		iPeriod = abs(bypEntry - bypNext);

		if (iPeriod == m_iBufferSize)
		{
			m_bypFront = bypEntry;

			// ----------------------------------------
			// Front가 Entry로 왔다는 것은 Rear와 위치가
			// 같거나, Rear보다 뒤에 있는 것이므로, 
			// m_bCycleFlag를 OFF한다.
			// ----------------------------------------
			m_bCycleFlag = false;
		}
		else
		{
			m_bypFront = bypNext;
		}
	}
}

void CStreamQ::ClearBuffer(void)
{
	m_bypRear = m_bypBuffer;
	m_bypFront = m_bypBuffer;
}

char* CStreamQ::GetFrontBufferPtr(void)
{
	return m_bypFront;
}

char* CStreamQ::GetRearBufferPtr(void)
{
	return m_bypRear;
}
