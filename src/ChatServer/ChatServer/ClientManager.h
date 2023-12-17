#pragma once
#include <string>
#include <functional>
#include <unordered_map>

#include "../netlib/Common.h"
#include "../netlib/CMemoryPool.h"

namespace ChatServer
{
	using ClientPool = cov1013::CMemoryPool<Client>;
	using ClientContiner = std::unordered_map<cov1013::SESSION_ID, Client*>;

	struct SectorPosInfo
	{
		WORD X;
		WORD Y;
	};

	struct Client
	{
		SectorPosInfo Sector = { -1, -1 };

		cov1013::SESSION_ID SessionId = -1;


		unsigned long LastRecvTime = 0;	// 타임스탬프?

		bool bLogind = false;
		char State = '\0';

		__int64 AccountNo = -1;


		std::string LoginId = {};

		std::string CharacterNickName = {};
	};


	
	class ClientManager
	{
	public :

		static ClientManager* GetInstance()
		{
			if (sm_pInstance == nullptr)
			{
				sm_pInstance = new ClientManager;
			}

			return sm_pInstance;
		}



	public:

		Client* Create(const cov1013::SESSION_ID sessionId)
		{
			Client* pNewClient = m_ClientPool.Alloc();
			
			
			//m_Clients.insert(std::make_pair<cov1013::SESSION_ID, Client*>(sessionId, pNewClient));
		}

		bool Delete(const cov1013::SESSION_ID sessionId)
		{
		
		
		
		}

		Client* Find(const cov1013::SESSION_ID sessionId)
		{
		
		}



	private:
		ClientManager();

		~ClientManager()
		{
			// 메모리풀 정리.
			if (m_ClientPool.GetUseCount() != 0)
			{
				// !!! Memory leak
			}

		}

	private:


		static ClientManager* sm_pInstance;


		ClientPool m_ClientPool;

		ClientContiner m_Clients; 

		unsigned int m_ClientMaxCount;

	};
}