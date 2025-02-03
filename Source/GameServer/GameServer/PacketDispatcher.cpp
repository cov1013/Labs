#include "PacketBuffer.h"
#include "PacketDispatcher.h"

PacketDispatcher::PacketDispatcher()
{
}

PacketDispatcher::~PacketDispatcher()
{
}

void PacketDispatcher::Regist(const int packetId, std::function<void(cov1013::PacketBuffer*)> handler)
{
	if (m_handleMap.find(packetId) == m_handleMap.end())
	{
		m_handleMap.insert(std::make_pair(packetId, handler));
	}
}
