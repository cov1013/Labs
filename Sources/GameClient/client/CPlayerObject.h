#pragma once
class CPlayerObject : public CBaseObject
{
private:
	BOOL	m_bPlayerCharacter;
	BOOL	m_bEffectFlag;
	DWORD	m_dwActionCur;		// 지금 실행중인 액션
	DWORD	m_dwActionOld;		// 이전 실행했던 액션
	int		m_iDirCur;			// 지금 방향
	int		m_iDirOld;			// 이전 방향
	char	m_chHP;

public:
	CPlayerObject();
	CPlayerObject(const BOOL bPlayerCharacter, const int iSessionID, const int iDir, const int iX, const int iY, const BYTE chHP);
	virtual ~CPlayerObject();

	void	ActionProc(void);
	void	InputActionProc(void);

	void	Update(void);
	void	Render(BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch);

	BOOL	IsPlayer(void);
	BOOL	IsEffect();

	BOOL    MoveRangeCheck(const BYTE byDirection, const WORD shX, const WORD shY);

	void	OnEffectFlag();
	void	OffEffectFlag();

	const DWORD	GetActionCur(void)	const;
	const int	GetDirection(void)	const;
	const char	GetHP(void)			const;

	void SetDirection(void);
	void SetCurDirection(int iDirection);

	void SetHP(const BYTE chHP);

	void SetActionAttack1(void);
	void SetActionAttack2(void);
	void SetActionAttack3(void);
	void SetActionMove(void);
	void SetActionStand(void);
};