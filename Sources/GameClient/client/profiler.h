#pragma once

#define TAG_LENGTH			(32)
#define FILE_NAME_LENGTH	(32)
#define PROFILING_DATA_MAX	(1000)

#define PROFILING_BEGIN(TAG) BeginProfiling(TAG);
#define PROFILING_END(TAG)   EndProfiling(TAG);

struct ProfilingDataSet
{
	unsigned int	Call;
	wchar_t			Tag[TAG_LENGTH];
	double			MinTime;
	double			MaxTime;
	double			TotTime;

	LARGE_INTEGER	StartCounting;
};

void BeginProfiling(const wchar_t* tag);
int EndProfiling(const wchar_t* tag);
void OutputProfilingData(void);
void ResetProfilingData(void);
void GetTimeString(wchar_t* dest);
void GetTimeStamp(WCHAR* szDest, const int iSize);