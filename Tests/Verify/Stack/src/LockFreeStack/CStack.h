#pragma once
#include <windows.h>
#include "Common.h"
#include "CMemoryPool.h"

namespace cov1013
{
	template <class T>
	class CStack
	{
	public:
		struct st_NODE
		{
			T			Data;
			st_NODE*	pNextNode;
		};

	public:
		CStack()
		{
			m_Capacity = 0;
			m_UseCount = 0;
			m_pTop = NULL;
			m_bResourceInUse = false;
			m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			SetEvent(m_hEvent);

			m_pMemoryPool = (CMemoryPool<st_NODE>*)_aligned_malloc(sizeof(CMemoryPool<st_NODE>), en_CACHE_ALIGN);
			new (m_pMemoryPool) CMemoryPool<st_NODE>(0);

			InitializeCriticalSection(&m_cs);
			InitializeSRWLock(&m_srw);
		}

		~CStack()
		{
			DeleteCriticalSection(&m_cs);
			CloseHandle(m_hEvent);
		}

		void Push(T Source)
		{
			st_NODE* pNewNode = m_pMemoryPool->Alloc();
			pNewNode->Data = Source;
			pNewNode->pNextNode = m_pTop;

			m_pTop = pNewNode;

			++m_Capacity;
		}
		bool Pop(T* pDestination)
		{
			st_NODE* pTop = m_pTop;
			T Data = pTop->Data;
			m_pTop = pTop->pNextNode;

			m_pMemoryPool->Free(pTop);

			*pDestination = Data;
			--m_Capacity;

			return true;
		}

		void Lock(void)
		{
			while (true == InterlockedExchange8((CHAR*)&m_bResourceInUse, true)) 
			{
				for (int i = 0; i < 1024; i++)
				{
					YieldProcessor();
				}
			};
			//AcquireSRWLockExclusive(&m_srw);
			//EnterCriticalSection(&m_cs);
		}

		void Unlock(void)
		{
			m_bResourceInUse = false;
			//ReleaseSRWLockExclusive(&m_srw);
			//LeaveCriticalSection(&m_cs);
		}

	private:
		int						m_Capacity;
		int						m_UseCount;
		st_NODE*				m_pTop;
		CMemoryPool<st_NODE>*	m_pMemoryPool;

		alignas(64)
		SRWLOCK					m_srw;
		CRITICAL_SECTION		m_cs;
		bool					m_bResourceInUse;
		HANDLE					m_hEvent;
	};

}