#include <direct.h>
#include <stdio.h>
#include <strsafe.h>
#include <windows.h>
#include "Profiler.h"

namespace covEngine
{
	//-------------------------------------------------------
	// TLS 인덱스
	//-------------------------------------------------------
	DWORD				g_dwTlsIndex;

	//-------------------------------------------------------
	// 샘플 메모리 인덱스
	//-------------------------------------------------------
	short				g_shSampleIndex = -1;

	//-------------------------------------------------------
	// 스레드에게 부여되는 샘플 메모리
	//-------------------------------------------------------
	st_THREAD_SAMPLE	g_ThreadSamples[eTHREAD_MAX];

	//-------------------------------------------------------
	// 주기 주파수 (시간 구할 때 사용)
	//-------------------------------------------------------
	LARGE_INTEGER		g_liPerfFreq;

	//-------------------------------------------------------
	// 프로파일링 단위
	//-------------------------------------------------------
	en_PROFILER_UNIT	g_iProfilerUnit;

	//-------------------------------------------------------
	// 디렉토리 이름
	//-------------------------------------------------------
	wchar_t				g_szDirectory[eDIRECTORY_MAX];

	//////////////////////////////////////////////////////////////////////////
	// 프로파일러 초기화
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
	// 프로파일러 전용 디렉토리 생성
	//////////////////////////////////////////////////////////////////////////
	bool SetProfilerDirectory(const wchar_t* szDirectory)
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
		StringCchPrintf(g_szDirectory, eDIRECTORY_MAX, szDirectory);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// 프로파일러 정리
	//////////////////////////////////////////////////////////////////////////
	void ReleaseProfiler(void)
	{
		TlsFree(g_dwTlsIndex);
	}

	//////////////////////////////////////////////////////////////////////////
	// 프로파일링 시작
	//////////////////////////////////////////////////////////////////////////
	void BeginProfiling(const wchar_t* szTagName)
	{
		//----------------------------------------------------------
		// 해당 TLS에 세팅된 프로파일링 샘플을 가져온다.
		//----------------------------------------------------------
		DWORD dwThreadID = GetCurrentThreadId();
		st_THREAD_SAMPLE* pThreadSample = (st_THREAD_SAMPLE*)TlsGetValue(g_dwTlsIndex);

		//----------------------------------------------------------
		// 해당 스레드 TLS에 프로파일링 샘플이 세팅되어 있지 않다면
		// 새로운 프로파일링 샘플을 세팅한다.
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
			// 동일 태그가 이미 존재한다.
			//----------------------------------------------------------
			if (0 == wcscmp(pThreadSample->Samples[iIndex].szTagName, szTagName))
			{
				break;
			}

			//----------------------------------------------------------
			// 비어있는 곳을 찾았다
			//----------------------------------------------------------
			if (0 == pThreadSample->Samples[i].liCall)
			{
				StringCchPrintf(pThreadSample->Samples[iIndex].szTagName, eTAG_NAME_MAX, szTagName);
				break;
			}
		}

		//----------------------------------------------------------
		// 호출 시작 시간 갱신
		//----------------------------------------------------------
		QueryPerformanceCounter(&pThreadSample->Samples[iIndex].liPerfStart);
	}

	//////////////////////////////////////////////////////////////////////////
	// 프로파일링 종료
	//////////////////////////////////////////////////////////////////////////
	void EndProfiling(const wchar_t* szTagName)
	{
		LARGE_INTEGER _liStart;
		QueryPerformanceCounter(&_liStart);

		DWORD dwThreadID = GetCurrentThreadId();
		st_THREAD_SAMPLE* pThreadSample = (st_THREAD_SAMPLE*)TlsGetValue(g_dwTlsIndex);

		//----------------------------------------------------------
		// TLS에 프로파일링 샘플이 세팅되어 있지 않다면 리턴
		//----------------------------------------------------------
		if (pThreadSample == nullptr)
		{
			return;
		}

		//----------------------------------------------------------
		// 해당 태그를 사용하고 있는 샘플을 찾는다.
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
		// 존재하지 않는 태그다.
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
		// 수행시간 측정
		//----------------------------------------------------------
		LARGE_INTEGER liPerfNow;
		QueryPerformanceCounter(&liPerfNow);
		liPerfNow.QuadPart -= _liElapsed.QuadPart;	// 태그 찾는 시간 제외

		LARGE_INTEGER liElapsed;
		liElapsed.QuadPart = liPerfNow.QuadPart - pSample->liPerfStart.QuadPart;

		double dElapsedTime = (double)liElapsed.QuadPart * (double)g_iProfilerUnit / (double)g_liPerfFreq.QuadPart;

		//----------------------------------------------------------
		// 최대/최소값 갱신
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
		// 전체 수행 시간 갱신
		//----------------------------------------------------------
		pSample->dTotal += dElapsedTime;

		//----------------------------------------------------------
		// 호출 횟수 갱신
		//----------------------------------------------------------
		pSample->liCall++;
	}

	//////////////////////////////////////////////////////////////////////////
	// 프로파일링 데이터 출력
	//////////////////////////////////////////////////////////////////////////
	void OutputProfilingData(void)
	{
		//----------------------------------------------------------
		// 파일 이름 생성
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
		// 파일 오픈
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
			// 프로파일러에 등록된 스레드 개수 만큼만 반복
			//----------------------------------------------------------
			for (int i = 0; i <= g_shSampleIndex; i++)
			{
				fwprintf(pFileStream, L"\nThread Num : %d\n", (i + 1));
				fwprintf(pFileStream, L"-------------------------------------------------------------------------------------------------------------\n");
				fwprintf(pFileStream, L" ThreadID |                Name  |           Average  |            Min   |            Max   |          Call |\n");
				fwprintf(pFileStream, L"-------------------------------------------------------------------------------------------------------------\n");

				//----------------------------------------------------------
				// 프로파일링 샘플 데이터 순회
				//----------------------------------------------------------
				for (int j = 0; j < eSAMPLE_MAX; j++)
				{
					st_SAMPLE Sample = g_ThreadSamples[i].Samples[j];

					//----------------------------------------------------------
					// 해당 샘플에 데이터가 없다면 반복문 종료
					//----------------------------------------------------------
					if (Sample.liCall == 0)
					{
						break;
					}

					//----------------------------------------------------------
					// 설정 단위에 맞게 출력
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