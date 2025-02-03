
#pragma comment(lib, "winmm.lib")

#include "DBConnector.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// 생성자
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
// 소멸자
//////////////////////////////////////////////////////////////////////
cov1013::DBConnector::~DBConnector()
{

}

//////////////////////////////////////////////////////////////////////
// MySQL DB 연결
//////////////////////////////////////////////////////////////////////
bool cov1013::DBConnector::Connect(void)
{
	//-------------------------------------------------------------
	// 접속 정보를 UTF-8 인코딩으로 변경
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
	// DB 연결
	//-------------------------------------------------------------
	m_pMySQL = mysql_real_connect(&m_MySQL, szDBIP_ASCII, szDBUser_ASCII, szDBPassword_ASCII, szDBName_ASCII, m_iDBPort, (char*)NULL, 0);
	if (m_pMySQL == nullptr)
	{
		//-------------------------------------------------------------
		// 연결에 실패했다면 에러 사항을 남긴다.
		//-------------------------------------------------------------
		SaveLastError();

		return false;
	}

	//-------------------------------------------------------------
	// 한글 사용을 위해 추가
	//-------------------------------------------------------------
	mysql_set_character_set(m_pMySQL, "utf8");
	//	mysql_query(connection,"set session character_set_connection=euckr;");
	//	mysql_query(connection,"set session character_set_results=euckr;");
	//	mysql_query(connection,"set session character_set_client=euckr;");

	return true;
}

//////////////////////////////////////////////////////////////////////
// MySQL DB 끊기
//////////////////////////////////////////////////////////////////////
bool cov1013::DBConnector::Disconnect(void)
{
	mysql_close(m_pMySQL);

	return true;
}


//////////////////////////////////////////////////////////////////////
// MySQL 쿼리 날리기
//////////////////////////////////////////////////////////////////////
#ifdef __TLS_MODE__
bool cov1013::DBConnector::Query(void)
{
	//-------------------------------------------------------------
	// 쿼리를 UTF-8로 인코딩
	//-------------------------------------------------------------
	UTF16ToUTF8(m_szQuery, m_szQueryUTF8);

	DWORD	dwStartTime = timeGetTime();
	int		iQueryStat	= mysql_query(m_pMySQL, m_szQueryUTF8);
	DWORD	dwCurTime	= timeGetTime();
	DWORD	dwElapsed	= dwCurTime - dwStartTime;

	//-------------------------------------------------------------
	// 쿼리 실행 시간 체크
	// 
	// 설정한 최대치가 초과하면 로그를 남긴다.
	//-------------------------------------------------------------
	if (dwElapsed >= eQUERY_TIME_OUT)
	{
		LOG(L"Query", en_LOG_LEVEL_DEBUG, L"Query Over Time : %s (%dms)", m_szQuery, dwElapsed);
	}

	//-------------------------------------------------------------
	// 쿼리가 실패했다면, 에러 사항 저장
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
	// 가변 인자를 쿼리에 적용
	//-------------------------------------------------------------
	va_list va;
	va_start(va, szStringFormat);
	StringCchVPrintf(m_szQuery, eQUERY_MAX_LEN, szStringFormat, va);
	va_end(va);

	//-------------------------------------------------------------
	// 쿼리를 UTF-8로 인코딩
	//-------------------------------------------------------------
	UTF16ToUTF8(m_szQuery, m_szQueryUTF8);

	DWORD	dwStartTime = timeGetTime();
	int		iQueryStat = mysql_query(m_pMySQL, m_szQueryUTF8);
	DWORD	dwCurTime = timeGetTime();
	DWORD	dwElapsed = dwCurTime - dwStartTime;

	//-------------------------------------------------------------
	// 쿼리 실행 시간 체크
	// 
	// 설정한 최대치가 초과하면 로그를 남긴다.
	//-------------------------------------------------------------
	if (dwElapsed >= eQUERY_TIME_OUT)
	{
		//LOG(L"Query", en_LOG_LEVEL_DEBUG, L"Query Over Time : %s (%dms)", m_szQuery, dwElapsed);
	}

	//-------------------------------------------------------------
	// 쿼리가 실패했다면, 에러 사항 저장
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
	// 쿼리를 UTF-8로 인코딩
	//-------------------------------------------------------------
	UTF16ToUTF8(m_szQuery, m_szQueryUTF8);

	DWORD	dwStartTime = timeGetTime();
	int		iQueryStat	= mysql_query(m_pMySQL, m_szQueryUTF8);
	DWORD	dwCurTime	= timeGetTime();
	DWORD	dwElapsed	= dwCurTime - dwStartTime;

	//-------------------------------------------------------------
	// 쿼리 실행 시간 체크
	// 
	// 설정한 최대치가 초과하면 로그를 남긴다.
	//-------------------------------------------------------------
	if (dwElapsed >= eQUERY_TIME_OUT)
	{
		LOG(L"Query", en_LOG_LEVEL_DEBUG, L"Query Over Time : %s (%dms)", m_szQuery, dwElapsed);
	}

	//-------------------------------------------------------------
	// 쿼리가 실패했다면, 에러 사항 저장
	//-------------------------------------------------------------
	if (0 != iQueryStat)
	{
		SaveLastError();

		return false;
	}

	//-------------------------------------------------------------
	// 쿼리 실행 결과를 메모리에 저장
	//-------------------------------------------------------------
	m_pSqlResult = mysql_store_result(m_pMySQL);

	return true;
}
#else
bool cov1013::DBConnector::Query_Save(const WCHAR* szStringFormat, ...)
{
	//-------------------------------------------------------------
	// 가변 인자를 쿼리에 적용
	//-------------------------------------------------------------
	va_list va;
	va_start(va, szStringFormat);
	StringCchVPrintf(m_szQuery, eQUERY_MAX_LEN, szStringFormat, va);
	va_end(va);

	//-------------------------------------------------------------
	// 쿼리를 UTF-8로 인코딩
	//-------------------------------------------------------------
	UTF16ToUTF8(m_szQuery, m_szQueryUTF8);

	DWORD	dwStartTime = timeGetTime();
	int		iQueryStat = mysql_query(m_pMySQL, m_szQueryUTF8);
	DWORD	dwCurTime = timeGetTime();
	DWORD	dwElapsed = dwCurTime - dwStartTime;

	//-------------------------------------------------------------
	// 쿼리 실행 시간 체크
	// 
	// 설정한 최대치가 초과하면 로그를 남긴다.
	//-------------------------------------------------------------
	if (dwElapsed >= eQUERY_TIME_OUT)
	{
		//LOG(L"Query", en_LOG_LEVEL_DEBUG, L"Query Over Time : %s (%dms)", m_szQuery, dwElapsed);
	}

	//-------------------------------------------------------------
	// 쿼리가 실패했다면, 에러 사항 저장
	//-------------------------------------------------------------
	if (0 != iQueryStat)
	{
		SaveLastError();

		return false;
	}

	//-------------------------------------------------------------
	// 쿼리 실행 결과를 메모리에 저장
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
