#pragma once

#include "CMemoryPool.h"
#include "CMemoryPool_TLS.h"

//#define __MULTI_THREAD_DEBUG_MODE__

namespace cov1013
{
	class CPacket
	{
		friend class CMemoryPool<CPacket>;
		friend class CMemoryPool_TLS<CPacket>;

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
		// LanServer ���� ��Ŷ ����ü
		//
		// Len		: ���̷ε� ���� (���)
		/*---------------------------------------------------------------------*/
		struct st_PACKET_LAN
		{
			unsigned short	Len;
		};

		/*---------------------------------------------------------------------*/
		// ��Ŷ ��� ����ü
		//
		// Code      : ��Ŷ Ŭ���� ���� ���� ��
		// Len       : ���̷ε� ����
		// RandKey   : ���ڵ��� ���� ���� Ű��
		// Checksnum : ��ȣȭ ���н� ��Ŷ ������ ����� �Ұ����ϰ� ��������� üũ��
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
		static CPacket* Alloc(int Logic, SESSION_ID SessionID);
		void AddRef(int Logic, SESSION_ID SessionID);
		void SubRef(int Logic, SESSION_ID SessionID);
#endif

		static void SetPacketKey(unsigned char byKey);
		static void SetPacketCode(unsigned char byCode);
		static CPacket* Alloc(void);
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
		CPacket& operator << (const BYTE src);
		CPacket& operator << (const char src);
		CPacket& operator << (const WORD src);
		CPacket& operator << (const short src);
		CPacket& operator << (const UINT src);
		CPacket& operator << (const int src);
		CPacket& operator << (const DWORD src);
		CPacket& operator << (const long src);
		CPacket& operator << (const float src);
		CPacket& operator << (const double src);
		CPacket& operator << (const __int64 src);
		CPacket& operator >> (BYTE& dest);
		CPacket& operator >> (char& dest);
		CPacket& operator >> (WORD& dest);
		CPacket& operator >> (short& dest);
		CPacket& operator >> (UINT& dest);
		CPacket& operator >> (int& dest);
		CPacket& operator >> (DWORD& dest);
		CPacket& operator >> (long& dest);
		CPacket& operator >> (float& dest);
		CPacket& operator >> (double& dest);
		CPacket& operator >> (__int64& dest);

#ifdef __MULTI_THREAD_DEBUG_MODE__
		static CPacket*						sm_AllocPacket[USHRT_MAX];
		static short						sm_AllocPacketIndex;
		int									m_iIndex;
#endif

		long								m_lRefCount;

	private:
		CPacket();
		virtual ~CPacket();
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

		static CMemoryPool_TLS<CPacket>		sm_MemoryPool;
		//static CMemoryPool<CPacket>			sm_MemoryPool;

#ifdef __MULTI_THREAD_DEBUG_MODE__
		st_LOG								m_Logs[st_LOG::en_LOG_MAX];
		short								m_LogIndex;
#endif
	};
}