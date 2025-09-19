#include "stdafx.h"
#include "INI.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CIniReader::CIniReader(LPCTSTR file_name)
{
	// 取得當前執行檔路徑的緩衝區
	TCHAR szPath[MAX_PATH];
	::GetModuleFileName(NULL, szPath, MAX_PATH);
	PathRemoveFileSpec(szPath);
	folder = szPath;

	CString str = file_name;
	if (str.GetLength())
		set_file(file_name);
}

CIniReader::~CIniReader()
{
}

// Used to retrieve a value give the section and key
LPCTSTR CIniReader::get_value(LPCTSTR strKey, LPCTSTR strSection)
{
	static TCHAR ac_Result[1024];

	// Get the info from the .ini file	
	m_lRetValue = GetPrivateProfileString(strSection, strKey, L"", ac_Result, 1024, (LPCTSTR)m_strFileName);

	//CString strResult(ac_Result);
	return ac_Result;
}

// Used to add or set a key value pair to a section
long CIniReader::set_value(LPCTSTR strValue, LPCTSTR strKey, LPCTSTR strSection)
{
	m_lRetValue = WritePrivateProfileString (strSection, strKey, strValue, m_strFileName);
	return m_lRetValue;
}

// Used to find out if a given section exists
BOOL CIniReader::is_section_exist(LPCTSTR strSection)
{
	TCHAR ac_Result[100];	
	CString csAux;
	// Get the info from the .ini file	
	m_lRetValue = GetPrivateProfileString(strSection,NULL, L"", ac_Result, 90, (LPCTSTR)m_strFileName);
	// Return if we could retrieve any info from that section
	return (m_lRetValue > 0);
}

// Used to retrieve all of the  section names in the ini file
deque<CString>& CIniReader::get_section_names()  //returns collection of section names
{
	vector<TCHAR> ac_Result(10 * 1000 * 36);
	m_lRetValue = GetPrivateProfileSectionNames(&ac_Result[0], ac_Result.size(), (LPCTSTR)m_strFileName);
	
	CString strSectionName;
	m_sections.clear();
	for(int i=0; i<m_lRetValue; i++)
	{
		if(ac_Result[i] != '\0') 
		{
			strSectionName += ac_Result[i];
		}
		else 
		{
			if(strSectionName != "") 
				m_sections.push_back(strSectionName);
			strSectionName = "";
		}
	}

	return m_sections;
}

// Used to retrieve all key/value pairs of a given section.  
deque<CString>& CIniReader::get_section_data(LPCTSTR strSection)
{
	deque<TCHAR> ac_Result(10 * 1000 * 36);  //change size depending on needs
	m_lRetValue = GetPrivateProfileSection(strSection, &ac_Result[0], ac_Result.size(), (LPCTSTR)m_strFileName);

	CString strSectionData;
	m_section_data.clear();
	for(int i=0; i<m_lRetValue; i++)
	{
		if(ac_Result[i] != '\0') 
		{
			strSectionData += ac_Result[i];
		} 
		else 
		{
			if(strSectionData != "") 
				m_section_data.push_back(strSectionData);
			strSectionData = "";
		}
	}

	return m_section_data;
}

void CIniReader::set_file(LPCTSTR strINIFile)
{
	m_strFileName = folder + strINIFile;
}
