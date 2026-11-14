#include <El/elementpch.hpp>

#include "localizeStringtable.hpp"
#include "stringtable.hpp"

RString StringtableFunctions::LocalizeString(const char *str)
{
	return ::LocalizeString(str);
}

