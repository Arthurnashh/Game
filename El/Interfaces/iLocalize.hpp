#ifdef _MSC_VER
#pragma once
#endif

#ifndef _I_LOCALIZE_HPP
#define _I_LOCALIZE_HPP

#include <Es/Strings/rString.hpp>

//! class of callback functions
class LocalizeStringFunctions
{
public:
	LocalizeStringFunctions() {}
	virtual ~LocalizeStringFunctions() {}

	//! callback function to load string from stringtable
	virtual RString LocalizeString(const char *str) {return str;}
	//! if function start with '@', use LocalizeString to remaining string
	RString Localize(const char *str)
	{
		if (*str == '@') return LocalizeString(str + 1);
		return str;
	}
};

#endif
