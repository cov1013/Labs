#pragma once

#include <new.h>

template <class DATA>
class CMemoryPool
{
private:
	struct st_BLOCK_NODE
	{
		unsigned __int64	FrontGuard;
		DATA				Data;
		unsigned __int64	RearGuard;
		st_BLOCK_NODE*		pNextNode;
	};
	struct alignas(16) st_TOP
	{
		st_BLOCK_NODE*		pNode;
		unsigned __int64	Key;
	};

	bool				m_bPlacementFlag;
	unsigned __int64	m_iGuardCode;
	long				m_lCapacity;
	long				m_lUseCount;
	st_TOP*				m_pTop;

public:
	long GetCapacity(void)
	{
		return m_lCapacity;
	}

	long GetUseCount(void)
	{
		return m_lUseCount;
	}

	CMemoryPool(long lCapacity = 0, bool bPlacementNew = false)
	{
		m_lUseCount = 0;
		m_lCapacity = lCapacity;
		m_bPlacementFlag = bPlacementNew;

		m_pTop = (st_TOP*)_aligned_malloc(sizeof(st_TOP), 16);
		m_pTop->pNode = NULL;
		m_pTop->Key = 0;
		m_iGuardCode = (unsigned __int64)m_pTop;

		for (int iCnt = 0; iCnt < m_lCapacity; iCnt++)
		{
			AddNode();
		}
	}

	virtual ~CMemoryPool()
	{
		st_BLOCK_NODE* pNode;

		if (m_bPlacementFlag == true)
		{
			while (m_pTop->pNode != nullptr)
			{
				pNode = m_pTop->pNode;
				m_pTop->pNode = m_pTop->pNode->pNextNode;
				free(pNode);

				m_lCapacity--;
			}
		}
		else
		{
			while (m_pTop->pNode != nullptr)
			{
				pNode = m_pTop->pNode;
				m_pTop->pNode = m_pTop->pNode->pNextNode;
				delete pNode;

				m_lCapacity--;
			}
		}

		_aligned_free(m_pTop);
	}

	void AddNode(void)
	{
		st_BLOCK_NODE* pNewNode;
		pNewNode->FrontGuard = m_iGuardCode;
		pNewNode->RearGuard = m_iGuardCode;
		if (true == m_bPlacementFlag)
		{
			pNewNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
		}
		else
		{
			pNewNode = new st_BLOCK_NODE;
		}

		st_BLOCK_NODE* pNode;
		do
		{
			pNode = m_pTop->pNode;
			pNewNode->pNextNode = pNode;
		} while (InterlockedCompareExchange64((volatile LONG64*)m_pTop, (LONG64)pNewNode, (LONG64)pNode) != (LONG64)pNode);
	}

	BOOL Free(DATA* pReturnData)
	{
		st_BLOCK_NODE* pReturnNode = (st_BLOCK_NODE*)((char*)pReturnData - (sizeof(m_iGuardCode)));

		if (m_iGuardCode != pReturnNode->FrontGuard || m_iGuardCode != pReturnNode->RearGuard)
		{
			return FALSE;
		}

		if (m_bPlacementFlag == TRUE)
		{
			pReturnData->~DATA();
		}

		st_BLOCK_NODE* pNode;
		do
		{
			pNode = m_pTop->pNode;
			pReturnNode->pNextNode = pNode;
		} while (InterlockedCompareExchange64((volatile LONG64*)m_pTop, (LONG64)pReturnNode, (LONG64)pNode) != (LONG64)pNode);

		InterlockedDecrement(&m_lUseCount);

		return TRUE;
	}

	DATA* Alloc(void)
	{
		long lCapacity = m_lCapacity;
		if (lCapacity < InterlockedIncrement(&m_lUseCount))
		{
			AddNode();
			InterlockedIncrement(&m_lCapacity);
		}

		st_TOP Top;
		do
		{
			Top.Key = m_pTop->Key;
			Top.pNode = m_pTop->pNode;
		} while (!InterlockedCompareExchange128((volatile LONG64*)m_pTop, (LONG64)(Top.Key + 1), (LONG64)Top.pNode->pNextNode, (LONG64*)&Top));

		DATA* pNodeData = &(Top.pNode->Data);
		if (m_bPlacementFlag == TRUE)
		{
			new (pNodeData) DATA();
		}

		return pNodeData;
	}
};