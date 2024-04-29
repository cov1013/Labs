#pragma once
#include <float.h>

namespace cov1013
{
	enum en_PROFILER_CONPIG
	{
		// String
		eTAG_NAME_MAX = 32,
		eDIRECTORY_MAX = 255,
		eFILE_NAME_MAX = 255,

		// Data
		eSAMPLE_MAX = 100,
		eTHREAD_MAX = 100,
	};

	enum class en_PROFILER_UNIT
	{
		eUNIT_SEC = 1,
		eUNIT_MILLI = 1000,
		eUNIT_MICRO = 1000000,
		eUNIT_NANO = 1000000000,
	};

	struct st_SAMPLE
	{
		st_SAMPLE()
		{
			dMax[0] = 0;
			dMax[1] = 0;
			dMin[0] = DBL_MAX;
			dMin[1] = DBL_MAX;
			liCall = 0;
		}
		wchar_t				szTagName[eTAG_NAME_MAX];
		double				dTotal;
		double				dMax[2];
		double				dMin[2];
		unsigned __int64	liCall;
		LARGE_INTEGER		liPerfStart;
	};

	struct st_THREAD_SAMPLE
	{
		DWORD		dwThreadID;
		st_SAMPLE	Samples[eSAMPLE_MAX];
	};

	bool InitializeProfiler(const wchar_t* szDirectory, en_PROFILER_UNIT eUnit);

	bool SetProfilerDirectory(const wchar_t* szDirectory);

	void ReleaseProfiler(void);

	void BeginProfiling(const wchar_t* szTagName);

	void EndProfiling(const wchar_t* szTagName);

	void OutputProfilingData(void);

	#ifndef __PROFILING__
		#define PRO_BEGIN(NONE)
		#define PRO_END(NONE)
	#else
		#define PRO_BEGIN(TAG)	BeginProfiling(TAG);
		#define PRO_END(TAG)	EndProfiling(TAG);
	#endif
}