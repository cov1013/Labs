#pragma once

namespace cov1013
{
	class CPUMeter
	{
	public:
		CPUMeter(HANDLE hProcess = INVALID_HANDLE_VALUE);
		~CPUMeter();
		void UpdateCpuTime(void);
		float ProcessorTotal(void) { return m_fProcessorTotal; }
		float ProcessorUser(void) { return m_fProcessorUser; }
		float ProcessorKernel(void) { return m_fProcessorKernel; }
		float ProcessTotal(void) { return m_fProcessTotal; }
		float ProcessUser(void) { return m_fProcessUser; }
		float ProcessKernel(void) { return m_fProcessKernel; }



	private:

		HANDLE m_hProcess;
		int	m_iNumberOfProcessors;

		float m_fProcessorTotal;
		float m_fProcessorUser;
		float m_fProcessorKernel;

		float m_fProcessTotal;
		float m_fProcessUser;
		float m_fProcessKernel;

		ULARGE_INTEGER	m_ftProcessor_LastKernel;
		ULARGE_INTEGER	m_ftProcessor_LastUser;
		ULARGE_INTEGER	m_ftProcessor_LastIdle;

		ULARGE_INTEGER	m_ftProcess_LastKernel;
		ULARGE_INTEGER	m_ftProcess_LastUser;
		ULARGE_INTEGER	m_ftProcess_LastTime;
	};
}


