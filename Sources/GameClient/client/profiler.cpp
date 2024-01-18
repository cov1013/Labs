#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "Profiler.h"

extern int g_iProfilingTime;

ProfilingDataSet g_dataes[PROFILING_DATA_MAX];

//-------------------------------------------------------------
// 프로파일링 시작 등록
//-------------------------------------------------------------
void BeginProfiling(const wchar_t* tag)
{
	int iStartTime;

	int index = 0;
	bool flag = false;

	iStartTime = timeGetTime();

	for (int i = 0; i < PROFILING_DATA_MAX; i++)
	{
		index = i;

		// -------------------------------------------------
		// 데이터 유무 판단
		// -------------------------------------------------
		if (g_dataes[i].Call > 0)
		{
			// -------------------------------------------------
			// 인자로 들어온 태그명과 동일한 인덱스 찾기
			// -------------------------------------------------
			if (wcscmp(g_dataes[i].Tag, tag) == 0)
			{
				flag = true;
				break;
			}
		}
		else
		{
			break;
		}
	}

	// -------------------------------------------------
	// 태그를 찾지 못했으면 신규등록
	// -------------------------------------------------
	if (flag == false)
	{
		wcscpy_s(g_dataes[index].Tag, TAG_LENGTH, tag);
	}

	// -------------------------------------------------
	// 시작 시간 갱신
	// -------------------------------------------------
	QueryPerformanceCounter(&g_dataes[index].StartCounting);

	// -------------------------------------------------
	// 호출 횟수 증가
	// -------------------------------------------------
	g_dataes[index].Call++;

	g_iProfilingTime += timeGetTime() - iStartTime;
}


//-------------------------------------------------------------
// 프로파일링 종료 
//-------------------------------------------------------------
int EndProfiling(const wchar_t* tag)
{
	int iStartTime;

	int index = 0;
	bool flag = false;

	// -------------------------------------------------
	// 인덱스 찾기
	// -------------------------------------------------
	iStartTime = timeGetTime();
	for (int i = 0; i < PROFILING_DATA_MAX; i++)
	{
		// -------------------------------------------------
		// 데이터 유무 판단
		// -------------------------------------------------
		if (g_dataes[i].Call > 0)
		{
			// -------------------------------------------------
			// 인자로 들어온 태그명과 동일한 인덱스 찾기
			// -------------------------------------------------
			if (wcscmp(g_dataes[i].Tag, tag) == 0)
			{
				index = i;
				flag = true;

				break;
			}
		}
		else
		{
			break;
		}
	}

	// -------------------------------------------------
	// 1. 동일한 태그가 있다면 갱신
	// -------------------------------------------------
	if (flag == true)
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		frequency.QuadPart /= 1000000;			// us 단위로 변경

		// -------------------------------------------------
		// 1. 차이값 구하기
		// -------------------------------------------------
		LARGE_INTEGER curCounting;
		QueryPerformanceCounter(&curCounting);

		__int64 period = curCounting.QuadPart - g_dataes[index].StartCounting.QuadPart;
		double takeTime = (double)period / (double)frequency.QuadPart;

		// -------------------------------------------------
		// 2. 최소값 갱신
		// -------------------------------------------------
		// 값이 없으면 그냥 넣기
		if (g_dataes[index].MinTime == 0)
		{
			g_dataes[index].MinTime = takeTime;
		}
		// 값이 있다면 비교
		else
		{
			if (g_dataes[index].MinTime > takeTime)
			{
				g_dataes[index].MinTime = takeTime;
			}
		}

		// -------------------------------------------------
		// 3. 최대값 갱신
		// -------------------------------------------------
		// 값이 없으면 그냥 넣기
		if (g_dataes[index].MaxTime == 0)
		{
			g_dataes[index].MaxTime = takeTime;
		}
		// 값이 있다면 비교
		else
		{
			if (g_dataes[index].MaxTime < takeTime)
			{
				g_dataes[index].MaxTime = takeTime;
			}
		}

		// -------------------------------------------------
		// 4. 전체 카운팅 갱신
		// -------------------------------------------------
		g_dataes[index].TotTime += takeTime;

		g_iProfilingTime += timeGetTime() - iStartTime;

		return 0;
	}

	// -------------------------------------------------
	// 2. 태그가 없다면 -1 리턴
	// -------------------------------------------------
	else
	{
		return -1;
	}
}


//-------------------------------------------------------------
// 프로파일링 데이터 출력
//-------------------------------------------------------------
void OutputProfilingData(void)
{
	WCHAR szTimeStamp[32];
	GetTimeStamp(szTimeStamp, 32);

	// -------------------------------------------------
	// 파일 이름 만들기
	// -------------------------------------------------
	wchar_t fileName[FILE_NAME_LENGTH] = L"";
	GetTimeString(fileName);

	// -------------------------------------------------
	// 파일에 프로파일링 데이터 쓰기
	// -------------------------------------------------
	FILE* stream;
	_wfopen_s(&stream, fileName, L"a");

	// -------------------------------------------------
	// micro 단위로 변경
	// -------------------------------------------------
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	frequency.QuadPart = frequency.QuadPart / 1000000;

	float avg;

	if (stream != nullptr)
	{
		// -------------------------------------------------
		// 쓰기 시작
		// -------------------------------------------------
		fwprintf(stream, L"%s\n", szTimeStamp);
		fwprintf(stream, L"====================================================================================================================\n");
		fwprintf(stream, L"             Tag |             Total |		     Avg |               Min |               Max |            Call |\n");
		fwprintf(stream, L"====================================================================================================================\n");
		for (int i = 0; i < PROFILING_DATA_MAX; i++)
		{
			if (g_dataes[i].Call > 0)
			{
				avg = (float)((float)g_dataes[i].TotTime - g_dataes[i].MinTime - g_dataes[i].MaxTime) / (float)(g_dataes[i].Call - 2);

				fwprintf(stream,
					L"%16s | %15.4fus | %15.4fus | %15.4fus | %15.4fus | %15u |\n",
					g_dataes[i].Tag,		// 태그
					g_dataes[i].TotTime,	// 총카운팅
					avg,					// 평균소요시간
					g_dataes[i].MinTime,	// 최소소요시간
					g_dataes[i].MaxTime,	// 최대소요시간
					g_dataes[i].Call		// 호출횟수
				);
			}
			else
			{
				break;
			}
		}
		fwprintf(stream, L"====================================================================================================================\n\n\n");
		fclose(stream);
	}
}


//-------------------------------------------------------------
// 프로파일링 데이터 리셋
//-------------------------------------------------------------
void ResetProfilingData(void)
{
	memset(&g_dataes[0], 0, sizeof(ProfilingDataSet) * PROFILING_DATA_MAX);
}


//-------------------------------------------------------------
// 시간 문자열 얻기
//-------------------------------------------------------------
void GetTimeString(wchar_t* dest)
{
	// -------------------------------------------------
	// 현재 시간 값 얻기
	// -------------------------------------------------
	time_t currentTime;
	time(&currentTime);

	// -------------------------------------------------
	// 현재 시간 값 가공
	// -------------------------------------------------
	tm today;
	localtime_s(&today, &currentTime);

	// -------------------------------------------------
	// 문자열 쓰기
	// -------------------------------------------------
	wchar_t buffer[17];
	swprintf_s(buffer, 17,
		L"%02d%02d%02d",
		(today.tm_year + 1900) - 2000,
		today.tm_mon + 1,
		today.tm_mday
	);
	wcscat_s(dest, FILE_NAME_LENGTH, buffer);
	wcscat_s(dest, FILE_NAME_LENGTH, L" Profiling.txt");
}

void GetTimeStamp(WCHAR* szDest, const int iSize)
{
	time_t	Time;
	tm		Today;

	time(&Time);
	localtime_s(&Today, &Time);

	swprintf_s(szDest, iSize, L"[%02d/%02d/%02d %02d:%02d:%02d]",
		Today.tm_mon + 1,
		Today.tm_mday,
		(Today.tm_year + 1900) - 2000,
		Today.tm_hour,
		Today.tm_min,
		Today.tm_sec
	);
}
