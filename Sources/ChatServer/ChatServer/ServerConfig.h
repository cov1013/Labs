#pragma once

struct ServerConfig
{
	unsigned int SessionMaxCount = 1;
	unsigned int SessionKeyMaxLength = 64;
	unsigned int NickNameMaxLength = 20;
	unsigned int SectorMaxX = 50;
	unsigned int SectorMaxY = 50;

	bool Verify()
	{
		return false;
	}
};