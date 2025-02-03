
#pragma comment(lib, "winmm.lib")

#include "DBConnector.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// ������
//////////////////////////////////////////////////////////////////////
cov1013::DBConnector::DBConnector(const WCHAR* szDBIP, const WCHAR* szUser, const WCHAR* szPassword, const WCHAR* szDBName, const int iDBPort)
{
	mysql_init(&m_MySQL);
	wcscpy_s(m_szDBIP, 16, szDBIP);
	wcscpy_s(m_szDBUser, 64, szUser);
	wcscpy_s(m_szDBPassword, 64, szPassword);
	wcscpy_s(m_szDBName, 64, szDBName);
	m_iDBPort = iDBPort;
	InitializeSRWLock(&m_srw);
}

//////////////////////////////////////////////////////////////////////
// �Ҹ���
//////////////////////////////////////////////////////////////////////
cov1013::DBConnector::~DBConnector()
{

}

//////////////////////////////////////////////////////////////////////
// MySQL DB ����
//////////////////////////////////////////////////////////////////////
bool cov1013::DBConnector::Connect(void)
{
	//-------------------------------------------------------------
	// ���� ������ UTF-8 ���ڵ����� ����
	//-------------------------------------------------------------
	char szDBIP_ASCII[16];
	char szDBUser_ASCII[16];
	char szDBPassword_ASCII[64];
	char szDBName_ASCII[64];
	size_t iSize16 = 16;
	size_t iSize64 = 64;
	wcstombs_s(&iSize16, szDBIP_ASCII, m_szDBIP, 16);
	wcstombs_s(&iSize64, szDBUser_ASCII, m_szDBUser, 64);
	wcstombs_s(&iSize64, szDBPassword_ASCII, m_szDBPassword, 64);
	wcstombs_s(&iSize64, szDBName_ASCII, m_szDBName, 64);

	//-------------------------------------------------------------
	// DB ����
	//-------------------------------------------------------------
	m_pMySQL = mysql_real_connect(&m_MySQL, szDBIP_ASCII, szDBUser_ASCII, szDBPassword_ASCII, szDBName_ASCII, m_iDBPort, (char*)NULL, 0);
	if (m_pMySQL == nullptr)
	{
		//-------------------------------------------------------------
		// ���ῡ �����ߴٸ� ���� ������ �����.
		//-------------------------------------------------------------
		SaveLastError();

		return false;
	}

	//-------------------------------------------------------------
	// �ѱ� ����� ���� �߰�
	//-------------------------------------------------------------
	mysql_set_character_set(m_pMySQL, "utf8");
	//	mysql_query(connection,"set session character_set_connection=euckr;");
	//	mysql_query(connection,"set session character_set_results=euckr;");
	//	mysql_query(connection,"set session character_set_client=euckr;");

	return true;
}

//////////////////////////////////////////////////////////////////////
// MySQL DB ����
//////////////////////////////////////////////////////////////////////
bool cov1013::DBConnector::Disconnect(void)
{
	mysql_close(m_pMySQL);

	return true;
}


//////////////////////////////////////////////////////////////////////
// MySQL ���� ������
//////////////////////////////////////////////////////////////////////
#ifdef __TLS_MODE__
bool cov1013::DBConnector::Query(void)
{
	//-------------------------------------------------------------
	// ������ UTF-8�� ���ڵ�
	//-------------------------------------------------------------
	UTF16ToUTF8(m_szQuery, m_szQueryUTF8);

	DWORD	dwStartTime = timeGetTime();
	int		iQueryStat	= mysql_query(m_pMySQL, m_szQueryUTF8);
	DWORD	dwCurTime	= timeGetTime();
	DWORD	dwElapsed	= dwCurTime - dwStartTime;

	//-------------------------------------------------------------
	// ���� ���� �ð� üũ
	// 
	// ������ �ִ�ġ�� �ʰ��ϸ� �α׸� �����.
	//-------------------------------------------------------------
	if (dwElapsed >= eQUERY_TIME_OUT)
	{
		LOG(L"Query", en_LOG_LEVEL_DEBUG, L"Query Over Time : %s (%dms)", m_szQuery, dwElapsed);
	}

	//-------------------------------------------------------------
	// ������ �����ߴٸ�, ���� ���� ����
	//-------------------------------------------------------------
	if (0 != iQueryStat)
	{
		SaveLastError();

		return false;
	}

	return true;
}
#else
bool cov1013::DBConnector::Query(const WCHAR* szStringFormat, ...)
{
	//-------------------------------------------------------------
	// ���� ���ڸ� ������ ����
	//-------------------------------------------------------------
	va_list va;
	va_start(va, szStringFormat);
	StringCchVPrintf(m_szQuery, eQUERY_MAX_LEN, szStringFormat, va);
	va_end(va);

	//-------------------------------------------------------------
	// ������ UTF-8�� ���ڵ�
	//-------------------------------------------------------------
	UTF16ToUTF8(m_szQuery, m_szQueryUTF8);

	DWORD	dwStartTime = timeGetTime();
	int		iQueryStat = mysql_query(m_pMySQL, m_szQueryUTF8);
	DWORD	dwCurTime = timeGetTime();
	DWORD	dwElapsed = dwCurTime - dwStartTime;

	//-------------------------------------------------------------
	// ���� ���� �ð� üũ
	// 
	// ������ �ִ�ġ�� �ʰ��ϸ� �α׸� �����.
	//-------------------------------------------------------------
	if (dwElapsed >= eQUERY_TIME_OUT)
	{
		//LOG(L"Query", en_LOG_LEVEL_DEBUG, L"Query Over Time : %s (%dms)", m_szQuery, dwElapsed);
	}

	//-------------------------------------------------------------
	// ������ �����ߴٸ�, ���� ���� ����
	//-------------------------------------------------------------
	if (0 != iQueryStat)
	{
		SaveLastError();

		return false;
	}

	return true;
}
#endif

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
#ifdef __TLS_MODE__
bool cov1013::DBConnector::Query_Save(void)
{
	//-------------------------------------------------------------
	// ������ UTF-8�� ���ڵ�
	//-------------------------------------------------------------
	UTF16ToUTF8(m_szQuery, m_szQueryUTF8);

	DWORD	dwStartTime = timeGetTime();
	int		iQueryStat	= mysql_query(m_pMySQL, m_szQueryUTF8);
	DWORD	dwCurTime	= timeGetTime();
	DWORD	dwElapsed	= dwCurTime - dwStartTime;

	//-------------------------------------------------------------
	// ���� ���� �ð� üũ
	// 
	// ������ �ִ�ġ�� �ʰ��ϸ� �α׸� �����.
	//-------------------------------------------------------------
	if (dwElapsed >= eQUERY_TIME_OUT)
	{
		LOG(L"Query", en_LOG_LEVEL_DEBUG, L"Query Over Time : %s (%dms)", m_szQuery, dwElapsed);
	}

	//-------------------------------------------------------------
	// ������ �����ߴٸ�, ���� ���� ����
	//-------------------------------------------------------------
	if (0 != iQueryStat)
	{
		SaveLastError();

		return false;
	}

	//-------------------------------------------------------------
	// ���� ���� ����� �޸𸮿� ����
	//-------------------------------------------------------------
	m_pSqlResult = mysql_store_result(m_pMySQL);

	return true;
}
#else
bool cov1013::DBConnector::Query_Save(const WCHAR* szStringFormat, ...)
{
	//-------------------------------------------------------------
	// ���� ���ڸ� ������ ����
	//-------------------------------------------------------------
	va_list va;
	va_start(va, szStringFormat);
	StringCchVPrintf(m_szQuery, eQUERY_MAX_LEN, szStringFormat, va);
	va_end(va);

	//-------------------------------------------------------------
	// ������ UTF-8�� ���ڵ�
	//-------------------------------------------------------------
	UTF16ToUTF8(m_szQuery, m_szQueryUTF8);

	DWORD	dwStartTime = timeGetTime();
	int		iQueryStat = mysql_query(m_pMySQL, m_szQueryUTF8);
	DWORD	dwCurTime = timeGetTime();
	DWORD	dwElapsed = dwCurTime - dwStartTime;

	//-------------------------------------------------------------
	// ���� ���� �ð� üũ
	// 
	// ������ �ִ�ġ�� �ʰ��ϸ� �α׸� �����.
	//-------------------------------------------------------------
	if (dwElapsed >= eQUERY_TIME_OUT)
	{
		//LOG(L"Query", en_LOG_LEVEL_DEBUG, L"Query Over Time : %s (%dms)", m_szQuery, dwElapsed);
	}

	//-------------------------------------------------------------
	// ������ �����ߴٸ�, ���� ���� ����
	//-------------------------------------------------------------
	if (0 != iQueryStat)
	{
		SaveLastError();

		return false;
	}

	//-------------------------------------------------------------
	// ���� ���� ����� �޸𸮿� ����
	//-------------------------------------------------------------
	m_pSqlResult = mysql_store_result(m_pMySQL);

	return true;
}
#endif

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
MYSQL_ROW cov1013::DBConnector::FetchRow(void)
{
	return mysql_fetch_row(m_pSqlResult);
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
void cov1013::DBConnector::FreeResult(void)
{
	mysql_free_result(m_pSqlResult);
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
void cov1013::DBConnector::SaveLastError(void)
{
	m_iLastError = mysql_errno(&m_MySQL);
	UTF8ToUTF16(mysql_error(&m_MySQL), m_szLastErrorMsg);
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
void cov1013::DBConnector::UTF16ToUTF8(const WCHAR* szSrc, char* szDest)
{
	int iLen = WideCharToMultiByte(CP_ACP, 0, szSrc, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, szSrc, -1, szDest, iLen, NULL, NULL);
}

//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
void cov1013::DBConnector::UTF8ToUTF16(const char* szSrc, WCHAR* szDest)
{
	int iLen = MultiByteToWideChar(CP_ACP, 0, szSrc, strlen(szSrc), NULL, NULL);
	MultiByteToWideChar(CP_ACP, 0, szSrc, strlen(szSrc), szDest, iLen);
}
