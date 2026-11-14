#pragma once


#define WIN32_LEAN_AND_MEAN

// The preprocessor COMPILING_SIMPLE3D_EMBEDDED shall be defined when source
// are directly added to the vcproj (instead of linking a static or dyn. library.
#ifdef WIN32
#	ifdef SIMPLE3D_EXPORTS
#		define SIMPLE3D_EXPORT _declspec(dllexport)
#	elif COMPILING_SIMPLE3D_EMBEDDED
#		define SIMPLE3D_EXPORT 
#	else
#		define SIMPLE3D_EXPORT _declspec(dllimport) 
#	endif
#
#	pragma warning (disable  : 4251 )
#	include <windows.h>
#elif defined(__linux)
#   define SIMPLE3D_EXPORT
#endif