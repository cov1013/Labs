#pragma once
#include <direct.h>
#include <strsafe.h>

namespace cov1013
{
	/*---------------------------------------------------------------------*/
	// 출력 레벨
	/*---------------------------------------------------------------------*/
	enum en_LOG_LEVEL
	{
		eLOG_LEVEL_OFF,
		eLOG_LEVEL_DEBUG,
		eLOG_LEVEL_ERROR,
		eLOG_LEVEL_SYSTEM
	};

	/*---------------------------------------------------------------------*/
	// 매크로
	/*---------------------------------------------------------------------*/
	#define SYSLOG_LEVEL(Level)								CLogger::SetLogLevel(Level)
	#define SYSLOG_DIRECTORY(szFolderName)					CLogger::SetLogDirectory(szFolderName)
	#define LOG(Type, Level, Fmt, ...)						CLogger::LogToFile(Type, Level, Fmt, ##__VA_ARGS__)
	#define CONSOLE(Level, Fmt, ...)						CLogger::LogToConsole(Level, Fmt, ##__VA_ARGS__)
	#define LOG_HEX(Type, Level, Entry, Len, row, Fmt, ...) CLogger::LogToFile_HEX(Type, Level, Entry, Len, row, Fmt, ##__VA_ARGS__)

	class CLogger
	{
	public:
		/*---------------------------------------------------------------------*/
		// 클래스 기본 구성
		//
		// eSTRING_MAX    : 스트링 최대 길이
		// eDIRECTORY_MAX : 디렉터리 최대 길이
		/*---------------------------------------------------------------------*/
		enum en_CONFIG
		{
			eSTRING_MAX = 256,
			eDIRECTORY_MAX = 256,
		};

	public:
		//////////////////////////////////////////////////////////////////////////
		// 생성자
		//////////////////////////////////////////////////////////////////////////
		CLogger(const wchar_t* szDirectory, int eLogLevel = eLOG_LEVEL_OFF)
		{
			//--------------------------------------------------------
			// 출력 디렉토리 설정
			//--------------------------------------------------------
			SetLogDirectory(szDirectory);

			//--------------------------------------------------------
			// 로그 레벨 설정
			//--------------------------------------------------------
			SetLogLevel(eLogLevel);

			//--------------------------------------------------------
			// 동기화 객체 초기화
			//--------------------------------------------------------
			InitializeSRWLock(&sm_pFileStream_srw);
		};

		//////////////////////////////////////////////////////////////////////////
		// 소멸자
		//////////////////////////////////////////////////////////////////////////
		virtual ~CLogger() {};

		//////////////////////////////////////////////////////////////////////////
		// 로그 레벨 설정
		//////////////////////////////////////////////////////////////////////////
		static bool SetLogLevel(int eLogLevel)
		{
			if (eLogLevel < eLOG_LEVEL_OFF || eLogLevel > eLOG_LEVEL_SYSTEM)
			{
				return false;
			}

			sm_eLogLevel = (en_LOG_LEVEL)eLogLevel;

			return true;
		};

		//////////////////////////////////////////////////////////////////////////
		// 로그 레벨 출력
		//////////////////////////////////////////////////////////////////////////
		static en_LOG_LEVEL GetLogLevel(void)
		{
			return sm_eLogLevel;
		}

		//////////////////////////////////////////////////////////////////////////
		// 출력 디렉토리 설정
		//////////////////////////////////////////////////////////////////////////
		static bool SetLogDirectory(const wchar_t* szDirectory)
		{
			//----------------------------------------------
			// 폴더 생성
			//----------------------------------------------
			int iResult = _wmkdir(szDirectory);
			if (iResult == ENOENT)
			{
				return false;
			}

			//----------------------------------------------
			// 폴더 경로 세팅
			//----------------------------------------------		
			StringCchPrintf(sm_szDirectory, eDIRECTORY_MAX, szDirectory);

			return true;
		};

		//////////////////////////////////////////////////////////////////////////
		// 파일에 로그 출력
		//////////////////////////////////////////////////////////////////////////
		static void LogToFile(const WCHAR* szType, en_LOG_LEVEL eLogLevel, const WCHAR* szStringFormat, ...)
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

		//////////////////////////////////////////////////////////////////////////
		// 파일에 Hex 메모리 로그 출력
		//////////////////////////////////////////////////////////////////////////
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

		//////////////////////////////////////////////////////////////////////////
		// 콘솔에 로그 출력
		//////////////////////////////////////////////////////////////////////////
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
		static en_LOG_LEVEL sm_eLogLevel;
		static DWORD		sm_dwLogCount;
		static FILE* sm_pFileStream;
		static SRWLOCK		sm_pFileStream_srw;
		static wchar_t		sm_szDirectory[eDIRECTORY_MAX];
	};
}

