#pragma once


//---------------------------------------------------
//---------------------------------------------------
void netPacketProc_CreateMyCharacter(CPacket* pPacket);

//---------------------------------------------------
//---------------------------------------------------
void netPacketProc_CreateOtherCharacter(CPacket* pPacket);

//---------------------------------------------------
//---------------------------------------------------
void netPacketProc_DeleteCharacter(CPacket* pPacket);

//---------------------------------------------------
//---------------------------------------------------
void netPacketProc_MoveStart(CPacket* pPacket);

//---------------------------------------------------
//---------------------------------------------------
void netPacketProc_MoveStop(CPacket* pPacket);

//---------------------------------------------------
//---------------------------------------------------
void netPacketProc_Attack1(CPacket* pPacket);

//---------------------------------------------------
//---------------------------------------------------
void netPacketProc_Attack2(CPacket* pPacket);

//---------------------------------------------------
//---------------------------------------------------
void netPacketProc_Attack3(CPacket* pPacket);

//---------------------------------------------------
//---------------------------------------------------
void netPacketProc_Damage(CPacket* pPacket);

//---------------------------------------------------
//---------------------------------------------------
void netPacketProc_Sync(CPacket* pPacket);

//BOOL PacketProc(const BYTE byPacketType, const char* Packet);
//void netPacketProc_CreateMyCharacter(const char* pPayloadBuffer);
//void netPacketProc_CreateOtherCharacter(const char* pPayloadBuffer);
//void netPacketProc_DeleteCharacter(const char* pPayloadBuffer);
//void netPacketProc_MoveStart(const char* pPayloadBuffer);
//void netPacketProc_MoveStop(const char* pPayloadBuffer);
//void netPacketProc_Attack1(const char* pPayloadBuffer);
//void netPacketProc_Attack2(const char* pPayloadBuffer);
//void netPacketProc_Attack3(const char* pPayloadBuffer);
//void netPacketProc_Damage(const char* pPayloadBuffer);