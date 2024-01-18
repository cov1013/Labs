#include <Windows.h>
#include "CStreamQ.h"

CStreamQ::CStreamQ()
{
	memset(m_bypBuffer, 0, dfMAX);	// �̰� ����?
	m_bypRear = m_bypBuffer;
	m_bypFront = m_bypBuffer;
	m_iBufferSize = dfMAX;
	m_bCycleFlag = FALSE;
}

//CStreamQ::CStreamQ(const int bufferSize)
//{
//	////-----------------------------------------------
//	//// �޸� �Ҵ� ��û
//	////-----------------------------------------------
//	//mBuffer = new char[bufferSize];
//
//	////-----------------------------------------------
//	//// ������Ƽ �ʱ�ȭ
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
	// �޸� ���� ��û
	//-----------------------------------------------
	//delete[] mBuffer;
}

void CStreamQ::Resize(const int size)
{
	////-----------------------------------------------
	//// �䱸�� ����� ���� ������� �۴ٸ� ����
	////-----------------------------------------------
	//if (size < m_iBufferSize)
	//{
	//	return;
	//}

	////-----------------------------------------------
	//// �޸� �Ҵ� ��û
	////-----------------------------------------------
	//char* newBuffer = new char[size];

	////-----------------------------------------------
	//// ������Ƽ ����
	////-----------------------------------------------
	//if (newBuffer != nullptr)
	//{
	//	//-----------------------------------------------
	//	// ���� ������ ����
	//	//-----------------------------------------------
	//	memset(newBuffer, 0, size);
	//	memcpy_s(newBuffer, size, mBuffer, m_iBufferSize);


	//	//-----------------------------------------------
	//	// Rear, Front ��ġ ����
	//	//-----------------------------------------------
	//	int rearPos = abs(mBuffer - mRear);
	//	int frontPos = abs(mBuffer - mFront);

	//	//-----------------------------------------------
	//	// ���� ���� ���� ��û
	//	//-----------------------------------------------
	//	delete[] mBuffer;

	//	//-----------------------------------------------
	//	// ���� �������� ��ġ�� ������� ���ο� ������ ����
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
	// Rear�� Front���� �ڿ� �ִ�.
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
	// Rear�� Front���� �տ� �ִ�.
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
	// ���ۿ� ���ο� ������ ���� ���� ���� �� ���
	// ----------------------------------------
	int iFreeSize = GetFreeSize();

	// ----------------------------------------
	// ���ۿ� ������ ������ ����
	// ----------------------------------------
	if (iFreeSize == 0)
	{
		return 0;
	}

	// ----------------------------------------
	// �������� ũ�Ⱑ ������ �� �������� ũ�ٸ�
	// ������ �� ������ŭ�� push�ϰԲ� size ����
	// ----------------------------------------
	if (iFreeSize < iSize)
	{
		iSize = iFreeSize;
	}

	// ----------------------------------------
	// Enqueue �۾� ����
	// ----------------------------------------
	int result = 0;

	for (int iCount = 0; iCount < iSize; iCount++)
	{
		// ----------------------------------------
		// 1. ������ ����
		// ----------------------------------------
		*m_bypRear = bypSource[iCount];

		// ----------------------------------------
		// 2. Rear�� ���� ��ġ���� ���� ��ġ ����
		//	  Entry�� �������� BufferSize��ŭ ���̰� ���ٸ�
		//	  �Ҵ�� ������ �뷮�� �ʰ��ϹǷ� Entry�� �����Ѵ�.
		// ----------------------------------------
		char* bypNext = m_bypRear + 1;
		char* bypEntry = m_bypBuffer;
		int iPeriod = abs(bypEntry - bypNext);

		if (iPeriod == m_iBufferSize)
		{
			m_bypRear = bypEntry;

			// ----------------------------------------
			// Rear�� Entry��� ���� ���۸� ��ȸ�ߴٴ� ���̹Ƿ�
			// CycleFlag�� ON�Ѵ�.
			// ----------------------------------------
			m_bCycleFlag = true;
		}
		else
		{
			m_bypRear = bypNext;

			// ----------------------------------------
			// CycleFlag�� ON�� ���¿��� Rear�� ��ġ��
			// Front ���� �ռ��ٸ�, CycleFlag�� OFF�Ѵ�.
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
	// ��� ���� ������ �� ���
	// ----------------------------------------
	int useSize = GetUseSize();

	// ----------------------------------------
	// ���ۿ� �����Ͱ� ������ ����
	// ----------------------------------------
	if (useSize == 0)
	{
		return 0;
	}

	// ----------------------------------------
	// �䱸�� ������� ���ۿ� �����ϴ� �������� ũ�Ⱑ
	// ���� ��� �����ϴ� ũ�⸸ŭ�� pop�Ѵ�.
	// ----------------------------------------
	if (useSize < iSize)
	{
		iSize = useSize;
	}

	// ----------------------------------------
	// Dequeue �۾� ����
	// ----------------------------------------
	int result = 0;
	for (int iCount = 0; iCount < iSize; iCount++)
	{
		// ----------------------------------------
		// 1. ������ ���
		// ----------------------------------------
		bypDest[iCount] = *m_bypFront;

		// ----------------------------------------
		// Front�� ���� ��ġ���� ���� ��ġ ����
		// Entry�� �������� BufferSize��ŭ ���̰� ���ٸ�
		// �Ҵ�� ������ �뷮�� �ʰ��ϹǷ� Entry�� �����Ѵ�.
		// ----------------------------------------
		char* bypNext = m_bypFront + 1;
		char* bypEntry = m_bypBuffer;
		int iPeriod = abs(bypEntry - bypNext);

		if (iPeriod == m_iBufferSize)
		{
			m_bypFront = bypEntry;
			// ----------------------------------------
			// Front�� Entry�� �Դٴ� ���� Rear�� ��ġ��
			// ���ų�, Rear���� �ڿ� �ִ� ���̹Ƿ�, m_bCycleFlag�� OFF�Ѵ�.
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
	// ���ۿ� �����Ͱ� ������ ����
	// ----------------------------------------
	if (iUseSize == 0)
	{
		return 0;
	}

	// ----------------------------------------
	// �䱸�� ������� ���ۿ� �����ϴ� �������� ũ�Ⱑ
	// ���� ��� �����ϴ� ũ�⸸ŭ�� pop�Ѵ�.
	// ----------------------------------------
	if (iUseSize < iSize)
	{
		iSize = iUseSize;
	}

	// ----------------------------------------
	// Peek �۾� ����
	// ----------------------------------------
	int iResult = 0;
	char* bypTemp = m_bypFront;
	for (int iCount = 0; iCount < iSize; iCount++)
	{
		// ----------------------------------------
		// 1. ������ ���
		// ----------------------------------------
		bypDest[iCount] = *m_bypFront;

		// ----------------------------------------
		// 2. Front�� ���� ��ġ���� ���� ��ġ ����
		//    Entry�� �������� BufferSize��ŭ ���̰� ���ٸ�
		//    �Ҵ�� ������ �뷮�� �ʰ��ϹǷ� Entry�� �����Ѵ�.
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
	// ���� Front ��ġ ����
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
		// 1. Rear�� ���� ��ġ���� ���� ��ġ ����
		//    Entry�� �������� BufferSize��ŭ ���̰� ���ٸ�
		//    �Ҵ�� ������ �뷮�� �ʰ��ϹǷ� Entry�� �����Ѵ�.
		// ----------------------------------------
		bypNext = m_bypRear + 1;
		iPeriod = abs(bypEntry - bypNext);

		if (iPeriod == m_iBufferSize)
		{
			m_bypRear = bypEntry;

			// ----------------------------------------
			// Rear�� Entry��� ���� ���۸� ��ȸ�ߴٴ� ���̹Ƿ�
			// CycleFlag�� ON�Ѵ�.
			// ----------------------------------------
			m_bCycleFlag = true;
		}
		else
		{
			m_bypRear = bypNext;

			// ----------------------------------------
			// CycleFlag�� ON�� ���¿��� Rear�� ��ġ��
			// Front ���� �ռ��ٸ�, CycleFlag�� OFF�Ѵ�.
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
		// Front�� ���� ��ġ���� ���� ��ġ ����
		// Entry�� �������� BufferSize��ŭ ���̰� ���ٸ�
		// �Ҵ�� ������ �뷮�� �ʰ��ϹǷ� Entry�� �����Ѵ�.
		// ----------------------------------------
		bypNext = m_bypFront + 1;
		iPeriod = abs(bypEntry - bypNext);

		if (iPeriod == m_iBufferSize)
		{
			m_bypFront = bypEntry;

			// ----------------------------------------
			// Front�� Entry�� �Դٴ� ���� Rear�� ��ġ��
			// ���ų�, Rear���� �ڿ� �ִ� ���̹Ƿ�, 
			// m_bCycleFlag�� OFF�Ѵ�.
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
