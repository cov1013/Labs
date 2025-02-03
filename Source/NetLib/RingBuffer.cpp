#include "RingBuffer.h"

namespace cov1013
{
	RingBuffer::RingBuffer(const int inCapacity)
	{
		assert(inCapacity > 0);

		m_pBuffer = new BYTE[inCapacity];
		m_Capacity = sizeof(BYTE) * inCapacity;
	}

	RingBuffer::~RingBuffer()
	{
		assert(m_pBuffer != nullptr);

		delete[] m_pBuffer;
	}

	const bool RingBuffer::IsEmpty() const
	{
		int Snap_Read = m_Read;
		int Snap_Write = m_Write;
		assert(Snap_Read >= 0);
		assert(Snap_Write >= 0);

		if (Snap_Read == Snap_Write)
		{
			return true;
		}
		return false;
	}

	const bool RingBuffer::IsFull() const
	{
		if (GetWritableLength() <= 0)
		{
			return true;
		}
		return false;
	}

	const int RingBuffer::GetReadableLength() const
	{
		int Snap_Read = m_Read;
		int Snap_Write = m_Write;
		assert(Snap_Read >= 0);
		assert(Snap_Write >= 0);

		if (Snap_Write < Snap_Read)
		{
			//-------------------------------------
			//#######W            R################
			//-------------------------------------
			return (m_Capacity - (Snap_Read - Snap_Write));
		}
		else
		{
			//-------------------------------------
			//	    R#############W
			//-------------------------------------
			return Snap_Write - Snap_Read;
		}
	}

	const int RingBuffer::GetWritableLength() const
	{
		int Snap_Read = m_Read;
		int Snap_Write = m_Write;
		assert(Snap_Read >= 0);
		assert(Snap_Write >= 0);

		if (Snap_Write < Snap_Read)
		{
			//-------------------------------------
			//        W###########%R
			//-------------------------------------
			return (Snap_Read - Snap_Write) - Config::eBlankSize;
		}
		else
		{
			//-------------------------------------
			//###%R             W##################
			//-------------------------------------
			return (m_Capacity - (Snap_Write - Snap_Read)) - Config::eBlankSize;
		}
	}

	const int RingBuffer::GetSerialWritableLength() const
	{
		int Snap_Read = m_Read;
		int Snap_Write = m_Write;
		assert(Snap_Read >= 0);
		assert(Snap_Write >= 0);

		if (Snap_Write < Snap_Read)
		{
			//-------------------------------------
			//      W############%R               
			//-------------------------------------
			return (Snap_Read - Snap_Write) - Config::eBlankSize;
		}
		else
		{
			//---------------------------------------------------
			//ReadPos가 시작점에 있다면, 맨 마지막 버퍼를 비워야한다.
			//---------------------------------------------------
			if (Snap_Read == 0)
			{
				//-------------------------------------
				//R          W########################%
				//-------------------------------------
				return (m_Capacity - Snap_Write) - Config::eBlankSize;
			}
			//---------------------------------------------------
			//ReadPos가 시작점이 아니라면, 남은 버퍼 공간이 전부 사용 가능 공간.
			//---------------------------------------------------
			else
			{
				//-------------------------------------
				//        R       W####################
				//-------------------------------------
				return (m_Capacity - Snap_Write);
			}
		}
	}

	const int RingBuffer::GetSerialReadableLength() const
	{
		int Snap_Read = m_Read;
		int Snap_Write = m_Write;
		assert(Snap_Read >= 0);
		assert(Snap_Write >= 0);

		if (Snap_Write < Snap_Read)
		{
			//-------------------------------------
			//    W      R#########################
			//-------------------------------------
			return m_Capacity - Snap_Read;
		}
		else
		{
			//-------------------------------------
			//     R#############W
			//-------------------------------------
			return Snap_Write - Snap_Read;
		}
	}

	int RingBuffer::Write(const BYTE* pSource, const int inLength)
	{
		// !!! Write 함수 동작 시 ReadPos가 움직일 수 있다. 

		////////////////////////////////////////////////////////////////
		// Multi Thread Case 1.
		// W가 R보다 작다면, WritableLenth보다 SerialWritableLenth가 크게 나올 수 있다.
		//---------------------------------
		//       W[##########]%R          |		1. WritableLenth
		//---------------------------------
		//       W##########-->>%R        |		2. Move ReadPop
		//---------------------------------
		//       W[#################]%R   |		3. SerialWritableLenth 
		//---------------------------------
		////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////
		// Multi Thread Case 2.
		// W가 R보다 크다면, WritableLenth보다 SerialWritableLenth가 크게 나올 수 없다.
		// 
		//---------------------------------
		//####]%R          W[#############|	    1. WritableLenth
		//---------------------------------
		//#####-->>>%R     W##############|	    2. Move ReadPop
		//---------------------------------
		//            %R   W[############]|		3. SerialWritableLenth 
		//---------------------------------
		////////////////////////////////////////////////////////////////
		if (inLength <= 0)
		{
			return 0;
		}

		const int WritableLegnth = GetWritableLength();
		if (WritableLegnth <= 0)
		{
			return 0;
		}

		int writeLength = inLength;
		if (WritableLegnth < inLength)
		{
			writeLength = WritableLegnth;
		}

		const int SerialWritableLength = GetSerialWritableLength();
		if (writeLength <= SerialWritableLength)
		{
			memcpy(m_pBuffer + m_Write, pSource, writeLength);
		}
		else
		{
			memcpy(m_pBuffer + m_Write, pSource, SerialWritableLength);
			memcpy(m_pBuffer, pSource + SerialWritableLength, static_cast<size_t>(writeLength) - SerialWritableLength);
		}

		return MoveWritePos(writeLength);
	}

	int RingBuffer::Read(BYTE* pDestination, const int inLength, bool bIsPeek)
	{
		// !!! Read 함수 동작 시 WritePos가 움직일 수 있다. 

		////////////////////////////////////////////////////////////////
		// Multi Thread Case 1.
		// R가 W보다 작다면, ReadableLenth보다 SerialReadableLenth가 크게 나올 수 있다.
		//---------------------------------
		//       [R#########]W            |		1. ReadableLenth
		//---------------------------------
		//       R###########-->>W        |		2. Move WritePos
		//---------------------------------
		//       R[################]W     |		3. SerialReadableLenth
		//---------------------------------
		////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////
		// Multi Thread Case 2.
		// R가 W보다 크다면, WritableLenth보다 SerialReadableLenth가 크게 나올 수 없다.
		// 
		//---------------------------------
		//#####]W          R[#############|	    1. ReadableLenth
		//---------------------------------
		//#####-->>>W      R##############|	    2. Move ReadPop
		//---------------------------------
		//            W    R[############]|		3. SerialReadableLenth 
		//---------------------------------
		////////////////////////////////////////////////////////////////

		if (inLength <= 0)
		{
			return 0;
		}

		const int ReadableLength = GetReadableLength();
		if (ReadableLength <= 0)
		{
			return 0;
		}

		int readLength = inLength;
		if (ReadableLength < inLength)
		{
			readLength = ReadableLength;
		}

		const int SerialReadableLength = GetSerialReadableLength();
		if (readLength <= SerialReadableLength)
		{
			memcpy(pDestination, m_pBuffer + m_Read, readLength);
		}
		else
		{
			memcpy(pDestination, m_pBuffer + m_Read, SerialReadableLength);
			memcpy(pDestination + SerialReadableLength, m_pBuffer, static_cast<size_t>(readLength) - SerialReadableLength);
		}

		if (bIsPeek == false)
		{
			return MoveReadPos(readLength);
		}
		return readLength;
	}

	int RingBuffer::Peek(BYTE* pDestination, const int inLength)
	{
		return Read(pDestination, inLength, true);
	}

	int RingBuffer::MoveWritePos(const int inLength)
	{
		assert(m_Write >= 0);
		m_Write = (m_Write + inLength) % m_Capacity;
		return inLength;
	}

	int RingBuffer::MoveReadPos(const int inLength)
	{
		assert(m_Read >= 0);
		m_Read = (m_Read + inLength) % m_Capacity;
		return inLength;
	}

	void RingBuffer::Clear()
	{
		m_Read = 0;
		m_Write = 0;
	}
}