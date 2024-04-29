#pragma once
#include <list>
#include "Common.h"
#include "MemoryPool.h"

using namespace std;

namespace covEngine
{
	template <class DATA>
	class MemoryPool_TLS
	{
	public:
		enum en_CONPIG
		{
			en_CHUNK_ELEMENT_MAX = 200
		};

		//////////////////////////////////////////////////////////////////////////
		// 청크 내부에 실질적인 요소 데이터
		//////////////////////////////////////////////////////////////////////////
		struct st_CHUNK;
		struct st_ELEMENT
		{
#ifdef __SAFE_MODE__
			char*		FrontGuard;					
			char		Padding[en_PADDING_SIZE];	
			DATA		Data;
			char*		RearGuard;
#else
			DATA		Data;
#endif
			st_CHUNK*	MyChunk;					
		};

		//////////////////////////////////////////////////////////////////////////
		// 청크 정의
		//////////////////////////////////////////////////////////////////////////
		struct st_CHUNK
		{
			st_ELEMENT	Elements[en_CHUNK_ELEMENT_MAX];	// 실제 할당해줄 메모리 요소
			long		lAllocCount;					// Element 할당 횟수 (MAX 도달 시 TLS 청크를 새롭게 세팅)
			long		lFreeCount;						// Element 반환 횟수 (MAX 도달 시 해당 청크는 메모리풀에 반환)

#ifdef __SAFE_MODE__
			char*		Code;							// 해당 메모리풀의 고유 키값
#endif
		};

	public:
		///////////////////////////////////////////////////////////////////////////////////
		// 생성자
		// 
		///////////////////////////////////////////////////////////////////////////////////
		MemoryPool_TLS(long lCapacity, bool bPlacementNew = false)
		{
			m_bPlacementNew = bPlacementNew;
			m_dwTlsIndex = TlsAlloc();
#ifdef __SAFE_MODE__
			m_Code = (char*)malloc(sizeof(char));
#endif
		}

		///////////////////////////////////////////////////////////////////////////////////
		// 소멸자
		// 
		///////////////////////////////////////////////////////////////////////////////////
		~MemoryPool_TLS()
		{
			TlsFree(m_dwTlsIndex);
#ifdef __SAFE_MODE__
			free(m_Code);
#endif
		}

		///////////////////////////////////////////////////////////////////////////////////
		// 요소 할당받기
		///////////////////////////////////////////////////////////////////////////////////
		DATA* Alloc(void)
		{
			st_CHUNK* pChunk = (st_CHUNK*)TlsGetValue(m_dwTlsIndex);

			//-------------------------------------------------------------------
			// TLS에 세팅되어있는 청크가 없다면 새로운 청크 할당.
			//-------------------------------------------------------------------
			if (pChunk == nullptr)
			{
				pChunk = m_MemoryPool.Alloc();	// 경합
				pChunk->lAllocCount = 0;
				pChunk->lFreeCount = 0;

				//-------------------------------------------------------------------
				// 메모리풀에서 최초로 할당받은 청크인 경우, 청크 세팅
				//-------------------------------------------------------------------
				bool* pbInitialize = (bool*)((char*)pChunk + sizeof(st_CHUNK));
				if (*pbInitialize == false)
				{
#ifdef __SAFE_MODE__
					pChunk->Code = m_Code;
#endif
					for (int i = 0; i < en_CHUNK_ELEMENT_MAX; i++)
					{
#ifdef __SAFE_MODE__
						pChunk->Elements[i].FrontGuard = pChunk->Code;
						pChunk->Elements[i].RearGuard = pChunk->Code;
#endif
						pChunk->Elements[i].MyChunk = pChunk;

						//-------------------------------------------------------------------
						// PlacementNew 미사용시 여기서 생성자 한 번만 호출.
						//-------------------------------------------------------------------
						if (m_bPlacementNew == false)
						{
							new (&pChunk->Elements[i].Data) DATA();
						}
					}

					//-------------------------------------------------------------------
					// 청크 세팅에 완료했다면, 다음 번에 해당 청크를 할당받은 경우 다시 세팅하지 않기 위해 플래그 갱신
					//-------------------------------------------------------------------
					*pbInitialize = true;
				}

				//-------------------------------------------------------
				// 할당받은 청크를 TLS에 저장
				//-------------------------------------------------------
				TlsSetValue(m_dwTlsIndex, (LPVOID)pChunk);
			}

			DATA* pData = &(pChunk->Elements[pChunk->lAllocCount++].Data);

			//-------------------------------------------------------------------
			// PlacementNew 사용시 요소마다 반환해주기 전에 생성자 호출
			//-------------------------------------------------------------------
			if (m_bPlacementNew == true)
			{
				new (pData) DATA();
			}

			//-------------------------------------------------------
			// 청크 요소를 전부 사용했다면 TLS에 NULL 세팅해서 다음 할당시 메모리풀에서 새로운 청크를 할당받을 수 있게 한다.
			//-------------------------------------------------------
			if (pChunk->lAllocCount == en_CHUNK_ELEMENT_MAX)
			{
				TlsSetValue(m_dwTlsIndex, (LPVOID)nullptr);
			}

			return pData;
		}

		//////////////////////////////////////////////////////////////////////////
		// 청크 메모리 반환
		//////////////////////////////////////////////////////////////////////////
		bool Free(DATA* pData)
		{
			st_ELEMENT* pElement;
			st_CHUNK*	pChunk;

#ifdef __SAFE_MODE__
			pElement = (st_ELEMENT*)((char*)pData - (sizeof(char*) + en_PADDING_SIZE));
			pChunk = (st_CHUNK*)(pElement->MyChunk);

			if (pChunk->Code != pElement->FrontGuard || pChunk->Code != pElement->RearGuard)
			{
				return false;
			}
#else
			pElement = (st_ELEMENT*)pData;
			pChunk = pElement->MyChunk;
#endif

			//----------------------------------------------------------------------
			// PlacementNew 사용시 요소를 청크에 반환할 때 마다 소멸자 호출
			//----------------------------------------------------------------------
			if (m_bPlacementNew == true)
			{
				(pData)->~DATA();
			}

			long lFreeCount = InterlockedIncrement(&pChunk->lFreeCount);

			//-------------------------------------------------------
			// 모든 요소를 반환했다면, 청크를 메모리풀에 반환한다.
			//-------------------------------------------------------
			if (lFreeCount == en_CHUNK_ELEMENT_MAX)
			{
				//----------------------------------------------------------------------
				// PlacementNew 미사용시 청크 반환할 때 모든 요소의 소멸자 호출 후 반환
				//----------------------------------------------------------------------
				if (m_bPlacementNew == false)
				{
					for (int i = 0; i < en_CHUNK_ELEMENT_MAX; i++)
					{
						(&pChunk->Elements[i].Data)->~DATA();
					}
				}
				m_MemoryPool.Free(pChunk);
			}

			return true;
		}

		//////////////////////////////////////////////////////////////////////////
		// 메모리풀 용량 출력
		//////////////////////////////////////////////////////////////////////////
		long GetCapacity(void)
		{
			return m_MemoryPool.GetCapacity();
		}

		//////////////////////////////////////////////////////////////////////////
		// 메모리풀에서 사용중인 메모리량 출력
		//////////////////////////////////////////////////////////////////////////
		long GetUseCount(void)
		{
			return m_MemoryPool.GetUseCount();
		}

	private:
		//---------------------------------------------------------------------
		// [0]   m_MemoryPool (192)
		//---------------------------------------------------------------------
		// [64]  m_bPlacementNew | m_dwTlsIndex | m_Code
		//---------------------------------------------------------------------
		MemoryPool<st_CHUNK>	m_MemoryPool = MemoryPool<st_CHUNK>(0, false);

		bool					m_bPlacementNew; // 1
												 // (3)
		DWORD					m_dwTlsIndex;	 // 4

#ifdef __SAFE_MODE__
		char*					m_Code;			 // 8
#endif
	};
}