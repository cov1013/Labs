#include "framework.h"
#include "resource.h"

// COMMOM
#include "CList.h"
#include "Protocol.h"
#include "Define.h"
#include "Struct.h"

// SYSTEM
#include "CLogger.h"
#include "profiler.h"

// CONTENTS
#include "CBaseObject.h"		
#include "CPlayerObject.h"		
#include "CEffectObject.h"		

#include "CObjectManager.h"	

CObjectManager::CObjectManager()
{
	m_iObjectCount = 0;
}

CObjectManager::~CObjectManager()
{
	CList<CBaseObject*>::iterator iter = m_cObjectList.begin();
	for (; iter != m_cObjectList.end();)
	{
		//---------------------------------------
		// 노드 안에 있는 Object 삭제
		//---------------------------------------
		delete (*iter);

		//---------------------------------------
		// 노드 자체를 삭제 후 다음 노드를 가리키는 이터레이터 반환
		//---------------------------------------
		iter = m_cObjectList.erase(iter);
	}
}

BOOL CObjectManager::AddObject(CBaseObject* cObject)
{
	if (NULL == cObject)
	{
		return FALSE;
	}

	m_cObjectList.push_back(cObject);
	++m_iObjectCount;

	return TRUE;
}

BOOL CObjectManager::DeleteObject(int iObjectID)
{
	CList<CBaseObject*>::iterator iter = m_cObjectList.begin();
	for (iter; iter != m_cObjectList.end();)
	{
		if ((*iter)->GetObjectID() == iObjectID)
		{
			//---------------------------------------
			// 찾았으면 노드 안의 포인터 삭제 후 노드 삭제 후 TRUE 리턴
			//---------------------------------------
			(*iter)->SetObjectID(-1);
			delete (*iter);
			m_cObjectList.erase(iter);

			--m_iObjectCount;
			return TRUE;
		}
		else
		{
			++iter;
		}
	}

	//---------------------------------------
	// 삭제 대상이 존재하지 않으면 FALSE 리턴
	//---------------------------------------
	return FALSE;
}

CBaseObject* CObjectManager::FindObject(int iObjectID)
{
	CList<CBaseObject*>::iterator iter = m_cObjectList.begin();
	for (iter; iter != m_cObjectList.end();)
	{
		if ((*iter)->GetObjectID() == iObjectID)
		{
			//---------------------------------------
			// 찾았으면 해당 객체의 포인터 리턴
			//---------------------------------------
			return (*iter);
		}
		else
		{
			++iter;
		}
	}

	//---------------------------------------
	// 없으면 NULL 리턴
	//---------------------------------------
	return NULL;
}

void CObjectManager::DestroyProc()
{
	BOOL bDestoryFlag;
	CList<CBaseObject*>::iterator iter = m_cObjectList.begin();

	for (; iter != m_cObjectList.end();)
	{
		bDestoryFlag = (*iter)->IsDestroy();

		if (bDestoryFlag)
		{
			//---------------------------------------
			// 노드 안에 있는 BaseObject 삭제
			//---------------------------------------
			delete (*iter);

			//---------------------------------------
			// 노드 자체를 삭제 후 다음 노드를 가리키는 이터레이터 반환
			//---------------------------------------
			iter = m_cObjectList.erase(iter);
			++m_iObjectCount;
		}
		else
		{
			//---------------------------------------
			// 삭제 대상이 아니면 그냥 다음 노드로 이동
			//---------------------------------------
			++iter;
		}
	}
}

// ==========================================
// Y 기준 오름차순 정렬
// ==========================================
void CObjectManager::AscSortY(void)
{
	CList<CBaseObject*>::iterator iter1 = m_cObjectList.begin();
	CList<CBaseObject*>::iterator iter2 = m_cObjectList.begin();

	// -------------------------------------------------------------
	// N-1 개만큼 반복한다.
	// -------------------------------------------------------------
	for (int i = 0; i < m_cObjectList.size() - 1; i++)
	{
		// -------------------------------------------------------------
		// 한 루프를 돌때마다 데이터 하나의 위치는 정해지므로
		// 매 번 루프를 새로 시작할 때 반복 횟수를 차감한다.
		// -------------------------------------------------------------
		for (int j = 0; j < m_cObjectList.size() - 1 - i; j++)
		{
			// -------------------------------------------------------------
			// 비교 데이터 갱신
			// -------------------------------------------------------------
			++iter2;

			// -------------------------------------------------------------
			// 오브젝트 타입이 이펙트라면 묻지말고 뒤로 이동
			// -------------------------------------------------------------
			if ((*iter1)->GetObjectType() == eEFFECT)
			{
				m_cObjectList.Swap(iter1.GetNode(), iter2.GetNode());

				// -------------------------------------------------------------
				// 둘의 위치가 바꼈으므로, iter의 위치를 변경한다.
				// -------------------------------------------------------------
				iter2 = ++iter2;
				iter1 = --iter1;

				++iter1;

				continue;
			}

			// -------------------------------------------------------------
			// 그게 아니면 Y좌표를 비교해서 스왑
			// -------------------------------------------------------------
			if ((*iter1)->GetCurY() > (*iter2)->GetCurY())
			{
				m_cObjectList.Swap(iter1.GetNode(), iter2.GetNode());

				// -------------------------------------------------------------
				// 둘의 위치가 바꼈으므로, iter의 위치를 변경한다.
				// -------------------------------------------------------------
				iter2 = ++iter2;
				iter1 = --iter1;
			}

			// -------------------------------------------------------------
			// 다음 데이터로 갱신.
			// 만약 스왑이 일어났다면 이전 노드를 가리키는 iter를 앞으로 땡긴다.
			// -------------------------------------------------------------
			++iter1;
		}

		// -------------------------------------------------------------
		// 안쪽 루프문이 종료될 때 마다 iter를 갱신한다.
		// -------------------------------------------------------------
		iter1 = m_cObjectList.begin();
		iter2 = m_cObjectList.begin();
	}
}

void CObjectManager::Update()
{
	//---------------------------------------
	// 이번 프레임에 삭제할 오브젝트를 삭제한다.
	//---------------------------------------
	DestroyProc();

	//---------------------------------------
	// 모든 객체를 순회하며 Update 호출
	//---------------------------------------
	CList<CBaseObject*>::iterator iter = m_cObjectList.begin();
	for (; iter != m_cObjectList.end(); ++iter)
	{
		(*iter)->Update();
	}

	//---------------------------------------
	// Y 축 오름차순 정렬
	//---------------------------------------
	AscSortY();
}

void CObjectManager::Render(BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch)
{
	//---------------------------------------
	// 모든 객체를 순회하며 Render 호출
	//---------------------------------------
	CList<CBaseObject*>::iterator iter = m_cObjectList.begin();
	for (; iter != m_cObjectList.end(); ++iter)
	{
		(*iter)->Render(bypDest, iDestWidth, iDestHeight, iDestPitch);
	}
}
