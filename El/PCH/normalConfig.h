// Configuration parameters for internal release / debug version

#ifndef _ENABLE_PERFLOG
  #ifdef _WIN32
    #if _PROFILE || _DEBUG
      // Profile configuration - enable more counters
      #define _ENABLE_PERFLOG 2
    #else
      // Basic configuration - enable basic counters
      #define _ENABLE_PERFLOG 1
    #endif
  #else
    #define _ENABLE_PERFLOG           0
  #endif
#endif
  #define _ENABLE_PATCHING					1
  #define _ENABLE_INJECTING					1

