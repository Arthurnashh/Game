#ifdef _MSC_VER
#pragma once
#endif

#ifndef _STRINGTABLE_EXT_HPP
#define _STRINGTABLE_EXT_HPP

#include <El/Stringtable/stringtable.hpp>

#define STRING(x) extern int IDS_##x;
#include "stringids.hpp"
#undef STRING

#endif
