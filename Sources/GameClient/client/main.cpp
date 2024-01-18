#include "framework.h"
#include "resource.h"

// COMMOM
#include "CList.h"
#include "Protocol.h"
#include "Define.h"
#include "Struct.h"

// SYSTEM
#include "CLogger.h"
#include "CFrameSkip.h"
#include "CScreenDib.h"
#include "CSpriteDib.h"
#include "profiler.h"

// NETWORK
#include "CStreamQ.h"
#include "CPacket.h"
#include "makePacket.h"
#include "packetProc.h"
#include "network.h"

// CONTENTS
#include "CKeyManager.h"
#include "CBaseObject.h"		
#include "CPlayerObject.h"		
#include "CEffectObject.h"		
#include "CObjectManager.h"		
#include "CCamera.h"

//---------------------------------------------------
// 클라이언트 가동을 위한 리소스 로드
//---------------------------------------------------
void LoadResource(void);

//---------------------------------------------------
// 리소스 정리
//---------------------------------------------------
void ClearResource(void);

//---------------------------------------------------
// 매 프레임 실행되어야할 게임 로직
//---------------------------------------------------
void Update(void);

//---------------------------------------------------
// 매 프레임 화면에 랜더링
//---------------------------------------------------
void Render(void);

void				WindowInit(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

HWND g_hWnd;
HIMC g_hOldMC;
BOOL g_bActiveApp = FALSE;

BOOL g_bConnected = FALSE;
BOOL g_bExitFlag = FALSE;

CFrameSkip		g_cFrameSkip(50);
CScreenDib		g_cScreenDib(dfSCREEN_WIDTH, dfSCREEN_HEIGHT, 32);
CSpriteDib		g_cSpriteDib(66, 0x00ffffff);
CKeyManager		g_cKeyManager;
CObjectManager	g_cObjectManager;

SOCKET			g_Socket;
CStreamQ		g_RecvRQ;
CStreamQ		g_SendRQ;

CPlayerObject*	g_cPlayerCharacter;
CCamera			g_CCamera(&g_cPlayerCharacter, dfSCREEN_WIDTH, dfSCREEN_HEIGHT);
//e_SPRITE		g_TIleMap[100][100];

DWORD g_dwSessionID;
int g_iLogTime;
int g_iProfilingTime;

extern int g_iUpdateFPS;
extern int g_iRenderFPS;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR    lpCmdLine, _In_ int       nCmdShow)
{
	//_CrtSetBreakAlloc(533);
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	FILE* pFile = freopen("CON", "w", stdout);

	MSG msg = { 0 };

	timeBeginPeriod(1);					// 시간 해상도 변경

	_wsetlocale(LC_ALL, L"Korean");		// 한글 출력을 위한 유니코드 세팅

	LoadResource();						// 스프라이트 및 게임 데이터 로드

	WindowInit(hInstance, nCmdShow);	// 윈도우 생성

	netStartUp(&g_hWnd);				// 네트워크 초기화

	while (1)
	{
		// 매 프레임 진입시 게임 종료 체크
		if (g_bExitFlag)	
		{
			break;
		}

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (WM_QUIT == msg.message)
			{
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// 네트워크 메세지가 없으면 게임 로직 업데이트
			if (g_bConnected)
			{
				Update();
			}
		}
	}

	OutputProfilingData();	// 프로파일링 데이터 출력
	
	ClearResource();		// 게임 리소스 정리
		
	netCleanUp();

	FreeConsole();

	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	timeEndPeriod(1);

	return (int)msg.wParam;
}

void LoadResource(void)
{
	g_cSpriteDib.LoadDibSprite(eMAP, L"SpriteData\\_Map.bmp", 0, 0);
	g_cSpriteDib.LoadDibSprite(ePLAYER_STAND_L01, L"SpriteData\\Stand_L_01.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_STAND_L02, L"SpriteData\\Stand_L_02.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_STAND_L03, L"SpriteData\\Stand_L_03.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_STAND_R01, L"SpriteData\\Stand_R_01.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_STAND_R02, L"SpriteData\\Stand_R_02.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_STAND_R03, L"SpriteData\\Stand_R_03.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L01, L"SpriteData\\Move_L_01.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L02, L"SpriteData\\Move_L_02.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L03, L"SpriteData\\Move_L_03.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L04, L"SpriteData\\Move_L_04.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L05, L"SpriteData\\Move_L_05.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L06, L"SpriteData\\Move_L_06.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L07, L"SpriteData\\Move_L_07.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L08, L"SpriteData\\Move_L_08.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L09, L"SpriteData\\Move_L_09.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L10, L"SpriteData\\Move_L_10.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L11, L"SpriteData\\Move_L_11.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_L12, L"SpriteData\\Move_L_12.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R01, L"SpriteData\\Move_R_01.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R02, L"SpriteData\\Move_R_02.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R03, L"SpriteData\\Move_R_03.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R04, L"SpriteData\\Move_R_04.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R05, L"SpriteData\\Move_R_05.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R06, L"SpriteData\\Move_R_06.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R07, L"SpriteData\\Move_R_07.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R08, L"SpriteData\\Move_R_08.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R09, L"SpriteData\\Move_R_09.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R10, L"SpriteData\\Move_R_10.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R11, L"SpriteData\\Move_R_11.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_MOVE_R12, L"SpriteData\\Move_R_12.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK1_R01, L"SpriteData\\Attack1_R_01.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK1_R02, L"SpriteData\\Attack1_R_02.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK1_R03, L"SpriteData\\Attack1_R_03.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK1_R04, L"SpriteData\\Attack1_R_04.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK1_L01, L"SpriteData\\Attack1_L_01.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK1_L02, L"SpriteData\\Attack1_L_02.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK1_L03, L"SpriteData\\Attack1_L_03.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK1_L04, L"SpriteData\\Attack1_L_04.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK2_R01, L"SpriteData\\Attack2_R_01.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK2_R02, L"SpriteData\\Attack2_R_02.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK2_R03, L"SpriteData\\Attack2_R_03.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK2_R04, L"SpriteData\\Attack2_R_04.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK2_L01, L"SpriteData\\Attack2_L_01.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK2_L02, L"SpriteData\\Attack2_L_02.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK2_L03, L"SpriteData\\Attack2_L_03.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK2_L04, L"SpriteData\\Attack2_L_04.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_R01, L"SpriteData\\Attack3_R_01.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_R02, L"SpriteData\\Attack3_R_02.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_R03, L"SpriteData\\Attack3_R_03.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_R04, L"SpriteData\\Attack3_R_04.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_R05, L"SpriteData\\Attack3_R_05.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_R06, L"SpriteData\\Attack3_R_06.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_L01, L"SpriteData\\Attack3_L_01.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_L02, L"SpriteData\\Attack3_L_02.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_L03, L"SpriteData\\Attack3_L_03.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_L04, L"SpriteData\\Attack3_L_04.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_L05, L"SpriteData\\Attack3_L_05.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(ePLAYER_ATTACK3_L06, L"SpriteData\\Attack3_L_06.bmp", 71, 90);
	g_cSpriteDib.LoadDibSprite(eEFFECT_SPARK_01, L"SpriteData\\xSpark_1.bmp", 0, 0);
	g_cSpriteDib.LoadDibSprite(eEFFECT_SPARK_02, L"SpriteData\\xSpark_2.bmp", 0, 0);
	g_cSpriteDib.LoadDibSprite(eEFFECT_SPARK_03, L"SpriteData\\xSpark_3.bmp", 0, 0);
	g_cSpriteDib.LoadDibSprite(eEFFECT_SPARK_04, L"SpriteData\\xSpark_4.bmp", 0, 0);
	g_cSpriteDib.LoadDibSprite(eGUAGE_HP, L"SpriteData\\HPGuage.bmp", 35, -12);
	g_cSpriteDib.LoadDibSprite(eSHADOW, L"SpriteData\\Shadow.bmp", 32, 4);
	g_cSpriteDib.LoadDibSprite(eSCROLL_TILE, L"SpriteData\\Tile_01.bmp", 0, 0);
}

void ClearResource(void)
{
	int iCount;
	int iMaxSprite = g_cSpriteDib.GetMaxSprite();

	for (iCount = 0; iCount < iMaxSprite; iCount++)
	{
		g_cSpriteDib.ReleaseSprite(iCount);
	}
}

void KeyProc(void)
{
	// 윈도우가 활성 상태인가?
	if (!g_bActiveApp)
	{
		return;
	}

	// 플레이어 캐릭터가 생성됐는가?
	if (g_cPlayerCharacter == NULL)
	{
		return;
	}

	g_cKeyManager.Update();

	//-----------------------------------------------------
	// 이전 프레임에 눌렀었고, 지금은 땠다.
	//-----------------------------------------------------
	if (g_cKeyManager.IsAway(VK_UP) || g_cKeyManager.IsAway(VK_DOWN) || g_cKeyManager.IsAway(VK_LEFT) || g_cKeyManager.IsAway(VK_RIGHT))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_STAND);
	}

	//-----------------------------------------------------
	// 누르고 있다
	//-----------------------------------------------------
	if (g_cKeyManager.IsHold(VK_UP))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_MOVE_UU);
	}
	if (g_cKeyManager.IsHold(VK_DOWN))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_MOVE_DD);
	}
	if (g_cKeyManager.IsHold(VK_LEFT))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_MOVE_LL);
	}
	if (g_cKeyManager.IsHold(VK_RIGHT))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_MOVE_RR);
	}
	if (g_cKeyManager.IsHold(VK_LEFT) && g_cKeyManager.IsHold(VK_UP))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_MOVE_LU);
	}
	if (g_cKeyManager.IsHold(VK_RIGHT) && g_cKeyManager.IsHold(VK_UP))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_MOVE_RU);
	}
	if (g_cKeyManager.IsHold(VK_LEFT) && g_cKeyManager.IsHold(VK_DOWN))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_MOVE_LD);
	}
	if (g_cKeyManager.IsHold(VK_RIGHT) && g_cKeyManager.IsHold(VK_DOWN))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_MOVE_RD);
	}
	if (g_cKeyManager.IsHold(dfVK_Z))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_ATTACK1);
	}
	if (g_cKeyManager.IsHold(dfVK_X))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_ATTACK2);
	}
	if (g_cKeyManager.IsHold(dfVK_C))
	{
		g_cPlayerCharacter->ActionInput(dfACTION_ATTACK3);
	}

	if (g_cKeyManager.IsTap(dfVK_1))
	{
		wprintf(L"Log Level - ERROR");
		CLogger::GetInstance()->ChangeToLogLevel(dfLOG_LEVEL_ERROR);
	}

	if (g_cKeyManager.IsTap(dfVK_2))
	{
		wprintf(L"Log Level - WARN");
		CLogger::GetInstance()->ChangeToLogLevel(dfLOG_LEVEL_WARN);
	}

	if (g_cKeyManager.IsTap(dfVK_3))
	{
		wprintf(L"Log Level - DEBUG");
		CLogger::GetInstance()->ChangeToLogLevel(dfLOG_LEVEL_DEBUG);
	}

	if (g_cKeyManager.IsTap(dfVK_4))
	{
		wprintf(L"Log Level - INFO");
		CLogger::GetInstance()->ChangeToLogLevel(dfLOG_LEVEL_INFO);
	}
}

void Update(void)
{
	CCamera::GetInstance()->Update();

	// 1. 키보드 입력 처리
	KeyProc();

	// 2. 객체 액션 처리
	g_cObjectManager.Update();

	// 3. 그리기 & 프레임 스킵
	// 4. 플립
	if (g_cFrameSkip.FrameSkip())	
	{
		Render();
	}

	// FPS 출력
	g_cFrameSkip.PrintFPS(g_hWnd);
}

void Render(void)
{
	if (NULL != g_cPlayerCharacter)
	{
		BYTE* bypDest = g_cScreenDib.GetDibBuffer();
		int iDestWidth = g_cScreenDib.GetWidth();
		int iDestHeight = g_cScreenDib.GetHeight();
		int iDestPitch = g_cScreenDib.GetPitch();

		int iCameraX = CCamera::GetInstance()->GetPosX();
		int iCameraY = CCamera::GetInstance()->GetPosY();

		//-------------------------------------------------
		// Tile Draw
		//-------------------------------------------------
		int iTileX = iCameraX / 64;
		int iTileY = iCameraY / 64;

		for (int iY = 0; iY < 9; iY++)
		{
			int iDrawY = iTileY + iY;

			for (int iX = 0; iX < 11; iX++)
			{
				int iDrawX = iTileX + iX;

				g_cSpriteDib.DrawImage(
					eSCROLL_TILE,
					((iDrawX) * 64) - iCameraX,
					((iDrawY) * 64) - iCameraY,
					bypDest, iDestWidth,
					iDestHeight, iDestPitch
				);
			}
		}

		//-------------------------------------------------
		// 객체 Draw
		//-------------------------------------------------
		g_cObjectManager.Render(bypDest, iDestWidth, iDestHeight, iDestPitch);

		//-------------------------------------------------
		// Flip
		//-------------------------------------------------
		g_cScreenDib.Flip(g_hWnd);
	}
}

void WindowInit(HINSTANCE hInstance, int nCmdShow)
{
	//-------------------------------------------
	// 윈도우 생성
	//-------------------------------------------
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = 0;
	wcex.lpszClassName = L"Online_TCP_Fighter";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassExW(&wcex);

	g_hWnd = CreateWindowW(
		L"Online_TCP_Fighter",
		L"Online_TCP_Fighter",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0,
		CW_USEDEFAULT, 0,
		NULL, NULL, hInstance, NULL
	);

	if (!g_hWnd)
	{
		return;
	}

	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);

	//-------------------------------------------
	// 윈도우 사이즈 조정
	//-------------------------------------------
	SetFocus(g_hWnd);
	RECT WindowRect;
	WindowRect.top = 0;
	WindowRect.left = 0;
	WindowRect.right = dfSCREEN_WIDTH;
	WindowRect.bottom = dfSCREEN_HEIGHT;

	AdjustWindowRectEx(
		&WindowRect,
		GetWindowStyle(g_hWnd),
		GetMenu(g_hWnd) != NULL,
		GetWindowExStyle(g_hWnd)
	);

	int iCenterPosX = (GetSystemMetrics(SM_CXSCREEN) / 2) - (640 / 2);
	int iCenterPosY = (GetSystemMetrics(SM_CYSCREEN) / 2) - (380 / 2);

	MoveWindow(
		g_hWnd,
		iCenterPosX, iCenterPosY,
		WindowRect.right - WindowRect.left,
		WindowRect.bottom - WindowRect.top,
		TRUE
	);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case ASYCN_SOCKET:	// 네트워크 송수신 처리
		if (!netIOProcess(wParam, lParam))
		{
			g_bExitFlag = TRUE;
			MessageBox(g_hWnd, L"접속이 종료되었습니다.", L"끊겼지롱", MB_OK);
		}
		break;

	case WM_ACTIVATEAPP:			// 서버 연결 성공
		g_bActiveApp = (BOOL)wParam;	
		break;

	case WM_CREATE:					
		g_hOldMC = ImmAssociateContext(hWnd, NULL);
		break;

	case WM_DESTROY:
		ImmAssociateContext(hWnd, g_hOldMC);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}