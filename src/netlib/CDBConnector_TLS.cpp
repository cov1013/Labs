#include "CDBConnector_TLS.h"

namespace cov1013
{
	CDBConnector_TLS::CDBConnector_TLS(const WCHAR* szDBIP, const WCHAR* szUser, const WCHAR* szPassword, const WCHAR* szDBName, const int iDBPort)
	{
		m_dwTlsIndex = TlsAlloc();
		wcscpy_s(m_szDBIP, 16, szDBIP);
		wcscpy_s(m_szDBUser, 64, szUser);
		wcscpy_s(m_szDBPassword, 64, szPassword);
		wcscpy_s(m_szDBName, 64, szDBName);
		m_iDBPort = iDBPort;
		InitializeSRWLock(&m_DBConnectors_srw);
	}
	
	CDBConnector_TLS::~CDBConnector_TLS()
	{
		list<CDBConnector*>::iterator iter = m_pDBConnectors.begin();
		for (iter; iter != m_pDBConnectors.end();)
		{
			delete (*iter);
			iter = m_pDBConnectors.erase(iter);
		}
		TlsFree(m_dwTlsIndex);
	}

	bool CDBConnector_TLS::Disconnect(void)
	{
		CDBConnector* pDBConnector = (CDBConnector*)TlsGetValue(m_dwTlsIndex);
		if (pDBConnector == nullptr)
		{
			return false;
		}

		pDBConnector->Disconnect();

		LockConnectors();
		list<CDBConnector*>::iterator iter = m_pDBConnectors.begin();
		for (iter; iter != m_pDBConnectors.end(); ++iter)
		{
			if (*iter == pDBConnector)
			{
				delete (*iter);
				m_pDBConnectors.erase(iter);

				UnlockConnectors();
				return true;
			}
		}
		UnlockConnectors();

		return false;
	}

	bool CDBConnector_TLS::Query(const WCHAR* szStringFormat, ...)
	{
		CDBConnector* pDBConnector = (CDBConnector*)TlsGetValue(m_dwTlsIndex);
		if (pDBConnector == nullptr)
		{
			pDBConnector = new CDBConnector(m_szDBIP, m_szDBUser, m_szDBPassword, m_szDBName, m_iDBPort);
			pDBConnector->Connect();
			TlsSetValue(m_dwTlsIndex, (LPVOID)pDBConnector);

			LockConnectors();
			m_pDBConnectors.push_back(pDBConnector);
			UnlockConnectors();
		}

		va_list va;
		va_start(va, szStringFormat);
		vswprintf_s(pDBConnector->m_szQuery, eQUERY_MAX_LEN, szStringFormat, va);
		va_end(va);

		return pDBConnector->Query();
	}

	bool CDBConnector_TLS::Query_Save(const WCHAR* szStringFormat, ...)
	{
		CDBConnector* pDBConnector = (CDBConnector*)TlsGetValue(m_dwTlsIndex);
		if (pDBConnector == nullptr)
		{
			pDBConnector = new CDBConnector(m_szDBIP, m_szDBUser, m_szDBPassword, m_szDBName, m_iDBPort);
			pDBConnector->Connect();
			TlsSetValue(m_dwTlsIndex, (LPVOID)pDBConnector);

			LockConnectors();
			m_pDBConnectors.push_back(pDBConnector);
			UnlockConnectors();
		}

		va_list va;
		va_start(va, szStringFormat);
		vswprintf_s(pDBConnector->m_szQuery, eQUERY_MAX_LEN, szStringFormat, va);
		va_end(va);

		return pDBConnector->Query_Save();
	}

	MYSQL_ROW CDBConnector_TLS::FetchRow(void)
	{
		CDBConnector* pDBConnector = (CDBConnector*)TlsGetValue(m_dwTlsIndex);
		if (pDBConnector == nullptr)
		{
			return nullptr;
		}

		return pDBConnector->FetchRow();
	}

	void CDBConnector_TLS::FreeResult(void)
	{
		CDBConnector* pDBConnector = (CDBConnector*)TlsGetValue(m_dwTlsIndex);
		if (pDBConnector == nullptr)
		{
			return;
		}

		return pDBConnector->FreeResult();
	}

	void CDBConnector_TLS::UTF16ToUTF8(const WCHAR* szSrc, char* szDest)
	{
		CDBConnector* pDBConnector = (CDBConnector*)TlsGetValue(m_dwTlsIndex);
		if (pDBConnector == nullptr)
		{
			return;
		}

		pDBConnector->UTF16ToUTF8(szSrc, szDest);
	}

	void CDBConnector_TLS::UTF8ToUTF16(const char* szSrc, WCHAR* szDest)
	{
		CDBConnector* pDBConnector = (CDBConnector*)TlsGetValue(m_dwTlsIndex);
		if (pDBConnector == nullptr)
		{
			return;
		}

		pDBConnector->UTF8ToUTF16(szSrc, szDest);
	}

	void CDBConnector_TLS::LockConnectors(void)
	{
		AcquireSRWLockExclusive(&m_DBConnectors_srw);
	}

	void CDBConnector_TLS::UnlockConnectors(void)
	{
		ReleaseSRWLockExclusive(&m_DBConnectors_srw);
	}
}