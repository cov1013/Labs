#include <windows.h>
#include "CKeyManager.h"

char g_VK_KeyCodes[dfKEY_MAX] = {
	VK_UP,
	VK_DOWN,
	VK_LEFT,
	VK_RIGHT,
	dfVK_Z,	// Z
	dfVK_X,	// X
	dfVK_C,	// C
	dfVK_1,
	dfVK_2,
	dfVK_3,
	dfVK_4
};

CKeyManager::CKeyManager()
{
	mKeyFlag = FALSE;
	for (int i = 0; i < dfKEY_MAX; i++)
	{
		mKeyList[i].KeyCode = g_VK_KeyCodes[i];
		mKeyList[i].KeyState = KEY_STATE_NONE;
	}
}

CKeyManager::~CKeyManager()
{
}

KEY_STATE CKeyManager::GetKeyState(int _keyCode)
{
	for (int i = 0; i < dfKEY_MAX; i++)
	{
		if (_keyCode == mKeyList[i].KeyCode)
		{
			return mKeyList[i].KeyState;
		}
	}

	return KEY_STATE::END;
}

void CKeyManager::Update()
{
	for (int i = 0; i < dfKEY_MAX; i++)
	{
		if (GetAsyncKeyState(mKeyList[i].KeyCode))
		{
			if (mKeyList[i].KeyState == KEY_STATE_NONE || mKeyList[i].KeyState == KEY_STATE_AWAY)
			{
				mKeyList[i].KeyState = KEY_STATE_TAP;	// 이전에 안눌렸고 지금 눌림 
			}
			else
			{
				mKeyList[i].KeyState = KEY_STATE_HOLD;	// 이전에 눌렸고 지금도 눌림
			}
		}
		else
		{
			if (mKeyList[i].KeyState == KEY_STATE_TAP || mKeyList[i].KeyState == KEY_STATE_HOLD)
			{
				mKeyList[i].KeyState = KEY_STATE_AWAY;	// 이전에 눌렸고, 지금은 안눌림
			}
			else
			{
				mKeyList[i].KeyState = KEY_STATE_NONE;	// 이전에 안눌리고, 지금도 안눌림
			}
		}
	}
}

BOOL CKeyManager::IsKeyDown(void)
{
	return mKeyFlag;
}

void CKeyManager::KeyFlagOn(void)
{
	mKeyFlag = TRUE;
}

void CKeyManager::KeyFlagOFF(void)
{
	mKeyFlag = FALSE;
}

BOOL CKeyManager::IsAway(const int _keyCode)
{
	for (int i = 0; i < dfKEY_MAX; i++)
	{
		if (mKeyList[i].KeyCode == _keyCode)
		{
			if (mKeyList[i].KeyState == KEY_STATE_AWAY)
			{
				return TRUE;
			}
			return FALSE;
		}
	}

	return FALSE;
}

BOOL CKeyManager::IsHold(const int _keyCode)
{

	for (int i = 0; i < dfKEY_MAX; i++)
	{
		if (mKeyList[i].KeyCode == _keyCode)
		{
			if (mKeyList[i].KeyState == KEY_STATE_HOLD)
			{
				return TRUE;
			}
			return FALSE;
		}
	}

	return FALSE;
}

BOOL CKeyManager::IsTap(const int _keyCode)
{

	for (int i = 0; i < dfKEY_MAX; i++)
	{
		if (mKeyList[i].KeyCode == _keyCode)
		{
			if (mKeyList[i].KeyState == KEY_STATE_TAP)
			{
				return TRUE;
			}
			return FALSE;
		}
	}

	return FALSE;
}

BOOL CKeyManager::IsNone(const int _keyCode)
{

	for (int i = 0; i < dfKEY_MAX; i++)
	{
		if (mKeyList[i].KeyCode == _keyCode)
		{
			if (mKeyList[i].KeyState == KEY_STATE_NONE)
			{
				return TRUE;
			}
			return FALSE;
		}
	}

	return FALSE;
}

