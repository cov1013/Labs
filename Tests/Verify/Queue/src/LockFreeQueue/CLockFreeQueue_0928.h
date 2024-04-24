
#include <windows.h>
#define dfLOG_MAX 1000000

enum en_LOG_LOGIC
{
	en_LOG_ENQ = 10000,
	en_LOG_DEQ = 20000,
};

struct st_LOG
{
	DWORD				ID;
	int					iLogic;
	long				lSize;
	long				lPoolCapacity;
	long				lPoolUseCount;
	unsigned __int64	key;
	void*				pHead;
	void*				pHeadNext;
	void*				pTail;
	void*				pTailNext;
	void*				pSnapHead;
	void*				pSnapHeadNext;
	void*				pSnapTail;
	void*				pSnapTailNext;
	void*				pAllocNode;
	void*				pAllocNodeNext;
	void*				pFreeNode;
	void*				pFreeNodeNext;
	void*				pEnQData;
	void*				pDeQData;
};

st_LOG	g_Logs[dfLOG_MAX];
short	g_shLogIndex = -1;

void _LOG(
	DWORD _ID,
	int   _iLogic,
	long  _lSize = -9999,
	long  _lPoolCapacity = -9999,
	long  _lPoolUseCount = -9999,
	void* _pHead = NULL,
	void* _pHeadNext = NULL,
	void* _pTail = NULL,
	void* _pTailNext = NULL,

	void* _pSnapHead = NULL,
	void* _pSnapHeadNext = NULL,
	void* _pSnapTail = NULL,
	void* _pSnapTailNext = NULL,
	void* _pAllocNode = NULL,
	void* _pAllocNodeNext = NULL,
	void* _pFreeNode = NULL,
	void* _pFreeNodeNext = NULL,

	unsigned __int64 _key = 0,

	void* _pEnQData = NULL,
	void* _pDeQData = NULL
)
{
	unsigned short lIndex = InterlockedIncrement16(&g_shLogIndex);

	g_Logs[lIndex].ID = _ID;
	g_Logs[lIndex].iLogic = _iLogic;
	g_Logs[lIndex].lSize = _lSize;
	g_Logs[lIndex].lPoolCapacity = _lPoolCapacity;
	g_Logs[lIndex].lPoolUseCount = _lPoolUseCount;

	g_Logs[lIndex].pHead = _pHead;
	g_Logs[lIndex].pHeadNext = _pHeadNext;
	g_Logs[lIndex].pTail = _pTail;
	g_Logs[lIndex].pTailNext = _pTailNext;

	g_Logs[lIndex].pSnapHead = _pSnapHead;
	g_Logs[lIndex].pSnapHeadNext = _pSnapHeadNext;
	g_Logs[lIndex].pSnapTail = _pSnapTail;
	g_Logs[lIndex].pSnapTailNext = _pSnapTailNext;
	g_Logs[lIndex].pAllocNode = _pAllocNode;
	g_Logs[lIndex].pAllocNodeNext = _pAllocNodeNext;
	g_Logs[lIndex].pFreeNode = _pFreeNode;
	g_Logs[lIndex].pFreeNodeNext = _pFreeNodeNext;

	g_Logs[lIndex].key = _key;

	g_Logs[lIndex].pEnQData = _pEnQData;
	g_Logs[lIndex].pDeQData = _pDeQData;
}

namespace cov1013
{
#pragma once
	template <typename T>
	class CLockFreeQueue
	{
	private:
		struct st_NODE
		{
			T           Data;
			st_NODE* pNextNode;
		};

		struct alignas(16) st_HEAD
		{
			st_NODE* pNode;
			unsigned __int64 Key;
		};

		struct alignas(16) st_TAIL
		{
			st_NODE* pNode;
			unsigned __int64 Key;
		};

		st_HEAD* m_pHead;
		st_TAIL* m_pTail;
		long                m_lCapacity;

	public:
		CMemoryPool<st_NODE> m_MemPool = CMemoryPool<st_NODE>(0);

	public:
		CLockFreeQueue()
		{
			m_lCapacity = 0;
			m_pHead = (st_HEAD*)_aligned_malloc(sizeof(st_HEAD), 16);
			m_pTail = (st_TAIL*)_aligned_malloc(sizeof(st_TAIL), 16);

			m_pHead->Key = 0;
			m_pHead->pNode = m_MemPool.Alloc();
			m_pHead->pNode->pNextNode = NULL;

			m_pTail->Key = 0;
			m_pTail->pNode = m_pHead->pNode;
			m_pTail->pNode->pNextNode = NULL;
		}

		void Enqueue(T _data)
		{
			DWORD ID = GetCurrentThreadId();

			// 10010. EnQ 시작
			_LOG(
				ID,
				en_LOG_ENQ + 10,
				m_lCapacity,
				m_MemPool.m_lCapacity,
				m_MemPool.m_lUseCount,
				m_pHead->pNode,
				m_pHead->pNode->pNextNode,
				m_pTail->pNode,
				m_pTail->pNode->pNextNode,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
				m_pTail->Key,
				(void*)_data
			);

			// 10020. 신규 노드 생성
			st_NODE* pNewNode = m_MemPool.Alloc();
			pNewNode->Data = _data;
			pNewNode->pNextNode = NULL;
			_LOG(ID, en_LOG_ENQ + 20, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, NULL, NULL, pNewNode, pNewNode->pNextNode, NULL, NULL, m_pTail->Key, (void*)_data);

			st_NODE* pNext;
			st_TAIL* pTail;
			st_TAIL  Tail = { 0 };

			//unsigned __int64 Key = InterlockedIncrement(&m_pTail->Key);

			while (TRUE)
			{
				// 10030. Key 갱신
				Tail.Key = m_pTail->Key;
				_LOG(ID, en_LOG_ENQ + 30, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, Tail.pNode, NULL, pNewNode, pNewNode->pNextNode, NULL, NULL, Tail.Key, (void*)_data);

				// 10040. Node 갱신
				Tail.pNode = m_pTail->pNode;
				_LOG(ID, en_LOG_ENQ + 40, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, Tail.pNode, Tail.pNode->pNextNode, pNewNode, pNewNode->pNextNode, NULL, NULL, Tail.Key, (void*)_data);

				pNext = Tail.pNode->pNextNode;
				if (pNext == NULL)
				{
					// 10050. Next == NULL
					_LOG(ID, en_LOG_ENQ + 50, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, Tail.pNode, Tail.pNode->pNextNode, pNewNode, pNewNode->pNextNode, NULL, NULL, Tail.Key, (void*)_data);

					if (InterlockedCompareExchange64((volatile LONG64*)&m_pTail->pNode->pNextNode, (LONG64)pNewNode, (LONG64)pNext) == (LONG64)pNext)
					{
						// 10060. 신규노드 추가 완료 (1번 과정)
						_LOG(ID, en_LOG_ENQ + 60, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, Tail.pNode, Tail.pNode->pNextNode, pNewNode, pNewNode->pNextNode, NULL, NULL, Tail.Key, (void*)_data);

						if (InterlockedCompareExchange128((volatile LONG64*)m_pTail, (LONG64)(Tail.Key + 1), (LONG64)(pNewNode), (LONG64*)&Tail) == FALSE)
						{
							// 10070. 사본 Tail이 현재의 Tail이 아니다.
							_LOG(ID, en_LOG_ENQ + 70, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, Tail.pNode, Tail.pNode->pNextNode, pNewNode, pNewNode->pNextNode, NULL, NULL, Tail.Key, (void*)_data);
						}
						else
						{
							// 10080. Tail을 신규노드로 갱신 완료
							_LOG(ID, en_LOG_ENQ + 80, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, Tail.pNode, Tail.pNode->pNextNode, pNewNode, pNewNode->pNextNode, NULL, NULL, Tail.Key, (void*)_data);
						}
						break;
					}
				}
				else
				{
					// 10090. Next != NULL
					_LOG(ID, en_LOG_ENQ + 90, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, Tail.pNode, Tail.pNode->pNextNode, pNewNode, pNewNode->pNextNode, NULL, NULL, Tail.Key, (void*)_data);

					if (InterlockedCompareExchange128((volatile LONG64*)m_pTail, (LONG64)(Tail.Key + 1), (LONG64)(pNext), (LONG64*)&Tail) == TRUE)
					{
						// 10100. Tail 한 칸 이동 완료
						_LOG(ID, en_LOG_ENQ + 100, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, Tail.pNode, Tail.pNode->pNextNode, pNewNode, pNewNode->pNextNode, NULL, NULL, m_pTail->Key, (void*)_data);
					}
					else
					{
						// 10110. Tail 한 칸 이동 실패 (다른 스레드가 Tail을 이동시켰다)
						_LOG(ID, en_LOG_ENQ + 110, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, Tail.pNode, Tail.pNode->pNextNode, pNewNode, pNewNode->pNextNode, NULL, NULL, m_pTail->Key, (void*)_data);
					}
				}
			}

			_LOG(ID, en_LOG_ENQ + 120, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, Tail.pNode, Tail.pNode->pNextNode, pNewNode, pNewNode->pNextNode, NULL, NULL, m_pTail->Key, (void*)_data);
			InterlockedIncrement(&m_lCapacity);
			_LOG(ID, en_LOG_ENQ + 130, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, Tail.pNode, Tail.pNode->pNextNode, pNewNode, pNewNode->pNextNode, NULL, NULL, m_pTail->Key, (void*)_data);
		}

		BOOL Dequeue(T* _pData)
		{
			DWORD ID = GetCurrentThreadId();

			// 20010 : DeQ 시작
			_LOG(ID, en_LOG_DEQ + 10, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, m_pHead->Key);

			if (0 > InterlockedDecrement(&m_lCapacity))
			{
				// 20020 : Capacity 부족
				_LOG(ID, en_LOG_DEQ + 20, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, m_pHead->Key);

				InterlockedIncrement(&m_lCapacity);

				// 20030 : Capacity 증가 후 리턴
				_LOG(ID, en_LOG_DEQ + 30, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, m_pHead->Key);

				return FALSE;
			}

			st_HEAD* pHead;
			st_NODE* pNext;
			st_TAIL Tail = { 0 };
			st_HEAD Head = { 0 };

			while (TRUE)
			{
				pHead = m_pHead;

				// 20060. Key 갱신
				Head.Key = pHead->Key;
				_LOG(ID, en_LOG_DEQ + 60, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, Head.pNode, NULL, NULL, NULL, NULL, NULL, NULL, NULL, Head.Key);

				// 20070. Node 갱신
				Head.pNode = pHead->pNode;
				_LOG(ID, en_LOG_DEQ + 70, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, Head.pNode, Head.pNode->pNextNode, NULL, NULL, NULL, NULL, NULL, NULL, Head.Key);

				pNext = pHead->pNode->pNextNode;

				if (pNext == NULL)
				{
					continue;
				}
				else
				{
					Tail.Key = m_pTail->Key;
					Tail.pNode = m_pTail->pNode;
					if (Tail.pNode->pNextNode != NULL)
					{
						_LOG(ID, en_LOG_DEQ + 40, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, m_pHead->Key);
						InterlockedCompareExchange128((volatile LONG64*)m_pTail, (LONG64)(Tail.Key + 1), (LONG64)(Tail.pNode->pNextNode), (LONG64*)&Tail);
						_LOG(ID, en_LOG_DEQ + 50, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, m_pHead->Key);
					}
				}

				if (InterlockedCompareExchange128((volatile LONG64*)m_pHead, (LONG64)(Head.Key + 1), (LONG64)(pNext), (LONG64*)&Head) == TRUE)
				{
					// 20080. m_pHead 갱신 완료 후
					_LOG(ID, en_LOG_DEQ + 80, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, Head.pNode, Head.pNode->pNextNode, NULL, NULL, NULL, NULL, NULL, NULL, m_pHead->Key);

					*_pData = pNext->Data;

					// 20100. Free 이전
					_LOG(ID, en_LOG_DEQ + 100, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, Head.pNode, Head.pNode->pNextNode, NULL, NULL, NULL, NULL, Head.pNode, Head.pNode->pNextNode, m_pHead->Key, NULL, (void*)*_pData);

					m_MemPool.Free(Head.pNode);

					// 20110. Free 이후
					_LOG(ID, en_LOG_DEQ + 110, m_lCapacity, m_MemPool.m_lCapacity, m_MemPool.m_lUseCount, m_pHead->pNode, m_pHead->pNode->pNextNode, m_pTail->pNode, m_pTail->pNode->pNextNode, Head.pNode, Head.pNode->pNextNode, NULL, NULL, NULL, NULL, Head.pNode, Head.pNode->pNextNode, m_pHead->Key, NULL, (void*)*_pData);

					return TRUE;
				}
			}
		}
	};
}

