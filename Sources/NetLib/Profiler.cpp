#include <direct.h>
#include <stdio.h>
#include <strsafe.h>
#include <windows.h>
#include "Profiler.h"

namespace covEngine
{
	//-------------------------------------------------------
	// TLS �ε���
	//-------------------------------------------------------
	DWORD				g_dwTlsIndex;

	//-------------------------------------------------------
	// ���� �޸� �ε���
	//-------------------------------------------------------
	short				g_shSampleIndex = -1;

	//-------------------------------------------------------
	// �����忡�� �ο��Ǵ� ���� �޸�
	//-------------------------------------------------------
	st_THREAD_SAMPLE	g_ThreadSamples[eTHREAD_MAX];

	//-------------------------------------------------------
	// �ֱ� ���ļ� (�ð� ���� �� ���)
	//-------------------------------------------------------
	LARGE_INTEGER		g_liPerfFreq;

	//-------------------------------------------------------
	// �������ϸ� ����
	//-------------------------------------------------------
	en_PROFILER_UNIT	g_iProfilerUnit;

	//-------------------------------------------------------
	// ���丮 �̸�
	//-------------------------------------------------------
	wchar_t				g_szDirectory[eDIRECTORY_MAX];

	//////////////////////////////////////////////////////////////////////////
	// �������Ϸ� �ʱ�ȭ
	//////////////////////////////////////////////////////////////////////////
	bool InitializeProfiler(const wchar_t* szDirectory, en_PROFILER_UNIT eUnit)
	{
		if (!SetProfilerDirectory(szDirectory))
		{
			return false;
		}

		QueryPerformanceFrequency(&g_liPerfFreq);
		g_dwTlsIndex = TlsAlloc();
		g_iProfilerUnit = eUnit;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// �������Ϸ� ���� ���丮 ����
	//////////////////////////////////////////////////////////////////////////
	bool SetProfilerDirectory(const wchar_t* szDirectory)
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
		StringCchPrintf(g_szDirectory, eDIRECTORY_MAX, szDirectory);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// �������Ϸ� ����
	//////////////////////////////////////////////////////////////////////////
	void ReleaseProfiler(void)
	{
		TlsFree(g_dwTlsIndex);
	}

	//////////////////////////////////////////////////////////////////////////
	// �������ϸ� ����
	//////////////////////////////////////////////////////////////////////////
	void BeginProfiling(const wchar_t* szTagName)
	{
		//----------------------------------------------------------
		// �ش� TLS�� ���õ� �������ϸ� ������ �����´�.
		//----------------------------------------------------------
		DWORD dwThreadID = GetCurrentThreadId();
		st_THREAD_SAMPLE* pThreadSample = (st_THREAD_SAMPLE*)TlsGetValue(g_dwTlsIndex);

		//----------------------------------------------------------
		// �ش� ������ TLS�� �������ϸ� ������ ���õǾ� ���� �ʴٸ�
		// ���ο� �������ϸ� ������ �����Ѵ�.
		//----------------------------------------------------------
		if (pThreadSample == nullptr)
		{
			pThreadSample = &(g_ThreadSamples[InterlockedIncrement16(&g_shSampleIndex)]);
			pThreadSample->dwThreadID = dwThreadID;

			TlsSetValue(g_dwTlsIndex, (LPVOID)pThreadSample);
		}

		int iIndex;
		for (int i = 0; i < eSAMPLE_MAX; i++)
		{
			iIndex = i;

			//----------------------------------------------------------
			// ���� �±װ� �̹� �����Ѵ�.
			//----------------------------------------------------------
			if (0 == wcscmp(pThreadSample->Samples[iIndex].szTagName, szTagName))
			{
				break;
			}

			//----------------------------------------------------------
			// ����ִ� ���� ã�Ҵ�
			//----------------------------------------------------------
			if (0 == pThreadSample->Samples[i].liCall)
			{
				StringCchPrintf(pThreadSample->Samples[iIndex].szTagName, eTAG_NAME_MAX, szTagName);
				break;
			}
		}

		//----------------------------------------------------------
		// ȣ�� ���� �ð� ����
		//----------------------------------------------------------
		QueryPerformanceCounter(&pThreadSample->Samples[iIndex].liPerfStart);
	}

	//////////////////////////////////////////////////////////////////////////
	// �������ϸ� ����
	//////////////////////////////////////////////////////////////////////////
	void EndProfiling(const wchar_t* szTagName)
	{
		LARGE_INTEGER _liStart;
		QueryPerformanceCounter(&_liStart);

		DWORD dwThreadID = GetCurrentThreadId();
		st_THREAD_SAMPLE* pThreadSample = (st_THREAD_SAMPLE*)TlsGetValue(g_dwTlsIndex);

		//----------------------------------------------------------
		// TLS�� �������ϸ� ������ ���õǾ� ���� �ʴٸ� ����
		//----------------------------------------------------------
		if (pThreadSample == nullptr)
		{
			return;
		}

		//----------------------------------------------------------
		// �ش� �±׸� ����ϰ� �ִ� ������ ã�´�.
		//----------------------------------------------------------
		st_SAMPLE* pSample = nullptr;
		for (int i = 0; i < eSAMPLE_MAX; i++)
		{
			if (0 == wcscmp(pThreadSample->Samples[i].szTagName, szTagName))
			{
				pSample = &pThreadSample->Samples[i];
				break;
			}
		}

		//----------------------------------------------------------
		// �������� �ʴ� �±״�.
		//----------------------------------------------------------
		if (pSample == nullptr)
		{
			return;
		}

		LARGE_INTEGER _liEnd;
		QueryPerformanceCounter(&_liEnd);
		LARGE_INTEGER _liElapsed;
		_liElapsed.QuadPart = _liEnd.QuadPart - _liStart.QuadPart;

		//----------------------------------------------------------
		// ����ð� ����
		//----------------------------------------------------------
		LARGE_INTEGER liPerfNow;
		QueryPerformanceCounter(&liPerfNow);
		liPerfNow.QuadPart -= _liElapsed.QuadPart;	// �±� ã�� �ð� ����

		LARGE_INTEGER liElapsed;
		liElapsed.QuadPart = liPerfNow.QuadPart - pSample->liPerfStart.QuadPart;

		double dElapsedTime = (double)liElapsed.QuadPart * (double)g_iProfilerUnit / (double)g_liPerfFreq.QuadPart;

		//----------------------------------------------------------
		// �ִ�/�ּҰ� ����
		//----------------------------------------------------------
		if (pSample->dMax[1] < dElapsedTime)
		{
			pSample->dMax[0] = pSample->dMax[1];
			pSample->dMax[1] = dElapsedTime;
		}
		if (pSample->dMin[1] > dElapsedTime)
		{
			pSample->dMin[0] = pSample->dMin[1];
			pSample->dMin[1] = dElapsedTime;
		}

		//----------------------------------------------------------
		// ��ü ���� �ð� ����
		//----------------------------------------------------------
		pSample->dTotal += dElapsedTime;

		//----------------------------------------------------------
		// ȣ�� Ƚ�� ����
		//----------------------------------------------------------
		pSample->liCall++;
	}

	//////////////////////////////////////////////////////////////////////////
	// �������ϸ� ������ ���
	//////////////////////////////////////////////////////////////////////////
	void OutputProfilingData(void)
	{
		//----------------------------------------------------------
		// ���� �̸� ����
		//----------------------------------------------------------
		wchar_t szFileName[eFILE_NAME_MAX];
		SYSTEMTIME stNowTime;
		GetLocalTime(&stNowTime);
		wsprintf(szFileName, L"%s/%04d%02d%02d.txt",
			g_szDirectory,
			stNowTime.wYear,
			stNowTime.wMonth,
			stNowTime.wDay
		);

		//----------------------------------------------------------
		// ���� ����
		//----------------------------------------------------------
		FILE* pFileStream;
		_wfopen_s(&pFileStream, szFileName, L"a");
		if (pFileStream != nullptr)
		{
			fwprintf(pFileStream, L"[%04d.%02d.%02d %02d.%02d.%02d]\n",
				stNowTime.wYear,
				stNowTime.wMonth,
				stNowTime.wDay,
				stNowTime.wHour,
				stNowTime.wMinute,
				stNowTime.wSecond
			);

			//----------------------------------------------------------
			// �������Ϸ��� ��ϵ� ������ ���� ��ŭ�� �ݺ�
			//----------------------------------------------------------
			for (int i = 0; i <= g_shSampleIndex; i++)
			{
				fwprintf(pFileStream, L"\nThread Num : %d\n", (i + 1));
				fwprintf(pFileStream, L"-------------------------------------------------------------------------------------------------------------\n");
				fwprintf(pFileStream, L" ThreadID |                Name  |           Average  |            Min   |            Max   |          Call |\n");
				fwprintf(pFileStream, L"-------------------------------------------------------------------------------------------------------------\n");

				//----------------------------------------------------------
				// �������ϸ� ���� ������ ��ȸ
				//----------------------------------------------------------
				for (int j = 0; j < eSAMPLE_MAX; j++)
				{
					st_SAMPLE Sample = g_ThreadSamples[i].Samples[j];

					//----------------------------------------------------------
					// �ش� ���ÿ� �����Ͱ� ���ٸ� �ݺ��� ����
					//----------------------------------------------------------
					if (Sample.liCall == 0)
					{
						break;
					}

					//----------------------------------------------------------
					// ���� ������ �°� ���
					//----------------------------------------------------------
					switch (g_iProfilerUnit)
					{
					case en_PROFILER_UNIT::eUNIT_SEC:
						fwprintf(pFileStream,
							L"%9u | %20s | %17.4fs | %15.4fs | %15.4fs | %13I64u |\n",
							g_ThreadSamples[i].dwThreadID,
							Sample.szTagName,
							(Sample.dTotal - (Sample.dMax[0] + Sample.dMax[1])) / (Sample.liCall - 2),
							Sample.dMin[1],
							Sample.dMax[1],
							Sample.liCall
						);

						break;
					case en_PROFILER_UNIT::eUNIT_MILLI:
						fwprintf(pFileStream,
							L"%9u | %20s | %16.4fms | %14.4fms | %14.4fms | %13I64u |\n",
							g_ThreadSamples[i].dwThreadID,
							Sample.szTagName,
							(Sample.dTotal - (Sample.dMax[0] + Sample.dMax[1])) / (Sample.liCall - 2),
							Sample.dMin[1],
							Sample.dMax[1],
							Sample.liCall
						);

						break;
					case en_PROFILER_UNIT::eUNIT_MICRO:
						fwprintf(pFileStream,
							L"%9u | %20s | %16.4fus | %14.4fus | %14.4fus | %13I64u |\n",
							g_ThreadSamples[i].dwThreadID,
							Sample.szTagName,
							(Sample.dTotal - (Sample.dMax[0] + Sample.dMax[1])) / (Sample.liCall - 2),
							Sample.dMin[1],
							Sample.dMax[1],
							Sample.liCall
						);

						break;
					case en_PROFILER_UNIT::eUNIT_NANO:
						fwprintf(pFileStream,
							L"%9u | %20s | %16.4fns | %14.4fns | %14.4fns | %13I64u |\n",
							g_ThreadSamples[i].dwThreadID,
							Sample.szTagName,
							(Sample.dTotal - (Sample.dMax[0] + Sample.dMax[1])) / (Sample.liCall - 2),
							Sample.dMin[1],
							Sample.dMax[1],
							Sample.liCall
						);

						break;

					default:
						fwprintf(pFileStream, L"Profiler Error\n");
						break;
					}
				}
				fwprintf(pFileStream, L"-------------------------------------------------------------------------------------------------------------\n");
			}
			fwprintf(pFileStream, L"\n\n\n");
			fclose(pFileStream);
		}
	}
}