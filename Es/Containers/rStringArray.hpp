#ifdef _MSC_VER
#pragma once
#endif

#ifndef __RSTRING_ARRAY_HPP
#define __RSTRING_ARRAY_HPP

#include <Es/Containers/array.hpp>
#include <Es/Strings/rString.hpp>

//! case insensitive traits that can be used for normal strings
struct FindArrayTraitsCI
{
	typedef const char *KeyType;
	static bool IsEqual(const char *a, const char *b)
	{
		return stricmp(a,b)==0;
	}
	static const char *GetKey(const RString &a) {return a;}
	static const char *GetKey(const RStringB &a) {return a;}
};

typedef FindArrayKey<RString,FindArrayTraitsCI> FindArrayRStringCI;
typedef FindArrayKey<RStringB,FindArrayTraitsCI> FindArrayRStringBCI;

#endif
