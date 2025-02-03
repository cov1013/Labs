#pragma once
#include "Common.h"
#include "libmysql/includes/mysql.h"
#include "libmysql/includes/errmsg.h"
//#include "_lib/mysql/include/mysql.h"
//#include "_lib/mysql/include/errmsg.h"

namespace cov1013
{
	class CDBConnector
	{
		friend class CDBConnector_TLS;

	public:
		/********************************************************************/
		// 클래스 기본 구성
		//
		// eQUERY_MAX_LEN  : 쿼리 최대 길이 (NULL포함)
		// eQUERY_TIME_OUT : 타임아웃 시간
		/********************************************************************/
		enum en_CONFIG
		{
			eQUERY_MAX_LEN = 2048,
			eQUERY_TIME_OUT = 20000
		};

		CDBConnector(const WCHAR* szDBIP, const WCHAR* szUser, const WCHAR* szPassword, const WCHAR* szDBName, const int iDBPort);
		virtual		~CDBConnector();
		bool		Connect(void);
		bool		Disconnect(void);
#ifdef __TLS_MODE__
		bool		Query(void);
#else
		bool		Query(const WCHAR* szStringFormat, ...);
#endif
#ifdef __TLS_MODE__
		bool		Query_Save(void);		// DBWriter 스레드의 Save 쿼리 전용
#else
		bool		Query_Save(const WCHAR* szStringFormat, ...);
#endif
		MYSQL_ROW	FetchRow(void);
		void		FreeResult(void);
		int			GetLastError(void) { return m_iLastError; };
		WCHAR*		GetLastErrorMsg(void) { return m_szLastErrorMsg; }
		void		UTF16ToUTF8(const WCHAR* szSrc, char* szDest);
		void		UTF8ToUTF16(const char* szSrc, WCHAR* szDest);
#ifndef __TLS_MODE__
		void		Lock(void) { AcquireSRWLockExclusive(&m_srw); };
		void		Unlock(void) { ReleaseSRWLockExclusive(&m_srw); };
#endif
	private:
		void		SaveLastError(void);

	private:
		MYSQL		m_MySQL;
		MYSQL*		m_pMySQL;
		MYSQL_RES*	m_pSqlResult;
		WCHAR		m_szDBIP[16];
		WCHAR		m_szDBUser[64];
		WCHAR		m_szDBPassword[64];
		WCHAR		m_szDBName[64];
		int			m_iDBPort;
		WCHAR		m_szQuery[eQUERY_MAX_LEN];
		char		m_szQueryUTF8[eQUERY_MAX_LEN];
		int			m_iLastError;
		WCHAR		m_szLastErrorMsg[128];
#ifndef __TLS_MODE__
		SRWLOCK		m_srw;
#endif
	};
}