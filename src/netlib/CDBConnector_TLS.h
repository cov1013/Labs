#pragma once

#include <list>
#include "CDBConnector.h"

using namespace std;

namespace cov1013
{
	class CDBConnector_TLS
	{
	public:
		/********************************************************************/
		// Ŭ���� �⺻ ����
		//
		// eCONNECTOR_MAX : ���� ���� Connector �ִ� ����
		// eQUERY_MAX_LEN : ���� �ִ� ����
		/********************************************************************/
		enum en_CONFIG
		{
			eCONNECTOR_MAX = 100,
			eQUERY_MAX_LEN = 2048,
		};

	public:
		CDBConnector_TLS(const WCHAR* szDBIP, const WCHAR* szUser, const WCHAR* szPassword, const WCHAR* szDBName, const int iDBPort);
		virtual ~CDBConnector_TLS();
		bool Disconnect(void);
		bool Query(const WCHAR* szStringFormat, ...);
		bool Query_Save(const WCHAR* szStringFormat, ...);
		MYSQL_ROW FetchRow(void);
		void FreeResult(void);
		void UTF16ToUTF8(const WCHAR* szSrc, char* szDest);
		void UTF8ToUTF16(const char* szSrc, WCHAR* szDest);

	private:
		void LockConnectors(void);
		void UnlockConnectors(void);

	private:
		DWORD					m_dwTlsIndex;
		list<CDBConnector*>		m_pDBConnectors;
		SRWLOCK					m_DBConnectors_srw;
		WCHAR					m_szDBIP[16];
		WCHAR					m_szDBUser[64];
		WCHAR					m_szDBPassword[64];
		WCHAR					m_szDBName[64];
		int						m_iDBPort;
	};
}