/////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------

메세지 구조체 정의

자신의 캐릭터에 대한 패킷을 서버에게 보낼 때, 각 동작에 대한
메세지를 제작하여 서버에 보내준다.

- 메세지 설정 함수는 Client -> Server 방향의 메세지만 설정한다.

---------------------------------------------------------------*/
#pragma once

struct st_PACKET_HEADER
{
	BYTE byCode;
	BYTE bySize;
	BYTE byType;
};
//---------------------------------------------------------------
// 헤더
// 
//---------------------------------------------------------------
/*
	BYTE	Code;			// 패킷코드 0x89 고정.
	BYTE	Size;			// 패킷 사이즈.
	BYTE	Type;			// 패킷타입.
*/


#pragma pack(push, 1)
struct stMSG_SC_CREATE_MY_CHARACTER
{
	DWORD	dwSessionID;
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
	BYTE	byHP;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 클라이언트 자신의 캐릭터 할당		Server -> Client
//
// 서버에 접속시 최초로 받게되는 패킷으로 자신이 할당받은 ID 와
// 자신의 최초 위치, HP 를 받게 된다. (처음에 한번 받게 됨)
// 
// 이 패킷을 받으면 자신의 ID,X,Y,HP 를 저장하고 캐릭터를 생성시켜야 한다.
//
//	4	-	ID
//	1	-	Direction	(LL / RR)
//	2	-	X
//	2	-	Y
//	1	-	HP
//
//---------------------------------------------------------------



#pragma pack(push, 1)
struct stMSG_SC_CREATE_OTHER_CHARACTER
{
	DWORD	dwSessionID;
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
	BYTE	byHP;
};
#pragma pack(push, 4)
//---------------------------------------------------------------
// 다른 클라이언트의 캐릭터 생성 패킷		Server -> Client
//
// 처음 서버에 접속시 이미 접속되어 있던 캐릭터들의 정보
// 또는 게임중에 접속된 클라이언트들의 생성용 정보.
//
//
//	4	-	ID
//	1	-	Direction	(LL / RR)
//	2	-	X
//	2	-	Y
//	1	-	HP
//
//---------------------------------------------------------------



struct stMSG_SC_DELETE_CHARACTER
{
	DWORD dwSessionID;
};
//---------------------------------------------------------------
// 캐릭터 삭제 패킷						Server -> Client
//
// 캐릭터의 접속해제 또는 캐릭터가 죽었을때 전송됨.
//
//	4	-	ID
//
//---------------------------------------------------------------



#pragma pack(push, 1)
struct stMSG_CS_MOVE_START
{
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 캐릭터 이동시작 패킷						Client -> Server
//
// 자신의 캐릭터 이동시작시 이 패킷을 보낸다.
// 이동 중에는 본 패킷을 보내지 않으며, 키 입력이 변경되었을 경우에만
// 보내줘야 한다.
//
// (왼쪽 이동중 위로 이동 / 왼쪽 이동중 왼쪽 위로 이동... 등등)
//
//	1	-	Direction	( 방향 디파인 값 8방향 사용 )
//	2	-	X
//	2	-	Y
//
// dfPACKET_MOVE_DIR_LL					0
// dfPACKET_MOVE_DIR_LU					1
// dfPACKET_MOVE_DIR_UU					2
// dfPACKET_MOVE_DIR_RU					3
// dfPACKET_MOVE_DIR_RR					4
// dfPACKET_MOVE_DIR_RD					5
// dfPACKET_MOVE_DIR_DD					6
// dfPACKET_MOVE_DIR_LD					7
// 
//---------------------------------------------------------------



#pragma pack(push, 1)
struct stMSG_SC_MOVE_START
{
	DWORD	dwSessionID;
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 캐릭터 이동시작 패킷						Server -> Client
//
// 다른 유저의 캐릭터 이동시 본 패킷을 받는다.
// 패킷 수신시 해당 캐릭터를 찾아 이동처리를 해주도록 한다.
// 
// 패킷 수신 시 해당 키가 계속해서 눌린것으로 생각하고
// 해당 방향으로 계속 이동을 하고 있어야만 한다.
//
//	4	-	ID
//	1	-	Direction	( 방향 디파인 값 8방향 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------


#pragma pack(push, 1)
struct stMSG_CS_MOVE_STOP
{
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 캐릭터 이동중지 패킷						Client -> Server
//
// 이동중 키보드 입력이 없어서 정지되었을 때, 이 패킷을 서버에 보내준다.
// 이동중 방향 전환시에는 스탑을 보내지 않는다.
//
//	1	-	Direction	( 방향 디파인 값 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------



#pragma pack(push, 1)
struct stMSG_SC_MOVE_STOP
{
	DWORD	dwSessionID;
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 캐릭터 이동중지 패킷						Server -> Client
//
// ID 에 해당하는 캐릭터가 이동을 멈춘것이므로 
// 캐릭터를 찾아서 방향과, 좌표를 입력해주고 멈추도록 처리한다.
//
//	4	-	ID
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------



#pragma pack(push, 1)
struct stMSG_CS_ATTACK1
{
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Client -> Server
//
// 공격 키 입력시 본 패킷을 서버에게 보낸다.
// 충돌 및 데미지에 대한 결과는 서버에서 알려 줄 것이다.
//
// 공격 동작 시작시 한번만 서버에게 보내줘야 한다.
//
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y	
//
//---------------------------------------------------------------



#pragma pack(push, 1)
struct stMSG_SC_ATTACK1
{
	DWORD	dwSessionID;
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Server -> Client
//
// 패킷 수신시 해당 캐릭터를 찾아서 공격1번 동작으로 액션을 취해준다.
// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
//
//	4	-	ID
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------



#pragma pack(push, 1)
struct stMSG_CS_ATTACK2
{
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Client -> Server
//
// 공격 키 입력시 본 패킷을 서버에게 보낸다.
// 충돌 및 데미지에 대한 결과는 서버에서 알려 줄 것이다.
//
// 공격 동작 시작시 한번만 서버에게 보내줘야 한다.
//
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------



#pragma pack(push, 1)
struct stMSG_SC_ATTACK2
{
	DWORD	dwSessionID;
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Server -> Client
//
// 패킷 수신시 해당 캐릭터를 찾아서 공격2번 동작으로 액션을 취해준다.
// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
//
//	4	-	ID
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------



#pragma pack(push, 1)
struct stMSG_CS_ATTACK3
{
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Client -> Server
//
// 공격 키 입력시 본 패킷을 서버에게 보낸다.
// 충돌 및 데미지에 대한 결과는 서버에서 알려 줄 것이다.
//
// 공격 동작 시작시 한번만 서버에게 보내줘야 한다.
//
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------



#pragma pack(push, 1)
struct stMSG_SC_ATTACK3
{
	DWORD	dwSessionID;
	BYTE	byDir;
	WORD	shX;
	WORD	shY;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Server -> Client
//
// 패킷 수신시 해당 캐릭터를 찾아서 공격2번 동작으로 액션을 취해준다.
// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
//
//	4	-	ID
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------



#pragma pack(push, 1)
struct stMSG_SC_DAMAGE
{
	DWORD	dwAttackID;
	DWORD	dwDamageID;
	BYTE	byDamageHP;
};
#pragma pack(pop, 1)
//---------------------------------------------------------------
// 캐릭터 데미지 패킷							Server -> Client
//
// 공격에 맞은 캐릭터의 정보를 보냄.
//
//	4	-	AttackID	( 공격자 ID )
//	4	-	DamageID	( 피해자 ID )
//	1	-	DamageHP	( 피해자 HP )
//
//---------------------------------------------------------------


//////////////////////////////////////////////////////////////////////
// 직렬화 버퍼를 이용한 방법

void mpMoveStart(st_PACKET_HEADER* pHeader, CPacket* pPayload, const BYTE byDir, const WORD shX, const WORD shY);
//---------------------------------------------------------------
// 캐릭터 이동시작 패킷 설정 프로시저			Client -> Server
//
// 패킷에 필요한 데이터를 넘겨받은 헤더와 패킷에 세팅한다.
//---------------------------------------------------------------



void mpMoveStop(st_PACKET_HEADER* pHeader, CPacket* pPayload, const BYTE byDir, const WORD shX, const WORD shY);
//---------------------------------------------------------------
// 캐릭터 이동중지 패킷 설정 프로시저			Client -> Server
//
// 패킷에 필요한 데이터를 넘겨받은 헤더와 패킷에 세팅한다.
//---------------------------------------------------------------



void mpAttack1(st_PACKET_HEADER* pHeader, CPacket* pPayload, const BYTE byDir, const WORD shX, const WORD shY);
//---------------------------------------------------------------
// 캐릭터 공격 패킷 설정 프로시저				Client -> Server
//
// 패킷에 필요한 데이터를 넘겨받은 헤더와 패킷에 세팅한다.
//---------------------------------------------------------------



void mpAttack2(st_PACKET_HEADER* pHeader, CPacket* pPayload, const BYTE byDir, const WORD shX, const WORD shY);
//---------------------------------------------------------------
// 캐릭터 공격 패킷 설정 프로시저				Client -> Server
//
// 패킷에 필요한 데이터를 넘겨받은 헤더와 패킷에 세팅한다.
//---------------------------------------------------------------



void mpAttack3(st_PACKET_HEADER* pHeader, CPacket* pPayload, const BYTE byDir, const WORD shX, const WORD shY);
//---------------------------------------------------------------
// 캐릭터 공격 패킷 설정 프로시저				Client -> Server
//
// 패킷에 필요한 데이터를 넘겨받은 헤더와 패킷에 세팅한다.
//---------------------------------------------------------------


//////////////////////////////////////////////////////////////////////
// 마샬링을 이용한 방법

void mpMoveStart(st_PACKET_HEADER* pHeader, stMSG_CS_MOVE_START* pPayload, const BYTE byDir, const WORD shX, const WORD shY);
//---------------------------------------------------------------
// 캐릭터 이동시작 패킷 설정 프로시저			Client -> Server
//
// 패킷에 필요한 데이터를 넘겨받은 헤더와 패킷에 세팅한다.
//---------------------------------------------------------------



void mpMoveStop(st_PACKET_HEADER* pHeader, stMSG_CS_MOVE_STOP* pPayload, const BYTE byDir, const WORD shX, const WORD shY);
//---------------------------------------------------------------
// 캐릭터 이동중지 패킷 설정 프로시저			Client -> Server
//
// 패킷에 필요한 데이터를 넘겨받은 헤더와 패킷에 세팅한다.
//---------------------------------------------------------------



void mpAttack1(st_PACKET_HEADER* pHeader, stMSG_CS_ATTACK1* pPayload, const BYTE byDir, const WORD shX, const WORD shY);
//---------------------------------------------------------------
// 캐릭터 공격 패킷 설정 프로시저				Client -> Server
//
// 패킷에 필요한 데이터를 넘겨받은 헤더와 패킷에 세팅한다.
//---------------------------------------------------------------



void mpAttack2(st_PACKET_HEADER* pHeader, stMSG_CS_ATTACK2* pPayload, const BYTE byDir, const WORD shX, const WORD shY);
//---------------------------------------------------------------
// 캐릭터 공격 패킷 설정 프로시저				Client -> Server
//
// 패킷에 필요한 데이터를 넘겨받은 헤더와 패킷에 세팅한다.
//---------------------------------------------------------------



void mpAttack3(st_PACKET_HEADER* pHeader, stMSG_CS_ATTACK3* pPayload, const BYTE byDir, const WORD shX, const WORD shY);
//---------------------------------------------------------------
// 캐릭터 공격 패킷 설정 프로시저				Client -> Server
//
// 패킷에 필요한 데이터를 넘겨받은 헤더와 패킷에 세팅한다.
//---------------------------------------------------------------