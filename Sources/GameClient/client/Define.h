#pragma once

// ---------------------------------------------------------
// 클라이언트 스크린 사이즈
// ---------------------------------------------------------
#define dfSCREEN_WIDTH		(640)
#define dfSCREEN_HEIGHT		(480)

// ---------------------------------------------------------
// 서버 포트
// ---------------------------------------------------------
#define dfNETWORK_PORT		(20000)

// ---------------------------------------------------------
// 비동기 네트워크 입출력을 위한 소켓
// ---------------------------------------------------------
#define ASYCN_SOCKET		(WM_USER + 1)

// ---------------------------------------------------------
// 이동 속도					// 50fps 기준
// ---------------------------------------------------------
#define dfSPEED_PLAYER_X	(3)
#define dfSPEED_PLAYER_Y	(2)

// ---------------------------------------------------------
// 이동 범위
// ---------------------------------------------------------
#define dfRANGE_MOVE_TOP    (0)
#define dfRANGE_MOVE_LEFT   (0)
#define dfRANGE_MOVE_RIGHT  (6400)
#define dfRANGE_MOVE_BOTTOM (6400)

// ---------------------------------------------------------
// 액션
// ---------------------------------------------------------
#define dfACTION_MOVE_LL	(0)
#define dfACTION_MOVE_LU	(1)
#define dfACTION_MOVE_UU	(2)
#define dfACTION_MOVE_RU	(3)
#define dfACTION_MOVE_RR	(4)
#define dfACTION_MOVE_RD	(5)
#define dfACTION_MOVE_DD	(6)
#define dfACTION_MOVE_LD	(7)
#define dfACTION_ATTACK1	(8)	
#define dfACTION_ATTACK2	(9)	
#define dfACTION_ATTACK3	(10)	
#define dfACTION_STAND		(11)

#define dfACTION_END        (12)

// ---------------------------------------------------------
// 데미지
// ---------------------------------------------------------
#define dfATTACK1_DAMAGE	(1)
#define dfATTACK2_DAMAGE	(2)
#define dfATTACK3_DAMAGE	(3)

// ---------------------------------------------------------
// 애니메이션 딜레이
// ---------------------------------------------------------
#define dfDELAY_STAND		(5)
#define dfDELAY_MOVE		(4)
#define dfDELAY_ATTACK1		(4)
#define dfDELAY_ATTACK2		(4)
#define dfDELAY_ATTACK3		(4)
#define dfDELAY_EFFECT		(3)

// ---------------------------------------------------------
// 오브젝트 타입
// ---------------------------------------------------------
enum e_OBJECT_TYPE
{
	ePLAYER = 0,
	eEFFECT,
};

// ---------------------------------------------------------
// 스프라이트 ID
// ---------------------------------------------------------
enum e_SPRITE
{
	eMAP = 0,
	ePLAYER_STAND_L01,
	ePLAYER_STAND_L02,
	ePLAYER_STAND_L03,
	ePLAYER_STAND_R01,
	ePLAYER_STAND_R02,
	ePLAYER_STAND_R03,
	ePLAYER_MOVE_L01,
	ePLAYER_MOVE_L02,
	ePLAYER_MOVE_L03,
	ePLAYER_MOVE_L04,
	ePLAYER_MOVE_L05,
	ePLAYER_MOVE_L06,
	ePLAYER_MOVE_L07,
	ePLAYER_MOVE_L08,
	ePLAYER_MOVE_L09,
	ePLAYER_MOVE_L10,
	ePLAYER_MOVE_L11,
	ePLAYER_MOVE_L12,
	ePLAYER_MOVE_R01,
	ePLAYER_MOVE_R02,
	ePLAYER_MOVE_R03,
	ePLAYER_MOVE_R04,
	ePLAYER_MOVE_R05,
	ePLAYER_MOVE_R06,
	ePLAYER_MOVE_R07,
	ePLAYER_MOVE_R08,
	ePLAYER_MOVE_R09,
	ePLAYER_MOVE_R10,
	ePLAYER_MOVE_R11,
	ePLAYER_MOVE_R12,
	ePLAYER_ATTACK1_R01,
	ePLAYER_ATTACK1_R02,
	ePLAYER_ATTACK1_R03,
	ePLAYER_ATTACK1_R04,
	ePLAYER_ATTACK1_L01,
	ePLAYER_ATTACK1_L02,
	ePLAYER_ATTACK1_L03,
	ePLAYER_ATTACK1_L04,
	ePLAYER_ATTACK2_R01,
	ePLAYER_ATTACK2_R02,
	ePLAYER_ATTACK2_R03,
	ePLAYER_ATTACK2_R04,
	ePLAYER_ATTACK2_L01,
	ePLAYER_ATTACK2_L02,
	ePLAYER_ATTACK2_L03,
	ePLAYER_ATTACK2_L04,
	ePLAYER_ATTACK3_R01,
	ePLAYER_ATTACK3_R02,
	ePLAYER_ATTACK3_R03,
	ePLAYER_ATTACK3_R04,
	ePLAYER_ATTACK3_R05,
	ePLAYER_ATTACK3_R06,
	ePLAYER_ATTACK3_L01,
	ePLAYER_ATTACK3_L02,
	ePLAYER_ATTACK3_L03,
	ePLAYER_ATTACK3_L04,
	ePLAYER_ATTACK3_L05,
	ePLAYER_ATTACK3_L06,
	eEFFECT_SPARK_01,
	eEFFECT_SPARK_02,
	eEFFECT_SPARK_03,
	eEFFECT_SPARK_04,
	eGUAGE_HP,
	eSHADOW,
	eSCROLL_TILE
};