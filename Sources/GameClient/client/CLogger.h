#pragma once

#define dfLOG_LEVEL_OFF		0
//---------------------------------------------
// �ΰŰ� �����ִ� ������ �α� ����
// �ΰŰ� �ش� �α� ������ �����Ǿ� ������ 
// �ƹ��� �α׵� ������ �ʴ´�.
// 
//---------------------------------------------


#define dfLOG_LEVEL_ERROR		1
//---------------------------------------------
// ó�� �� ������ �߻����� �� ���Ǵ� �α� ����
// 
// Format
// ##### ErrorName >>> Error Code
//---------------------------------------------


#define dfLOG_LEVEL_WARN		2
//---------------------------------------------
// �ɰ��� ����. 
// ���μ����� ����Ǳ� ���� ����� �α� ����
// 
//---------------------------------------------


#define dfLOG_LEVEL_DEBUG		3
//---------------------------------------------
// ����뿡 �ʿ��� �α׸� ����� �α� ����
// 
//---------------------------------------------


#define dfLOG_LEVEL_INFO		4
//---------------------------------------------
// ����, �������� ����� �α� ����
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