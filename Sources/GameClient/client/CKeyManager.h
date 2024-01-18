#pragma once

#define dfKEY_MAX (15)
#define dfVK_Z (0x5A)
#define dfVK_X (0x58)
#define dfVK_C (0x43)

#define dfVK_1 (0x31)
#define dfVK_2 (0x32)
#define dfVK_3 (0x33)
#define dfVK_4 (0x34)

enum KEY_STATE
{
	KEY_STATE_NONE,
	KEY_STATE_TAP,
	KEY_STATE_AWAY,
	KEY_STATE_HOLD,
	END,
};

class CKeyManager
{
public:
	CKeyManager();
	~CKeyManager();

private:
	struct stKEY_INFO
	{
		int		  KeyCode;
		KEY_STATE KeyState;
	};

	stKEY_INFO mKeyList[dfKEY_MAX];
	BOOL mKeyFlag;

public:
	KEY_STATE GetKeyState(int _keyCode);
	void Update();

	BOOL IsKeyDown(void);
	void KeyFlagOn(void);
	void KeyFlagOFF(void);

	BOOL IsAway(const int _keyCode);
	BOOL IsHold(const int _keyCode);
	BOOL IsTap(const int _keyCode);
	BOOL IsNone(const int _keyCode);
};