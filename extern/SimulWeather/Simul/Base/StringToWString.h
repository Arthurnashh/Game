#ifndef SIMUL_BASE_STRINGTOWSTRING_H
#define SIMUL_BASE_STRINGTOWSTRING_H
#include <string>
#include "Simul/Base/Export.h"
namespace simul
{
	namespace base
	{
		extern SIMUL_BASE_EXPORT const std::wstring &StringToWString(const std::string &text);
		extern SIMUL_BASE_EXPORT const std::string &WStringToString(const std::wstring &text);
		extern SIMUL_BASE_EXPORT void WStringToAsciiString(char *output,const std::wstring &text);
	}
}
#endif