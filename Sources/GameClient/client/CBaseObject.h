#pragma once
class CBaseObject
{
protected:
	BOOL	m_bDestroyFlag;

	DWORD	m_dwActionInput;

	int		m_iCurX;
	int		m_iCurY;

	int		m_iObjectID;
	int		m_iObjectType;

	BOOL	m_bEndFrame;
	int		m_iDelayCount;
	int		m_iFrameDelay;
	int		m_iSpriteEnd;
	int		m_iSpriteNow;
	int		m_iSpriteStart;

public:
	CBaseObject();
	virtual ~CBaseObject();

	void	ActionInput(const DWORD dwAction);
	void	NextFrame(void);
	BOOL	IsEndFrame(void);
	BOOL	IsDestroy(void);

	const int GetCurX()			const;
	const int GetCurY()			const;
	const int GetObjectID()		const;
	const int GetObjectType()	const;
	const int GetSprite()		const;

	void SetObjectID(const int iID);
	void SetPosition(const int iX, const int iY);
	void SetAnimation(const int iStart, const int iEnd, const int iFramDelay);

	virtual void Update(void) = 0;
	virtual void Render(BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch) = 0;
};