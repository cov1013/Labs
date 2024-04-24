#pragma once
#include <windows.h>
#include <new.h>
#include "Common.h"

//#define __SAFE_MODE__

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
			CODE				FrontGuard;					// 8
			PADDING				Padding[en_PADDING_SIZE];	// 56 (Data�� 64 ��迡 �����ֱ� ���� �е�)
			DATA				Data;
			CODE				RearGuard;					// 8
#else
			DATA				Data;
#endif
			st_BLOCK_NODE* pNextNode;					
		};

		struct alignas(en_DCAS_ALIGN) st_TOP
		{
			st_BLOCK_NODE* pNode;						
			unsigned __int64	Key;					
		};

	public:
		//////////////////////////////////////////////////////////////////////////
		// ������
		//
		// PlacementNew True  : Alloc �� ������ ������ ȣ��
		// PlacementNew False : ó�� �޸� �Ҵ� ���� ���� ������ ȣ��
		//////////////////////////////////////////////////////////////////////////
		CMemoryPool(LONG lCapacity, BOOL bPlacementNew = FALSE)
		{
			m_bPlacementNew = bPlacementNew;
			m_pTop = (st_TOP*)_aligned_malloc(sizeof(st_TOP), en_CACHE_ALIGN);
			m_pTop->pNode = NULL;
			m_pTop->Key = 0;

			//-------------------------------------------------------
			// Heap���� �Ҵ���� �ּҴ� �޸�Ǯ �Ҹ��ڰ� ȣ��� ������ 
			// �ش� �Ÿ�Ǯ�� ����ϹǷ�, �ش� �޸�Ǯ�� ���� Ű������ ���
			//-------------------------------------------------------
			m_Code = (CODE)m_pTop;
			m_lUseCount = 0;
			m_lCapacity = 0;

			//-------------------------------------------------------
			// �ʱ� ������ ������ŭ �޸� Ȯ��
			//-------------------------------------------------------
			for (int i = 0; i < lCapacity; i++)
			{
				AddNode();
				m_lCapacity++;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// �Ҹ���
		//////////////////////////////////////////////////////////////////////////
		~CMemoryPool()
		{
			st_BLOCK_NODE* pTemp;

			if (m_bPlacementNew == TRUE)
			{
				//-------------------------------------------------------
				// �÷��̽���Ʈ New�� ������� ���, Free���� �ش� �޸𸮿� ����
				// �Ҹ��ڸ� ȣ���������Ƿ�, Heap�� ��ȯ�� �Ѵ�.
				//-------------------------------------------------------
				while (m_pTop->pNode != NULL)
				{
					pTemp = m_pTop->pNode;
					m_pTop->pNode = m_pTop->pNode->pNextNode;
					_aligned_free(pTemp);

					m_lCapacity--;
				}
			}
			else
			{
				//-------------------------------------------------------
				// �÷��̽���Ʈ New�� ������� �ʾ������
				// �̰����� �Ҹ��ڸ� ȣ���� �� Heap�� ��ȯ�Ѵ�.
				//-------------------------------------------------------
				while (m_pTop->pNode != NULL)
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

		//////////////////////////////////////////////////////////////////////////
		// ���ο� ��� �߰�
		//////////////////////////////////////////////////////////////////////////
		void AddNode(void)
		{
			int iSize;
			st_BLOCK_NODE* pNewNode;
			st_BLOCK_NODE* Snapshot_pNode;

#ifdef __SAFE_MODE__
			iSize = (sizeof(CODE) * 2) + en_PADDING_SIZE + sizeof(DATA) + sizeof(st_BLOCK_NODE*);
			pNewNode = (st_BLOCK_NODE*)_aligned_malloc(iSize, en_CACHE_ALIGN);
			pNewNode->FrontGuard = m_Code;
			pNewNode->RearGuard = m_Code;
#else
			iSize = sizeof(DATA) + sizeof(st_BLOCK_NODE*);
			pNewNode = (st_BLOCK_NODE*)_aligned_malloc(iSize, en_CACHE_ALIGN);
#endif
			if (FALSE == m_bPlacementNew)
			{
				new (&pNewNode->Data) DATA();
			}

			do
			{
				Snapshot_pNode = m_pTop->pNode;
				pNewNode->pNextNode = Snapshot_pNode;
			} while (CAS(m_pTop, pNewNode, Snapshot_pNode) != (LONG64)Snapshot_pNode);
		}

		//////////////////////////////////////////////////////////////////////////
		// �޸� �Ҵ�
		//////////////////////////////////////////////////////////////////////////
		DATA* Alloc(void)
		{
			LONG	lCapacity;			// 4
			DATA*	pNodeData;			// 8
			st_TOP	Snapshot_Top;		// 16 [8|8]

			lCapacity = m_lCapacity;
			if (lCapacity < InterlockedIncrement(&m_lUseCount))
			{
				AddNode();
				InterlockedIncrement(&m_lCapacity);
			}

			Snapshot_Top.Key = m_pTop->Key;
			Snapshot_Top.pNode = m_pTop->pNode;
			while (DCAS(m_pTop, Snapshot_Top.Key + 1, Snapshot_Top.pNode->pNextNode, &Snapshot_Top) == FALSE) {};

			pNodeData = &(Snapshot_Top.pNode->Data);
			if (m_bPlacementNew == TRUE)
			{
				new (pNodeData) DATA();
			}

			return pNodeData;
		}

		//////////////////////////////////////////////////////////////////////////
		// �޸� ��ȯ
		//////////////////////////////////////////////////////////////////////////
		const BOOL Free(DATA* pData)
		{
			st_BLOCK_NODE* pReturnNode;
			st_BLOCK_NODE* pSnapshot_TopNode;

#ifdef __SAFE_MODE__
			pReturnNode = (st_BLOCK_NODE*)((PADDING*)pData - (sizeof(CODE) + en_PADDING_SIZE));
			if (m_Code != pReturnNode->FrontGuard || m_Code != pReturnNode->RearGuard)
			{
				return false;
			}
#else
			pReturnNode = (st_BLOCK_NODE*)pData;
#endif

			if (m_bPlacementNew == TRUE)
			{
				pData->~DATA();
			}

			do
			{
				pSnapshot_TopNode = m_pTop->pNode;
				pReturnNode->pNextNode = pSnapshot_TopNode;
			} while (CAS(m_pTop, pReturnNode, pSnapshot_TopNode) != (LONG64)pSnapshot_TopNode);

			InterlockedDecrement(&m_lUseCount);

			return TRUE;
		}

		//////////////////////////////////////////////////////////////////////////
		// �޸�Ǯ �뷮 ���
		//////////////////////////////////////////////////////////////////////////
		const LONG GetCapacity(void) const
		{
			return m_lCapacity;
		}

		//////////////////////////////////////////////////////////////////////////
		// �޸� ��뷮 ���
		//////////////////////////////////////////////////////////////////////////
		const LONG GetUseCount(void) const
		{
			return m_lUseCount;
		}

	public:
								BOOL	m_bPlacementNew;
								CODE	m_Code;
								st_TOP* m_pTop;
		alignas(en_CACHE_ALIGN) LONG	m_lUseCount;
		alignas(en_CACHE_ALIGN) LONG	m_lCapacity;
	};
}