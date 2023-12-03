
#pragma comment(lib, "tacopie/tacopie.lib")
#pragma comment(lib, "cpp_redis/cpp_redis.lib")
#include "CRedis_TLS.h"

namespace cov1013
{
	CRedis_TLS::CRedis_TLS()
	{
		m_dwTlsIndex = TlsAlloc();
		InitializeSRWLock(&m_Clients_srw);
	}

	CRedis_TLS::~CRedis_TLS()
	{
		list<client*>::iterator ClientIter = m_Clients.begin();
		list<client*>::iterator ClientEnd  = m_Clients.end();
		for (ClientIter; ClientIter != ClientEnd;)
		{
			delete (*ClientIter);
			ClientIter = m_Clients.erase(ClientIter);
		}

		TlsFree(m_dwTlsIndex);
	}

	void CRedis_TLS::Set(const char* Key, char* Value)
	{
		client* pClient = (client*)TlsGetValue(m_dwTlsIndex);

		//----------------------------------------------------------------
		// �����͸� ������ ������ �Ҵ�� ���� �ִ��� Ȯ���Ѵ�.
		//----------------------------------------------------------------
		if (pClient == nullptr)
		{
			//----------------------------------------------------------------
			// ������ �Ҵ�Ǿ� ���� �ʴٸ�, �� �Լ���
			// �ش� �����忡 ���� ���ʷ� ȣ��� ����̴�.
			//----------------------------------------------------------------
			pClient = new client;
			pClient->connect();

			//----------------------------------------------------------------
			// ���� �����̳ʿ� �߰�
			//----------------------------------------------------------------
			LockClients();
			m_Clients.push_back(pClient);
			UnlockClients();

			//----------------------------------------------------------------
			// ���Ӱ� ���޵� ���� �����Ѵ�.
			//----------------------------------------------------------------
			TlsSetValue(m_dwTlsIndex, (LPVOID)pClient);
		}

		pClient->set(Key, Value);
		pClient->sync_commit();
	}

	void CRedis_TLS::Get(const char* Key, future<reply>* pReply)
	{
		client* pClient = (client*)TlsGetValue(m_dwTlsIndex);

		//----------------------------------------------------------------
		// �����͸� ������ ������ �Ҵ�� ���� �ִ��� Ȯ���Ѵ�.
		//----------------------------------------------------------------
		if (pClient == nullptr)
		{
			//----------------------------------------------------------------
			// ������ �Ҵ�Ǿ� ���� �ʴٸ�, �� �Լ���
			// �ش� �����忡 ���� ���ʷ� ȣ��� ����̴�.
			//----------------------------------------------------------------
			pClient = new client;
			pClient->connect();

			//----------------------------------------------------------------
			// ���� �����̳ʿ� �߰�
			//----------------------------------------------------------------
			LockClients();
			m_Clients.push_back(pClient);
			UnlockClients();

			//----------------------------------------------------------------
			// ���Ӱ� ���޵� ���� �����Ѵ�.
			//----------------------------------------------------------------
			TlsSetValue(m_dwTlsIndex, (LPVOID)pClient);
		}

		*pReply = pClient->get(Key);
		pClient->sync_commit();
	}

	bool CRedis_TLS::Disconnect(void)
	{
		client* pClient = (client*)TlsGetValue(m_dwTlsIndex);

		//----------------------------------------------------------------
		// �����͸� ������ ������ �Ҵ�� ���� �ִ��� Ȯ���Ѵ�.
		//----------------------------------------------------------------
		if (pClient == nullptr)
		{
			return false;
		}

		pClient->disconnect();

		//----------------------------------------------------------------
		// ���� �����̳ʿ��� ����
		//----------------------------------------------------------------
		LockClients();
		list<client*>::iterator ClientIter = m_Clients.begin();
		list<client*>::iterator ClientEnd = m_Clients.end();
		for (ClientIter; ClientIter != ClientEnd; ++ClientIter)
		{
			if (*ClientIter == pClient)
			{
				delete (*ClientIter);
				m_Clients.erase(ClientIter);

				UnlockClients();
				return true;
			}
		}
		UnlockClients();

		return false;
	}
	void CRedis_TLS::LockClients(void)
	{
		AcquireSRWLockExclusive(&m_Clients_srw);
	}
	void CRedis_TLS::UnlockClients(void)
	{
		ReleaseSRWLockExclusive(&m_Clients_srw);
	}
}