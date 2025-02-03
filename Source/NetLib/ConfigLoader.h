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

			// ���� ������ ���
			fseek(pFileStream, 0, SEEK_END);
			m_dwFileSize = ftell(pFileStream);
			fseek(pFileStream, 0, SEEK_SET);

			// ������ �޸𸮷� ����
			m_szBuffer = (WCHAR*)malloc(sizeof(char) * m_dwFileSize);
			if (m_szBuffer == nullptr)
			{
				fclose(pFileStream);
				return false;
			}

			m_szOrigin = m_szBuffer;
			fread(m_szBuffer, sizeof(char) * m_dwFileSize, 1, pFileStream);

			// BOM üũ (UTF-16 LE)
			if (*m_szBuffer != 0xfeff)
			{
				free(m_szOrigin);
				fclose(pFileStream);
				return false;
			}

			// BOM Code �ǳʶٱ�
			m_szBuffer = m_szBuffer + 1;
			fclose(pFileStream);

			return true;
		}

		// ��ŵ ���� ó��
		void SkipNoneCommand()
		{
			while (1)
			{
				WCHAR wch = *m_szBuffer;

				// ��ŵ ���ڸ� ������.
				if (
					wch != 0x0020 && wch != 0x000a && wch != 0x0008 &&
					wch != 0x0009 && wch != 0x000a && wch != 0x000d &&
					wch != L'\t' && wch != L'\n' && wch != L',' &&
					wch != L'{' && wch != L'}' && wch != ':'
					)
				{
					// �ּ� ���� ó��
					if (wch == L'/' && *(m_szBuffer + 1) == L'/')
					{
						// ���� ���ڸ� �߰��� ������ �̵��Ѵ�.
						while (*m_szBuffer != '\n')
						{
							m_szBuffer++;
						}

						// ���� ���� ���� ��ŵ ���� ó��(���� ���� �ٽ� �ּ� �� ���� ����)
						SkipNoneCommand();
					}
					break;
				}
				// ��ŵ ���ڰ� �ƴϸ� ���� �޸𸮷� �̵�
				else
				{
					m_szBuffer++;
				}
			}
		}

		// ���� ���
		bool GetNextWord(WCHAR** ppDest, int* npLength)
		{
			// ��ŵ ���� ó��
			SkipNoneCommand();

			int iLength = 0;
			WCHAR* szWordEntry = m_szBuffer;

			// ��ŵ ���ڸ� �߰��� ������ ��� �̵�
			while (true)
			{
				WCHAR wch = *m_szBuffer;

				// ��ŵ ���ڰ� ������ ����
				if (wch == L',' || wch == L'.' ||
					wch == 0x0020 || wch == 0x0008 || wch == 0x0009 ||
					wch == 0x000a || wch == 0x000d ||
					wch == L'\t' || wch == L'\n' ||
					wch == L'{' || wch == L'}' || wch == L':')
				{
					break;
				}

				// ��ŵ ���ڰ� �ƴ� ��� ������ �� ���� ����
				iLength++;
				m_szBuffer++;
			}

			// �� ���̶� ã�� ���ڰ� ������ ��쿡�� ���� �������ش�.
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
			// ��ŵ ���� ó��
			SkipNoneCommand();

			bool	bFlag = false;
			int		iLength = 0;
			WCHAR* szStrEntry = m_szBuffer;

			// ���ڿ��� ������ ã�� �� ���� �ݺ�
			while ( true )
			{
				WCHAR wch = *m_szBuffer;

				if (wch == L'"')
				{
					// ���ڿ��� ������ ã�Ҵ�.
					if (!bFlag)
					{
						bFlag = true;
					}
					// ���ڿ��� ���� ������.
					else
					{
						break;
					}
				}

				// ��ŵ ���ڰ� �ƴ� ��� ������ �� ���� ����
				iLength++;
				m_szBuffer++;
			}

			// �� ���̶� ã�� ���ڰ� ������ ��쿡�� ���� �������ش�.
			if (iLength > 0)
			{
				*ppDest = szStrEntry + 1;	// " ���� ������ ��ȯ
				*npLength = iLength - 1;	// " ���������� ���� ��ȯ

				return true;
			}

			return false;
		}

		// �ؽ�Ʈ�� �����Ǵ� �� ���
		template <typename T>
		BOOL GetValue(const WCHAR* szFindName, T* pDest, const int nDestLength = 0)
		{
			WCHAR	szWord[eWORD_MAX];
			WCHAR*	szBuffer;
			int		iLength;

			// ���ڸ� ã�� �� ���� �ݺ�
			while (GetNextWord(&szBuffer, &iLength) == true)
			{
				// � ���ڸ� ã�Ҵ�.
				memset(szWord, 0, sizeof(WCHAR) * eWORD_MAX);
				memcpy_s(szWord, sizeof(WCHAR) * eWORD_MAX, szBuffer, iLength * sizeof(WCHAR));
				szWord[iLength] = '\0';

				// ���ڰ��� ���ڰ� ���� ������ üũ
				if (wcscmp(szFindName, szWord) == 0)
				{
					// ���� ���ڷ� �̵�
					if (GetNextWord(&szBuffer, &iLength) == true)
					{
						memset(szWord, 0, sizeof(WCHAR) * eWORD_MAX);
						memcpy_s(szWord, sizeof(WCHAR) * eWORD_MAX, szBuffer, iLength * sizeof(WCHAR));

						if (wcscmp(L"=", szWord) == 0)
						{
							// ���̰� ������ ���ڿ��� �����ָ�ȴ�.
							if (nDestLength > 0)
							{
								// ���� ���ڿ��� �̵�
								if (true == GetNextString(&szBuffer, &iLength))
								{
									memset(szWord, 0, sizeof(WCHAR) * eWORD_MAX);
									memcpy_s(szWord, sizeof(WCHAR) * eWORD_MAX, szBuffer, iLength * sizeof(WCHAR));
									wcscpy_s((WCHAR*)pDest, nDestLength, szWord);

									return true;
								}
							}
							// ���̰� ������ �׳� �����Ǵ� ��
							else
							{
								// ���� ���ڷ� �̵�
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
