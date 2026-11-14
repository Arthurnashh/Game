#include <Es/essencepch.hpp>
#include "appFrame.hpp"

#if _MSC_VER && !defined INIT_SEG_COMPILER
	#pragma warning(disable:4074)
	#pragma init_seg(compiler)
	#define INIT_SEG_COMPILER
#endif

static AppFrameFunctions GAppFrameFunctions INIT_PRIORITY_URGENT;
AppFrameFunctions *CurrentAppFrameFunctions = &GAppFrameFunctions;
