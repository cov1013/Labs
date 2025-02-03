#pragma once	
#include <assert.h>
#include <windows.h>

namespace cov1013
{
	class RingBuffer
	{
		enum Config : int
		{
			eBlankSize = 8
		};

	public:
		RingBuffer(const int inCapacity);
		~RingBuffer();
		const int Capacity() const { return m_Capacity; };
		BYTE* GetReadPtr() const { return m_pBuffer + m_Read; };
		BYTE* GetWritePtr() const { return m_pBuffer + m_Write; };
		BYTE* GetBufferPtr() const { return m_pBuffer; };

		void Clear();

		const bool IsFull() const;
		const bool IsEmpty() const;

		const int GetReadableLength() const;
		const int GetWritableLength() const;
		const int GetSerialReadableLength() const;
		const int GetSerialWritableLength() const;

		int Write(const BYTE* pSource, const int inLength);
		int Read(BYTE* pDestination, const int inLength, bool bIsPeek = false);
		int Peek(BYTE* pDestination, const int inLength);

	private:
		int MoveWritePos(const int inLength);
		int MoveReadPos(const int inLength);

	private:
		int		m_Capacity = 0;
		int		m_Read = 0;
		int		m_Write = 0;
		BYTE*	m_pBuffer = nullptr;
	};
}


