#include "CPacket.h"
#include "Profiler.h"
#include "CCrashDump.h"

namespace cov1013
{
	CMemoryPool_TLS<CPacket> CPacket::sm_MemoryPool = CMemoryPool_TLS<CPacket>(0);
	//CMemoryPool<CPacket> CPacket::sm_MemoryPool = CMemoryPool<CPacket>(0);
	unsigned char CPacket::sm_byPacketKey = -1;
	unsigned char CPacket::sm_byPacketCode = -1;

#ifdef __MULTI_THREAD_DEBUG_MODE__
	CPacket*	CPacket::sm_AllocPacket[USHRT_MAX] = { NULL };
	short		CPacket::sm_AllocPacketIndex = -1;
#endif

#ifdef __MULTI_THREAD_DEBUG_MODE__
	CPacket* CPacket::Alloc(int Logic, SESSION_ID SessionID)
	{
		CPacket* pPacket = sm_MemoryPool.Alloc();
		pPacket->Initial();
		
		WORD Index = InterlockedIncrement16(&pPacket->m_LogIndex);
		Index %= st_LOG::en_LOG_MAX;
		pPacket->m_Logs[Index].ID = GetCurrentThreadId();
		pPacket->m_Logs[Index].Logic = Logic;
		pPacket->m_Logs[Index].Time = timeGetTime();
		pPacket->m_Logs[Index].RefCount = pPacket->m_lRefCount;
		pPacket->m_Logs[Index].SessionID = SessionID;

		return pPacket;
	}

	void CPacket::AddRef(int Logic, SESSION_ID SessionID)
	{
		InterlockedIncrement(&m_lRefCount);

		WORD Index = InterlockedIncrement16(&m_LogIndex);
		Index %= st_LOG::en_LOG_MAX;
		m_Logs[Index].ID = GetCurrentThreadId();
		m_Logs[Index].Logic = Logic;
		m_Logs[Index].Time = timeGetTime();
		m_Logs[Index].RefCount = m_lRefCount;
		m_Logs[Index].SessionID = SessionID;
	}

	void CPacket::SubRef(int Logic, SESSION_ID SessionID)
	{
		//-----------------------------------------------------
		// ���۷��� ī���� ����
		//-----------------------------------------------------
		long lRefCount = InterlockedDecrement(&m_lRefCount);

		WORD Index = InterlockedIncrement16(&m_LogIndex);
		Index %= st_LOG::en_LOG_MAX;
		m_Logs[Index].ID = GetCurrentThreadId();
		m_Logs[Index].Logic = Logic;
		m_Logs[Index].Time = timeGetTime();
		m_Logs[Index].RefCount = m_lRefCount;
		m_Logs[Index].SessionID = SessionID;

		//-----------------------------------------------------
		// �ش� ��Ŷ�� ������� ���� ���ٸ� ���� �� ûũ�� ��ȯ�Ѵ�.
		//-----------------------------------------------------
		if (lRefCount == 0)
		{
			this->Release();
			sm_MemoryPool.Free(this);
		}
	}
#endif

	//////////////////////////////////////////////////////////////////
	// ��Ŷ �޸� �Ҵ� �ޱ�
	//////////////////////////////////////////////////////////////////
	CPacket* CPacket::Alloc(void)
	{
		//PRO_BEGIN(L"CPacket::Alloc()");

		CPacket* pPacket = sm_MemoryPool.Alloc();
		pPacket->Initial();

		//PRO_END(L"CPacket::Alloc()");

		return pPacket;
	}

	//////////////////////////////////////////////////////////////////
	// ���� ī��Ʈ ���
	//////////////////////////////////////////////////////////////////
	void CPacket::AddRef(void)
	{
		//PRO_BEGIN(L"CPacket::AddRef()");

		InterlockedIncrement(&m_lRefCount);

		//PRO_END(L"CPacket::AddRef()");
	}

	//////////////////////////////////////////////////////////////////
	// ���� ī��Ʈ ����
	//////////////////////////////////////////////////////////////////
	void CPacket::SubRef(void)
	{
		//PRO_BEGIN(L"CPacket::SubRef()");

		//-----------------------------------------------------
		// ���۷��� ī���� ����
		//-----------------------------------------------------
		long lRefCount = InterlockedDecrement(&m_lRefCount);

		//-----------------------------------------------------
		// �ش� ��Ŷ�� ������� ���� ���ٸ� ���� �� ûũ�� ��ȯ�Ѵ�.
		//-----------------------------------------------------
		if (lRefCount == 0)
		{
			this->Release();

			//PRO_BEGIN(L"CPacket::SubRef::Free()");
			sm_MemoryPool.Free(this);
			//PRO_END(L"CPacket::SubRef::Free()");
		}

		//PRO_END(L"CPacket::SubRef()");
	}

	//////////////////////////////////////////////////////////////////
	// ������ ���
	//////////////////////////////////////////////////////////////////
	const int CPacket::Get(char* bypDest, const int iLength)
	{
		//----------------------------------------------------
		// ���� ó��
		//----------------------------------------------------
		if (iLength <= 0)
		{
			CCrashDump::Crash();
		}
		if (m_iUseSize < iLength)
		{
			CCrashDump::Crash();
		}

		//PRO_BEGIN(L"CPacket::Get()");

		//----------------------------------------------------
		// �������� �����ŭ ������ ����
		//----------------------------------------------------
		memcpy_s(bypDest, iLength, m_bypReadPos, iLength);

		//----------------------------------------------------
		// �����͸� �о���� ��ŭ �б� ������ �̵�
		//----------------------------------------------------
		m_bypReadPos = m_bypReadPos + iLength;

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�⸦ ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::Get()");

		return iLength;
	}

	const int CPacket::Get_UNI16(wchar_t* bypDest, int iLength)
	{
		//----------------------------------------------------
		// �����ڵ� ũ�⿡ �°Բ� ������ * 2
		//----------------------------------------------------
		iLength = sizeof(wchar_t) * iLength;

		//----------------------------------------------------
		// ���� ó��
		//----------------------------------------------------
		if (iLength <= 0)
		{
			CCrashDump::Crash();
		}
		if (m_iUseSize < iLength)
		{
			CCrashDump::Crash();
		}

		//PRO_BEGIN(L"CPacket::Get_UNI16()");

		//----------------------------------------------------
		// �������� �����ŭ ������ ����
		//----------------------------------------------------
		memcpy_s(bypDest, iLength, m_bypReadPos, iLength);

		//----------------------------------------------------
		// �����͸� �о���� ��ŭ �б� ������ �̵�
		//----------------------------------------------------
		m_bypReadPos = m_bypReadPos + iLength;

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�⸦ ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::Get_UNI16()");

		return iLength;
	}

	//////////////////////////////////////////////////////////////////
	// ������ �ֱ�
	//////////////////////////////////////////////////////////////////
	const int CPacket::Put(char* bypSrc, const int iLength)
	{
		//----------------------------------------------------
		// ���� ó��
		//----------------------------------------------------
		if (iLength <= 0)
		{
			CCrashDump::Crash();
		}
		if (m_iFreeSize < iLength)
		{
			CCrashDump::Crash();
		}

		//PRO_BEGIN(L"CPacket::Put()");

		//----------------------------------------------------
		// ����ȭ���۷� �����ŭ ������ ����
		//----------------------------------------------------
		memcpy_s(m_bypWritePos, iLength, bypSrc, iLength);

		//----------------------------------------------------
		// �����͸� ������ ��ŭ ���� ������ �̵�
		//----------------------------------------------------
		m_bypWritePos = m_bypWritePos + iLength;

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::Put()");

		return iLength;
	}

	const int CPacket::Put_UNI16(const wchar_t* bypSrc, int iLength)
	{
		//----------------------------------------------------
		// �����ڵ� ũ�⿡ �°Բ� ������ * 2
		//----------------------------------------------------
		iLength = sizeof(wchar_t) * iLength;

		//----------------------------------------------------
		// ���� ó��
		//----------------------------------------------------
		if (iLength <= 0)
		{
			CCrashDump::Crash();
		}
		if (m_iFreeSize < iLength)
		{
			CCrashDump::Crash();
		}

		//PRO_BEGIN(L"CPacket::Put_UNI16()");

		//----------------------------------------------------
		// ����ȭ���۷� �����ŭ ������ ����
		//----------------------------------------------------
		memcpy_s(m_bypWritePos, iLength, bypSrc, iLength);

		//----------------------------------------------------
		// �����͸� ������ ��ŭ ���� ������ �̵�
		//----------------------------------------------------
		m_bypWritePos = m_bypWritePos + iLength;

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::Put_UNI16()");

		return iLength;
	}

	//////////////////////////////////////////////////////////////////
	// ������ ������
	//////////////////////////////////////////////////////////////////
	const int CPacket::Peek(char* bypDest, const int iLength)
	{
		//----------------------------------------------------
		// ���� ó��
		//----------------------------------------------------
		if (iLength <= 0)
		{
			CCrashDump::Crash();
		}
		if (m_iUseSize < iLength)
		{
			CCrashDump::Crash();
		}

		//PRO_BEGIN(L"CPacket::Peek()");

		//----------------------------------------------------
		// �������� �����ŭ ������ ����
		//----------------------------------------------------
		memcpy_s(bypDest, iLength, m_bypReadPos, iLength);

		//PRO_END(L"CPacket::Peek()");

		return iLength;
	}

	const int CPacket::Peek_UNI16(wchar_t* bypDest, int iLength)
	{
		//----------------------------------------------------
		// �����ڵ� ũ�⿡ �°Բ� ������ * 2
		//----------------------------------------------------
		iLength = sizeof(wchar_t) * iLength;

		//----------------------------------------------------
		// ���� ó��
		//----------------------------------------------------
		if (iLength <= 0)
		{
			CCrashDump::Crash();
		}
		if (m_iUseSize < iLength)
		{
			CCrashDump::Crash();
		}

		//PRO_BEGIN(L"CPacket::Peek_UNI16()");

		//----------------------------------------------------
		// �������� �����ŭ ������ ����
		//----------------------------------------------------
		memcpy_s(bypDest, iLength, m_bypReadPos, iLength);

		//PRO_END(L"CPacket::Peek_UNI16()");

		return iLength;
	}

	//////////////////////////////////////////////////////////////////
	// ���� ������ �̵�
	//////////////////////////////////////////////////////////////////
	const int CPacket::MoveWritePos(const int iLength)
	{
		//----------------------------------------------------
		// ���� ó��
		//----------------------------------------------------
		if (iLength <= 0)
		{
			CCrashDump::Crash();
		}
		if (iLength >= m_iBufferSize)
		{
			CCrashDump::Crash();
		}

		//PRO_BEGIN(L"CPacket::MoveWritePos()");

		//----------------------------------------------------
		// ���� ������ �̵�
		//----------------------------------------------------
		m_bypWritePos = m_bypWritePos + iLength;

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::MoveWritePos()");

		return iLength;
	}

	//////////////////////////////////////////////////////////////////
	// �б� ������ �̵�
	//////////////////////////////////////////////////////////////////
	const int CPacket::MoveReadPos(const int iLength)
	{
		//----------------------------------------------------
		// ���� ó��
		//----------------------------------------------------
		if (iLength <= 0)
		{
			CCrashDump::Crash();
		}
		if (iLength >= m_iBufferSize)
		{
			CCrashDump::Crash();
		}

		//PRO_BEGIN(L"CPacket::MoveReadPos()");

		//----------------------------------------------------
		// �б� ������ �̵�
		//----------------------------------------------------
		m_bypReadPos = m_bypReadPos + iLength;

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::MoveReadPos()");

		return iLength;
	}

	//////////////////////////////////////////////////////////////////
	// ���� ���� ������ ���
	//////////////////////////////////////////////////////////////////
	char* CPacket::GetBufferPtr(void) const
	{
		return (CHAR*)m_bypBuffer;
	}

	//////////////////////////////////////////////////////////////////
	// �б� ��ġ ���
	//////////////////////////////////////////////////////////////////
	char* CPacket::GetReadPos(void) const
	{
		return m_bypReadPos;
	}

	//////////////////////////////////////////////////////////////////
	// ���� ��ġ ���
	//////////////////////////////////////////////////////////////////
	char* CPacket::GetWritePos(void) const
	{
		return m_bypWritePos;
	}

	//////////////////////////////////////////////////////////////////
	// ��� ���� ������ ���
	//////////////////////////////////////////////////////////////////
	const int CPacket::GetFreeSize(void) const
	{
		return m_iFreeSize;
	}

	//////////////////////////////////////////////////////////////////
	// ������� ������ ���
	//////////////////////////////////////////////////////////////////
	const int CPacket::GetUseSize(void) const
	{
		return m_iUseSize;
	}

	//////////////////////////////////////////////////////////////////
	// ��� ũ�� ������ ��ü ��Ŷ ������ ���
	//////////////////////////////////////////////////////////////////
	const int CPacket::GetPacketSize(void) const
	{
		return eHEADER_LEN + m_iUseSize;
	}

	//////////////////////////////////////////////////////////////////
	// �޸�Ǯ �뷮 ���
	//////////////////////////////////////////////////////////////////
	const long CPacket::GetPoolCapacity(void)
	{
		return sm_MemoryPool.GetCapacity();
	}

	//////////////////////////////////////////////////////////////////
	// �޸�Ǯ ������� ũ�� ���
	//////////////////////////////////////////////////////////////////
	const long CPacket::GetPoolUseSize(void)
	{
		return sm_MemoryPool.GetUseCount();
	}

	//////////////////////////////////////////////////////////////////
	// ��ȣȭ
	//////////////////////////////////////////////////////////////////
	void CPacket::Encode(void)
	{
		//----------------------------------------------------
		// �̹� ���ڵ��� �Ǿ��ִٸ� ����
		//----------------------------------------------------
		if (true == (bool)InterlockedExchange8((CHAR*)&m_bEncode, true))
		{
			return;
		}

		//PRO_BEGIN(L"CPacket::Encode()");

		//----------------------------------------------------
		// 1. ��� ���� �� ���ۿ� �ֱ�
		//----------------------------------------------------
		st_PACKET_HEADER Header;
		Header.Code = sm_byPacketCode;
		Header.Len = GetUseSize();
		Header.RandKey = (rand() % 65536);
		Header.CheckSum = MakeChecksum();
		memcpy_s(m_bypBuffer, eHEADER_LEN, (void*)&Header, eHEADER_LEN);

		//----------------------------------------------------
		// 2. ���ڵ� ����
		//----------------------------------------------------
		unsigned char P = 0;
		unsigned char E = 0;
		int iLen = 1 + GetUseSize(); // Checksum + Payload

		for (int i = 0; i < iLen; i++)
		{
			//----------------------------------------------------
			// ó�� P�� E�� 0, 0 �̰� �� �� ���ڵ� �� �������ʹ�
			// ������ P�� E�� ������ ���ڵ��Ѵ�.
			//----------------------------------------------------
			P = (m_bypReadPos - 1)[i] ^ (P + Header.RandKey + (i + 1));
			E = P ^ (E + sm_byPacketKey + (i + 1));

			//----------------------------------------------------
			// ���ڵ��� ����� ���ۿ� �����
			//----------------------------------------------------
			(m_bypReadPos - 1)[i] = E;
		}

		//PRO_END(L"CPacket::Encode()");

	}

	//////////////////////////////////////////////////////////////////
	// ��ȣȭ
	//////////////////////////////////////////////////////////////////
	bool CPacket::Decode(void)
	{
		//----------------------------------------------------
		// 1. ������ �������� ����� �ؼ�
		//----------------------------------------------------
		st_PACKET_HEADER* pHeader = (st_PACKET_HEADER*)m_bypBuffer;

		//----------------------------------------------------
		// 2. ��Ŷ �ڵ� Ȯ��
		//----------------------------------------------------
		if (pHeader->Code != sm_byPacketCode)
		{
			return false;
		}

		//PRO_BEGIN(L"CPacket::Decode()");

		//----------------------------------------------------
		// 3. ���ڵ�
		//----------------------------------------------------
		unsigned char D = 0;
		unsigned char P = 0;
		unsigned char E = 0;
		unsigned char _E = 0;			// ������ �����
		unsigned char _P = 0;			// ������ �����
		int iLen = GetUseSize() + 1;	// Checksum + Payload

		for (int i = 0; i < iLen; i++)
		{
			E = (m_bypReadPos - 1)[i];

			P = E ^ (_E + sm_byPacketKey + (i + 1));	// P = E ^ FK
			D = P ^ (_P + pHeader->RandKey + (i + 1));	// D = P ^ RK

			_E = (m_bypReadPos - 1)[i];					// ���� E ����		
			_P = P;										// ���� P ����

			(m_bypReadPos - 1)[i] = D;
		}

		//----------------------------------------------------
		// 4. üũ�� ���� �� ��
		//----------------------------------------------------
		unsigned char byCheckSum = MakeChecksum();
		if (byCheckSum != pHeader->CheckSum)
		{
			return false;
		}

		//----------------------------------------------------
		// 5. ���ڵ� ���� ����
		//----------------------------------------------------
		m_bEncode = false;

		//PRO_END(L"CPacket::Decode()");

		return true;
	}

	//////////////////////////////////////////////////////////////////
	// üũ�� ����
	//////////////////////////////////////////////////////////////////
	unsigned char CPacket::MakeChecksum(void)
	{
		//PRO_BEGIN(L"CPacket::MakeChecksum()");

		int iSum = 0;
		int iLen = GetUseSize();

		//----------------------------------------------------
		// ���̷ε带 ���� ���� �� % 256
		//----------------------------------------------------
		for (int i = 0; i < iLen; i++)
		{
			iSum += m_bypReadPos[i];
		}

		//PRO_END(L"CPacket::MakeChecksum()");

		return (unsigned char)(iSum % 256);
	}

	//////////////////////////////////////////////////////////////////
	// ������ �����ε�
	//////////////////////////////////////////////////////////////////
	CPacket& CPacket::operator<<(const BYTE src)
	{
		//PRO_BEGIN(L"CPacket::<< BYTE");

		if (m_iFreeSize < sizeof(BYTE))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���ۿ� ������ ����
		//----------------------------------------------------
		*m_bypWritePos = src;
		m_bypWritePos = m_bypWritePos + sizeof(BYTE);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::<< BYTE");

		return *this;
	}

	CPacket& CPacket::operator<<(const char src)
	{
		//PRO_BEGIN(L"CPacket::<<char");

		if (m_iFreeSize < sizeof(char))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���ۿ� ������ ����
		//----------------------------------------------------
		*m_bypWritePos = src;
		m_bypWritePos = m_bypWritePos + sizeof(char);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::<<char");

		return *this;
	}

	CPacket& CPacket::operator<<(const WORD src)
	{
		//PRO_BEGIN(L"CPacket::<<WORD");

		if (m_iFreeSize < sizeof(WORD))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���ۿ� ������ ����
		//----------------------------------------------------
		*((WORD*)m_bypWritePos) = src;
		m_bypWritePos = m_bypWritePos + sizeof(WORD);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::<<WORD");

		return *this;
	}

	CPacket& CPacket::operator<<(const short src)
	{
		//PRO_BEGIN(L"CPacket::<<short");

		if (m_iFreeSize < sizeof(short))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���ۿ� ������ ����
		//----------------------------------------------------
		*((short*)m_bypWritePos) = src;
		m_bypWritePos = m_bypWritePos + sizeof(short);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::<<short");

		return *this;
	}

	CPacket& CPacket::operator<<(const DWORD src)
	{
		//PRO_BEGIN(L"CPacket::<<DWORD");

		if (m_iFreeSize < sizeof(DWORD))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���ۿ� ������ ����
		//----------------------------------------------------
		*((DWORD*)m_bypWritePos) = src;
		m_bypWritePos = m_bypWritePos + sizeof(DWORD);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::<<DWORD");

		return *this;
	}

	CPacket& CPacket::operator<<(const UINT src)
	{
		//PRO_BEGIN(L"CPacket::<<UINT");

		if (m_iFreeSize < sizeof(UINT))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���ۿ� ������ ����
		//----------------------------------------------------
		*((UINT*)m_bypWritePos) = src;
		m_bypWritePos = m_bypWritePos + sizeof(UINT);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::<<UINT");

		return *this;
	}

	CPacket& CPacket::operator<<(const int src)
	{
		//PRO_BEGIN(L"CPacket::<<int");

		if (m_iFreeSize < sizeof(int))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���ۿ� ������ ����
		//----------------------------------------------------
		*((int*)m_bypWritePos) = src;
		m_bypWritePos = m_bypWritePos + sizeof(int);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::<<int");

		return *this;
	}

	CPacket& CPacket::operator<<(const long src)
	{
		//PRO_BEGIN(L"CPacket::<<long");

		if (m_iFreeSize < sizeof(long))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���ۿ� ������ ����
		//----------------------------------------------------
		*((long*)m_bypWritePos) = src;
		m_bypWritePos = m_bypWritePos + sizeof(long);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::<<long");

		return *this;
	}

	CPacket& CPacket::operator<<(const float src)
	{
		//PRO_BEGIN(L"CPacket::<<float");

		if (m_iFreeSize < sizeof(float))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���ۿ� ������ ����
		//----------------------------------------------------
		*((float*)m_bypWritePos) = src;
		m_bypWritePos = m_bypWritePos + sizeof(float);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::<<float");

		return *this;
	}

	CPacket& CPacket::operator<<(const __int64 src)
	{
		//PRO_BEGIN(L"CPacket::<<__int64");

		if (m_iFreeSize < sizeof(__int64))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���ۿ� ������ ����
		//----------------------------------------------------
		*((__int64*)m_bypWritePos) = src;
		m_bypWritePos = m_bypWritePos + sizeof(__int64);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::<<__int64");

		return *this;
	}

	CPacket& CPacket::operator<<(const double src)
	{
		//PRO_BEGIN(L"CPacket::<<double");

		if (m_iFreeSize < sizeof(double))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���ۿ� ������ ����
		//----------------------------------------------------
		*((double*)m_bypWritePos) = src;
		m_bypWritePos = m_bypWritePos + sizeof(double);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_BEGIN(L"CPacket::<<double");

		return *this;
	}

	CPacket& CPacket::operator>>(BYTE& dest)
	{
		//PRO_BEGIN(L"CPacket::>>BYTE");

		if (m_iUseSize < sizeof(BYTE))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���� �����͸� �ܺη� ����
		//----------------------------------------------------
		dest = *m_bypReadPos;
		m_bypReadPos = m_bypReadPos + sizeof(BYTE);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::>>BYTE");

		return *this;
	}

	CPacket& CPacket::operator>>(char& dest)
	{
		//PRO_BEGIN(L"CPacket::>>char");

		if (m_iUseSize < sizeof(char))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���� �����͸� �ܺη� ����
		//----------------------------------------------------
		dest = *m_bypReadPos;
		m_bypReadPos = m_bypReadPos + sizeof(char);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::>>char");

		return *this;
	}

	CPacket& CPacket::operator>>(short& dest)
	{
		//PRO_BEGIN(L"CPacket::>>short");

		if (m_iUseSize < sizeof(short))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���� �����͸� �ܺη� ����
		//----------------------------------------------------
		dest = *((short*)m_bypReadPos);
		m_bypReadPos = m_bypReadPos + sizeof(short);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::>>short");

		return *this;
	}

	CPacket& CPacket::operator>>(WORD& dest)
	{
		//PRO_BEGIN(L"CPacket::>>WORD");

		if (m_iUseSize < sizeof(WORD))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���� �����͸� �ܺη� ����
		//----------------------------------------------------
		dest = *((WORD*)m_bypReadPos);
		m_bypReadPos = m_bypReadPos + sizeof(WORD);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::>>WORD");

		return *this;
	}

	CPacket& CPacket::operator>>(UINT& dest)
	{
		//PRO_BEGIN(L"CPacket::>>UINT");

		if (m_iUseSize < sizeof(UINT))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���� �����͸� �ܺη� ����
		//----------------------------------------------------
		dest = *((UINT*)m_bypReadPos);
		m_bypReadPos = m_bypReadPos + sizeof(UINT);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::>>UINT");

		return *this;
	}

	CPacket& CPacket::operator>>(int& dest)
	{
		//PRO_BEGIN(L"CPacket::>>int");

		if (m_iUseSize < sizeof(int))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���� �����͸� �ܺη� ����
		//----------------------------------------------------
		dest = *((int*)m_bypReadPos);
		m_bypReadPos = m_bypReadPos + sizeof(int);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::>>int");

		return *this;
	}

	CPacket& CPacket::operator>>(DWORD& dest)
	{
		//PRO_BEGIN(L"CPacket::>>DWORD");

		if (m_iUseSize < sizeof(DWORD))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���� �����͸� �ܺη� ����
		//----------------------------------------------------
		dest = *((DWORD*)m_bypReadPos);
		m_bypReadPos = m_bypReadPos + sizeof(DWORD);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::>>DWORD");

		return *this;
	}

	CPacket& CPacket::operator>>(long& dest)
	{
		//PRO_BEGIN(L"CPacket::>>long");

		if (m_iUseSize < sizeof(long))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���� �����͸� �ܺη� ����
		//----------------------------------------------------
		dest = *((long*)m_bypReadPos);
		m_bypReadPos = m_bypReadPos + sizeof(long);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::>>long");

		return *this;
	}

	CPacket& CPacket::operator>>(float& dest)
	{
		//PRO_BEGIN(L"CPacket::>>float");

		if (m_iUseSize < sizeof(float))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���� �����͸� �ܺη� ����
		//----------------------------------------------------
		dest = *((float*)m_bypReadPos);
		m_bypReadPos = m_bypReadPos + sizeof(float);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::>>float");

		return *this;
	}

	CPacket& CPacket::operator>>(double& dest)
	{
		//PRO_BEGIN(L"CPacket::>>double");

		if (m_iUseSize < sizeof(double))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���� �����͸� �ܺη� ����
		//----------------------------------------------------
		dest = *((double*)m_bypReadPos);
		m_bypReadPos = m_bypReadPos + sizeof(double);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::>>double");

		return *this;
	}

	CPacket& CPacket::operator>>(__int64& dest)
	{
		//PRO_BEGIN(L"CPacket::>>__in64");

		if (m_iUseSize < sizeof(__int64))
		{
			return *this;
		}

		//----------------------------------------------------
		// �Ķ���� �����ŭ ����ȭ ���� �����͸� �ܺη� ����
		//----------------------------------------------------
		dest = *((__int64*)m_bypReadPos);
		m_bypReadPos = m_bypReadPos + sizeof(__int64);

		//----------------------------------------------------
		// ������� ũ��� ��� ���� ũ�� ����
		//----------------------------------------------------
		m_iUseSize = (int)(m_bypWritePos - m_bypReadPos);
		m_iFreeSize = ePAYLOAD_LEN - m_iUseSize;

		//PRO_END(L"CPacket::>>__in64");

		return *this;
	}

	////////////////////////////////////////////////////////////////////
	// LanServer ����
	////////////////////////////////////////////////////////////////////
	char* CPacket::GetLanHeaderPtr(void) const
	{
		return (CHAR*)m_bypBuffer + 3;
	}

	const int CPacket::GetLanPacketSize(void) const
	{
		return 2 + m_iUseSize;
	}

	void CPacket::SetLanHeader(void)
	{
		short shPayloadLen = GetUseSize();
		*((short*)&m_bypBuffer[3]) = shPayloadLen;

		//memcpy_s(m_bypBuffer + 3, sizeof(short), &shPayloadLen, sizeof(short));
	}

	//////////////////////////////////////////////////////////////////
	// ������
	//////////////////////////////////////////////////////////////////
	CPacket::CPacket()
	{
		if (m_bInitialize == false)
		{
			m_bInitialize = true;
		}
		else
		{
			CCrashDump::Crash();
		}
#ifdef __MULTI_THREAD_DEBUG_MODE__
		m_iIndex = -1;
		m_LogIndex = -1;
#endif
	}

	//////////////////////////////////////////////////////////////////
	// �Ҹ���
	//////////////////////////////////////////////////////////////////
	CPacket::~CPacket()
	{

	}

	//////////////////////////////////////////////////////////////////
	// ��Ŷ �ʱ�ȭ
	//////////////////////////////////////////////////////////////////
	void CPacket::Initial(void)
	{
		//PRO_BEGIN(L"CPacket::Initial()");

		m_bEncode		= false;
		//memset(m_bypBuffer, 0, eHEADER_LEN + ePAYLOAD_LEN);
		m_bypWritePos	= m_bypBuffer + eHEADER_LEN;	// ��� ���� ����
		m_bypReadPos	= m_bypBuffer + eHEADER_LEN;	// ��� ���� ����
		m_iBufferSize	= eHEADER_LEN + ePAYLOAD_LEN;	// ��� + ���̷ε�
		m_iFreeSize		= ePAYLOAD_LEN;					// ���̷ε� ������
		m_iUseSize		= 0;
		m_lRefCount		= 1;

		//PRO_END(L"CPacket::Initial()");

#ifdef __MULTI_THREAD_DEBUG_MODE__
		if (m_iIndex == -1)
		{
			m_iIndex = (WORD)InterlockedIncrement16(&sm_AllocPacketIndex);
			sm_AllocPacket[m_iIndex] = this;
		}
#endif
	}

	//////////////////////////////////////////////////////////////////
	// ��Ŷ ����
	//////////////////////////////////////////////////////////////////
	void CPacket::Release(void)
	{
		//PRO_BEGIN(L"CPacket::Release()");

		m_bEncode = false;
		memset(m_bypBuffer, 0, eHEADER_LEN + ePAYLOAD_LEN);
		m_bypWritePos = m_bypBuffer + eHEADER_LEN;
		m_bypReadPos = m_bypBuffer + eHEADER_LEN;
		m_iFreeSize = ePAYLOAD_LEN;
		m_iUseSize = 0;

		//PRO_END(L"CPacket::Release()");
	}

#ifdef __MULTI_THREAD_DEBUG_MODE__

	SESSION_ID CPacket::OutputDebugData(void)
	{
		for (int i = 0; i < USHRT_MAX; i++)
		{
			if (sm_AllocPacket[i] == nullptr)
			{
				break;
			}

			if (sm_AllocPacket[i]->m_lRefCount != 0)
			{
				//CCrashDump::Crash();
				return sm_AllocPacket[i]->m_Logs[sm_AllocPacket[i]->m_LogIndex].SessionID;
			}
		}
	}

#endif

	//////////////////////////////////////////////////////////////////
	// ��Ŷ ����Ű ����
	//////////////////////////////////////////////////////////////////
	void CPacket::SetPacketKey(unsigned char byKey)
	{
		sm_byPacketKey = byKey;
	}

	//////////////////////////////////////////////////////////////////
	// ��Ŷ �ڵ� ����
	//////////////////////////////////////////////////////////////////
	void CPacket::SetPacketCode(unsigned char byCode)
	{
		sm_byPacketCode = byCode;
	}
}