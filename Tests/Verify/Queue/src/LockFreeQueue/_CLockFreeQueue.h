#pragma once
template <typename T>
class CLockFreeQueue
{
private:
	struct st_NODE
	{
		T Data;
		st_NODE* pNextNode;
	};

	struct alignas(16) st_CONTAINER
	{
		st_NODE* pNode;
		unsigned __int64 Key;
	};

	st_CONTAINER* m_pHead;
	st_CONTAINER* m_pTail;
	long m_lCapacity;

public:
	CMemoryPool<st_NODE> m_MemPool = CMemoryPool<st_NODE>(0);

public:
	CLockFreeQueue()
	{
		m_lCapacity = 0;

		m_pHead = (st_CONTAINER*)_aligned_malloc(sizeof(st_CONTAINER), 16);
		m_pHead->Key = 0;
		m_pHead->pNode = m_MemPool.Alloc();
		m_pHead->pNode->pNextNode = NULL;

		m_pTail = (st_CONTAINER*)_aligned_malloc(sizeof(st_CONTAINER), 16);
		m_pTail->Key = 0;
		m_pTail->pNode = m_pHead->pNode;
		m_pTail->pNode->pNextNode = NULL;
	}

	void Enqueue(T Source)
	{
		st_NODE* pNewNode = m_MemPool.Alloc();
		pNewNode->Data = Source;
		pNewNode->pNextNode = NULL;

		st_CONTAINER Tail;
		st_NODE* pNext;

		while (TRUE)
		{
			Tail.Key = m_pTail->Key;
			Tail.pNode = m_pTail->pNode;
			pNext = Tail.pNode->pNextNode;

			if (pNext == NULL)
			{
				if (InterlockedCompareExchange64((volatile LONG64*)&m_pTail->pNode->pNextNode, (LONG64)pNewNode, (LONG64)pNext) == (LONG64)pNext)
				{
					InterlockedCompareExchange128((volatile LONG64*)m_pTail, (LONG64)(Tail.Key + 1), (LONG64)(pNewNode), (LONG64*)&Tail);
					break;
				}
			}
			else
			{
				InterlockedCompareExchange128((volatile LONG64*)m_pTail, (LONG64)(Tail.Key + 1), (LONG64)(pNext), (LONG64*)&Tail);
			}
		}

		InterlockedIncrement(&m_lCapacity);
	}

	BOOL Dequeue(T* pDestination)
	{
		if (0 > InterlockedDecrement(&m_lCapacity))
		{
			InterlockedIncrement(&m_lCapacity);
			return FALSE;
		}

		st_CONTAINER Head;
		st_CONTAINER Tail;
		st_NODE* pNext;

		while (TRUE)
		{
			Head.Key = m_pHead->Key;
			Head.pNode = m_pHead->pNode;
			pNext = Head.pNode->pNextNode;

			if (pNext == NULL)
			{
				continue;
			}
			else
			{
				Tail.Key = m_pTail->Key;
				Tail.pNode = m_pTail->pNode;
				if (Tail.pNode->pNextNode != NULL)
				{
					if (InterlockedCompareExchange128((volatile LONG64*)m_pTail, (LONG64)(Tail.Key + 1), (LONG64)(Tail.pNode->pNextNode), (LONG64*)&Tail) == FALSE)
					{
						continue;
					}
				}
			}

			*pDestination = pNext->Data;
			if (InterlockedCompareExchange128((volatile LONG64*)m_pHead, (LONG64)(Head.Key + 1), (LONG64)(pNext), (LONG64*)&Head) == TRUE)
			{
				m_MemPool.Free(Head.pNode);
				return TRUE;
			}
		}
	}

};