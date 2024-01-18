#pragma once

#define dfLOG_LEVEL_OFF		0
//---------------------------------------------
// 로거가 꺼져있는 상태의 로그 레벨
// 로거가 해당 로그 레벨로 설정되어 있으면 
// 아무런 로그도 남기지 않는다.
// 
//---------------------------------------------


#define dfLOG_LEVEL_ERROR		1
//---------------------------------------------
// 처리 중 문제가 발생했을 때 사용되는 로그 레벨
// 
// Format
// ##### ErrorName >>> Error Code
//---------------------------------------------


#define dfLOG_LEVEL_WARN		2
//---------------------------------------------
// 심각한 에러. 
// 프로세스가 종료되기 전에 남기는 로그 레벨
// 
//---------------------------------------------


#define dfLOG_LEVEL_DEBUG		3
//---------------------------------------------
// 디버깅에 필요한 로그를 남기는 로그 레벨
// 
//---------------------------------------------


#define dfLOG_LEVEL_INFO		4
//---------------------------------------------
// 상태, 정보들을 남기는 로그 레벨
// 
//---------------------------------------------

#define _LOG(LogLevel, fmt, ...)											 \
do{																			 \
	CLogger::GetInstance()->WriteLogToConsole(LogLevel, fmt, ##__VA_ARGS__); \
	CLogger::GetInstance()->WriteLogToFile(LogLevel, fmt, ##__VA_ARGS__);	 \
} while(0)																	 \

#define _LOG_CON(LogLevel, fmt, ...)										 \
do{																			 \
	CLogger::GetInstance()->WriteLogToConsole(LogLevel, fmt, ##__VA_ARGS__); \
} while(0)		

#define _LOG_NCON(LogLevel, fmt, ...)										 \
do{																			 \
	CLogger::GetInstance()->WriteLogToConsoleNotTime(LogLevel, fmt, ##__VA_ARGS__); \
} while(0)																	 \

#define _LOG_FILE(LogLevel, fmt, ...)										 \
do{																			 \
	CLogger::GetInstance()->WriteLogToFile(LogLevel, fmt, ##__VA_ARGS__);	 \
} while(0)																	 \

class CLogger
{
private:
	CLogger(const DWORD dwLogLevel);
	~CLogger();

public:
	static CLogger* GetInstance(void)
	{
		if (nullptr == m_pInstance)
		{
			m_pInstance = new CLogger(dfLOG_LEVEL_DEBUG);
			atexit(&DestroyInstance);
		}
		return m_pInstance;
	};

	static void DestroyInstance(void)
	{
		if (nullptr != m_pInstance)
		{
			delete m_pInstance;
		}
	};

	void ChangeToLogLevel(const DWORD dwLogLevel);
	//---------------------------------------------
	// 
	//---------------------------------------------

	void WriteLogToConsole(const DWORD dwLogLevel, const WCHAR* szFormat, ...);
	//---------------------------------------------
	// 
	//---------------------------------------------

	void WriteLogToConsoleNotTime(const DWORD dwLogLevel, const WCHAR* szFormat, ...);
	//---------------------------------------------
	// 
	//---------------------------------------------

	void WriteLogToFile(const DWORD dwLogLevel, const WCHAR* szFormat, ...);
	//---------------------------------------------
	// 
	//---------------------------------------------

	void GetFileName(WCHAR* szDest, const int iSize);
	//---------------------------------------------
	// 
	//---------------------------------------------

	void GetTimeStamp(WCHAR* szDest, const int iSize);
	//---------------------------------------------
	// 
	//---------------------------------------------

private:
	static CLogger* m_pInstance;
	DWORD	m_dwLogLevel;
	WCHAR	m_szBuffer[1024];
};