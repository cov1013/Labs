#pragma once
#include <windows.h>
#include "Common.h"
#include "CMemoryPool.h"
#include "Profiler.h"

namespace cov1013
{
	template <typename DATA>
	class CLockFreeStack
	{
	public:
		struct st_NODE
		{
			DATA				Data;
			st_NODE*			pNextNode;
		};

		struct alignas(en_DCAS_ALIGN) st_TOP
		{
			st_NODE*			pNode;
			unsigned __int64	Key;
		};

	public:

		//////////////////////////////////////////////////////////////////////////
		// 생성자
		//////////////////////////////////////////////////////////////////////////
		CLockFreeStack()
		{
			m_pMemoryPool = (CMemoryPool<st_NODE>*)_aligned_malloc(sizeof(CMemoryPool<st_NODE>), en_CACHE_ALIGN);
			new (m_pMemoryPool) CMemoryPool<st_NODE>(0);

			m_pTop = (st_TOP*)_aligned_malloc(sizeof(st_TOP), en_CACHE_ALIGN);
			m_pTop->pNode = NULL;
			m_pTop->Key = 0;

			m_lCapacity = 0;
		}

		//////////////////////////////////////////////////////////////////////////
		// 소멸자
		//////////////////////////////////////////////////////////////////////////
		~CLockFreeStack()
		{
			st_NODE* pTemp;

			while (m_pTop->pNode != nullptr)
			{
				pTemp = m_pTop->pNode;
				m_pTop->pNode = m_pTop->pNode->pNextNode;
				m_pMemoryPool->Free(pTemp);

				m_lCapacity--;
			}

			(m_pMemoryPool)->~CMemoryPool();
			_aligned_free(m_pMemoryPool);
			_aligned_free(m_pTop);
		}

		//////////////////////////////////////////////////////////////////////////
		// Push
		//////////////////////////////////////////////////////////////////////////
		void Push(DATA Data)
		{
			st_NODE* pNewNode;
			st_NODE* pSnapshot_TopNode;

			pNewNode = m_pMemoryPool->Alloc();
			pNewNode->Data = Data;
			pSnapshot_TopNode = m_pTop->pNode;
			pNewNode->pNextNode = pSnapshot_TopNode;

			while (CAS(m_pTop, pNewNode, pSnapshot_TopNode) != (LONG64)pSnapshot_TopNode)
			{
				pSnapshot_TopNode = m_pTop->pNode;
				pNewNode->pNextNode = pSnapshot_TopNode;
				/*for (int i = 0; i < 1024; i++)
				{
					YieldProcessor();
				}*/
			}

			InterlockedIncrement(&m_lCapacity);
		}

		//////////////////////////////////////////////////////////////////////////
		// Pop
		//////////////////////////////////////////////////////////////////////////
		const BOOL Pop(DATA* pDest)
		{
			st_TOP Snapshot_Top;

			if (0 > InterlockedDecrement(&m_lCapacity))
			{
				InterlockedIncrement(&m_lCapacity);
				return FALSE;
			}

			Snapshot_Top.Key = m_pTop->Key;
			Snapshot_Top.pNode = m_pTop->pNode;
			while (DCAS(m_pTop, Snapshot_Top.Key + 1, Snapshot_Top.pNode->pNextNode, &Snapshot_Top) == FALSE)
			{
				/*for (int i = 0; i < 1024; i++)
				{
					YieldProcessor();
				}*/
			}

			*pDest = Snapshot_Top.pNode->Data;
			m_pMemoryPool->Free(Snapshot_Top.pNode);

			return TRUE;
		}

		//////////////////////////////////////////////////////////////////////////
		// 스택 용량 출력
		//////////////////////////////////////////////////////////////////////////
		const LONG GetCapacity(void) const
		{
			return m_lCapacity;
		};

	public:

	private:
		CMemoryPool<st_NODE>*			m_pMemoryPool;

		alignas(en_DCAS_ALIGN)	st_TOP*	m_pTop;
		alignas(en_CACHE_ALIGN) LONG	m_lCapacity;
	};
}