#pragma once
#include "CMemoryPool.h"

namespace cov1013
{
	template <typename T>
	class CQueue
	{
	public :
		struct st_NODE
		{
			T			Data;
			st_NODE*	pNextNode;
		};

	public:
		CQueue()
		{
			m_pMemoryPool = (CMemoryPool<st_NODE>*)_aligned_malloc(sizeof(CMemoryPool<st_NODE>), en_CACHE_ALIGN);
			new (m_pMemoryPool) CMemoryPool<st_NODE>(0);

			m_pHead = m_pMemoryPool->Alloc();
			m_pHead->pNextNode = NULL;

			m_pTail = m_pHead;
			m_pTail->pNextNode = NULL;

			m_Capacity = 0;

			InitializeCriticalSection(&m_cs);
		}

		~CQueue()
		{
			m_pMemoryPool->Free(m_pHead);
			_aligned_free(m_pMemoryPool);

			DeleteCriticalSection(&m_cs);
		}

		void Enqueue(T data)
		{
			st_NODE* pNewNode = m_pMemoryPool->Alloc();
			pNewNode->Data = data;
			pNewNode->pNextNode = NULL;

			m_pTail->pNextNode = pNewNode;
			m_pTail = pNewNode;

			InterlockedIncrement(&m_Capacity);
		}

		bool Dequeue(T* pDestination)
		{
			if (0 > InterlockedDecrement(&m_Capacity))
			{
				InterlockedIncrement(&m_Capacity);
				return FALSE;
			}

			st_NODE* pHeadTemp = m_pHead;
			*pDestination = m_pHead->pNextNode->Data;	// ÀÐ±â
			m_pHead = m_pHead->pNextNode;
			
			m_pMemoryPool->Free(pHeadTemp);
		}

		void Lock(void)
		{
			//while (true == (bool)InterlockedExchange8((CHAR*)&m_bResourceInUse, true)) {};
			//AcquireSRWLockExclusive(&m_srw);
			EnterCriticalSection(&m_cs);
		}

		void Unlock(void)
		{
			//m_bResourceInUse = false;
			//ReleaseSRWLockExclusive(&m_srw);
			LeaveCriticalSection(&m_cs);
		}

	public:
		CMemoryPool<st_NODE>*	m_pMemoryPool;

	private:
		st_NODE*				m_pHead;
		st_NODE*				m_pTail; 
		long					m_Capacity;

		alignas(64)
		SRWLOCK					m_srw;
		CRITICAL_SECTION		m_cs;
		bool					m_bResourceInUse;
	};
}