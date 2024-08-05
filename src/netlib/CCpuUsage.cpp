#include <windows.h>
#include "CCpuUsage.h"

namespace cov1013
{
	//--------------------------------------------------------------------
// 생성자, 확인대상 프로세스 핸들. 미입력시 자기 자신.
//--------------------------------------------------------------------
	CCpuUsage::CCpuUsage(HANDLE hProcess)
	{
		//--------------------------------------------------------------------
		// 프로세스 핸들 입력이 없다면 자기 자신이 대상
		//--------------------------------------------------------------------
		if (hProcess == INVALID_HANDLE_VALUE)
		{
			m_hProcess = GetCurrentProcess();
		}

		//--------------------------------------------------------------------
		// 프로세서 개수를 확인한다.
		// 
		// 프로세스 (exe) 실행률 계산시 cpu 개수로 나누기를 하여 실제 사용률을 구함.
		//--------------------------------------------------------------------
		SYSTEM_INFO SystemInfo;

		GetSystemInfo(&SystemInfo);
		m_iNumberOfProcessors = SystemInfo.dwNumberOfProcessors;

		m_fProcessorTotal = 0;
		m_fProcessorUser = 0;
		m_fProcessorKernel = 0;

		m_fProcessTotal = 0;
		m_fProcessUser = 0;
		m_fProcessKernel = 0;

		m_ftProcessor_LastKernel.QuadPart = 0;
		m_ftProcessor_LastUser.QuadPart = 0;
		m_ftProcessor_LastIdle.QuadPart = 0;

		m_ftProcess_LastUser.QuadPart = 0;
		m_ftProcess_LastKernel.QuadPart = 0;
		m_ftProcess_LastTime.QuadPart = 0;

		UpdateCpuTime();
	}

	//////////////////////////////////////////////////////////////////////
	// 소멸자
	//////////////////////////////////////////////////////////////////////
	CCpuUsage::~CCpuUsage()
	{
	}

	///////////////////////////////////////////////////////////////////
	// CPU 사용률 갱신
	///////////////////////////////////////////////////////////////////
	void CCpuUsage::UpdateCpuTime(void)
	{
		//--------------------------------------------------------------------
		// 프로세서 사용률을 갱신한다.
		// 
		// 본래의 사용 구조체는 FILETIME 이지만, ULARGE_INTEGER 와 구조가 같으므로 이를 사용함.
		// FILETIME 구조체는 100 나노세컨드 단위의 시간 단위를 표현하는 구조체임.
		//--------------------------------------------------------------------
		ULARGE_INTEGER Idle;
		ULARGE_INTEGER Kernel;
		ULARGE_INTEGER User;

		//--------------------------------------------------------------------
		// 시스템 사용 시간을 구한다.
		// 
		// 아이들(가동되지 않는) 타임 / 커널 사용 타임 (아이들포함) / 유저 사용 타임
		//--------------------------------------------------------------------
		if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false)
		{
			return;
		}

		// 커널 타임에는 아이들 타임이 포함됨.
		ULONGLONG KernelDiff = Kernel.QuadPart - m_ftProcessor_LastKernel.QuadPart;
		ULONGLONG UserDiff = User.QuadPart - m_ftProcessor_LastUser.QuadPart;
		ULONGLONG IdleDiff = Idle.QuadPart - m_ftProcessor_LastIdle.QuadPart;

		ULONGLONG Total = KernelDiff + UserDiff;
		ULONGLONG TimeDiff;

		if (Total == 0)
		{
			m_fProcessorUser = 0.0f;
			m_fProcessorKernel = 0.0f;
			m_fProcessorTotal = 0.0f;
		}
		else
		{
			// 커널 타임에 아이들 타임이 있으므로 빼서 계산.
			m_fProcessorTotal = (float)((double)(Total - IdleDiff) / Total * 100.0f);
			m_fProcessorUser = (float)((double)UserDiff / Total * 100.0f);
			m_fProcessorKernel = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
		}

		m_ftProcessor_LastKernel = Kernel;
		m_ftProcessor_LastUser = User;
		m_ftProcessor_LastIdle = Idle;

		//--------------------------------------------------------------------
		// 지정된 프로세스 사용률을 갱신한다.
		//--------------------------------------------------------------------
		ULARGE_INTEGER None;
		ULARGE_INTEGER NowTime;

		//--------------------------------------------------------------------
		// 현재의 100 나노세컨드 단위 시간을 구한다. UTC 시간.
		// 
		// 프로세스 사용률 판단의 공식
		// 
		// a = 샘플간격의 시스템 시간을 구함. (그냥 실제로 지나간 시간)
		// b = 프로세스의 CPU 사용 시간을 구함.
		// 
		// a : 100 = b : 사용률  공식으로 사용률을 구함.
		//--------------------------------------------------------------------

		//--------------------------------------------------------------------
		// 얼마의 시간이 지났는지 100 나노세컨드 시간을 구함.
		//--------------------------------------------------------------------
		GetSystemTimeAsFileTime((LPFILETIME)&NowTime);

		//--------------------------------------------------------------------
		// 해당 프로세스가 사용한 시간을 구함.
		// 
		// 두번째, 세번째는 실행, 종료 시간으로 미사용.
		//--------------------------------------------------------------------
		GetProcessTimes(m_hProcess, (LPFILETIME)&None, (LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);

		//--------------------------------------------------------------------
		// 이전에 저장된 프로세스 시간과의 차를 구해서 실제로 얼마의 시간이 지났는지 확인.
		// 
		// 그리고 실제 지나온 시간으로 나누면 사용률이 나옴.
		//--------------------------------------------------------------------
		TimeDiff = NowTime.QuadPart - m_ftProcess_LastTime.QuadPart;
		UserDiff = User.QuadPart - m_ftProcess_LastUser.QuadPart;
		KernelDiff = Kernel.QuadPart - m_ftProcess_LastKernel.QuadPart;

		Total = KernelDiff + UserDiff;

		m_fProcessTotal = (float)(Total / (double)m_iNumberOfProcessors / (double)TimeDiff * 100.0f);
		m_fProcessKernel = (float)(KernelDiff / (double)m_iNumberOfProcessors / (double)TimeDiff * 100.0f);
		m_fProcessUser = (float)(UserDiff / (double)m_iNumberOfProcessors / (double)TimeDiff * 100.0f);

		m_ftProcess_LastTime = NowTime;
		m_ftProcess_LastKernel = Kernel;
		m_ftProcess_LastUser = User;
	}
}