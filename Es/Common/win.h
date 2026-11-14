#ifndef _EL_WIN_H
#define _EL_WIN_H

#define WIN32_LEAN_AND_MEAN // no OLE or other complicated stuff

#if defined(_WIN32)

	#if defined _WINDOWS_ && !defined _MFC_VER
		#error Windows.h used not through Es/Common/win.h in non-MFC application.
	#endif

  #ifndef _WINDOWS_ // in MFC applications we cannot prevent windows.h to be included before
    #define _WIN32_WINNT 0x0501 // some WinXP features are used
    #define STRICT 1
    #include <windows.h>
    #if _GAMES_FOR_WINDOWS
      #include <Es/Memory/normalNew.hpp> // some GfW libraries override new/delete
      #include <winlive.h>
      #include <Es/Memory/debugNew.hpp>
    #endif
  #endif

  // various CPU intrinsics - interlocked operations, barriers, ...
  #if _MSC_VER
    #include <intrin.h>
  #endif
  
  // Platform SDK as of Visual Studio 2005 does not define interlocked operations as intrinsics for x86 platform
  #ifndef InterlockedIncrement
    #define InterlockedIncrement _InterlockedIncrement
  #endif
  #ifndef InterlockedDecrement
    #define InterlockedDecrement _InterlockedDecrement
  #endif
    
  #ifndef InterlockedExchange
    #define InterlockedExchange _InterlockedExchange
  #endif
  #ifndef InterlockedCompareExchange
    #define InterlockedCompareExchange _InterlockedCompareExchange
  #endif
  #ifndef InterlockedExchangeAdd
    #define InterlockedExchangeAdd _InterlockedExchangeAdd
  #endif


#else

  #undef Verify
  #define Verify( expr ) (expr)

#endif

// #undef SearchPath
// #undef LoadString
#undef DrawText
#undef GetObject
#undef GetMessage
#undef GetCurrentTime

#endif
