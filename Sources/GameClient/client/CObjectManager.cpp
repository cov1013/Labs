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
		// ��� �ȿ� �ִ� Object ����
		//---------------------------------------
		delete (*iter);

		//---------------------------------------
		// ��� ��ü�� ���� �� ���� ��带 ����Ű�� ���ͷ����� ��ȯ
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
			// ã������ ��� ���� ������ ���� �� ��� ���� �� TRUE ����
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
	// ���� ����� �������� ������ FALSE ����
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
			// ã������ �ش� ��ü�� ������ ����
			//---------------------------------------
			return (*iter);
		}
		else
		{
			++iter;
		}
	}

	//---------------------------------------
	// ������ NULL ����
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
			// ��� �ȿ� �ִ� BaseObject ����
			//---------------------------------------
			delete (*iter);

			//---------------------------------------
			// ��� ��ü�� ���� �� ���� ��带 ����Ű�� ���ͷ����� ��ȯ
			//---------------------------------------
			iter = m_cObjectList.erase(iter);
			++m_iObjectCount;
		}
		else
		{
			//---------------------------------------
			// ���� ����� �ƴϸ� �׳� ���� ���� �̵�
			//---------------------------------------
			++iter;
		}
	}
}

// ==========================================
// Y ���� �������� ����
// ==========================================
void CObjectManager::AscSortY(void)
{
	CList<CBaseObject*>::iterator iter1 = m_cObjectList.begin();
	CList<CBaseObject*>::iterator iter2 = m_cObjectList.begin();

	// -------------------------------------------------------------
	// N-1 ����ŭ �ݺ��Ѵ�.
	// -------------------------------------------------------------
	for (int i = 0; i < m_cObjectList.size() - 1; i++)
	{
		// -------------------------------------------------------------
		// �� ������ �������� ������ �ϳ��� ��ġ�� �������Ƿ�
		// �� �� ������ ���� ������ �� �ݺ� Ƚ���� �����Ѵ�.
		// -------------------------------------------------------------
		for (int j = 0; j < m_cObjectList.size() - 1 - i; j++)
		{
			// -------------------------------------------------------------
			// �� ������ ����
			// -------------------------------------------------------------
			++iter2;

			// -------------------------------------------------------------
			// ������Ʈ Ÿ���� ����Ʈ��� �������� �ڷ� �̵�
			// -------------------------------------------------------------
			if ((*iter1)->GetObjectType() == eEFFECT)
			{
				m_cObjectList.Swap(iter1.GetNode(), iter2.GetNode());

				// -------------------------------------------------------------
				// ���� ��ġ�� �ٲ����Ƿ�, iter�� ��ġ�� �����Ѵ�.
				// -------------------------------------------------------------
				iter2 = ++iter2;
				iter1 = --iter1;

				++iter1;

				continue;
			}

			// -------------------------------------------------------------
			// �װ� �ƴϸ� Y��ǥ�� ���ؼ� ����
			// -------------------------------------------------------------
			if ((*iter1)->GetCurY() > (*iter2)->GetCurY())
			{
				m_cObjectList.Swap(iter1.GetNode(), iter2.GetNode());

				// -------------------------------------------------------------
				// ���� ��ġ�� �ٲ����Ƿ�, iter�� ��ġ�� �����Ѵ�.
				// -------------------------------------------------------------
				iter2 = ++iter2;
				iter1 = --iter1;
			}

			// -------------------------------------------------------------
			// ���� �����ͷ� ����.
			// ���� ������ �Ͼ�ٸ� ���� ��带 ����Ű�� iter�� ������ �����.
			// -------------------------------------------------------------
			++iter1;
		}

		// -------------------------------------------------------------
		// ���� �������� ����� �� ���� iter�� �����Ѵ�.
		// -------------------------------------------------------------
		iter1 = m_cObjectList.begin();
		iter2 = m_cObjectList.begin();
	}
}

void CObjectManager::Update()
{
	//---------------------------------------
	// �̹� �����ӿ� ������ ������Ʈ�� �����Ѵ�.
	//---------------------------------------
	DestroyProc();

	//---------------------------------------
	// ��� ��ü�� ��ȸ�ϸ� Update ȣ��
	//---------------------------------------
	CList<CBaseObject*>::iterator iter = m_cObjectList.begin();
	for (; iter != m_cObjectList.end(); ++iter)
	{
		(*iter)->Update();
	}

	//---------------------------------------
	// Y �� �������� ����
	//---------------------------------------
	AscSortY();
}

void CObjectManager::Render(BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch)
{
	//---------------------------------------
	// ��� ��ü�� ��ȸ�ϸ� Render ȣ��
	//---------------------------------------
	CList<CBaseObject*>::iterator iter = m_cObjectList.begin();
	for (; iter != m_cObjectList.end(); ++iter)
	{
		(*iter)->Render(bypDest, iDestWidth, iDestHeight, iDestPitch);
	}
}
