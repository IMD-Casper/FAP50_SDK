#pragma once

#include <atlimage.h>

class C_IMG
{
public:
	CString m_OpenFilePath;
	CString m_SaveFilePath;

	PBYTE m_byRaw;
	WORD x, y;
	DWORD m_dwRawSize;

public:
	C_IMG()
	{
		Init();
	}

	virtual ~C_IMG()
	{
		if (m_byRaw != NULL)
		{
			delete[] m_byRaw;
			m_byRaw = NULL;
		}
	}

	inline virtual void Init()
	{
		m_byRaw = NULL;
		x = 0;
		y = 0;
		m_dwRawSize = 0;
	}

	inline virtual void SetXY(WORD _x, WORD _y)
	{
		x = _x;
		y = _y;
		m_dwRawSize = x*y;
	}

	inline virtual DWORD GetSize()
	{
		return m_dwRawSize;
	}

	inline virtual PBYTE GetRaw()
	{
		return m_byRaw;
	}

	virtual void SetBuf(PBYTE buf)
	{
		//DWORD size = (m_dwRawSize != 0) ? m_dwRawSize : 25600;
		//m_dwRawSize = size;

		if (m_dwRawSize == 0)
			return;

		if (buf == NULL)
			return;

		if (m_byRaw == NULL)
			m_byRaw = new BYTE[m_dwRawSize];

		CopyMemory(m_byRaw, buf, m_dwRawSize);
	}

	BOOL OpenBMP(LPCTSTR lpszFileName)
	{
		BOOL ret = FALSE;
		{
			CImage img;
			if (img.Load(m_OpenFilePath) != S_OK)
				goto exit;

			x = img.GetWidth();
			y = img.GetHeight();
			m_dwRawSize = x*y;

			if (m_byRaw != NULL)
			{
				delete[] m_byRaw;
				m_byRaw = NULL;
			}
			m_byRaw = new BYTE[m_dwRawSize];
			ZeroMemory(m_byRaw, m_dwRawSize);

			//COLORREF color;
			//DWORD dwPixel = 0;
			//for (int i = 0; i < y; i++)
			//	for (int j = 0; j < x; j++)
			//	{
			//		color = img.GetPixel(j, i);
			//		m_byRaw[dwPixel++] = color & 0xFF;
			//	}
			LPBYTE ps = m_byRaw;
			for (int k = 0; k < y; k++)
			{
				memcpy(ps, img.GetPixelAddress(0, k), x);
				ps += x;
			}
			ret = TRUE;
		}
	exit:
		return ret;
	}

	BOOL OpenPGM(LPCTSTR lpszFileName)
	{
		BOOL ret = FALSE;
		CStdioFile file(m_OpenFilePath, CFile::modeRead);
		CString str;
	
		file.ReadString(str);
		if (str != L"P5")
			goto exit;
	{
		while (1)
		{
			file.ReadString(str);

			if (str.GetBuffer()[0] == L'#')
				continue;

			int curPos = 0;
			CString resToken = str.Tokenize(_T("% #"), curPos);
			if (resToken != _T(""))
			{
				x = _tstoi(resToken);
				resToken = str.Tokenize(_T("% #"), curPos);

				y = _tstoi(resToken);
				resToken = str.Tokenize(_T("% #"), curPos);

				m_dwRawSize = x*y;

				break;
			}
		}

		//DWORD nPos = file.GetPosition();
		file.Close();

		if (m_byRaw != NULL)
		{
			delete[] m_byRaw;
			m_byRaw = NULL;
		}

		m_byRaw = new BYTE[m_dwRawSize];
		ZeroMemory(m_byRaw, m_dwRawSize);

		CFile f(m_OpenFilePath, CFile::modeRead);
		f.Read(m_byRaw, UINT(f.GetLength() - m_dwRawSize));
		int n = f.Read(m_byRaw, m_dwRawSize);
		ret = TRUE;
	}
	exit:
		return ret;
	}

	virtual void Open(LPCTSTR lpszFileName)
	{
		if (lpszFileName != NULL)
			m_OpenFilePath = lpszFileName;
		else
			goto exit;

		if (OpenPGM(m_OpenFilePath))
			goto exit;

		if (OpenBMP(m_OpenFilePath))
			goto exit;

	exit:
		return;
	}

	virtual BOOL Save(LPCTSTR lpszFileName = NULL)
	{
		return TRUE;
	}
};

class C_PGM : public C_IMG
{
public:
	virtual BOOL Save(LPCTSTR lpszFileName = NULL)
	{
		BOOL ret = FALSE;

		if (lpszFileName == NULL)
		{
			CString tmp = m_OpenFilePath;
			PathRemoveExtension((LPTSTR)(LPCTSTR)tmp);
			m_SaveFilePath.Format(L"%s.pgm", tmp);
		}
		else
			m_SaveFilePath = lpszFileName;

		{
			CStdioFile file(m_SaveFilePath, CFile::modeWrite | CFile::modeCreate);
			file.WriteString(L"P5 \n");
			file.WriteString(L"#Create by class C_PGM \n");

			CString str;
			str.Format(L"%d %d \n", x, y);
			file.WriteString(str);
			file.WriteString(L"255");
			file.Close();

			CFile f(m_SaveFilePath, CFile::modeWrite);
			f.SeekToEnd();
			BYTE _0x0D = 0x0d;
			f.Write(&_0x0D, 1);
			f.Write(m_byRaw, m_dwRawSize);
			ret = TRUE;
		}
		return ret;
	}
};

class C_BMP : public C_IMG
{
public:
	virtual BOOL Save(LPCTSTR lpszFileName = NULL)
	{
		BOOL ret = FALSE;

		if (lpszFileName == NULL)
		{
			CString tmp = m_OpenFilePath;
			PathRemoveExtension((LPTSTR)(LPCTSTR)tmp);
			m_SaveFilePath.Format(L"%s.bmp", tmp);
		}
		else
			m_SaveFilePath = lpszFileName;

		if (m_byRaw == NULL)
			goto exit;

		{
			CImage img;
			img.Create(x, y, 24 /* bpp */, 0 /* No alpha channel */);

			//DWORD dwPixel = 0;
			//for (int i = 0; i < y; i++)
			//{
			//	for (int j = 0; j < x; j++)
			//	{
			//		BYTE r = m_byRaw[dwPixel++];
			//		img.SetPixel(j, i, RGB(r, r, r));
			//	}
			//}
			LPBYTE ps = m_byRaw;
			for (int k = 0; k < y; k++)
			{
				memcpy(img.GetPixelAddress(0, k), ps, x);
				ps += x;
			}

			img.Save(m_SaveFilePath, Gdiplus::ImageFormatBMP);
		}

		ret = TRUE;

	exit:
		return ret;
	}
};

class C_HEX : public C_IMG
{
public:
	virtual BOOL Save(LPCTSTR lpszFileName = NULL)
	{
		BOOL ret = FALSE;

		if (lpszFileName == NULL)
		{
			CString tmp = m_OpenFilePath;
			PathRemoveExtension((LPTSTR)(LPCTSTR)tmp);
			m_SaveFilePath.Format(L"%s.hex", tmp);
		}
		else
			m_SaveFilePath = lpszFileName;

		{
			CStdioFile file(m_SaveFilePath, CFile::modeWrite | CFile::modeCreate);
			CString str;
			for (DWORD i = 0; i < m_dwRawSize; i++)
			{
				str.Format(L"%X\n", m_byRaw[i]);
				file.WriteString(str);
			}

			file.Close();
			ret = TRUE;
		}

		return ret;
	}
};