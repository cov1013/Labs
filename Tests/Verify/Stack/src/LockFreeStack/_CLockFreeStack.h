#pragma once
#include "CMemoryPool.h"

template <typename DATA>
class CLockFreeStack
{
private:
	struct st_NODE
	{
		DATA Data;
		st_NODE* pNextNode;
	};

	struct alignas(16) st_TOP
	{
		st_NODE* pNode;
		unsigned __int64 Key;
	};

	st_TOP*	m_pTop;
	long	m_lCapacity;			

public:
	CMemoryPool<st_NODE> m_MemoryPool = CMemoryPool<st_NODE>(0);

public:

	CLockFreeStack()
	{
		m_lCapacity = 0;

		m_pTop = (st_TOP*)_aligned_malloc(sizeof(st_TOP), 16);
		m_pTop->Key = 0;
		m_pTop->pNode = NULL;
	}

	~CLockFreeStack()
	{
		_aligned_free(m_pTop);
	}

	void Push(DATA Data)
	{
		st_NODE* pNewNode = m_MemoryPool.Alloc();
		pNewNode->Data = Data;

		st_NODE* pNode;
		do
		{
			pNode = m_pTop->pNode;
			pNewNode->pNextNode = pNode;
		} while (InterlockedCompareExchange64((volatile LONG64*)m_pTop, (LONG64)pNewNode, (LONG64)pNode) != (LONG64)pNode);

		InterlockedIncrement(&m_lCapacity);
	}

	BOOL Pop(DATA* pDest)
	{
		if (0 > InterlockedDecrement(&m_lCapacity))
		{
			InterlockedIncrement(&m_lCapacity);
			return FALSE;
		}

		st_TOP Top;
		do {
			Top.Key = m_pTop->Key;
			Top.pNode = m_pTop->pNode;
		} while (!InterlockedCompareExchange128((volatile LONG64*)m_pTop, (LONG64)(Top.Key + 1), (LONG64)Top.pNode->pNextNode, (LONG64*)&Top));

		*pDest = Top.pNode->Data;
		m_MemoryPool.Free(Top.pNode);

		return TRUE;
	}
};