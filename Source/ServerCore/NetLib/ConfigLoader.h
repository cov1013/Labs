#pragma once
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace cov1013
{
	class ConfigLoader
	{
		enum en_CONFIG
		{
			eWORD_MAX = 256,
		};

	public:
		ConfigLoader() = default;
		~ConfigLoader()
		{
			if (m_szOrigin == nullptr)
			{
				return;
			}
			free(m_szOrigin);
		}

		bool LoadFile(const wchar_t* szFileName)
		{
			FILE* pFileStream;
			_wfopen_s(&pFileStream, szFileName, L"r");
			if (pFileStream == nullptr)
			{
				return false;
			}

			// 파일 사이즈 계산
			fseek(pFileStream, 0, SEEK_END);
			m_dwFileSize = ftell(pFileStream);
			fseek(pFileStream, 0, SEEK_SET);

			// 파일을 메모리로 복사
			m_szBuffer = (WCHAR*)malloc(sizeof(char) * m_dwFileSize);
			if (m_szBuffer == nullptr)
			{
				fclose(pFileStream);
				return false;
			}

			m_szOrigin = m_szBuffer;
			fread(m_szBuffer, sizeof(char) * m_dwFileSize, 1, pFileStream);

			// BOM 체크 (UTF-16 LE)
			if (*m_szBuffer != 0xfeff)
			{
				free(m_szOrigin);
				fclose(pFileStream);
				return false;
			}

			// BOM Code 건너뛰기
			m_szBuffer = m_szBuffer + 1;
			fclose(pFileStream);

			return true;
		}

		// 스킵 문자 처리
		void SkipNoneCommand()
		{
			while (1)
			{
				WCHAR wch = *m_szBuffer;

				// 스킵 문자를 만났다.
				if (
					wch != 0x0020 && wch != 0x000a && wch != 0x0008 &&
					wch != 0x0009 && wch != 0x000a && wch != 0x000d &&
					wch != L'\t' && wch != L'\n' && wch != L',' &&
					wch != L'{' && wch != L'}' && wch != ':'
					)
				{
					// 주석 문자 처리
					if (wch == L'/' && *(m_szBuffer + 1) == L'/')
					{
						// 개행 문자를 발견할 때까지 이동한다.
						while (*m_szBuffer != '\n')
						{
							m_szBuffer++;
						}

						// 개행 문자 이후 스킵 문자 처리(개행 이후 다시 주석 일 수도 있음)
						SkipNoneCommand();
					}
					break;
				}
				// 스킵 문자가 아니면 다음 메모리로 이동
				else
				{
					m_szBuffer++;
				}
			}
		}

		// 문자 얻기
		bool GetNextWord(WCHAR** ppDest, int* npLength)
		{
			// 스킵 문자 처리
			SkipNoneCommand();

			int iLength = 0;
			WCHAR* szWordEntry = m_szBuffer;

			// 스킵 문자를 발견할 때까지 계속 이동
			while (true)
			{
				WCHAR wch = *m_szBuffer;

				// 스킵 문자가 나오면 종료
				if (wch == L',' || wch == L'.' ||
					wch == 0x0020 || wch == 0x0008 || wch == 0x0009 ||
					wch == 0x000a || wch == 0x000d ||
					wch == L'\t' || wch == L'\n' ||
					wch == L'{' || wch == L'}' || wch == L':')
				{
					break;
				}

				// 스킵 문자가 아닌 경우 포인터 및 길이 갱신
				iLength++;
				m_szBuffer++;
			}

			// 한 번이라도 찾은 문자가 존재할 경우에만 값을 리턴해준다.
			if (iLength > 0)
			{
				*ppDest = szWordEntry;
				*npLength = iLength;

				return true;
			}

			return false;
		}

		bool GetNextString(WCHAR** ppDest, int* npLength)
		{
			// 스킵 문자 처리
			SkipNoneCommand();

			bool	bFlag = false;
			int		iLength = 0;
			WCHAR* szStrEntry = m_szBuffer;

			// 문자열의 시작을 찾을 때 까지 반복
			while ( true )
			{
				WCHAR wch = *m_szBuffer;

				if (wch == L'"')
				{
					// 문자열의 시작을 찾았다.
					if (!bFlag)
					{
						bFlag = true;
					}
					// 문자열의 끝을 만났다.
					else
					{
						break;
					}
				}

				// 스킵 문자가 아닌 경우 포인터 및 길이 갱신
				iLength++;
				m_szBuffer++;
			}

			// 한 번이라도 찾은 문자가 존재할 경우에만 값을 리턴해준다.
			if (iLength > 0)
			{
				*ppDest = szStrEntry + 1;	// " 이후 포인터 반환
				*npLength = iLength - 1;	// " 이전까지의 길이 반환

				return true;
			}

			return false;
		}

		// 텍스트와 대응되는 값 얻기
		template <typename T>
		BOOL GetValue(const WCHAR* szFindName, T* pDest, const int nDestLength = 0)
		{
			WCHAR	szWord[eWORD_MAX];
			WCHAR*	szBuffer;
			int		iLength;

			// 문자를 찾을 때 까지 반복
			while (GetNextWord(&szBuffer, &iLength) == true)
			{
				// 어떤 문자를 찾았다.
				memset(szWord, 0, sizeof(WCHAR) * eWORD_MAX);
				memcpy_s(szWord, sizeof(WCHAR) * eWORD_MAX, szBuffer, iLength * sizeof(WCHAR));
				szWord[iLength] = '\0';

				// 인자값과 문자가 같은 값인지 체크
				if (wcscmp(szFindName, szWord) == 0)
				{
					// 다음 문자로 이동
					if (GetNextWord(&szBuffer, &iLength) == true)
					{
						memset(szWord, 0, sizeof(WCHAR) * eWORD_MAX);
						memcpy_s(szWord, sizeof(WCHAR) * eWORD_MAX, szBuffer, iLength * sizeof(WCHAR));

						if (wcscmp(L"=", szWord) == 0)
						{
							// 길이가 있으면 문자열을 얻어다주면된다.
							if (nDestLength > 0)
							{
								// 다음 문자열로 이동
								if (true == GetNextString(&szBuffer, &iLength))
								{
									memset(szWord, 0, sizeof(WCHAR) * eWORD_MAX);
									memcpy_s(szWord, sizeof(WCHAR) * eWORD_MAX, szBuffer, iLength * sizeof(WCHAR));
									wcscpy_s((WCHAR*)pDest, nDestLength, szWord);

									return true;
								}
							}
							// 길이가 없으면 그냥 대응되는 값
							else
							{
								// 다음 문자로 이동
								if (true == GetNextWord(&szBuffer, &iLength))
								{
									memset(szWord, 0, sizeof(WCHAR) * eWORD_MAX);
									memcpy_s(szWord, sizeof(WCHAR) * eWORD_MAX, szBuffer, iLength * sizeof(WCHAR));
									*pDest = _wtoi(szWord);

									return true;
								}
							}
						}
						return false;
					}
					return false;
				}
			}
			return false;
		}

	private:
		DWORD m_dwFileSize = 0;
		wchar_t* m_szBuffer = nullptr;
		wchar_t* m_szOrigin = nullptr;
	};
}
