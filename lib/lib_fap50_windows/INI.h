#ifdef _WIN32
#pragma once
#include "imd_framework.h"


class CIniReader  
{
	deque<CString> m_section_data;
	deque<CString> m_sections;

	CString m_strSection;
	long m_lRetValue;

	CString m_strFileName;
	CString folder;

public:
	CIniReader(LPCTSTR file_name=L"/config.ini");
	~CIniReader();

	deque<CString>& get_section_data(LPCTSTR strSection);
	deque<CString>& get_section_names();
	
	void set_file(LPCTSTR strINIFile);
	BOOL is_section_exist(LPCTSTR strSection);
	long set_value(LPCTSTR strValue, LPCTSTR strKey, LPCTSTR strSection);
	LPCTSTR get_value(LPCTSTR strKey, LPCTSTR strSection);
};

#endif
