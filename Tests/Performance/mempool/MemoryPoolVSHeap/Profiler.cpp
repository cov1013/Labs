#include <direct.h>
#include <stdio.h>
#include <strsafe.h>
#include <windows.h>
#include "Profiler.h"

namespace cov1013
{
	DWORD				g_dwTlsIndex;
	short				g_shSampleIndex = -1;
	st_THREAD_SAMPLE	g_ThreadSamples[eTHREAD_MAX];
	LARGE_INTEGER		g_liPerfFreq;
	en_PROFILER_UNIT	g_iProfilerUnit;
	wchar_t				g_szDirectory[eDIRECTORY_MAX];

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

	bool SetProfilerDirectory(const wchar_t* szDirectory)
	{
		int iResult = _wmkdir(szDirectory);
		if (iResult == ENOENT)
		{
			return false;
		}

		StringCchPrintf(g_szDirectory, eDIRECTORY_MAX, szDirectory);

		return true;
	}

	void ReleaseProfiler(void)
	{
		TlsFree(g_dwTlsIndex);
	}

	void BeginProfiling(const wchar_t* szTagName)
	{
		DWORD dwThreadID = GetCurrentThreadId();
		st_THREAD_SAMPLE* pThreadSample = (st_THREAD_SAMPLE*)TlsGetValue(g_dwTlsIndex);

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

			if (0 == wcscmp(pThreadSample->Samples[iIndex].szTagName, szTagName))
			{
				break;
			}

			if (0 == pThreadSample->Samples[i].liCall)
			{
				StringCchPrintf(pThreadSample->Samples[iIndex].szTagName, eTAG_NAME_MAX, szTagName);
				break;
			}
		}

		QueryPerformanceCounter(&pThreadSample->Samples[iIndex].liPerfStart);
	}

	void EndProfiling(const wchar_t* szTagName)
	{
		LARGE_INTEGER _liStart;
		QueryPerformanceCounter(&_liStart);

		DWORD dwThreadID = GetCurrentThreadId();
		st_THREAD_SAMPLE* pThreadSample = (st_THREAD_SAMPLE*)TlsGetValue(g_dwTlsIndex);

		if (pThreadSample == nullptr)
		{
			return;
		}

		st_SAMPLE* pSample = nullptr;
		for (int i = 0; i < eSAMPLE_MAX; i++)
		{
			if (0 == wcscmp(pThreadSample->Samples[i].szTagName, szTagName))
			{
				pSample = &pThreadSample->Samples[i];
				break;
			}
		}

		if (pSample == nullptr)
		{
			return;
		}

		LARGE_INTEGER _liEnd;
		QueryPerformanceCounter(&_liEnd);
		LARGE_INTEGER _liElapsed;
		_liElapsed.QuadPart = _liEnd.QuadPart - _liStart.QuadPart;

		LARGE_INTEGER liPerfNow;
		QueryPerformanceCounter(&liPerfNow);
		liPerfNow.QuadPart -= _liElapsed.QuadPart;	// 태그 찾는 시간 제외

		LARGE_INTEGER liElapsed;
		liElapsed.QuadPart = liPerfNow.QuadPart - pSample->liPerfStart.QuadPart;

		double dElapsedTime = (double)liElapsed.QuadPart * (double)g_iProfilerUnit / (double)g_liPerfFreq.QuadPart;
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

		pSample->dTotal += dElapsedTime;
		pSample->liCall++;
	}

	void OutputProfilingData(void)
	{
		wchar_t szFileName[eFILE_NAME_MAX];
		SYSTEMTIME stNowTime;
		GetLocalTime(&stNowTime);
		wsprintf(szFileName, L"%s/%04d%02d%02d.txt",
			g_szDirectory,
			stNowTime.wYear,
			stNowTime.wMonth,
			stNowTime.wDay
		);

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

			for (int i = 0; i <= g_shSampleIndex; i++)
			{
				fwprintf(pFileStream, L"\nThread Num : %d\n", (i + 1));
				fwprintf(pFileStream, L"-------------------------------------------------------------------------------------------------------------\n");
				fwprintf(pFileStream, L" ThreadID |                Name  |           Average  |            Min   |            Max   |          Call |\n");
				fwprintf(pFileStream, L"-------------------------------------------------------------------------------------------------------------\n");

				for (int j = 0; j < eSAMPLE_MAX; j++)
				{
					st_SAMPLE Sample = g_ThreadSamples[i].Samples[j];

					if (Sample.liCall == 0)
					{
						break;
					}

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