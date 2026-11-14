#ifdef _MSC_VER
#pragma once
#endif

#ifndef _LOCALIZE_STRINGTABLE_HPP
#define _LOCALIZE_STRINGTABLE_HPP

#include <El/Interfaces/iLocalize.hpp>

//! class of callback functions
class StringtableFunctions : public LocalizeStringFunctions
{
public:
	//! callback function to load string from stringtable
	virtual RString LocalizeString(const char *str);
};

#endif
