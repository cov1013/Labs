#pragma once
#include <windows.h>
#include "Common.h"
#include "CMemoryPool.h"

//#define __MULTI_THREAD_DEBUG_MODE__

namespace cov1013
{
	template <typename DATA>
	class CLockFreeQueue
	{
	public:
		struct st_NODE
		{
			DATA		Data;
			st_NODE*	pNextNode;
		};

		struct alignas(16) st_CONTAINER
		{
			st_NODE*			pNode;
			unsigned __int64	Key;
		};

#ifdef __MULTI_THREAD_DEBUG_MODE__

		enum en_LOG
		{
			en_LOG_LOGIC_ENQ	= 10000,
			en_LOG_LOGIC_DEQ	= 20000,
			en_LOG_MAX			= 100000
		};

		struct st_LOG
		{
			DWORD	Id;
			int		Logic;
			void*	pHead;
			void*	pHeadNext;
			__int64	HeadKey;
			void*	pTail;
			void*	pTailNext;
			__int64	TailKey;
			void*	pSnapHead;
			void*	pSnapHeadNext;
			__int64	SnapHeadKey;
			void*	pSnapTail;
			void*	pSnapTailNext;
			__int64	SnapTailKey;
			void*	pAllocNode;
			void*	pAllocNodeNext;
			void*	pFreeNode;
			void*	pFreeNodeNext;
			long	Capacity;
			void*	pEnQData;
			void*	pDeQData;
		};

#endif

	public:
		//////////////////////////////////////////////////////////////////////////////
		// 생성자
		//////////////////////////////////////////////////////////////////////////////
		CLockFreeQueue()
		{
			m_pMemoryPool = (CMemoryPool<st_NODE>*)_aligned_malloc(sizeof(CMemoryPool<st_NODE>), en_CACHE_ALIGN);
			new (m_pMemoryPool) CMemoryPool<st_NODE>(0);


			m_pHead = (st_CONTAINER*)_aligned_malloc(sizeof(st_CONTAINER), en_CACHE_ALIGN);
			m_pHead->pNode = m_pMemoryPool->Alloc();
			m_pHead->Key = 0;
			m_pHead->pNode->pNextNode = NULL;


			m_pTail = (st_CONTAINER*)_aligned_malloc(sizeof(st_CONTAINER), en_CACHE_ALIGN);
			m_pTail->pNode = m_pHead->pNode;
			m_pTail->Key = 0;
			m_pTail->pNode->pNextNode = NULL;

			m_lCapacity = 0;
		}

		//////////////////////////////////////////////////////////////////////////////
		// 소멸자
		//////////////////////////////////////////////////////////////////////////////
		virtual ~CLockFreeQueue()
		{
			m_pMemoryPool->Free(m_pHead->pNode);

			(m_pMemoryPool)->~CMemoryPool();

			_aligned_free(m_pMemoryPool);

			_aligned_free(m_pHead);
			_aligned_free(m_pTail);
		}

		//////////////////////////////////////////////////////////////////////////////
		// Enqueue
		//////////////////////////////////////////////////////////////////////////////
		void Enqueue(DATA data)
		{
			st_NODE* pAllocNode = m_pMemoryPool->Alloc();
			pAllocNode->Data = data;
			pAllocNode->pNextNode = NULL;
#ifdef __MULTI_THREAD_DEBUG_MODE__
			SetLog(en_LOG_LOGIC_ENQ + 10, NULL, NULL, NULL, NULL, NULL, NULL, pAllocNode);
#endif

			st_CONTAINER	Snapshot_Tail;
			st_NODE*		Snapshot_TailNextNode;

			while (TRUE)
			{
				Snapshot_Tail.Key = m_pTail->Key;
#ifdef __MULTI_THREAD_DEBUG_MODE__
				SetLog(en_LOG_LOGIC_ENQ + 20, NULL, NULL, NULL, Snapshot_Tail.Key);
#endif

				Snapshot_Tail.pNode = m_pTail->pNode;
#ifdef __MULTI_THREAD_DEBUG_MODE__
				SetLog(en_LOG_LOGIC_ENQ + 30, NULL, NULL, NULL, Snapshot_Tail.Key, Snapshot_Tail.pNode);
#endif

				Snapshot_TailNextNode = Snapshot_Tail.pNode->pNextNode;
#ifdef __MULTI_THREAD_DEBUG_MODE__
				SetLog(en_LOG_LOGIC_ENQ + 40, NULL, NULL, NULL, Snapshot_Tail.Key, Snapshot_Tail.pNode, Snapshot_TailNextNode);
#endif

				if (Snapshot_TailNextNode == NULL)
				{
					if (CAS(&m_pTail->pNode->pNextNode, pAllocNode, Snapshot_TailNextNode) == (LONG64)Snapshot_TailNextNode)
					{
						DCAS(m_pTail, Snapshot_Tail.Key + 1, pAllocNode, &Snapshot_Tail);

#ifdef __MULTI_THREAD_DEBUG_MODE__
						st_CONTAINER	Snapshot_TailTemp;
						Snapshot_TailTemp.Key = Snapshot_Tail.Key;
						Snapshot_TailTemp.pNode = Snapshot_Tail.pNode;
						if (DCAS(m_pTail, Snapshot_Tail.Key + 1, pAllocNode, &Snapshot_Tail) == FALSE)
						{
							SetLog(en_LOG_LOGIC_ENQ + 50, NULL, NULL, NULL, Snapshot_TailTemp.Key, Snapshot_TailTemp.pNode);
						}
						else
						{
							SetLog(en_LOG_LOGIC_ENQ + 60, NULL, NULL, NULL, Snapshot_TailTemp.Key, Snapshot_TailTemp.pNode);
						}
#endif
						break;
					}
				}
				else
				{
					DCAS(m_pTail, Snapshot_Tail.Key + 1, Snapshot_TailNextNode, &Snapshot_Tail);
				}
			}

#ifdef __MULTI_THREAD_DEBUG_MODE__
			SetLog(en_LOG_LOGIC_ENQ + 200);
#endif
			InterlockedIncrement(&m_lCapacity);

#ifdef __MULTI_THREAD_DEBUG_MODE__
			SetLog(en_LOG_LOGIC_ENQ + 210);
#endif
		}

		///////////////////////////////////////////////////////////////////////////////////
		// Dequeue
		///////////////////////////////////////////////////////////////////////////////////
		bool Dequeue(DATA* pDestination)
		{
#ifdef __MULTI_THREAD_DEBUG_MODE__
			SetLog(en_LOG_LOGIC_DEQ + 10);
#endif

			if (0 > InterlockedDecrement(&m_lCapacity))
			{
				InterlockedIncrement(&m_lCapacity);
				return FALSE;
			}

			st_CONTAINER	Snapshot_Tail;
			st_CONTAINER	Snapshot_Head;
			st_NODE*		Snapshot_HeadNextNode;
			st_NODE*		Snapshot_TailNextNode;

			while (TRUE)
			{
				Snapshot_Head.Key = m_pHead->Key;
				Snapshot_Head.pNode = m_pHead->pNode;
				Snapshot_HeadNextNode = Snapshot_Head.pNode->pNextNode;

				if (Snapshot_HeadNextNode == NULL)
				{
					continue;
				}
				else
				{
					Snapshot_Tail.Key = m_pTail->Key;
					Snapshot_Tail.pNode = m_pTail->pNode;
					Snapshot_TailNextNode = Snapshot_Tail.pNode->pNextNode;
					if (Snapshot_TailNextNode != NULL)
					{
						if (DCAS(m_pTail, Snapshot_Tail.Key + 1, Snapshot_TailNextNode, &Snapshot_Tail) == FALSE)
						{
							continue;
						}
					}
				}

				*pDestination = Snapshot_HeadNextNode->Data;

				// Head 갱신
				if (DCAS(m_pHead, Snapshot_Head.Key + 1, Snapshot_HeadNextNode, &Snapshot_Head))
				{
					m_pMemoryPool->Free(Snapshot_Head.pNode);

#ifdef __MULTI_THREAD_DEBUG_MODE__
					SetLog(en_LOG_LOGIC_DEQ + 150, Snapshot_Head.Key, Snapshot_Head.pNode, Snapshot_HeadNextNode, NULL, NULL, NULL, NULL, NULL, Snapshot_Head.pNode);
#endif

					return TRUE;
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////////
		// 큐 용량 얻기
		//////////////////////////////////////////////////////////////////////////////
		LONG GetCapacity(void)
		{
			return m_lCapacity;
		}

#ifdef __MULTI_THREAD_DEBUG_MODE__
		//////////////////////////////////////////////////////////////////////////////
		// 로그 남기기
		//////////////////////////////////////////////////////////////////////////////
		void SetLog(
			int		Logic,

			__int64 SnapHeadKey		= -1,
			void*	pSnapHead		= NULL,
			void*	pSnapHeadNext	= NULL,

			__int64 SnapTailKey		= -1,
			void*	pSnapTail		= NULL,
			void*	pSnapTailNext	= NULL,

			void*	pAllocNode		= NULL,
			void*	pAllocNodeNext	= NULL,

			void*	pFreeNode		= NULL,
			void*	pFreeNodeNext	= NULL,

			void*	pEnQData		= NULL,
			void*	pDeQData		= NULL
		)
		{
			long Index = InterlockedIncrement(&m_LogIndex) % en_LOG_MAX;

			m_Logs[Index].Id = GetCurrentThreadId();
			m_Logs[Index].Logic = Logic;

			m_Logs[Index].pHead = m_pHead->pNode;
			m_Logs[Index].pHeadNext = m_pHead->pNode->pNextNode;
			m_Logs[Index].HeadKey = m_pHead->Key;

			m_Logs[Index].pTail = m_pTail->pNode;
			m_Logs[Index].pTailNext = m_pTail->pNode->pNextNode;
			m_Logs[Index].TailKey = m_pTail->Key;

			m_Logs[Index].pSnapHead = pSnapHead;
			m_Logs[Index].pSnapHeadNext = pSnapHeadNext;
			m_Logs[Index].SnapHeadKey = SnapHeadKey;

			m_Logs[Index].pSnapTail = pSnapTail;
			m_Logs[Index].pSnapTailNext = pSnapTailNext;
			m_Logs[Index].SnapTailKey = SnapTailKey;

			m_Logs[Index].pAllocNode = pAllocNode;
			m_Logs[Index].pAllocNodeNext = pAllocNodeNext;

			m_Logs[Index].pFreeNode = pFreeNode;
			m_Logs[Index].pFreeNodeNext = pFreeNodeNext;

			m_Logs[Index].Capacity = m_lCapacity;

			m_Logs[Index].pEnQData = pEnQData;
			m_Logs[Index].pDeQData = pDeQData;
		}
#endif

	public:
								CMemoryPool<st_NODE>*	m_pMemoryPool;

	private:
#ifdef __MULTI_THREAD_DEBUG_MODE__
								st_LOG					m_Logs[en_LOG_MAX];
								long					m_LogIndex = -1;
#endif

								st_CONTAINER*			m_pHead;
		alignas(en_CACHE_ALIGN) st_CONTAINER*			m_pTail;
		alignas(en_CACHE_ALIGN) LONG					m_lCapacity;
	};
}