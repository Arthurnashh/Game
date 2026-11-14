/*

  This header enables to each module define own startup procedure

  Usage:
  
	1) Define static procedure, which must be called on application startup
	  
		example: static void OnStartup() 
				  {
				  //enter code here
				  }

    2) Assign name of startup procedure to the one of following macros that defines
		startup priority 
		 ON_STARTUP_PRIORITY_USERMEMORYINIT // for user memory managers
		 ON_STARTUP_PRIORITY_VERYHIGH 
		 ON_STARTUP_PRIORITY_HIGH 
		 ON_STARTUP_PRIORITY_MEDIUM
		 ON_STARTUP_PRIORITY_NORMAL	//all static constructors are called with this priority
		 ON_STARTUP_PRIORITY_LOW	//called after all statics objects are inicialized
		 ON_STARTUP_PRIORITY_VERYLOW	

		example: #define ON_STARTUP_PRIORITY_MEDIUM OnStartup


	3) Include this header into your source code. Do not include into another 
		headers (.h) files, only into .c or .cpp files.

		#include <es/common/startup.h>

  You can define startup function for each priority repeating steps 1)-2) You cannot
  define two startup functions for one priority in single module. Only this way is possible:

	  static void OnStartup()
	  {
	   OnStartup1();
	   OnStartup2();
	  }
	  #define ON_STARTUP_PRIORITY_MEDIUM OnStartup

  To define OnExit() procedure, use atexit function

	  static void OnExit()
	  {
		//exit code
	  }
  
	  static void OnStartup()
	  {
		//startup code
		atexit(OnExit);
	  }
	  #define ON_STARTUP_PRIORITY_MEDIUM OnStartup

  */



#if defined(__GNUC__) || defined(_STARTUP_INIT_PRIORITY_)

#define DeclareStartupPriorityClass(fn,n) \
class __Startup##n \
{\
public:\
  __Startup##n() {fn();}\
};\
static __Startup##n __startup_##n __attribute__ ((init_priority(n)));\

#ifdef ON_STARTUP_PRIORITY_USERMEMORYINIT 
DeclareStartupPriorityClass(ON_STARTUP_PRIORITY_USERMEMORYINIT,7000)
#endif

#ifdef ON_STARTUP_PRIORITY_VERYHIGH
DeclareStartupPriorityClass(ON_STARTUP_PRIORITY_VERYHIGH,8000)
#endif

#ifdef ON_STARTUP_PRIORITY_HIGH
DeclareStartupPriorityClass(ON_STARTUP_PRIORITY_HIGH,9000)
#endif

#ifdef ON_STARTUP_PRIORITY_MEDIUM
DeclareStartupPriorityClass(ON_STARTUP_PRIORITY_MEDIUM,9500)
#endif

#ifdef ON_STARTUP_PRIORITY_NORMAL
DeclareStartupPriorityClass(ON_STARTUP_PRIORITY_NORMAL,10000)
#endif

#ifdef ON_STARTUP_PRIORITY_LOW
DeclareStartupPriorityClass(ON_STARTUP_PRIORITY_LOW,20000)
#endif

#ifdef ON_STARTUP_PRIORITY_VERYLOW
DeclareStartupPriorityClass(ON_STARTUP_PRIORITY_VERYLOW,30000)
#endif

#elif  _MSC_VER > 1000

#ifdef ON_STARTUP_PRIORITY_USERMEMORYINIT 
#pragma data_seg(".CRT$XCB")
static void *___startupXCB[] = {ON_STARTUP_PRIORITY_USERMEMORYINIT};
#pragma data_seg()
#endif

#ifdef ON_STARTUP_PRIORITY_VERYHIGH
#pragma data_seg(".CRT$XCE")
static void *___startupXCE[] = {ON_STARTUP_PRIORITY_VERYHIGH};
#pragma data_seg()
#endif

#ifdef ON_STARTUP_PRIORITY_HIGH
#pragma data_seg(".CRT$XCI")
static void *___startupXCI[] = {ON_STARTUP_PRIORITY_HIGH};
#pragma data_seg()
#endif

#ifdef ON_STARTUP_PRIORITY_MEDIUM
#pragma data_seg(".CRT$XCO")
static void *___startupXCO[] = {ON_STARTUP_PRIORITY_MEDIUM};
#pragma data_seg()
#endif

#ifdef ON_STARTUP_PRIORITY_NORMAL
#pragma data_seg(".CRT$XCU")
static void *___startupXCU[] = {ON_STARTUP_PRIORITY_NORMAL};
#pragma data_seg()
#endif

#ifdef ON_STARTUP_PRIORITY_LOW
#pragma data_seg(".CRT$XCY")
static void *___startupXCY[] = {ON_STARTUP_PRIORITY_LOW};
#pragma data_seg()
#endif

#ifdef ON_STARTUP_PRIORITY_VERYLOW
#pragma data_seg(".CRT$XCYY")
static void *___startupXCYY[] = {ON_STARTUP_PRIORITY_VERYLOW};
#pragma data_seg()
#endif


#else
#pragma message("Warning: Startup functions is disabled!")
#endif
