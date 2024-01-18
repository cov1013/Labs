#pragma once
#include "Common.h"
//==================================================================
// 서버 타입
//==================================================================
enum en_SERVER_TYPE
{
	MASTER = 0,
	LOGIN = 1,
	GAME = 2,
	CHAT = 3
};

//==================================================================
// 모니터링 서버에서 관리할 서버 구조체 (관리 단위)
//==================================================================


//==================================================================
// DB 저장 데이터 세트
//==================================================================
struct st_DB_WRITE_DATA_SET
{
	int		DataValue;
	int		TimeStamp;
	int		ServerNo;
	BYTE    Type;
};

//==================================================================
// 모니터링 데이터 세트
//==================================================================
struct st_MONITERING_DATA_SET
{
	st_MONITERING_DATA_SET()
	{
		DataValue = 0;
		TimeStamp = 0;
		Total = 0;
		Min[0] = INT_MAX;
		Min[1] = INT_MAX;
		Max[0] = 0;
		Max[1] = 0;
		Count = 0;
		DataType = -1;
	}

	int		DataValue;
	int		TimeStamp;
	int		Total;
	int		Min[2];
	int		Max[2];
	int		Count;
	BYTE    DataType;
};

struct st_SERVER
{
	int								ServerNo;
	cov1013::SESSION_ID						SessionID;
	DWORD							LastRecvTime;

	//===============================================
	// Default Data
	//===============================================
	st_MONITERING_DATA_SET			ServerRun;
	st_MONITERING_DATA_SET			ServerCpu;
	st_MONITERING_DATA_SET			ServerMem;
	st_MONITERING_DATA_SET			SessionCount;
	st_MONITERING_DATA_SET			PacketPoolSize;

	//===============================================
	// Login Server
	//===============================================
	st_MONITERING_DATA_SET			AuthTPS;

	//===============================================
	// Game Server
	//===============================================
	st_MONITERING_DATA_SET			AuthPlayerCount;
	st_MONITERING_DATA_SET			GamePlayerCount;
	st_MONITERING_DATA_SET			AcceptTPS;
	st_MONITERING_DATA_SET			RecvPacketTPS;
	st_MONITERING_DATA_SET			SendPacketTPS;
	st_MONITERING_DATA_SET			DBWriteTPS;
	st_MONITERING_DATA_SET			DBWriteMSG;
	st_MONITERING_DATA_SET			AuthThreadTPS;
	st_MONITERING_DATA_SET			GameThreadTPS;

	//===============================================
	// Chat Server
	//===============================================
	st_MONITERING_DATA_SET			PlayerCount;
	st_MONITERING_DATA_SET			UpdateTPS;
	st_MONITERING_DATA_SET			UpdateMsgPoolSize;
};