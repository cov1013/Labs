#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "CLogger.h"

extern int g_iLogTime;

CLogger* CLogger::m_pInstance = nullptr;

CLogger::CLogger(const DWORD dwLogLevel)
{
	m_dwLogLevel = dwLogLevel;
	memset(m_szBuffer, 0, sizeof(WCHAR) * 1024);
}

CLogger::~CLogger()
{
}

void CLogger::ChangeToLogLevel(const DWORD dwLogLevel)
{
	if (dfLOG_LEVEL_OFF > dwLogLevel || dfLOG_LEVEL_INFO < dwLogLevel)
	{
		return;
	}

	m_dwLogLevel = dwLogLevel;
}

void CLogger::WriteLogToConsole(const DWORD dwLogLevel, const WCHAR* szFormat, ...)
{
	if (dfLOG_LEVEL_OFF > dwLogLevel || dfLOG_LEVEL_INFO < dwLogLevel)
	{
		return;
	}
	if (dwLogLevel != m_dwLogLevel)
	{
		return;
	}

	int iStartTime;

	va_list ap;
	WCHAR szTime[31];

	iStartTime = timeGetTime();

	//-------------------------
	// 시간 출력
	//-------------------------
	GetTimeStamp(szTime, 31);
	wprintf(L"%s", szTime);

	//-------------------------
	// 로그 내용 출력
	//-------------------------
	va_start(ap, szFormat);
	vwprintf_s(szFormat, ap);
	va_end(ap);

	g_iLogTime += timeGetTime() - iStartTime;
}

void CLogger::WriteLogToConsoleNotTime(const DWORD dwLogLevel, const WCHAR* szFormat, ...)
{
	if (dfLOG_LEVEL_OFF > dwLogLevel || dfLOG_LEVEL_INFO < dwLogLevel)
	{
		return;
	}
	if (dwLogLevel != m_dwLogLevel)
	{
		return;
	}

	int iStartTime;
	va_list ap;

	iStartTime = timeGetTime();

	// 로그 출력
	va_start(ap, szFormat);
	vwprintf_s(szFormat, ap);
	va_end(ap);

	g_iLogTime += timeGetTime() - iStartTime;
}

void CLogger::WriteLogToFile(const DWORD dwLogLevel, const WCHAR* szFormat, ...)
{
	if (dfLOG_LEVEL_OFF > dwLogLevel || dfLOG_LEVEL_INFO < dwLogLevel)
	{
		return;
	}

	int iStartTime;

	va_list ap;
	FILE* stream;
	WCHAR szTime[31];
	WCHAR szFileName[17];

	GetFileName(szFileName, 17);
	_wfopen_s(&stream, szFileName, L"a+");
	if (nullptr != stream)
	{
		iStartTime = timeGetTime();

		// 로그 타입
		switch (dwLogLevel)
		{
		case dfLOG_LEVEL_ERROR:
			fwprintf_s(stream, L"[ERROR] ");
			break;
		case dfLOG_LEVEL_WARN:
			fwprintf_s(stream, L"[WARNNING] ");
			break;
		case dfLOG_LEVEL_DEBUG:
			fwprintf_s(stream, L"[DEBUG] ");
			break;
		case dfLOG_LEVEL_INFO:
			fwprintf_s(stream, L"[INFO] ");
			break;
		default:
			break;
		}

		// 시간 쓰기
		GetTimeStamp(szTime, 31);
		fwprintf_s(stream, L"%s", szTime);

		// 로그 쓰기
		va_start(ap, szFormat);
		vswprintf_s(m_szBuffer, szFormat, ap);
		va_end(ap);
		fwprintf_s(stream, L"%s", m_szBuffer);

		// 정리
		//memset(m_szBuffer, 0, sizeof(WCHAR) * 1024);
		fclose(stream);

		g_iLogTime += timeGetTime() - iStartTime;
	}
}

void CLogger::GetFileName(WCHAR* szDest, const int iSize)
{
	if (nullptr == szDest || iSize < 1)
	{
		return;
	}

	time_t	Time;
	tm		Today;

	time(&Time);
	localtime_s(&Today, &Time);

	swprintf(szDest, iSize, L"%04d%02d Log.txt",
		(Today.tm_year + 1900),
		Today.tm_mon + 1
	);
}

void CLogger::GetTimeStamp(WCHAR* szDest, const int iSize)
{
	if (nullptr == szDest || iSize < 1)
	{
		return;
	}

	time_t	Time;
	tm		Today;

	time(&Time);
	localtime_s(&Today, &Time);

	swprintf_s(szDest, iSize, L"[%02d/%02d/%02d %02d:%02d:%02d]",
		Today.tm_mon + 1,
		Today.tm_mday,
		(Today.tm_year + 1900) - 2000,
		Today.tm_hour,
		Today.tm_min,
		Today.tm_sec
	);
}
