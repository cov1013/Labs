#include "DBConnector_TLS.h"

//////////////////////////////////////////////////////////////////////
// ������
//////////////////////////////////////////////////////////////////////
cov1013::DBConnector_TLS::DBConnector_TLS(const WCHAR* szDBIP, const WCHAR* szUser, const WCHAR* szPassword, const WCHAR* szDBName, const int iDBPort)
{
	InitializeSRWLock(&m_srw);
	m_dwTlsIndex = TlsAlloc();
	wcscpy_s(m_szDBIP, 16, szDBIP);
	wcscpy_s(m_szDBUser, 64, szUser);
	wcscpy_s(m_szDBPassword, 64, szPassword);
	wcscpy_s(m_szDBName, 64, szDBName);
	m_iDBPort = iDBPort;
}

//////////////////////////////////////////////////////////////////////
// �Ҹ���
//////////////////////////////////////////////////////////////////////
cov1013::DBConnector_TLS::~DBConnector_TLS()
{
	list<DBConnector*>::iterator iter = m_pDBConnectors.begin();
	for (iter; iter != m_pDBConnectors.end();)
	{
		delete (*iter);
		iter = m_pDBConnectors.erase(iter);
	}
}

//////////////////////////////////////////////////////////////////////
// MySQL DB ����
//////////////////////////////////////////////////////////////////////
bool cov1013::DBConnector_TLS::Disconnect(void)
{
	DBConnector* pDBConnector = (DBConnector*)TlsGetValue(m_dwTlsIndex);
	if (pDBConnector == nullptr)
	{
		return false;
	}

	list<DBConnector*>::iterator iter = m_pDBConnectors.begin();
	for (iter; iter != m_pDBConnectors.end(); ++iter)
	{
		if (*iter == pDBConnector)
		{
			(*iter)->Disconnect();
			delete (*iter);
			m_pDBConnectors.erase(iter);

			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////
// MySQL ���� ������
//////////////////////////////////////////////////////////////////////
bool cov1013::DBConnector_TLS::Query(const WCHAR* szStringFormat, ...)
{
	DBConnector* pDBConnector = (DBConnector*)TlsGetValue(m_dwTlsIndex);
	if (pDBConnector == nullptr)
	{
		//AcquireSRWLockExclusive(&m_srw);
		pDBConnector = new DBConnector(m_szDBIP, m_szDBUser, m_szDBPassword, m_szDBName, m_iDBPort);
		pDBConnector->Connect();
		//ReleaseSRWLockExclusive(&m_srw);

		m_pDBConnectors.push_back(pDBConnector);

		TlsSetValue(m_dwTlsIndex, (LPVOID)pDBConnector);
	}

	va_list va;
	va_start(va, szStringFormat);
	vswprintf_s(pDBConnector->m_szQuery, eQUERY_MAX_LEN, szStringFormat, va);
	va_end(va);

	pDBConnector->Query(szStringFormat);

	return false;
}

//////////////////////////////////////////////////////////////////////
// MySQL ���� ������
//////////////////////////////////////////////////////////////////////
bool cov1013::DBConnector_TLS::Query_Save(const WCHAR* szStringFormat, ...)
{
	DBConnector* pDBConnector = (DBConnector*)TlsGetValue(m_dwTlsIndex);
	if (pDBConnector == nullptr)
	{
		//AcquireSRWLockExclusive(&m_srw);
		pDBConnector = new DBConnector(m_szDBIP, m_szDBUser, m_szDBPassword, m_szDBName, m_iDBPort);
		pDBConnector->Connect();
		//ReleaseSRWLockExclusive(&m_srw);

		m_pDBConnectors.push_back(pDBConnector);

		TlsSetValue(m_dwTlsIndex, (LPVOID)pDBConnector);
	}

	va_list va;
	va_start(va, szStringFormat);
	vswprintf_s(pDBConnector->m_szQuery, eQUERY_MAX_LEN, szStringFormat, va);
	va_end(va);

	pDBConnector->Query_Save(szStringFormat);

	return false;
}

//////////////////////////////////////////////////////////////////////
// ������ ���� �ڿ� ��� �̾ƿ���.
//
// ����� ���ٸ� NULL ����.
//////////////////////////////////////////////////////////////////////
MYSQL_ROW cov1013::DBConnector_TLS::FetchRow(void)
{
	DBConnector* pDBConnector = (DBConnector*)TlsGetValue(m_dwTlsIndex);
	if (pDBConnector == nullptr)
	{
		return nullptr;
	}

	return pDBConnector->FetchRow();
}

//////////////////////////////////////////////////////////////////////
// ����� ���� ������� ����
//////////////////////////////////////////////////////////////////////
void cov1013::DBConnector_TLS::FreeResult(void)
{
	DBConnector* pDBConnector = (DBConnector*)TlsGetValue(m_dwTlsIndex);
	if (pDBConnector == nullptr)
	{
		return;
	}

	return pDBConnector->FreeResult();
}
