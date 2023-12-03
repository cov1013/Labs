#include <windows.h>
#include "CCpuUsage.h"

namespace cov1013
{
	//--------------------------------------------------------------------
// ������, Ȯ�δ�� ���μ��� �ڵ�. ���Է½� �ڱ� �ڽ�.
//--------------------------------------------------------------------
	CCpuUsage::CCpuUsage(HANDLE hProcess)
	{
		//--------------------------------------------------------------------
		// ���μ��� �ڵ� �Է��� ���ٸ� �ڱ� �ڽ��� ���
		//--------------------------------------------------------------------
		if (hProcess == INVALID_HANDLE_VALUE)
		{
			m_hProcess = GetCurrentProcess();
		}

		//--------------------------------------------------------------------
		// ���μ��� ������ Ȯ���Ѵ�.
		// 
		// ���μ��� (exe) ����� ���� cpu ������ �����⸦ �Ͽ� ���� ������ ����.
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
	// �Ҹ���
	//////////////////////////////////////////////////////////////////////
	CCpuUsage::~CCpuUsage()
	{
	}

	///////////////////////////////////////////////////////////////////
	// CPU ���� ����
	///////////////////////////////////////////////////////////////////
	void CCpuUsage::UpdateCpuTime(void)
	{
		//--------------------------------------------------------------------
		// ���μ��� ������ �����Ѵ�.
		// 
		// ������ ��� ����ü�� FILETIME ������, ULARGE_INTEGER �� ������ �����Ƿ� �̸� �����.
		// FILETIME ����ü�� 100 ���뼼���� ������ �ð� ������ ǥ���ϴ� ����ü��.
		//--------------------------------------------------------------------
		ULARGE_INTEGER Idle;
		ULARGE_INTEGER Kernel;
		ULARGE_INTEGER User;

		//--------------------------------------------------------------------
		// �ý��� ��� �ð��� ���Ѵ�.
		// 
		// ���̵�(�������� �ʴ�) Ÿ�� / Ŀ�� ��� Ÿ�� (���̵�����) / ���� ��� Ÿ��
		//--------------------------------------------------------------------
		if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false)
		{
			return;
		}

		// Ŀ�� Ÿ�ӿ��� ���̵� Ÿ���� ���Ե�.
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
			// Ŀ�� Ÿ�ӿ� ���̵� Ÿ���� �����Ƿ� ���� ���.
			m_fProcessorTotal = (float)((double)(Total - IdleDiff) / Total * 100.0f);
			m_fProcessorUser = (float)((double)UserDiff / Total * 100.0f);
			m_fProcessorKernel = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
		}

		m_ftProcessor_LastKernel = Kernel;
		m_ftProcessor_LastUser = User;
		m_ftProcessor_LastIdle = Idle;

		//--------------------------------------------------------------------
		// ������ ���μ��� ������ �����Ѵ�.
		//--------------------------------------------------------------------
		ULARGE_INTEGER None;
		ULARGE_INTEGER NowTime;

		//--------------------------------------------------------------------
		// ������ 100 ���뼼���� ���� �ð��� ���Ѵ�. UTC �ð�.
		// 
		// ���μ��� ���� �Ǵ��� ����
		// 
		// a = ���ð����� �ý��� �ð��� ����. (�׳� ������ ������ �ð�)
		// b = ���μ����� CPU ��� �ð��� ����.
		// 
		// a : 100 = b : ����  �������� ������ ����.
		//--------------------------------------------------------------------

		//--------------------------------------------------------------------
		// ���� �ð��� �������� 100 ���뼼���� �ð��� ����.
		//--------------------------------------------------------------------
		GetSystemTimeAsFileTime((LPFILETIME)&NowTime);

		//--------------------------------------------------------------------
		// �ش� ���μ����� ����� �ð��� ����.
		// 
		// �ι�°, ����°�� ����, ���� �ð����� �̻��.
		//--------------------------------------------------------------------
		GetProcessTimes(m_hProcess, (LPFILETIME)&None, (LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);

		//--------------------------------------------------------------------
		// ������ ����� ���μ��� �ð����� ���� ���ؼ� ������ ���� �ð��� �������� Ȯ��.
		// 
		// �׸��� ���� ������ �ð����� ������ ������ ����.
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