#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "Profiler.h"

extern int g_iProfilingTime;

ProfilingDataSet g_dataes[PROFILING_DATA_MAX];

//-------------------------------------------------------------
// �������ϸ� ���� ���
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
		// ������ ���� �Ǵ�
		// -------------------------------------------------
		if (g_dataes[i].Call > 0)
		{
			// -------------------------------------------------
			// ���ڷ� ���� �±׸�� ������ �ε��� ã��
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
	// �±׸� ã�� �������� �űԵ��
	// -------------------------------------------------
	if (flag == false)
	{
		wcscpy_s(g_dataes[index].Tag, TAG_LENGTH, tag);
	}

	// -------------------------------------------------
	// ���� �ð� ����
	// -------------------------------------------------
	QueryPerformanceCounter(&g_dataes[index].StartCounting);

	// -------------------------------------------------
	// ȣ�� Ƚ�� ����
	// -------------------------------------------------
	g_dataes[index].Call++;

	g_iProfilingTime += timeGetTime() - iStartTime;
}


//-------------------------------------------------------------
// �������ϸ� ���� 
//-------------------------------------------------------------
int EndProfiling(const wchar_t* tag)
{
	int iStartTime;

	int index = 0;
	bool flag = false;

	// -------------------------------------------------
	// �ε��� ã��
	// -------------------------------------------------
	iStartTime = timeGetTime();
	for (int i = 0; i < PROFILING_DATA_MAX; i++)
	{
		// -------------------------------------------------
		// ������ ���� �Ǵ�
		// -------------------------------------------------
		if (g_dataes[i].Call > 0)
		{
			// -------------------------------------------------
			// ���ڷ� ���� �±׸�� ������ �ε��� ã��
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
	// 1. ������ �±װ� �ִٸ� ����
	// -------------------------------------------------
	if (flag == true)
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		frequency.QuadPart /= 1000000;			// us ������ ����

		// -------------------------------------------------
		// 1. ���̰� ���ϱ�
		// -------------------------------------------------
		LARGE_INTEGER curCounting;
		QueryPerformanceCounter(&curCounting);

		__int64 period = curCounting.QuadPart - g_dataes[index].StartCounting.QuadPart;
		double takeTime = (double)period / (double)frequency.QuadPart;

		// -------------------------------------------------
		// 2. �ּҰ� ����
		// -------------------------------------------------
		// ���� ������ �׳� �ֱ�
		if (g_dataes[index].MinTime == 0)
		{
			g_dataes[index].MinTime = takeTime;
		}
		// ���� �ִٸ� ��
		else
		{
			if (g_dataes[index].MinTime > takeTime)
			{
				g_dataes[index].MinTime = takeTime;
			}
		}

		// -------------------------------------------------
		// 3. �ִ밪 ����
		// -------------------------------------------------
		// ���� ������ �׳� �ֱ�
		if (g_dataes[index].MaxTime == 0)
		{
			g_dataes[index].MaxTime = takeTime;
		}
		// ���� �ִٸ� ��
		else
		{
			if (g_dataes[index].MaxTime < takeTime)
			{
				g_dataes[index].MaxTime = takeTime;
			}
		}

		// -------------------------------------------------
		// 4. ��ü ī���� ����
		// -------------------------------------------------
		g_dataes[index].TotTime += takeTime;

		g_iProfilingTime += timeGetTime() - iStartTime;

		return 0;
	}

	// -------------------------------------------------
	// 2. �±װ� ���ٸ� -1 ����
	// -------------------------------------------------
	else
	{
		return -1;
	}
}


//-------------------------------------------------------------
// �������ϸ� ������ ���
//-------------------------------------------------------------
void OutputProfilingData(void)
{
	WCHAR szTimeStamp[32];
	GetTimeStamp(szTimeStamp, 32);

	// -------------------------------------------------
	// ���� �̸� �����
	// -------------------------------------------------
	wchar_t fileName[FILE_NAME_LENGTH] = L"";
	GetTimeString(fileName);

	// -------------------------------------------------
	// ���Ͽ� �������ϸ� ������ ����
	// -------------------------------------------------
	FILE* stream;
	_wfopen_s(&stream, fileName, L"a");

	// -------------------------------------------------
	// micro ������ ����
	// -------------------------------------------------
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	frequency.QuadPart = frequency.QuadPart / 1000000;

	float avg;

	if (stream != nullptr)
	{
		// -------------------------------------------------
		// ���� ����
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
					g_dataes[i].Tag,		// �±�
					g_dataes[i].TotTime,	// ��ī����
					avg,					// ��ռҿ�ð�
					g_dataes[i].MinTime,	// �ּҼҿ�ð�
					g_dataes[i].MaxTime,	// �ִ�ҿ�ð�
					g_dataes[i].Call		// ȣ��Ƚ��
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
// �������ϸ� ������ ����
//-------------------------------------------------------------
void ResetProfilingData(void)
{
	memset(&g_dataes[0], 0, sizeof(ProfilingDataSet) * PROFILING_DATA_MAX);
}


//-------------------------------------------------------------
// �ð� ���ڿ� ���
//-------------------------------------------------------------
void GetTimeString(wchar_t* dest)
{
	// -------------------------------------------------
	// ���� �ð� �� ���
	// -------------------------------------------------
	time_t currentTime;
	time(&currentTime);

	// -------------------------------------------------
	// ���� �ð� �� ����
	// -------------------------------------------------
	tm today;
	localtime_s(&today, &currentTime);

	// -------------------------------------------------
	// ���ڿ� ����
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
