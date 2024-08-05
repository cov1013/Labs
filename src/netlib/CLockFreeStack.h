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
		// ��� ����
		//////////////////////////////////////////////////////////////////////////
		struct st_NODE
		{
			DATA				Data;
			st_NODE*			pNextNode;
		};

		//////////////////////////////////////////////////////////////////////////
		// DACS�� ���� �����̳�
		//////////////////////////////////////////////////////////////////////////
		struct st_TOP
		{
			st_NODE*			pNode;
			unsigned __int64	Key;
		};

	public:

		//////////////////////////////////////////////////////////////////////////
		// ������
		// 
		//////////////////////////////////////////////////////////////////////////
		CLockFreeStack()
		{
			m_Top.pNode = nullptr;
			m_Top.Key = 0;
			m_lCapacity = 0;
		}

		//////////////////////////////////////////////////////////////////////////
		// �Ҹ���
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
		// ������ ����
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
		// ������ ���
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
			// [��带 ���� �����ϸ� �߻��ϴ� ����]
			// 
			// 1. [������ A]���� pop�� �����ϴٰ� Top(30)�� Node�� �ް� ���ؽ�Ʈ ����Ī
			// 2. �ٸ� �����忡�� pop(30, 1)->pop(20, 2)->push(30, 2)�� ����.
			// 3. �ٽ� 1�� ���ƿͼ� Ű��(2) ȹ���ϰ� DCAS ���� �� ����
			// 4. ABA ���� �߻�.
			//---------------------------------------------------------------------
			Top.Key = m_Top.Key;
			Top.pNode = m_Top.pNode;
			while (!DCAS(&m_Top, Top.Key + 1, Top.pNode->pNextNode, &Top)) {};

			*pDest = Top.pNode->Data;
			m_MemoryPool.Free(Top.pNode);

			return true;
		}

		//////////////////////////////////////////////////////////////////////////
		// �뷮 ���
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