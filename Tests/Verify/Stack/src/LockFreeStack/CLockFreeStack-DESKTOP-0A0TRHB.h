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
			DATA		Data;
			st_NODE*	pNextNode;
		};

		struct st_TOP
		{
			st_NODE*			pNode;
			unsigned __int64	Key;
		};

	public:

		//////////////////////////////////////////////////////////////////////////
		// ������
		//////////////////////////////////////////////////////////////////////////
		CLockFreeStack()
		{
			m_pMemoryPool = (CMemoryPool<st_NODE>*)_aligned_malloc(sizeof(CMemoryPool<st_NODE>), en_CACHE_ALIGN);
			new (m_pMemoryPool) CMemoryPool<st_NODE>(0);

			m_pTop = (st_TOP*)_aligned_malloc(sizeof(st_TOP), en_CACHE_ALIGN);
			m_pTop->pNode = nullptr;
			m_pTop->Key = 0;
			m_lCapacity = 0;
		}

		//////////////////////////////////////////////////////////////////////////
		// �Ҹ���
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
		// ������ �߰�
		//////////////////////////////////////////////////////////////////////////
		void Push(DATA Data)
		{
			//----------------------------------------------------------
			// �޸�Ǯ���� ���ο� ��带 �Ҵ�ް� ������ ����
			//----------------------------------------------------------
			st_NODE* pNewNode = m_pMemoryPool->Alloc();
			pNewNode->Data = Data;

			//----------------------------------------------------------
			// ������ ������� ���ÿ� ��� �߰�
			//----------------------------------------------------------
			st_NODE* Snapshot_pNode;

			PRO_BEGIN(L"Push-1");
			do
			{
				Snapshot_pNode = m_pTop->pNode;
				pNewNode->pNextNode = Snapshot_pNode;
			} while (CAS(m_pTop, pNewNode, Snapshot_pNode) != (LONG64)Snapshot_pNode);
			PRO_END(L"Push-1");

			//----------------------------------------------------------
			// �뷮 ����
			//----------------------------------------------------------
			InterlockedIncrement(&m_lCapacity);
		}

		//////////////////////////////////////////////////////////////////////////
		// ������ �̱�
		//////////////////////////////////////////////////////////////////////////
		bool Pop(DATA* pDest)
		{
			PRO_BEGIN(L"Pop-1");
			//----------------------------------------------------------
			// ��带 �̰� �����ϸ�, �ٸ� �����忡�� �ش� ��� ������ �ν��� �� �ִ�.
			//----------------------------------------------------------
			if (0 > InterlockedDecrement(&m_lCapacity))
			{
				PRO_END(L"Pop-1");

				//----------------------------------------------------------
				// �����ߴµ� �������, �뷮�� ���ٴ� ���̹Ƿ� �ٽ� ������Ű�� ������.
				//----------------------------------------------------------
				PRO_BEGIN(L"Pop-2");
				InterlockedIncrement(&m_lCapacity);
				PRO_END(L"Pop-2");
				return false;
			}
			PRO_END(L"Pop-1");

			//----------------------------------------------------------
			// ������ ������� Top ����
			//----------------------------------------------------------
			st_TOP Snapshot_Top;

			PRO_BEGIN(L"Pop-2");

			do {
				Snapshot_Top.Key = m_pTop->Key;
				Snapshot_Top.pNode = m_pTop->pNode;
			} while (!DCAS(m_pTop, Snapshot_Top.Key + 1, Snapshot_Top.pNode->pNextNode, &Snapshot_Top));

			PRO_END(L"Pop-2");

			//----------------------------------------------------------
			// �����͸� �̰� ������̴� ��带 �޸�Ǯ�� ��ȯ�Ѵ�.
			//----------------------------------------------------------
			PRO_BEGIN(L"Pop-3");
			*pDest = Snapshot_Top.pNode->Data;
			PRO_END(L"Pop-3");

			PRO_BEGIN(L"Pop-4");
			m_pMemoryPool->Free(Snapshot_Top.pNode);
			PRO_END(L"Pop-4");

			return true;
		}

		//////////////////////////////////////////////////////////////////////////
		// ���� �뷮 ���
		//////////////////////////////////////////////////////////////////////////
		long GetCapacity(void)
		{
			return m_lCapacity;
		};


	public:
		CMemoryPool<st_NODE>* m_pMemoryPool;

	private:
		st_TOP* m_pTop;
		alignas(en_CACHE_ALIGN) long m_lCapacity;
	};
}