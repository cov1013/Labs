/************************************************
*	내가 만든 링버퍼
*
*	버퍼가 끝점에 도달하면 다음 데이터 삽입 포인터가
*	시작점으로 이동하므로 EnQ와 DeQ를 잘 한다면 정해진 사이즈 만큼의
*	버퍼 공간을 리사이징 할 필요없이 계속 사용할 수 있다.
*
************************************************/
#ifndef __STREAM_SQ__
#define __STREAM_SQ__

#define dfMAX 3000

class CStreamQ
{
private:
	//char* mBuffer;
	char			m_bypBuffer[dfMAX];		// 실제 데이터가 저장되어있는 버퍼
	char* m_bypRear;				// 쓸 곳을 가리키는 포인터
	char* m_bypFront;				// 읽을 곳을 가리키는 포인터
	unsigned int	m_iBufferSize;			// 버퍼 사이즈
	bool			m_bCycleFlag;			// 순회 판단 플래그

public:
	CStreamQ();
	//CStreamQ(const int bufferSize);
	~CStreamQ();

	//////////////////////////////////////////
	// 
	//////////////////////////////////////////
	void Resize(const int size);


	//////////////////////////////////////////
	// 
	//////////////////////////////////////////
	unsigned int GetBufferSize(void);

	//////////////////////////////////////////
	// 현재 사용중인 용량 얻기
	//
	// Parameters: 없음
	// Return: (int)사용중인 용량
	//////////////////////////////////////////
	unsigned int GetUseSize(void);


	//////////////////////////////////////////
	// 현재 버퍼에 남은 용량 얻기
	//
	// Parameters: 없음
	// Return: (int)남은 용량
	//////////////////////////////////////////
	unsigned int GetFreeSize(void);


	//////////////////////////////////////////
	// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이
	// (끊기지 않는 길이)
	//
	// 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
	// 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않는 길이를 의미
	//
	// Parameters: 없음
	// Return: (int)사용가능 용량
	//////////////////////////////////////////
	int DirectEnqueueSize(void);
	int DirectDequeueSize(void);


	//////////////////////////////////////////
	// WritePos에 데이터 넣음
	// 
	// Parameters: (char*)데이터 포인터. (int) 크기
	// Return: (int)넣은 크기
	//////////////////////////////////////////
	int Put(const char* bypSource, int iSize);


	//////////////////////////////////////////
	// ReadPos에 데이터 읽어옴. ReadPos 이동
	//
	// Parameters: (char*) 데이터 포인터, (int) 크기
	// Return: (int)가져온 크기
	//////////////////////////////////////////
	int Get(char* bypDest, int iSize);

	//////////////////////////////////////////
	// ReadPos에서 데이터 읽어옴. ReadPos 고정.
	//
	// Parameters: (char*)데이터포인터. (int)크기
	// Return: (int) 가져온 크기
	//////////////////////////////////////////
	int Peek(char* bypDest, int iSize);

	//////////////////////////////////////////
	// 원하는 길이만큼 읽기위치에서 삭제/쓰기 위치 이동
	//
	// Parameters: 없음.
	// Return: 없음.
	//////////////////////////////////////////
	void MoveRear(const int iSize);
	void MoveFront(const int iSize);

	//////////////////////////////////////////
	// 버퍼의 모든 데이터 삭제
	//
	// Parameters: 없음.
	// Return: 없음
	//////////////////////////////////////////
	void ClearBuffer(void);


	//////////////////////////////////////////
	// 버퍼의 Front 포인터 얻음.
	//
	// Parameters: 없음
	// Return: (char*) 버퍼 포인터.
	//////////////////////////////////////////
	char* GetFrontBufferPtr(void);


	//////////////////////////////////////////
	// 버퍼의 RearPos 포인터 얻음.
	//
	// Parameters: 없음
	// Return: (char*) 버퍼 포인터.
	//////////////////////////////////////////
	char* GetRearBufferPtr(void);
};

#endif