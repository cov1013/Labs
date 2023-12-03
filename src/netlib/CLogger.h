#pragma once
#include <direct.h>
#include <strsafe.h>

namespace cov1013
{
	/*---------------------------------------------------------------------*/
	// ��� ����
	/*---------------------------------------------------------------------*/
	enum en_LOG_LEVEL
	{
		eLOG_LEVEL_OFF,
		eLOG_LEVEL_DEBUG,
		eLOG_LEVEL_ERROR,
		eLOG_LEVEL_SYSTEM
	};

	/*---------------------------------------------------------------------*/
	// ��ũ��
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
		// Ŭ���� �⺻ ����
		//
		// eSTRING_MAX    : ��Ʈ�� �ִ� ����
		// eDIRECTORY_MAX : ���͸� �ִ� ����
		/*---------------------------------------------------------------------*/
		enum en_CONFIG
		{
			eSTRING_MAX = 256,
			eDIRECTORY_MAX = 256,
		};

	public:
		//////////////////////////////////////////////////////////////////////////
		// ������
		//////////////////////////////////////////////////////////////////////////
		CLogger(const wchar_t* szDirectory, int eLogLevel = eLOG_LEVEL_OFF)
		{
			//--------------------------------------------------------
			// ��� ���丮 ����
			//--------------------------------------------------------
			SetLogDirectory(szDirectory);

			//--------------------------------------------------------
			// �α� ���� ����
			//--------------------------------------------------------
			SetLogLevel(eLogLevel);

			//--------------------------------------------------------
			// ����ȭ ��ü �ʱ�ȭ
			//--------------------------------------------------------
			InitializeSRWLock(&sm_pFileStream_srw);
		};

		//////////////////////////////////////////////////////////////////////////
		// �Ҹ���
		//////////////////////////////////////////////////////////////////////////
		virtual ~CLogger() {};

		//////////////////////////////////////////////////////////////////////////
		// �α� ���� ����
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
		// �α� ���� ���
		//////////////////////////////////////////////////////////////////////////
		static en_LOG_LEVEL GetLogLevel(void)
		{
			return sm_eLogLevel;
		}

		//////////////////////////////////////////////////////////////////////////
		// ��� ���丮 ����
		//////////////////////////////////////////////////////////////////////////
		static bool SetLogDirectory(const wchar_t* szDirectory)
		{
			//----------------------------------------------
			// ���� ����
			//----------------------------------------------
			int iResult = _wmkdir(szDirectory);
			if (iResult == ENOENT)
			{
				return false;
			}

			//----------------------------------------------
			// ���� ��� ����
			//----------------------------------------------		
			StringCchPrintf(sm_szDirectory, eDIRECTORY_MAX, szDirectory);

			return true;
		};

		//////////////////////////////////////////////////////////////////////////
		// ���Ͽ� �α� ���
		//////////////////////////////////////////////////////////////////////////
		static void LogToFile(const WCHAR* szType, en_LOG_LEVEL eLogLevel, const WCHAR* szStringFormat, ...)
		{
			//----------------------------------------------
			// �α� ī���� ����
			//----------------------------------------------
			DWORD dwLogCount = InterlockedIncrement(&sm_dwLogCount);

			//----------------------------------------------
			// �������� ��� ���ڿ� ����
			//----------------------------------------------
			wchar_t szMessage[eSTRING_MAX];

			va_list va;
			va_start(va, szStringFormat);
			StringCchVPrintf(szMessage, eSTRING_MAX, szStringFormat, va);
			va_end(va);

			//----------------------------------------------
			// �α� Ÿ�� ���ڿ� ����
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
			// ���� ��¥�� �ð��� �˾ƿ´�.
			//----------------------------------------------
			wchar_t szFileName[eSTRING_MAX];
			SYSTEMTIME stNowTime;
			GetLocalTime(&stNowTime);
			wsprintf(szFileName,
				L"%s/%d%02d%_%s.txt",
				sm_szDirectory, stNowTime.wYear, stNowTime.wMonth, szType
			);

			//----------------------------------------------
			// �ش� �����尡 ���� �ڵ��� ȹ���� �� ���� ���� �ݺ�
			//----------------------------------------------
			AcquireSRWLockExclusive(&sm_pFileStream_srw);
			_wfopen_s(&sm_pFileStream, szFileName, L"a+");

			//----------------------------------------------
			// ���Ͽ� ����
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
		// ���Ͽ� Hex �޸� �α� ���
		//////////////////////////////////////////////////////////////////////////
		static void LogToFile_HEX(const WCHAR* szType, en_LOG_LEVEL eLogLevel, char* pByte, int iByteLen, int iRow, const WCHAR* szStringFormat, ...)
		{
			//----------------------------------------------
			// �α� ī���� ����
			//----------------------------------------------
			DWORD dwLogCount = InterlockedIncrement(&sm_dwLogCount);

			//----------------------------------------------
			// �������� ��� ���ڿ� ����
			//----------------------------------------------
			wchar_t szMessage[eSTRING_MAX];

			va_list va;
			va_start(va, szStringFormat);
			StringCchVPrintf(szMessage, eSTRING_MAX, szStringFormat, va);
			va_end(va);

			//----------------------------------------------
			// �α� Ÿ�� ���ڿ� ����
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
			// ���� ��¥�� �ð��� �˾ƿ´�.
			//----------------------------------------------
			wchar_t szFileName[eSTRING_MAX];
			SYSTEMTIME stNowTime;
			GetLocalTime(&stNowTime);
			wsprintf(szFileName,
				L"%s/%d%02d%_%s.txt",
				sm_szDirectory, stNowTime.wYear, stNowTime.wMonth, szType
			);

			//----------------------------------------------
			// �ش� �����尡 ���� �ڵ��� ȹ���� �� ���� ���� �ݺ�
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
		// �ֿܼ� �α� ���
		//////////////////////////////////////////////////////////////////////////
		static void LogToConsole(en_LOG_LEVEL eLogLevel, const WCHAR* szStringFormat, ...)
		{
			//----------------------------------------------
			// ���õǾ��ִ� �α� ������ �ƴϸ� ����
			//----------------------------------------------
			if (sm_eLogLevel != eLogLevel)
			{
				return;
			}

			//----------------------------------------------
			// �������� ��� ���ڿ� ����
			//----------------------------------------------
			wchar_t szMessage[MAX_PATH];
			va_list va;
			va_start(va, szStringFormat);
			StringCchVPrintf(szMessage, MAX_PATH, szStringFormat, va);
			va_end(va);

			//----------------------------------------------
			// �ܼ� ���
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

