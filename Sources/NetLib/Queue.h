#pragma once
#include "MemoryPool.h"

namespace covEngine
{
	template <typename T>
	class Queue
	{
	public:
		struct st_NODE
		{
			T			Data;
			st_NODE*	pNextNode;
		};

	public:
		Queue()
		{
			m_pMemoryPool = (MemoryPool<st_NODE>*)_aligned_malloc(sizeof(MemoryPool<st_NODE>), en_CACHE_ALIGN);
			new (m_pMemoryPool) MemoryPool<st_NODE>(0);

			m_pHead = m_pMemoryPool->Alloc();
			m_pHead->pNextNode = NULL;

			m_pTail = m_pHead;
			m_pTail->pNextNode = NULL;

			m_Capacity = 0;
		}

		~Queue()
		{
			m_pMemoryPool->Free(m_pHead);
			_aligned_free(m_pMemoryPool);
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
			*pDestination = m_pHead->pNextNode->Data;
			m_pHead = m_pHead->pNextNode;

			m_pMemoryPool->Free(pHeadTemp);
		}

		long GetCapacity(void)
		{
			return m_Capacity;
		}

	private:
		MemoryPool<st_NODE>* m_pMemoryPool;
		st_NODE* m_pHead;
		st_NODE* m_pTail;
		long	 m_Capacity;
	};
}