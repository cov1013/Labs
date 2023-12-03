#include <windows.h>
#include "CPDH.h"

namespace cov1013
{
	CPDH::CPDH(const WCHAR* szProcessName)
	{
		WCHAR szBuffer[1024];

		PdhOpenQuery(NULL, NULL, &m_szQuery);

		//------------------------------------------------
		// ���μ��� �����Ҵ� �޸� ���� ���
		//------------------------------------------------
		StringCchPrintf(szBuffer, 1024, L"\\Process(%s)\\Private Bytes", szProcessName);
		PdhAddCounter(m_szQuery, szBuffer, NULL, &m_ProcessCommitMemory);

		//------------------------------------------------
		// ���μ��� ���������� �޸� ���� ���
		//------------------------------------------------
		StringCchPrintf(szBuffer, 1024, L"\\Process(%s)\\Pool Nonpaged Bytes", szProcessName);
		PdhAddCounter(m_szQuery, szBuffer, NULL, &m_ProcessNonPagedMemory);

		//------------------------------------------------
		// �ý��� ��� ���� �޸� ���� ���
		//------------------------------------------------
		PdhAddCounter(m_szQuery, L"\\Memory\\Available Bytes", NULL, &m_AvailableMemory);

		//------------------------------------------------
		// �ý��� ���������� �޸� ���� ���
		//------------------------------------------------
		PdhAddCounter(m_szQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &m_NonPagedMemory);

		//------------------------------------------------
		// ������ ��Ʈ ���� ���
		//------------------------------------------------
		PdhAddCounter(m_szQuery, L"\\Memory\\Page Faults/sec", NULL, &m_PageFaults);

		//------------------------------------------------
		// TCP ������ ���� ���
		//------------------------------------------------
		PdhAddCounter(m_szQuery, L"\\TCPv4\\Segments Retransmitted/sec", NULL, &m_TCPv4Retransmitted);

		//------------------------------------------------
		// �̴��� ���� ���
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
		// ��ϵ� �̴����� ��� Ȯ���ϸ鼭 ���� ����Ʈ ���
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
		// ��ϵ� �̴����� ��� Ȯ���ϸ鼭 �۽� ����Ʈ ���
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
		// �̴��� ������ ���� Ȯ��
		//------------------------------------------------
		PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);

		szCounters = new WCHAR[dwCounterSize];
		szInterfaces = new WCHAR[dwInterfaceSize];

		//------------------------------------------------
		// �̴��� �̸� ���
		//------------------------------------------------
		if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
		{
			delete[] szCounters;
			delete[] szInterfaces;

			return FALSE;
		}

		//------------------------------------------------
		// ���� ���
		//------------------------------------------------
		szCur = szInterfaces;
		for (int i = 0; *szCur != L'\0' && i < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, i++)
		{
			m_Ethernetes[i].bUseFlag = TRUE;

			//------------------------------------------------
			// �̸� ����
			//------------------------------------------------
			StringCchCopy(m_Ethernetes[i].szName, 128, szCur);

			//------------------------------------------------
			// ���� ���� ���
			//------------------------------------------------
			StringCchPrintf(szQuery, 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);
			PdhAddCounter(m_szQuery, szQuery, NULL, &m_Ethernetes[i].PDHCounterNetworkRecvBytes);

			//------------------------------------------------
			// �۽� ���� ���
			//------------------------------------------------
			StringCchPrintf(szQuery, 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);
			PdhAddCounter(m_szQuery, szQuery, NULL, &m_Ethernetes[i].PDHCounterNetworkSendBytes);
		}

		delete[] szCounters;
		delete[] szInterfaces;

		return false;
	}
}


