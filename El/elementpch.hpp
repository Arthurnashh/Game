#ifdef _MSC_VER
#pragma once
#endif

#ifndef __EL_ELEMENTPCH_HPP
#define __EL_ELEMENTPCH_HPP

// precompiled header files - many files use something of Windows API
#include "PCH/ext_options.hpp"

// platform-dependent definitions
#include <Es/essencepch.hpp>

#include <Es/Types/enum_decl.hpp>

#ifndef _ENABLE_REPORT
#error _ENABLE_REPORT must be defined for Elements
/*
_ENABLE_REPORT is defined by Essence header, which are always used when using Element
*/
#endif

#if _SUPER_RELEASE
	#include "PCH/sReleaseConfig.h"
#elif _USE_AFX
	#include "PCH/afxConfig.h"
#else
	#include "PCH/normalConfig.h"
#endif

#include "PCH/stdIncludes.h"

// global support for array new/delete
/*
// init_seg user is default
#if _MSC_VER
	#pragma init_seg(user)
#endif
*/

void *ArrayNew( size_t size );
void *ArrayNew( size_t size, const char *file, int line );
void ArrayDelete( void *mem );


#define WIN32_LEAN_AND_MEAN // no OLE or other complicated stuff
#define STRICT 1


// user general include
#include "PCH/libIncludes.hpp"

typedef void AtAliveFunction();

// I_AM_ALIVE default (empty) implementation
class GlobalAliveInterface
{
  
public:
	static void Set(GlobalAliveInterface *functor);

  //! should be called periodically - typically game uses I_AM_ALIVE()
	virtual void Alive() {}
	//! register function that should be called requlary
	virtual void AtAlive(AtAliveFunction *function){}
	//! cancel function that should be called requlary
	virtual void CancelAtAlive(AtAliveFunction *function){}
};
void GlobalAlive();
void GlobalAtAlive(AtAliveFunction *function);
void GlobalCancelAtAlive(AtAliveFunction *function);

#define I_AM_ALIVE() GlobalAlive()

// enum declaration seems not to be supported by some compilers
#ifndef DECL_ENUM_LSERROR
#define DECL_ENUM_LSERROR
DECL_ENUM(LSError)
#endif

#ifdef NULL
#undef NULL
#endif
#define NULL 0

#endif

