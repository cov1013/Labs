#pragma once
#include <windows.h>
#include <new.h>
#include "Common.h"

namespace cov1013
{
	template <class DATA>
	class CMemoryPool
	{
	public:
		typedef unsigned char		PADDING;
		typedef unsigned __int64	CODE;

	public:
		struct st_BLOCK_NODE
		{
#ifdef __SAFE_MODE__
			CODE				FrontGuard;					
			PADDING				Padding[en_PADDING_SIZE];	
			DATA				Data;
			CODE				RearGuard;					
#else
			DATA				Data;
#endif
			st_BLOCK_NODE* pNextNode;					
		};

		struct st_TOP
		{
			st_BLOCK_NODE* pNode;						
			unsigned __int64	Key;					
		};

	public:
		CMemoryPool(long lCapacity, bool bPlacementNew = false)
		{
			m_bPlacementNew = bPlacementNew;
			m_pTop = (st_TOP*)_aligned_malloc(sizeof(st_TOP), en_CACHE_ALIGN);
			m_pTop->pNode = nullptr;
			m_pTop->Key = 0;

			m_Code = (CODE)m_pTop;
			m_lUseCount = 0;
			m_lCapacity = 0;

			for (int i = 0; i < lCapacity; i++)
			{
				AddNode();
				m_lCapacity++;
			}
		}

		~CMemoryPool()
		{
			st_BLOCK_NODE* pTemp;

			if (m_bPlacementNew == true)
			{
				while (m_pTop->pNode != nullptr)
				{
					pTemp = m_pTop->pNode;
					m_pTop->pNode = m_pTop->pNode->pNextNode;
					_aligned_free(pTemp);

					m_lCapacity--;
				}
			}
			else
			{
				while (m_pTop->pNode != nullptr)
				{
					pTemp = m_pTop->pNode;
					m_pTop->pNode = m_pTop->pNode->pNextNode;

					(&pTemp->Data)->~DATA();
					_aligned_free(pTemp);

					m_lCapacity--;
				}
			}

			_aligned_free(m_pTop);
		}

		void AddNode(void)
		{
			int iSize;
			st_BLOCK_NODE* pNewNode;

#ifdef __SAFE_MODE__
			iSize = (sizeof(CODE) * 2) + en_PADDING_SIZE + sizeof(DATA) + sizeof(st_BLOCK_NODE*);
			pNewNode = (st_BLOCK_NODE*)_aligned_malloc(iSize, en_CACHE_ALIGN);
			pNewNode->FrontGuard = m_Code;
			pNewNode->RearGuard = m_Code;
#else
			iSize = sizeof(DATA) + sizeof(st_BLOCK_NODE*);
			pNewNode = (st_BLOCK_NODE*)_aligned_malloc(iSize, en_CACHE_ALIGN);
#endif
			if (false == m_bPlacementNew)
			{
				new (&pNewNode->Data) DATA();
			}

			st_BLOCK_NODE* Snapshot_pNode;
			do
			{
				Snapshot_pNode = m_pTop->pNode;
				pNewNode->pNextNode = Snapshot_pNode;
			} while (CAS(m_pTop, pNewNode, Snapshot_pNode) != (LONG64)Snapshot_pNode);
		}

		DATA* Alloc(void)
		{
			long lCapacity = m_lCapacity;
			if (lCapacity < InterlockedIncrement(&m_lUseCount))
			{
				AddNode();
				InterlockedIncrement(&m_lCapacity);
			}

			alignas(en_CACHE_ALIGN) st_TOP Snapshot_Top;
			Snapshot_Top.Key = m_pTop->Key;
			Snapshot_Top.pNode = m_pTop->pNode;
			while (!DCAS(m_pTop, Snapshot_Top.Key + 1, Snapshot_Top.pNode->pNextNode, &Snapshot_Top)) {};

			DATA* pNodeData = &(Snapshot_Top.pNode->Data);
			if (m_bPlacementNew == true)
			{
				new (pNodeData) DATA();
			}

			return pNodeData;
		}

		bool Free(DATA* pData)
		{
			st_BLOCK_NODE* pReturnNode;

#ifdef __SAFE_MODE__
			pReturnNode = (st_BLOCK_NODE*)((PADDING*)pData - (sizeof(CODE) + en_PADDING_SIZE));
			if (m_Code != pReturnNode->FrontGuard || m_Code != pReturnNode->RearGuard)
			{
				return false;
			}
#else
			pReturnNode = (st_BLOCK_NODE*)pData;
#endif

			if (m_bPlacementNew == true)
			{
				pData->~DATA();
			}

			st_BLOCK_NODE* Snapshot_pNode;
			do
			{
				Snapshot_pNode = m_pTop->pNode;
				pReturnNode->pNextNode = Snapshot_pNode;
			} while (CAS(m_pTop, pReturnNode, Snapshot_pNode) != (LONG64)Snapshot_pNode);

			InterlockedDecrement(&m_lUseCount);

			return true;
		}

		long GetCapacity(void)
		{
			return m_lCapacity;
		}

		long GetUseCount(void)
		{
			return m_lUseCount;
		}

	private:	
		bool	m_bPlacementNew;					
		st_TOP* m_pTop;								
		CODE	m_Code;								
		alignas(en_CACHE_ALIGN) long m_lUseCount;	
		alignas(en_CACHE_ALIGN) long m_lCapacity;	
	};
}