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
		// ť ��� ����
		//////////////////////////////////////////////////////////////////////////
		struct st_NODE
		{
			DATA				Data;
			st_NODE*			pNextNode;
		};

		//////////////////////////////////////////////////////////////////////////
		// DCAS�� ���� �����̳�
		//////////////////////////////////////////////////////////////////////////
		struct st_CONTAINER
		{
			st_NODE*			pNode;
			unsigned __int64	Key;
		};

	public:
		//////////////////////////////////////////////////////////////////////////
		// ������
		//////////////////////////////////////////////////////////////////////////
		ConcurrencyQueue()
		{
			m_Head.pNode = m_MemoryPool.Alloc();	// ���� ��� ����
			m_Head.Key = 0;
			m_Head.pNode->pNextNode = nullptr;

			m_Tail.pNode = m_Head.pNode;
			m_Tail.Key = 0;
			m_Tail.pNode->pNextNode = nullptr;

			m_lCapacity = 0;
		}

		//////////////////////////////////////////////////////////////////////////
		// �Ҹ���
		//////////////////////////////////////////////////////////////////////////
		~ConcurrencyQueue()
		{
			m_MemoryPool.Free(m_Head.pNode);		// ���� ��� ��ȯ

			if (m_lCapacity != 0)
			{
				CrashDumper::Crash();
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// ������ ����
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
						// ���� 1. ���� TailNext�� NULL�̹Ƿ� ��带 �߰�������, SnapKey�� ���� Key�� �ٸ� ���.
						// ���� 2. SnapTail�� ���� Tail�� �ƴ� ���.
						//---------------------------------------------------------------------
						DCAS(&m_Tail, Tail.Key + 1, pNewNode, &Tail);
						break;
					}
				}
				else
				{
					//---------------------------------------------------------------------
					// TailNext�� NULL�� �ƴϸ� �� ĭ �а� �ٽ� �õ�
					// ���� �ʰ� �׳� Enqueue�ϸ� ���� TailNext �޸� ���� + Capacity�� ���� ��� ������ ��ġ���� ����.
					//---------------------------------------------------------------------
					DCAS(&m_Tail, Tail.Key + 1, pNext, &Tail);
				}
			}

			InterlockedIncrement(&m_lCapacity);
		}

		//////////////////////////////////////////////////////////////////////////
		// ������ ���
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
				// [Enqueue ����2]�� ��� SnapTail�̶�� ������ ��� �ڿ� �Ҵ���� ��带 ������ �� Capacity�� ������ �� �����µ�,
				// �� Capacity�� �ν��ϰ� ���Դ�. ������ �ش� ���� ���� ť�� ����Ǳ� ���� ��Ȳ. �ᱹ ť�� ����� ���̹Ƿ�, ����Ǳ� �� ���� ��� �õ�.
				// 
				// �ش� ������ �����ϸ� HeadNext�� NULL�� ��츦 �����ϰ� �ȴ�.
				//---------------------------------------------------------------------
				if (pHeadNext == nullptr)
				{
					continue;
				}
				else
				{
					//---------------------------------------------------------------------
					// ���� ����� ��尡 �־, TailNext�� NULL�� �ƴ� ���¿��� ����ϰ� Head�� �����ϸ�
					// Head�� Tail�� �߿��ϴ� ��Ȳ�� �߻��ȴ�. �� ��� Tail ��尡 ��ȯ�Ǵ� ��찡 �����
					// Tail�� Next�� Tail�� �Ǵ� ��Ȳ�� Capacity�� ���� ��� ���� �ٸ� ��Ȳ�� �߻��Ѵ�.
					// 
					// �� ��Ȳ�� ���, Enqueue���� Tail�� Next�� ��� Tail�� �����ϸ鼭 ���������� ���ϴ� ��Ȳ �߻��ϰ�,
					// Dequeue������ pHeadNext�� ��� NULL�̹Ƿ� ���������� ���ϴ� ��Ȳ�� �߻��Ѵ�.
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
				// Head ���� �� �����͸� ����ϰ� �Ǹ�, pHeadNext�� Head�� �Ǹ鼭 �ٸ� �������� ���� ����� �ȴ�.
				// ��, ���� �˰��ִ� pHeadNext�� ���� �ٸ� ������ ����Ǵ� ��Ȳ�� �߻��Ѵ�.
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
		// �뷮 ���
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