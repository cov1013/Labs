#pragma once
#include <new.h>
#include <windows.h>
#include "Common.h"

#define __SAFE_MODE__

namespace cov1013
{
	constexpr static int CacheLineLength = 64;
#ifdef __SAFE_MODE__
	constexpr static int PaddingSize = CacheLineLength - sizeof(char*);
#endif

	template <class DATA>
	class MemoryPool
	{
	private:
		struct st_BLOCK_NODE
		{
#ifdef __SAFE_MODE__
			char*				FrontGuard = nullptr;
			char				Padding[PaddingSize];
			DATA				Data;
			bool				bInitialize = false;
			char*				RearGuard = nullptr;
#else
			DATA				Data;
			bool				bInitialize = false;
#endif
			st_BLOCK_NODE*		pNextNode = nullptr;
		};

		struct st_TOP
		{
			st_BLOCK_NODE*		pNode = nullptr;
			unsigned __int64	Key = 0;
		};

	public:
		//////////////////////////////////////////////////////////////////////////
		// bPlacementNew �Ű����� ����
		// True  : Alloc() �� ������ ������ ȣ��
		// False : ó�� �޸� �Ҵ� ���� ���� ������ ȣ��
		//////////////////////////////////////////////////////////////////////////
		MemoryPool(long lCapacity, bool bPlacementNew = false, bool bInitialize = true)
		{
			m_bInitialize = bInitialize;
			m_bPlacementNew = bPlacementNew;

#ifdef __SAFE_MODE__
			m_Code = (char*)malloc(sizeof(char));
#endif
			for (int i = 0; i < lCapacity; i++)
			{
				NewNode();
				m_lCapacity++;
			}
		}

		~MemoryPool()
		{
			st_BLOCK_NODE* pTemp;

			if (m_bPlacementNew == true)
			{
				//----------------------------------------------------------------------
				// �÷��̽���Ʈ New�� ������� ���, Free���� �ش� �޸𸮿� ���� �Ҹ��ڸ� ȣ���������Ƿ�, Heap�� ��ȯ�� �Ѵ�.
				//----------------------------------------------------------------------
				while (m_Top.pNode != nullptr)
				{
					pTemp = m_Top.pNode;
					m_Top.pNode = m_Top.pNode->pNextNode;
					_aligned_free(pTemp);

					m_lCapacity--;
				}
			}
			else
			{
				//----------------------------------------------------------------------
				// �÷��̽���Ʈ New�� ������� �ʾ������, �̰����� �Ҹ���(TLS�� ��� TLS�� ���Ƿ� �̹� ȣ������)�� ȣ���� �� Heap�� ��ȯ�Ѵ�.
				//----------------------------------------------------------------------
				while (m_Top.pNode != nullptr)
				{
					pTemp = m_Top.pNode;
					m_Top.pNode = m_Top.pNode->pNextNode;

					if (m_bInitialize == true)
					{
						(&pTemp->Data)->~DATA();
					}
					_aligned_free(pTemp);

					m_lCapacity--;
				}
			}

#ifdef __SAFE_MODE__
			free(m_Code);
#endif
		}

		void NewNode()
		{
			int iSize = 0;
			st_BLOCK_NODE* pNewNode = nullptr;

#ifdef __SAFE_MODE__
			iSize = sizeof(char*) + PaddingSize + sizeof(DATA) + sizeof(bool) + sizeof(char*) + sizeof(st_BLOCK_NODE*);
			pNewNode = (st_BLOCK_NODE*)_aligned_malloc(iSize, CacheLineLength);
			pNewNode->FrontGuard = m_Code;
			pNewNode->RearGuard = m_Code;
			pNewNode->bInitialize = false;
#else
			iSize = sizeof(DATA) + sizeof(bool) + sizeof(st_BLOCK_NODE*);
			pNewNode = (st_BLOCK_NODE*)_aligned_malloc(iSize, en_CACHE_ALIGN);
			pNewNode->bInitialize = false;
#endif
			//----------------------------------------------------------------------
			// �޸� �Ҵ� �� ������ ȣ�� ���� �Ǵ�.
			// 
			// 
			// �޸�Ǯ TLS�� ���� �Լ� �ϳ� ���� ������.
			//----------------------------------------------------------------------
			if (m_bInitialize == true && m_bPlacementNew == false)
			{
				new (&pNewNode->Data) DATA();
			}

			st_BLOCK_NODE* pNode;
			do
			{
				pNode = m_Top.pNode;
				pNewNode->pNextNode = pNode;
			} while (CAS(&m_Top, pNewNode, pNode) != (LONG64)pNode);
		}

		DATA* Alloc()
		{
			long lCapacity = m_lCapacity;
			if (lCapacity < InterlockedIncrement(&m_lUseCount))
			{
				NewNode();
				InterlockedIncrement(&m_lCapacity);
			}

			alignas(16) st_TOP Top;
			Top.Key = m_Top.Key;
			Top.pNode = m_Top.pNode;
			while (!DCAS(&m_Top, Top.Key + 1, Top.pNode->pNextNode, &Top)) {};

			DATA* pData = &(Top.pNode->Data);
			if (m_bPlacementNew == true)
			{
				new (pData) DATA();
			}

			return pData;
		}

		bool Free(DATA* pData)
		{
			st_BLOCK_NODE* pReturnNode;

#ifdef __SAFE_MODE__
			pReturnNode = (st_BLOCK_NODE*)((char*)pData - (sizeof(char*) + en_PADDING_SIZE));	// 64byte �ڷ� �̵�
			if (m_Code != pReturnNode->FrontGuard || m_Code != pReturnNode->RearGuard)
			{
				return false;
			}
#else
			pReturnNode = (st_BLOCK_NODE*)pData;
#endif

			if (m_bPlacementNew == true)
			{
				pData->~DATA();
			}

			st_BLOCK_NODE* pNode;
			do
			{
				pNode = m_Top.pNode;
				pReturnNode->pNextNode = pNode;
			} while (CAS(&m_Top, pReturnNode, pNode) != (LONG64)pNode);

			InterlockedDecrement(&m_lUseCount);

			return true;
		}

		long GetCapacity() { return m_lCapacity; }
		long GetUseCount() { return m_lUseCount; }

	private:
		alignas(CacheLineLength)
		bool	m_bPlacementNew = false;
		bool	m_bInitialize = false;
#ifdef __SAFE_MODE__
		char*	m_Code;
#endif
		alignas(CacheLineLength) st_TOP m_Top;
		alignas(CacheLineLength) long m_lUseCount = 0;
		alignas(CacheLineLength) long m_lCapacity = 0;
	};
}