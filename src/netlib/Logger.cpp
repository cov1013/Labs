#include <stdio.h>
#include <windows.h>
#include "Logger.h"

namespace cov1013
{
	Logger* Logger::GetInstance()
	{
		if (m_pInstance == nullptr)
		{
			m_pInstance = new Logger();
		}

		return m_pInstance;
	}

	const bool Logger::Initialize(
		const Logger::eLogLevel eLogLevel,
		const std::wstring_view& directoryPath)
	{
		// 로그 레벨 세팅
		if (eLogLevel < Logger::eLogLevel::None || eLogLevel >= Logger::eLogLevel::Max)
		{
			return false;
		}

		sm_eLogLevel = eLogLevel;

		// 디렉토리 생성
		if (!std::filesystem::exists(directoryPath) == false
			|| !std::filesystem::is_directory(directoryPath) == false)
		{
			std::filesystem::create_directory(directoryPath);
		}

		InitializeSRWLock(&sm_pFileStream_srw);
	}

}