#pragma once

#include "MemoryPool.h"
#include "MemoryPool_TLS.h"

//#define __MULTI_THREAD_DEBUG_MODE__

namespace covEngine
{
	class PacketBuffer
	{
		friend class MemoryPool<PacketBuffer>;
		friend class MemoryPool_TLS<PacketBuffer>;

	public:
		enum en_CONFIG
		{
			eHEADER_LEN = 5,
			ePAYLOAD_LEN = 500
		};

#ifdef __MULTI_THREAD_DEBUG_MODE__

		// 10 : Alloc
		// 20 : AddRef
		// 30 : SubRef
		enum en_LOGIC
		{
			//================================================
			// Library
			// 
			// 1000 ~ 9999		: THREAD
			// 10000 ~ 99999	: LOGIC
			//================================================
			ACCEPT_THREAD			= 10000,
			ACCEPT_THREAD_PART1		= 11000,
			ACCEPT_THREAD_PART2		= 12000,
			ACCEPT_THREAD_PART3		= 13000,
			ACCEPT_THREAD_PART4		= 14000,
			ACCEPT_THREAD_PART5		= 15000,

			AUTH_THREAD				= 20000,
			AUTH_THREAD_PART1		= 21000,
			AUTH_THREAD_PART2		= 22000,
			AUTH_THREAD_PART3		= 23000,
			AUTH_THREAD_PART4		= 24000,
			AUTH_THREAD_PART5		= 25000,

			SEND_THREAD				= 30000,

			WORKER_THREAD			= 40000,

			GAME_THREAD				= 50000,
			GAME_THREAD_PART1		= 51000,
			GAME_THREAD_PART2		= 52000,
			GAME_THREAD_PART3		= 53000,
			GAME_THREAD_PART4		= 54000,
			GAME_THREAD_PART5		= 55000,

			NEW_SESSION				= 1000,
			RECV_PROC				= 2000,
			SEND_PROC				= 3000,
			RECV_POST				= 4000,
			SEND_POST				= 5000,
			SEND_PACKET				= 6000,
			SEND_PACKET_PRIVATE		= 6500,
			DISCONNECT				= 7000,
			DISCONNECT_PRIVATE		= 7500,
			RELEASE_SESSION			= 8000,

			//================================================
			// Contents
			//================================================
			PACKET_PROC_LOGIN		= 90000,
			PACKET_PROC_ECHO		= 100000,
		};

		struct st_LOG
		{
			enum
			{
				en_LOG_MAX = 100,
			};
			DWORD		ID;			// 4
			int			Logic;		// 4
			DWORD		Time;		// 4
			long		RefCount;	// 4
			SESSION_ID	SessionID;	// 8
		};

#endif

		/*---------------------------------------------------------------------*/
		// LanServer 전용 패킷 구조체
		//
		// Len		: 페이로드 길이 (헤더)
		/*---------------------------------------------------------------------*/
		struct st_PACKET_LAN
		{
			unsigned short	Len;
		};

		/*---------------------------------------------------------------------*/
		// 패킷 헤더 구조체
		//
		// Code      : 패킷 클래스 전용 고유 값
		// Len       : 페이로드 길이
		// RandKey   : 디코딩을 위한 랜덤 키값
		// Checksnum : 복호화 실패시 패킷 데이터 사용을 불가능하게 만들기위한 체크섬
		/*---------------------------------------------------------------------*/
#pragma pack(push, 1)
		struct st_PACKET_HEADER
		{
			unsigned char	Code;
			unsigned short	Len;
			unsigned char	RandKey;
			unsigned char	CheckSum;
		};
#pragma pack(pop)

	public:

#ifdef __MULTI_THREAD_DEBUG_MODE__
		static SESSION_ID OutputDebugData(void);
		static PacketBuffer* Alloc(int Logic, SESSION_ID SessionID);
		void AddRef(int Logic, SESSION_ID SessionID);
		void SubRef(int Logic, SESSION_ID SessionID);
#endif

		static void SetPacketKey(unsigned char byKey);
		static void SetPacketCode(unsigned char byCode);
		static PacketBuffer* Alloc(void);
		void AddRef(void);
		void SubRef(void);
		const int Get(char* bypDest, const int iLength);
		const int Get_UNI16(wchar_t* bypDest, int iLength);
		const int Put(char* bypSrc, const int iLength);
		const int Put_UNI16(const wchar_t* bypSrc, int iLength);
		const int Peek(char* bypDest, const int iLength);
		const int Peek_UNI16(wchar_t* bypDest, int iLength);
		const int MoveWritePos(const int iLength);
		const int MoveReadPos(const int iLength);
		char* GetBufferPtr(void) const;
		char* GetReadPos(void) const;
		char* GetWritePos(void) const;
		const int GetFreeSize(void) const;
		const int GetUseSize(void) const;
		const int GetPacketSize(void) const;
		static const long GetPoolCapacity(void);
		static const long GetPoolUseSize(void);
		void Encode(void);
		bool Decode(void);
		unsigned char MakeChecksum(void);
		void  SetLanHeader(void);
		char* GetLanHeaderPtr(void) const;
		const int GetLanPacketSize(void) const;
		PacketBuffer& operator << (const BYTE src);
		PacketBuffer& operator << (const char src);
		PacketBuffer& operator << (const WORD src);
		PacketBuffer& operator << (const short src);
		PacketBuffer& operator << (const UINT src);
		PacketBuffer& operator << (const int src);
		PacketBuffer& operator << (const DWORD src);
		PacketBuffer& operator << (const long src);
		PacketBuffer& operator << (const float src);
		PacketBuffer& operator << (const double src);
		PacketBuffer& operator << (const __int64 src);
		PacketBuffer& operator >> (BYTE& dest);
		PacketBuffer& operator >> (char& dest);
		PacketBuffer& operator >> (WORD& dest);
		PacketBuffer& operator >> (short& dest);
		PacketBuffer& operator >> (UINT& dest);
		PacketBuffer& operator >> (int& dest);
		PacketBuffer& operator >> (DWORD& dest);
		PacketBuffer& operator >> (long& dest);
		PacketBuffer& operator >> (float& dest);
		PacketBuffer& operator >> (double& dest);
		PacketBuffer& operator >> (__int64& dest);

#ifdef __MULTI_THREAD_DEBUG_MODE__
		static PacketBuffer*						sm_AlloPacketBuffer[USHRT_MAX];
		static short						sm_AlloPacketBufferIndex;
		int									m_iIndex;
#endif

		long								m_lRefCount;

	private:
		PacketBuffer();
		virtual ~PacketBuffer();
		void Initial(void);
		void Release(void);

	private:
		bool								m_bInitialize = false;
		bool								m_bEncode;
		char								m_bypBuffer[eHEADER_LEN + ePAYLOAD_LEN];
		char*								m_bypWritePos;
		char*								m_bypReadPos;
		int									m_iBufferSize;
		int									m_iFreeSize;
		int									m_iUseSize;


		static unsigned char				sm_byPacketKey;
		static unsigned char				sm_byPacketCode;

		static MemoryPool_TLS<PacketBuffer>		sm_MemoryPool;
		//static MemoryPool<PacketBuffer>			sm_MemoryPool;

#ifdef __MULTI_THREAD_DEBUG_MODE__
		st_LOG								m_Logs[st_LOG::en_LOG_MAX];
		short								m_LogIndex;
#endif
	};
}