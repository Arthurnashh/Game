#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_PARAM_FILE_DECL_HPP
#define _EL_PARAM_FILE_DECL_HPP

#include <El/elementpch.hpp>

/*!
\file
Basic declarations necessary for using ParamFile and ParamEntry classes.
*/

class ParamEntry;
class ParamFile;
class ParamClass;

class ParamEntryVal;
class IParamArrayValue;

#if _MSC_VER<=1300 && defined(_MSC_VER)
// due to a bug in MSVC .NET 7.0 compiler we cannot use const ParamEntry & for argument passing
// we do not want to pass ParamEntryVal by value, as it requires copying, 
// which causes superfluous ParamEntry locking / unlocking
#define ParamEntryPar const ParamEntryVal &
#else
// passing reference to ParamEntry can be more efficient
// as it can be passed in register and is 4B only
#define ParamEntryPar const ParamEntry &
#endif

#endif
