#pragma once
#include <windows.h>
#include "Common.h"
#include "CMemoryPool.h"

namespace cov1013
{
	template <typename DATA>
	class CLockFreeStack
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// 노드 정의
		//////////////////////////////////////////////////////////////////////////
		struct st_NODE
		{
			DATA				Data;
			st_NODE*			pNextNode;
		};

		//////////////////////////////////////////////////////////////////////////
		// DACS를 위한 컨테이너
		//////////////////////////////////////////////////////////////////////////
		struct st_TOP
		{
			st_NODE*			pNode;
			unsigned __int64	Key;
		};

	public:

		//////////////////////////////////////////////////////////////////////////
		// 생성자
		// 
		//////////////////////////////////////////////////////////////////////////
		CLockFreeStack()
		{
			m_Top.pNode = nullptr;
			m_Top.Key = 0;
			m_lCapacity = 0;
		}

		//////////////////////////////////////////////////////////////////////////
		// 소멸자
		// 
		//////////////////////////////////////////////////////////////////////////
		~CLockFreeStack()
		{
			st_NODE* pTemp;

			while (m_Top.pNode != nullptr)
			{
				pTemp = m_Top.pNode;
				m_Top.pNode = m_Top.pNode->pNextNode;
				m_MemoryPool.Free(pTemp);

				m_lCapacity--;
			}

			if (m_lCapacity != 0)
			{
				CCrashDump::Crash();
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// 데이터 삽입
		// 
		//////////////////////////////////////////////////////////////////////////
		void Push(DATA Data)
		{
			st_NODE* pNewNode = m_MemoryPool.Alloc();
			pNewNode->Data = Data;

			st_NODE* pNode;
			do
			{
				pNode = m_Top.pNode;
				pNewNode->pNextNode = pNode;
			} while (CAS(&m_Top, pNewNode, pNode) != (LONG64)pNode);

			InterlockedIncrement(&m_lCapacity);
		}

		//////////////////////////////////////////////////////////////////////////
		// 데이터 출력
		// 
		//////////////////////////////////////////////////////////////////////////
		bool Pop(DATA* pDest)
		{
			if (0 > InterlockedDecrement(&m_lCapacity))
			{
				InterlockedIncrement(&m_lCapacity);
				return false;
			}

			st_TOP Top;

			//---------------------------------------------------------------------
			// [노드를 먼저 스냅하면 발생하는 문제]
			// 
			// 1. [스레드 A]에서 pop을 실행하다가 Top(30)의 Node만 받고 컨텍스트 스위칭
			// 2. 다른 스레드에서 pop(30, 1)->pop(20, 2)->push(30, 2)를 진행.
			// 3. 다시 1로 돌아와서 키값(2) 획득하고 DCAS 진행 후 성공
			// 4. ABA 문제 발생.
			//---------------------------------------------------------------------
			Top.Key = m_Top.Key;
			Top.pNode = m_Top.pNode;
			while (!DCAS(&m_Top, Top.Key + 1, Top.pNode->pNextNode, &Top)) {};

			*pDest = Top.pNode->Data;
			m_MemoryPool.Free(Top.pNode);

			return true;
		}

		//////////////////////////////////////////////////////////////////////////
		// 용량 출력
		// 
		//////////////////////////////////////////////////////////////////////////
		long GetCapacity(void)
		{
			return m_lCapacity;
		};

	private:
									CMemoryPool<st_NODE>	m_MemoryPool = CMemoryPool<st_NODE>(0);
		alignas(en_CACHE_ALIGN)		st_TOP					m_Top;			
		alignas(en_CACHE_ALIGN)		long					m_lCapacity;	
	};
}