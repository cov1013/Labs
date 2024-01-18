#pragma once
class CEffectObject : public CBaseObject
{
private:
	BOOL			m_bActionFlag;
	DWORD			m_dwAttackID;
	CPlayerObject*  m_pPlayer;

public:
	CEffectObject();
	CEffectObject(CPlayerObject* pPlayer);
	virtual ~CEffectObject();

	void Update(void);
	void Render(BYTE* bypDest, const int iDestWidth, const int iDestHeight, const int iDestPitch);
};