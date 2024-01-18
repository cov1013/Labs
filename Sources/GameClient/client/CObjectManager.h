#pragma once
class CObjectManager
{
private:
	int					m_iObjectCount;
	CList<CBaseObject*> m_cObjectList;

public:
	CObjectManager();
	~CObjectManager();

	void Update(void);
	void Render(BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch);
	void DestroyProc(void);
	void AscSortY(void);
	BOOL AddObject(CBaseObject* cObject);
	BOOL DeleteObject(int iObjectID);
	CBaseObject* FindObject(int iObjectID);
};

