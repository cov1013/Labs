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
		// 생성자
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
		// 데이터 추가
		//////////////////////////////////////////////////////////////////////////
		void Push(DATA Data)
		{
			//----------------------------------------------------------
			// 메모리풀에서 새로운 노드를 할당받고 데이터 세팅
			//----------------------------------------------------------
			st_NODE* pNewNode = m_pMemoryPool->Alloc();
			pNewNode->Data = Data;

			//----------------------------------------------------------
			// 락프리 방식으로 스택에 노드 추가
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
			// 용량 갱신
			//----------------------------------------------------------
			InterlockedIncrement(&m_lCapacity);
		}

		//////////////////////////////////////////////////////////////////////////
		// 데이터 뽑기
		//////////////////////////////////////////////////////////////////////////
		bool Pop(DATA* pDest)
		{
			PRO_BEGIN(L"Pop-1");
			//----------------------------------------------------------
			// 노드를 뽑고 차감하면, 다른 스레드에서 해당 노드 개수를 인식할 수 있다.
			//----------------------------------------------------------
			if (0 > InterlockedDecrement(&m_lCapacity))
			{
				PRO_END(L"Pop-1");

				//----------------------------------------------------------
				// 차감했는데 음수라면, 용량이 없다는 것이므로 다시 증가시키고 나간다.
				//----------------------------------------------------------
				PRO_BEGIN(L"Pop-2");
				InterlockedIncrement(&m_lCapacity);
				PRO_END(L"Pop-2");
				return false;
			}
			PRO_END(L"Pop-1");

			//----------------------------------------------------------
			// 락프리 방식으로 Top 갱신
			//----------------------------------------------------------
			st_TOP Snapshot_Top;

			PRO_BEGIN(L"Pop-2");

			do {
				Snapshot_Top.Key = m_pTop->Key;
				Snapshot_Top.pNode = m_pTop->pNode;
			} while (!DCAS(m_pTop, Snapshot_Top.Key + 1, Snapshot_Top.pNode->pNextNode, &Snapshot_Top));

			PRO_END(L"Pop-2");

			//----------------------------------------------------------
			// 데이터를 뽑고 사용중이던 노드를 메모리풀에 반환한다.
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
		// 스택 용량 출력
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