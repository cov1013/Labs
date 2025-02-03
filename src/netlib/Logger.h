#pragma once
#include <direct.h>
#include <strsafe.h>

namespace cov1013
{
	class Logger
	{
	public:
		enum class eLogLevel : UINT32
		{
			None = 0,
			Degun = 1,
			Error = 2,
			Warning = 3,
			System = 4,
			Max, // Not Uesd
		};

	public:
		static Logger* GetInstance();

	private:
		static Logger* m_pInstance;

	public:
		const bool Initialize(const eLogLevel eLogLevel, const std::wstring_view& directoryPath);
		void LogToFile(const std::string_view* logType, const Logger::eLogLevel eLogLevel, const WCHAR* szStringFormat, ...);

	private:
		Logger() = default;
		Logger(const Logger& other) = delete;
		virtual ~Logger() = default;
		
		static void LogToFile(const std::st* szType, eLevel eLogLevel, const WCHAR* szStringFormat, ...)
		{
			//----------------------------------------------
			// 로그 카운터 갱신
			//----------------------------------------------
			DWORD dwLogCount = InterlockedIncrement(&sm_dwLogCount);

			//----------------------------------------------
			// 가변인자 기반 문자열 복사
			//----------------------------------------------
			wchar_t szMessage[eSTRING_MAX];

			va_list va;
			va_start(va, szStringFormat);
			StringCchVPrintf(szMessage, eSTRING_MAX, szStringFormat, va);
			va_end(va);

			//----------------------------------------------
			// 로그 타입 문자열 생성
			//----------------------------------------------
			wchar_t szLogLevel[32];
			switch (eLogLevel)
			{
			case eLOG_LEVEL_DEBUG:
				wsprintf(szLogLevel, L"%s", L"DEBUG");
				break;
			case eLOG_LEVEL_ERROR:
				wsprintf(szLogLevel, L"%s", L"ERROR");
				break;
			case eLOG_LEVEL_SYSTEM:
				wsprintf(szLogLevel, L"%s", L"SYSTEM");
				break;
			default:
				break;
			}

			//----------------------------------------------
			// 현재 날짜와 시간을 알아온다.
			//----------------------------------------------
			wchar_t szFileName[eSTRING_MAX];
			SYSTEMTIME stNowTime;
			GetLocalTime(&stNowTime);
			wsprintf(szFileName,
				L"%s/%d%02d%_%s.txt",
				sm_szDirectory, stNowTime.wYear, stNowTime.wMonth, szType
			);

			//----------------------------------------------
			// 해당 스레드가 파일 핸들을 획득할 때 까지 무한 반복
			//----------------------------------------------
			AcquireSRWLockExclusive(&sm_pFileStream_srw);
			_wfopen_s(&sm_pFileStream, szFileName, L"a+");

			//----------------------------------------------
			// 파일에 쓰기
			//----------------------------------------------
			fwprintf_s(sm_pFileStream,
				L"[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %09d] %s\n",
				szType,
				stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay,
				stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond,
				szLogLevel, dwLogCount, szMessage
			);
			fclose(sm_pFileStream);
			ReleaseSRWLockExclusive(&sm_pFileStream_srw);
		};
		static void LogToFile_HEX(const WCHAR* szType, en_LOG_LEVEL eLogLevel, char* pByte, int iByteLen, int iRow, const WCHAR* szStringFormat, ...)
		{
			//----------------------------------------------
			// 로그 카운터 갱신
			//----------------------------------------------
			DWORD dwLogCount = InterlockedIncrement(&sm_dwLogCount);

			//----------------------------------------------
			// 가변인자 기반 문자열 복사
			//----------------------------------------------
			wchar_t szMessage[eSTRING_MAX];

			va_list va;
			va_start(va, szStringFormat);
			StringCchVPrintf(szMessage, eSTRING_MAX, szStringFormat, va);
			va_end(va);

			//----------------------------------------------
			// 로그 타입 문자열 생성
			//----------------------------------------------
			wchar_t szLogLevel[32];
			switch (eLogLevel)
			{
			case eLOG_LEVEL_DEBUG:
				wsprintf(szLogLevel, L"%s", L"DEBUG");
				break;
			case eLOG_LEVEL_ERROR:
				wsprintf(szLogLevel, L"%s", L"ERROR");
				break;
			case eLOG_LEVEL_SYSTEM:
				wsprintf(szLogLevel, L"%s", L"SYSTEM");
				break;
			default:
				break;
			}

			//----------------------------------------------
			// 현재 날짜와 시간을 알아온다.
			//----------------------------------------------
			wchar_t szFileName[eSTRING_MAX];
			SYSTEMTIME stNowTime;
			GetLocalTime(&stNowTime);
			wsprintf(szFileName,
				L"%s/%d%02d%_%s.txt",
				sm_szDirectory, stNowTime.wYear, stNowTime.wMonth, szType
			);

			//----------------------------------------------
			// 해당 스레드가 파일 핸들을 획득할 때 까지 무한 반복
			//----------------------------------------------
			AcquireSRWLockExclusive(&sm_pFileStream_srw);
			_wfopen_s(&sm_pFileStream, szFileName, L"a+");

			fwprintf_s(sm_pFileStream,
				L"[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %09d] %s\n",
				szType,
				stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay,
				stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond,
				szLogLevel, dwLogCount, szMessage
			);

			for (int i = 0; i < iByteLen; i++)
			{
				fwprintf_s(sm_pFileStream, L"%02x ", pByte[i] & 0xff);
				if ((i + 1) % iRow == 0)
				{
					fwprintf_s(sm_pFileStream, L"\n");
				}
			}

			fwprintf_s(sm_pFileStream, L"\n");
			fclose(sm_pFileStream);
			ReleaseSRWLockExclusive(&sm_pFileStream_srw);
		};
		static void LogToConsole(en_LOG_LEVEL eLogLevel, const WCHAR* szStringFormat, ...)
		{
			//----------------------------------------------
			// 세팅되어있는 로그 레벨이 아니면 리턴
			//----------------------------------------------
			if (sm_eLogLevel != eLogLevel)
			{
				return;
			}

			//----------------------------------------------
			// 가변인자 기반 문자열 복사
			//----------------------------------------------
			wchar_t szMessage[MAX_PATH];
			va_list va;
			va_start(va, szStringFormat);
			StringCchVPrintf(szMessage, MAX_PATH, szStringFormat, va);
			va_end(va);

			//----------------------------------------------
			// 콘솔 출력
			//----------------------------------------------
			wprintf_s(L"%s", szMessage);
		};

	private:
		SRWLOCK	m_srw = {};
		int32_t	m_Index = -1;
		eLogLevel m_eLevel = eLogLevel::None;
	};

	#define LOG(Type, Level, Format, ...) Logger::LogToFile(Type, Level, Fmt, ##__VA_ARGS__)
	#define LOG_CONSOLE(Level, Format, ...)	Logger::LogToConsole(Level, Fmt, ##__VA_ARGS__)
	#define LOG_HEX(Type, Level, Entry, Length, Row, Format, ...) Logger::LogToFile_HEX(Type, Level, Entry, Len, row, Fmt, ##__VA_ARGS__)
}