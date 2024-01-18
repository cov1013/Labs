#include "framework.h"
#include "resource.h"

// COMMOM
#include "Protocol.h"
#include "Define.h"
#include "Struct.h"

// NETWORK
#include "CStreamQ.h"
#include "CPacket.h"

#include "makePacket.h"

void mpMoveStart(st_PACKET_HEADER* Header, CPacket* pPayload, const BYTE byDir, const WORD shX, const WORD shY)
{
	pPayload->Clear();

	//------------------------------------------
	// 페이로드 세팅
	//------------------------------------------
	*pPayload << byDir;
	*pPayload << shX;
	*pPayload << shY;

	//------------------------------------------
	// 헤더 세팅
	//------------------------------------------
	Header->byCode = dfNETWORK_PACKET_CODE;
	Header->bySize = pPayload->GetUseSize();
	Header->byType = dfPACKET_CS_MOVE_START;
}

void mpMoveStop(st_PACKET_HEADER* Header, CPacket* pPayload, const BYTE byDir, const WORD shX, const WORD shY)
{
	pPayload->Clear();

	//------------------------------------------
	// 페이로드 세팅
	//------------------------------------------
	*pPayload << byDir;
	*pPayload << shX;
	*pPayload << shY;

	//------------------------------------------
	// 헤더 세팅
	//------------------------------------------
	Header->byCode = dfNETWORK_PACKET_CODE;
	Header->bySize = pPayload->GetUseSize();
	Header->byType = dfPACKET_CS_MOVE_STOP;
}

void mpAttack1(st_PACKET_HEADER* Header, CPacket* pPayload, const BYTE byDir, const WORD shX, const WORD shY)
{
	pPayload->Clear();

	//------------------------------------------
	// 페이로드 세팅
	//------------------------------------------
	*pPayload << byDir;
	*pPayload << shX;
	*pPayload << shY;

	//------------------------------------------
	// 헤더 세팅
	//------------------------------------------
	Header->byCode = dfNETWORK_PACKET_CODE;
	Header->bySize = pPayload->GetUseSize();
	Header->byType = dfPACKET_CS_ATTACK1;
}

void mpAttack2(st_PACKET_HEADER* Header, CPacket* pPayload, const BYTE byDir, const WORD shX, const WORD shY)
{
	pPayload->Clear();

	//------------------------------------------
	// 페이로드 세팅
	//------------------------------------------
	*pPayload << byDir;
	*pPayload << shX;
	*pPayload << shY;

	//------------------------------------------
	// 헤더 세팅
	//------------------------------------------
	Header->byCode = dfNETWORK_PACKET_CODE;
	Header->bySize = pPayload->GetUseSize();
	Header->byType = dfPACKET_CS_ATTACK2;
}

void mpAttack3(st_PACKET_HEADER* Header, CPacket* pPayload, const BYTE byDir, const WORD shX, const WORD shY)
{
	pPayload->Clear();

	//------------------------------------------
	// 페이로드 세팅
	//------------------------------------------
	*pPayload << byDir;
	*pPayload << shX;
	*pPayload << shY;

	//------------------------------------------
	// 헤더 세팅
	//------------------------------------------
	Header->byCode = dfNETWORK_PACKET_CODE;
	Header->bySize = pPayload->GetUseSize();
	Header->byType = dfPACKET_CS_ATTACK3;
}

void mpMoveStart(st_PACKET_HEADER* Header, stMSG_CS_MOVE_START* Packet, const BYTE byDir, const WORD shX, const WORD shY)
{
	//------------------------------------------
	// 헤더 세팅
	//------------------------------------------
	Header->byCode = dfNETWORK_PACKET_CODE;
	Header->bySize = sizeof(stMSG_CS_MOVE_START);
	Header->byType = dfPACKET_CS_MOVE_START;

	//------------------------------------------
	// 패킷 세팅
	//------------------------------------------
	Packet->byDir = byDir;
	Packet->shX = shX;
	Packet->shY = shY;
}

void mpMoveStop(st_PACKET_HEADER* Header, stMSG_CS_MOVE_STOP* Packet, const BYTE byDir, const WORD shX, const WORD shY)
{
	//------------------------------------------
	// 헤더 세팅
	//------------------------------------------
	Header->byCode = dfNETWORK_PACKET_CODE;
	Header->bySize = sizeof(stMSG_CS_MOVE_STOP);
	Header->byType = dfPACKET_CS_MOVE_STOP;

	//------------------------------------------
	// 패킷 세팅
	//------------------------------------------
	Packet->byDir = byDir;
	Packet->shX = shX;
	Packet->shY = shY;
}

void mpAttack1(st_PACKET_HEADER* Header, stMSG_CS_ATTACK1* Packet, const BYTE byDir, const WORD shX, const WORD shY)
{
	//------------------------------------------
	// 헤더 세팅
	//------------------------------------------
	Header->byCode = dfNETWORK_PACKET_CODE;
	Header->bySize = sizeof(stMSG_CS_ATTACK1);
	Header->byType = dfPACKET_CS_ATTACK1;

	//------------------------------------------
	// 패킷 세팅
	//------------------------------------------
	Packet->byDir = byDir;
	Packet->shX = shX;
	Packet->shY = shY;
}

void mpAttack2(st_PACKET_HEADER* Header, stMSG_CS_ATTACK2* Packet, const BYTE byDir, const WORD shX, const WORD shY)
{
	//------------------------------------------
	// 헤더 세팅
	//------------------------------------------
	Header->byCode = dfNETWORK_PACKET_CODE;
	Header->bySize = sizeof(stMSG_CS_ATTACK2);
	Header->byType = dfPACKET_CS_ATTACK2;

	//------------------------------------------
	// 패킷 세팅
	//------------------------------------------
	Packet->byDir = byDir;
	Packet->shX = shX;
	Packet->shY = shY;
}

void mpAttack3(st_PACKET_HEADER* Header, stMSG_CS_ATTACK3* Packet, const BYTE byDir, const WORD shX, const WORD shY)
{
	//------------------------------------------
	// 헤더 세팅
	//------------------------------------------
	Header->byCode = dfNETWORK_PACKET_CODE;
	Header->bySize = sizeof(stMSG_CS_ATTACK3);
	Header->byType = dfPACKET_CS_ATTACK3;

	//------------------------------------------
	// 패킷 세팅
	//------------------------------------------
	Packet->byDir = byDir;
	Packet->shX = shX;
	Packet->shY = shY;
}