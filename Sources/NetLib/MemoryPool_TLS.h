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
		// ûũ ���ο� �������� ��� ������
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
		// ûũ ����
		//////////////////////////////////////////////////////////////////////////
		struct st_CHUNK
		{
			st_ELEMENT	Elements[en_CHUNK_ELEMENT_MAX];	// ���� �Ҵ����� �޸� ���
			long		lAllocCount;					// Element �Ҵ� Ƚ�� (MAX ���� �� TLS ûũ�� ���Ӱ� ����)
			long		lFreeCount;						// Element ��ȯ Ƚ�� (MAX ���� �� �ش� ûũ�� �޸�Ǯ�� ��ȯ)

#ifdef __SAFE_MODE__
			char*		Code;							// �ش� �޸�Ǯ�� ���� Ű��
#endif
		};

	public:
		///////////////////////////////////////////////////////////////////////////////////
		// ������
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
		// �Ҹ���
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
		// ��� �Ҵ�ޱ�
		///////////////////////////////////////////////////////////////////////////////////
		DATA* Alloc(void)
		{
			st_CHUNK* pChunk = (st_CHUNK*)TlsGetValue(m_dwTlsIndex);

			//-------------------------------------------------------------------
			// TLS�� ���õǾ��ִ� ûũ�� ���ٸ� ���ο� ûũ �Ҵ�.
			//-------------------------------------------------------------------
			if (pChunk == nullptr)
			{
				pChunk = m_MemoryPool.Alloc();	// ����
				pChunk->lAllocCount = 0;
				pChunk->lFreeCount = 0;

				//-------------------------------------------------------------------
				// �޸�Ǯ���� ���ʷ� �Ҵ���� ûũ�� ���, ûũ ����
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
						// PlacementNew �̻��� ���⼭ ������ �� ���� ȣ��.
						//-------------------------------------------------------------------
						if (m_bPlacementNew == false)
						{
							new (&pChunk->Elements[i].Data) DATA();
						}
					}

					//-------------------------------------------------------------------
					// ûũ ���ÿ� �Ϸ��ߴٸ�, ���� ���� �ش� ûũ�� �Ҵ���� ��� �ٽ� �������� �ʱ� ���� �÷��� ����
					//-------------------------------------------------------------------
					*pbInitialize = true;
				}

				//-------------------------------------------------------
				// �Ҵ���� ûũ�� TLS�� ����
				//-------------------------------------------------------
				TlsSetValue(m_dwTlsIndex, (LPVOID)pChunk);
			}

			DATA* pData = &(pChunk->Elements[pChunk->lAllocCount++].Data);

			//-------------------------------------------------------------------
			// PlacementNew ���� ��Ҹ��� ��ȯ���ֱ� ���� ������ ȣ��
			//-------------------------------------------------------------------
			if (m_bPlacementNew == true)
			{
				new (pData) DATA();
			}

			//-------------------------------------------------------
			// ûũ ��Ҹ� ���� ����ߴٸ� TLS�� NULL �����ؼ� ���� �Ҵ�� �޸�Ǯ���� ���ο� ûũ�� �Ҵ���� �� �ְ� �Ѵ�.
			//-------------------------------------------------------
			if (pChunk->lAllocCount == en_CHUNK_ELEMENT_MAX)
			{
				TlsSetValue(m_dwTlsIndex, (LPVOID)nullptr);
			}

			return pData;
		}

		//////////////////////////////////////////////////////////////////////////
		// ûũ �޸� ��ȯ
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
			// PlacementNew ���� ��Ҹ� ûũ�� ��ȯ�� �� ���� �Ҹ��� ȣ��
			//----------------------------------------------------------------------
			if (m_bPlacementNew == true)
			{
				(pData)->~DATA();
			}

			long lFreeCount = InterlockedIncrement(&pChunk->lFreeCount);

			//-------------------------------------------------------
			// ��� ��Ҹ� ��ȯ�ߴٸ�, ûũ�� �޸�Ǯ�� ��ȯ�Ѵ�.
			//-------------------------------------------------------
			if (lFreeCount == en_CHUNK_ELEMENT_MAX)
			{
				//----------------------------------------------------------------------
				// PlacementNew �̻��� ûũ ��ȯ�� �� ��� ����� �Ҹ��� ȣ�� �� ��ȯ
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
		// �޸�Ǯ �뷮 ���
		//////////////////////////////////////////////////////////////////////////
		long GetCapacity(void)
		{
			return m_MemoryPool.GetCapacity();
		}

		//////////////////////////////////////////////////////////////////////////
		// �޸�Ǯ���� ������� �޸𸮷� ���
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