#pragma once
#include <unordered_map>
#include <functional>
class PacketDispatcher
{
public:
	PacketDispatcher();
	~PacketDispatcher();

	void Regist(const int packetId, std::function<void(cov1013::PacketBuffer*)> handler);

private:
	std::unordered_map<int, std::function<void(cov1013::PacketBuffer*)>> m_handleMap;
};