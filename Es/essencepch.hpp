#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   essencepch.hpp
    @brief  Global pre-compiled headers for all "Essence" subprojects.

    Copyright &copy; 2001-2002 by BIStudio (www.bistudio.com)
    @author PE
    @since  21.11.2001
    @date   19.9.2002
*/

#ifndef _COMMONPCH_H
#define _COMMONPCH_H

//-------------------------------------------------------------------------
//  External include files:

#include "platform.hpp"
#include "Common/langExt.hpp"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>

//-------------------------------------------------------------------------
//  Internal include files:

#if _SUPER_RELEASE
#  define _ENABLE_REPORT  0
#else
#  define _ENABLE_REPORT  1
#endif

#include <Es/Memory/checkMem.hpp>

#ifdef NULL
#undef NULL
#endif
#define NULL 0

#endif
