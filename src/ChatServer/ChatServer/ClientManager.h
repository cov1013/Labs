#pragma once
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

#include "Common.h"
#include "MemoryPool.h"

namespace ChatServer
{
	struct Player;
	using PlayerPtr = std::shared_ptr<Player>;
	using PlayerPool = cov1013::MemoryPool<Player>;
	using PlayerContiner = std::unordered_map<cov1013::SESSION_ID, PlayerPtr>;

	struct SectorPos
	{
		WORD X;
		WORD Y;
	};

	struct Player
	{
		// Network Info
		cov1013::SESSION_ID SessionId = -1;
		unsigned long LastReceivedTime = 0;

		char State = '\0';

		SectorPos Sector = { 0, 0 };

		bool bLogined = false;

		__int64 AccountNo = -1;

		std::string LoginId = {};

		std::string CharacterNickName = {};
	};
	
	class PlayerManager
	{
	public :

		static PlayerManager* GetInstance()
		{
			if (sm_pInstance == nullptr)
			{
				sm_pInstance = new PlayerManager;
			}

			return sm_pInstance;
		}

	public:

		Player* Create(const cov1013::SESSION_ID sessionId)
		{
			auto* pNewClient = m_PlayerPool.Alloc();
			
			
			//m_Clients.insert(std::make_pair<cov1013::SESSION_ID, Client*>(sessionId, pNewClient));
		}

		bool Delete(const cov1013::SESSION_ID sessionId)
		{
		
		
		
		}

		Player* Find(const cov1013::SESSION_ID sessionId) const
		{
			
		}

	private:
		PlayerManager();

		virtual ~PlayerManager() noexcept
		{
			// 메모리풀 정리.
			if (m_PlayerPool.GetUseCount() != 0)
			{
				// !!! Memory leak
			}

		}

	private:
		static PlayerManager* sm_pInstance;
		PlayerPool m_PlayerPool;
		PlayerContiner m_Players;
		unsigned int m_PlayerMaxCount;
	};
}