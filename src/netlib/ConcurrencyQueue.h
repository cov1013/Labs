#pragma once
#include <windows.h>
#include "Common.h"
#include "MemoryPool.h"

namespace cov1013
{
	template <typename DATA>
	class ConcurrencyQueue
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// 큐 노드 정의
		//////////////////////////////////////////////////////////////////////////
		struct st_NODE
		{
			DATA				Data;
			st_NODE*			pNextNode;
		};

		//////////////////////////////////////////////////////////////////////////
		// DCAS를 위한 컨테이너
		//////////////////////////////////////////////////////////////////////////
		struct st_CONTAINER
		{
			st_NODE*			pNode;
			unsigned __int64	Key;
		};

	public:
		//////////////////////////////////////////////////////////////////////////
		// 생성자
		//////////////////////////////////////////////////////////////////////////
		ConcurrencyQueue()
		{
			m_Head.pNode = m_MemoryPool.Alloc();	// 더미 노드 생성
			m_Head.Key = 0;
			m_Head.pNode->pNextNode = nullptr;

			m_Tail.pNode = m_Head.pNode;
			m_Tail.Key = 0;
			m_Tail.pNode->pNextNode = nullptr;

			m_lCapacity = 0;
		}

		//////////////////////////////////////////////////////////////////////////
		// 소멸자
		//////////////////////////////////////////////////////////////////////////
		~ConcurrencyQueue()
		{
			m_MemoryPool.Free(m_Head.pNode);		// 더미 노드 반환

			if (m_lCapacity != 0)
			{
				CrashDumper::Crash();
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// 데이터 삽입
		//////////////////////////////////////////////////////////////////////////
		void Enqueue(DATA data)
		{
			st_NODE* pNewNode = m_MemoryPool.Alloc();
			pNewNode->Data = data;
			pNewNode->pNextNode = nullptr;		

			st_CONTAINER Tail;
			st_NODE* pNext;

			while (true)
			{
				Tail.Key = m_Tail.Key;
				Tail.pNode = m_Tail.pNode;
				pNext = Tail.pNode->pNextNode;

				if (pNext == nullptr)
				{
					if (CAS(&m_Tail.pNode->pNextNode, pNewNode, pNext) == (LONG64)pNext)
					{
						//---------------------------------------------------------------------
						// 실패 1. 현재 TailNext가 NULL이므로 노드를 추가했지만, SnapKey가 현재 Key와 다른 경우.
						// 실패 2. SnapTail이 현재 Tail이 아닌 경우.
						//---------------------------------------------------------------------
						DCAS(&m_Tail, Tail.Key + 1, pNewNode, &Tail);
						break;
					}
				}
				else
				{
					//---------------------------------------------------------------------
					// TailNext가 NULL이 아니면 한 칸 밀고 다시 시도
					// 밀지 않고 그냥 Enqueue하면 기존 TailNext 메모리 유실 + Capacity와 실제 노드 개수가 일치하지 않음.
					//---------------------------------------------------------------------
					DCAS(&m_Tail, Tail.Key + 1, pNext, &Tail);
				}
			}

			InterlockedIncrement(&m_lCapacity);
		}

		//////////////////////////////////////////////////////////////////////////
		// 데이터 출력
		//////////////////////////////////////////////////////////////////////////
		bool Dequeue(DATA* pDestination)
		{
			if (0 > InterlockedDecrement(&m_lCapacity))
			{
				InterlockedIncrement(&m_lCapacity);
				return false;
			}

			st_CONTAINER Tail;
			st_CONTAINER Head;
			st_NODE* pHeadNext;
			st_NODE* pTailNext;

			while (true)
			{
				Head.Key = m_Head.Key;
				Head.pNode = m_Head.pNode;
				pHeadNext = Head.pNode->pNextNode;

				//---------------------------------------------------------------------
				// [Enqueue 실패2]의 경우 SnapTail이라고 생각한 노드 뒤에 할당받은 노드를 삽입한 후 Capacity를 증가한 후 나가는데,
				// 그 Capacity를 인식하고 들어왔다. 하지만 해당 노드는 실제 큐에 적용되기 전인 상황. 결국 큐에 적용될 것이므로, 적용되기 전 까지 계속 시도.
				// 
				// 해당 과정을 무시하면 HeadNext가 NULL인 경우를 참조하게 된다.
				//---------------------------------------------------------------------
				if (pHeadNext == nullptr)
				{
					continue;
				}
				else
				{
					//---------------------------------------------------------------------
					// 실제 출력할 노드가 있어도, TailNext가 NULL이 아닌 상태에서 출력하고 Head를 갱신하면
					// Head가 Tail을 추월하는 상황이 발생된다. 이 경우 Tail 노드가 반환되는 경우가 생기고
					// Tail의 Next가 Tail이 되는 상황과 Capacity가 실제 노드 수와 다른 상황이 발생한다.
					// 
					// 위 상황의 경우, Enqueue에서 Tail의 Next를 계속 Tail로 갱신하면서 빠져나오지 못하는 상황 발생하고,
					// Dequeue에서는 pHeadNext가 계속 NULL이므로 빠져나오지 못하는 상황이 발생한다.
					//---------------------------------------------------------------------
					Tail.Key = m_Tail.Key;
					Tail.pNode = m_Tail.pNode;
					pTailNext = Tail.pNode->pNextNode;
					if (pTailNext != nullptr)
					{
						DCAS(&m_Tail, Tail.Key + 1, pTailNext, &Tail);
					}
				}

				//---------------------------------------------------------------------
				// Head 갱신 후 데이터를 출력하게 되면, pHeadNext가 Head가 되면서 다른 스레드의 접근 대상이 된다.
				// 즉, 내가 알고있던 pHeadNext의 값이 다른 값으로 변경되는 상황이 발생한다.
				//---------------------------------------------------------------------
				*pDestination = pHeadNext->Data;
				if (DCAS(&m_Head, Head.Key + 1, pHeadNext, &Head))
				{
					m_MemoryPool.Free(Head.pNode);
					return true;
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// 용량 출력
		// 
		//////////////////////////////////////////////////////////////////////////
		long GetCapacity(void)
		{
			return m_lCapacity;
		}

	private:
								MemoryPool<st_NODE>	m_MemoryPool = MemoryPool<st_NODE>(0);
		alignas(en_CACHE_ALIGN) st_CONTAINER			m_Head;
		alignas(en_CACHE_ALIGN) st_CONTAINER			m_Tail;
		alignas(en_CACHE_ALIGN) long					m_lCapacity;
	};
}