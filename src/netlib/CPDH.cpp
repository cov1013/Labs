#include <windows.h>
#include "CPDH.h"

namespace cov1013
{
	CPDH::CPDH(const WCHAR* szProcessName)
	{
		WCHAR szBuffer[1024];

		PdhOpenQuery(NULL, NULL, &m_szQuery);

		//------------------------------------------------
		// 프로세스 유저할당 메모리 쿼리 등록
		//------------------------------------------------
		StringCchPrintf(szBuffer, 1024, L"\\Process(%s)\\Private Bytes", szProcessName);
		PdhAddCounter(m_szQuery, szBuffer, NULL, &m_ProcessCommitMemory);

		//------------------------------------------------
		// 프로세스 논페이지드 메모리 쿼리 등록
		//------------------------------------------------
		StringCchPrintf(szBuffer, 1024, L"\\Process(%s)\\Pool Nonpaged Bytes", szProcessName);
		PdhAddCounter(m_szQuery, szBuffer, NULL, &m_ProcessNonPagedMemory);

		//------------------------------------------------
		// 시스템 사용 가능 메모리 쿼리 등록
		//------------------------------------------------
		PdhAddCounter(m_szQuery, L"\\Memory\\Available Bytes", NULL, &m_AvailableMemory);

		//------------------------------------------------
		// 시스템 논페이지드 메모리 쿼리 등록
		//------------------------------------------------
		PdhAddCounter(m_szQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &m_NonPagedMemory);

		//------------------------------------------------
		// 페이지 폴트 쿼리 등록
		//------------------------------------------------
		PdhAddCounter(m_szQuery, L"\\Memory\\Page Faults/sec", NULL, &m_PageFaults);

		//------------------------------------------------
		// TCP 재전송 쿼리 등록
		//------------------------------------------------
		PdhAddCounter(m_szQuery, L"\\TCPv4\\Segments Retransmitted/sec", NULL, &m_TCPv4Retransmitted);

		//------------------------------------------------
		// 이더넷 쿼리 등록
		//------------------------------------------------
		AddNetworkInterfaces();
	}

	CPDH::~CPDH()
	{
	}

	void CPDH::Collect(void)
	{
		PdhCollectQueryData(m_szQuery);
	}

	const LONGLONG CPDH::GetProcessCommitMemory(void)
	{
		PDH_FMT_COUNTERVALUE CounterValue;
		PdhGetFormattedCounterValue(m_ProcessCommitMemory, PDH_FMT_LARGE, NULL, &CounterValue);

		return CounterValue.largeValue;
	}

	const LONGLONG CPDH::GetProcessNonPagedMemory(void)
	{
		PDH_FMT_COUNTERVALUE CounterValue;
		PdhGetFormattedCounterValue(m_ProcessNonPagedMemory, PDH_FMT_LARGE, NULL, &CounterValue);

		return CounterValue.largeValue;
	}

	const LONGLONG CPDH::GetAvailableMemory(void)
	{
		PDH_FMT_COUNTERVALUE CounterValue;
		PdhGetFormattedCounterValue(m_AvailableMemory, PDH_FMT_LARGE, NULL, &CounterValue);

		return CounterValue.largeValue;
	}

	const LONGLONG CPDH::GetNonPagedMemory(void)
	{
		PDH_FMT_COUNTERVALUE CounterValue;
		PdhGetFormattedCounterValue(m_NonPagedMemory, PDH_FMT_LARGE, NULL, &CounterValue);

		return CounterValue.largeValue;
	}

	const LONGLONG CPDH::GetRecvBytes(void)
	{
		LONGLONG llRecvBytes = 0;
		PDH_FMT_COUNTERVALUE CounterValue;

		//------------------------------------------------
		// 등록된 이더넷을 모두 확인하면서 수신 바이트 계산
		//------------------------------------------------
		for (int i = 0; i < df_PDH_ETHERNET_MAX; i++)
		{
			if (m_Ethernetes[i].bUseFlag == TRUE)
			{
				PdhGetFormattedCounterValue(m_Ethernetes[i].PDHCounterNetworkRecvBytes, PDH_FMT_LARGE, NULL, &CounterValue);
				llRecvBytes += CounterValue.largeValue;
			}
		}

		return llRecvBytes;
	}

	const LONGLONG CPDH::GetSendBytes(void)
	{
		LONGLONG llSendBytes = 0;
		PDH_FMT_COUNTERVALUE CounterValue;

		//------------------------------------------------
		// 등록된 이더넷을 모두 확인하면서 송신 바이트 계산
		//------------------------------------------------
		for (int i = 0; i < df_PDH_ETHERNET_MAX; i++)
		{
			if (m_Ethernetes[i].bUseFlag == TRUE)
			{
				PdhGetFormattedCounterValue(m_Ethernetes[i].PDHCounterNetworkSendBytes, PDH_FMT_LARGE, NULL, &CounterValue);
				llSendBytes += CounterValue.largeValue;
			}
		}

		return llSendBytes;
	}

	const LONGLONG CPDH::GetTCPv4Retransmitted(void)
	{
		PDH_FMT_COUNTERVALUE CounterValue;
		PdhGetFormattedCounterValue(m_TCPv4Retransmitted, PDH_FMT_LARGE, NULL, &CounterValue);

		return CounterValue.largeValue;
	}

	const double CPDH::GetPageFaults(void)
	{
		PDH_FMT_COUNTERVALUE CounterValue;
		PdhGetFormattedCounterValue(m_PageFaults, PDH_FMT_DOUBLE, NULL, &CounterValue);

		return CounterValue.doubleValue;
	}

	bool CPDH::AddNetworkInterfaces(void)
	{
		bool	bErr = false;
		WCHAR* szCur = NULL;
		WCHAR* szCounters = NULL;
		WCHAR* szInterfaces = NULL;
		DWORD	dwCounterSize = 0;
		DWORD	dwInterfaceSize = 0;
		WCHAR	szQuery[1024] = { 0 };

		//------------------------------------------------
		// 이더넷 개수와 길이 확인
		//------------------------------------------------
		PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);

		szCounters = new WCHAR[dwCounterSize];
		szInterfaces = new WCHAR[dwInterfaceSize];

		//------------------------------------------------
		// 이더넷 이름 얻기
		//------------------------------------------------
		if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
		{
			delete[] szCounters;
			delete[] szInterfaces;

			return FALSE;
		}

		//------------------------------------------------
		// 쿼리 등록
		//------------------------------------------------
		szCur = szInterfaces;
		for (int i = 0; *szCur != L'\0' && i < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, i++)
		{
			m_Ethernetes[i].bUseFlag = TRUE;

			//------------------------------------------------
			// 이름 저장
			//------------------------------------------------
			StringCchCopy(m_Ethernetes[i].szName, 128, szCur);

			//------------------------------------------------
			// 수신 쿼리 등록
			//------------------------------------------------
			StringCchPrintf(szQuery, 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);
			PdhAddCounter(m_szQuery, szQuery, NULL, &m_Ethernetes[i].PDHCounterNetworkRecvBytes);

			//------------------------------------------------
			// 송신 쿼리 등록
			//------------------------------------------------
			StringCchPrintf(szQuery, 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);
			PdhAddCounter(m_szQuery, szQuery, NULL, &m_Ethernetes[i].PDHCounterNetworkSendBytes);
		}

		delete[] szCounters;
		delete[] szInterfaces;

		return false;
	}
}


