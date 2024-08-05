#pragma once

#include <list>

#include "cpp_redis/cpp_redis"

using namespace std;

namespace cov1013
{
	class CRedis_TLS
	{
	public:
		CRedis_TLS();
		~CRedis_TLS();
		void Set(const char* Key, char* Value);
		void Get(const char* Key, future<cpp_redis::reply>* pReply);
		bool Disconnect(void);

	private:
		void LockClients(void);
		void UnlockClients(void);

	private:
		DWORD						m_dwTlsIndex;
		list<cpp_redis::client*>	m_Clients;
		SRWLOCK						m_Clients_srw;
	};
}