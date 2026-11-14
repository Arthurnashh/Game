#include "precomp.h"

#include "Simul/Base/StringToWString.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
namespace simul
{
	namespace base
	{
		const std::wstring &StringToWString(const std::string &text)
		{
			size_t origsize = strlen(text.c_str()) + 1;
			const size_t newsize = origsize;
			//size_t convertedChars = 0;
			wchar_t *wcstring=new wchar_t[newsize];
		//	mbstowcs_s(&convertedChars, wcstring, origsize, text.c_str(), _TRUNCATE);
			mbstowcs(wcstring,text.c_str(),origsize);
			static std::wstring str;
			str=std::wstring(wcstring);
			delete [] wcstring;
			return str;
		}

		const std::string &WStringToString(const std::wstring &text)
		{
			size_t origsize = text.length()+ 1;
			const size_t newsize = origsize;
			//size_t convertedChars = 0;
			char *cstring=new char[newsize];
	//		wcstombs_s(&convertedChars, cstring, (size_t)origsize, text.c_str(), (size_t)newsize );
			wcstombs(cstring, text.c_str(), (size_t)newsize );

			static std::string str;
			str=std::string(cstring);
			delete [] cstring;
			return str;
		}
		void WStringToAsciiString(char *output,const std::wstring &text)
		{
			std::string str=WStringToString(text);
			strcpy(output,str.c_str());
		}
	}
}
