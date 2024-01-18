#pragma once

struct ServerConfigBase
{
	unsigned int SessionMaxCount = 1;
	unsigned int SessionKeyMaxLength = 64;
};

struct ChatServerConfig : public ServerConfigBase
{
	unsigned int NickNameMaxLength = 20;

	// 섹터 관련
	unsigned int SectorMaxX = 50;
	unsigned int SectorMaxY = 50;
};