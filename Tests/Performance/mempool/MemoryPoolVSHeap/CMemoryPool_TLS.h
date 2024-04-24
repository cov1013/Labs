#pragma once
#include <list>
#include "Common.h"
#include "CMemoryPool.h"

using namespace std;

namespace cov1013
{
	template <class DATA>
	class CMemoryPool_TLS
	{
	public:
		typedef unsigned char		PADDING;
		typedef unsigned __int64	CODE;

		enum en_CONPIG
		{
			en_CHUNK_ELEMENT_MAX = 200
		};

		struct st_CHUNK;
		struct st_ELEMENT
		{
#ifdef __SAFE_MODE__
			CODE		FrontGuard;					
			PADDING		Padding[en_PADDING_SIZE];	
			DATA		Data;
			CODE		RearGuard;					
#else
			DATA		Data;
#endif
			st_CHUNK* MyChunk;					
		};

		struct st_CHUNK
		{
			st_CHUNK()
			{
				bInitialize = false;
				lAllocCount = 0;
				lFreeCount = 0;
			}

			st_ELEMENT	Elements[en_CHUNK_ELEMENT_MAX];

			bool		bInitialize;						
			long		lAllocCount;						
			long		lFreeCount;							
															
			CODE		Code;								
		};

	public:
		CMemoryPool_TLS(long lCapacity, bool bPlacementNew = false)
		{
			m_bPlacementNew = bPlacementNew;
			m_dwTlsIndex = TlsAlloc();
			m_pMemoryPool = (CMemoryPool<st_CHUNK>*)_aligned_malloc(sizeof(CMemoryPool<st_CHUNK>), en_CACHE_ALIGN);
			new (m_pMemoryPool) CMemoryPool<st_CHUNK>(0);
			m_Code = (CODE)m_pMemoryPool;
		}

		~CMemoryPool_TLS()
		{
			m_pMemoryPool->~CMemoryPool();
			_aligned_free(m_pMemoryPool);
			TlsFree(m_dwTlsIndex);
		}

		DATA* Alloc(void)
		{
			st_CHUNK* pChunk = (st_CHUNK*)TlsGetValue(m_dwTlsIndex);

			if (pChunk == nullptr)
			{
				pChunk = m_pMemoryPool->Alloc();	
				pChunk->lAllocCount = 0;
				pChunk->lFreeCount = 0;

				if (pChunk->bInitialize == false)
				{
					pChunk->Code = m_Code;
					for (int i = 0; i < en_CHUNK_ELEMENT_MAX; i++)
					{
#ifdef __SAFE_MODE__
						pChunk->Elements[i].FrontGuard = pChunk->Code;
						pChunk->Elements[i].RearGuard = pChunk->Code;
#endif
						pChunk->Elements[i].MyChunk = pChunk;
					}
					pChunk->bInitialize = true;
				}

				TlsSetValue(m_dwTlsIndex, (LPVOID)pChunk);
			}

			DATA* pData = &(pChunk->Elements[pChunk->lAllocCount++].Data);

			if (m_bPlacementNew)
			{
				new (pData) DATA();
			}

			if (pChunk->lAllocCount == en_CHUNK_ELEMENT_MAX)
			{
				TlsSetValue(m_dwTlsIndex, (LPVOID)nullptr);
			}

			return pData;
		}

		bool Free(DATA* pData, bool bFlag = false)
		{
			st_ELEMENT* pElement;
			st_CHUNK* pChunk;

#ifdef __SAFE_MODE__
			pElement = (st_ELEMENT*)((PADDING*)pData - (sizeof(CODE) + en_PADDING_SIZE));
			pChunk = (st_CHUNK*)(pElement->MyChunk);

			if (pChunk->Code != pElement->FrontGuard || pChunk->Code != pElement->RearGuard)
			{
				return false;
			}
#else
			pElement = (st_ELEMENT*)pData;
			pChunk = pElement->MyChunk;
#endif
			if (m_bPlacementNew)
			{
				(pData)->~DATA();
			}

			long lFreeCount = InterlockedIncrement(&pChunk->lFreeCount);

			if (lFreeCount == en_CHUNK_ELEMENT_MAX)
			{
				m_pMemoryPool->Free(pChunk);
			}

			return true;
		}

		long GetCapacity(void)
		{
			return m_pMemoryPool->GetCapacity();
		}

		long GetUseCount(void)
		{
			return m_pMemoryPool->GetUseCount();
		}

	private:
		bool					m_bPlacementNew;	
		DWORD					m_dwTlsIndex;		
		CODE					m_Code;				
		CMemoryPool<st_CHUNK>* m_pMemoryPool;		
		list<st_CHUNK*>			m_AllocList;
	};
}