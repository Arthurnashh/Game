// to change version number
// change following lines

// #else
  // DayZ version
  #define APP_VERSION_MAJOR 0
  #define APP_VERSION_MINOR 45
  #define APP_VERSION_TEXT "0.45"
  #define APP_VERSION_ADDITION ""
// #endif

#include "../buildNo.h"

// end of changes
// do not change following lines

#define STRINGIFY_X(x)            #x
#define STRINGIFY(x)              STRINGIFY_X(x)

#define GET_VER_X(a, b, c, d)     a.b.c.d
#define GET_VER(a, b, c, d)       GET_VER_X(a, b, c, d)

// use APP_VERSION_NUM to retrieve version number as integer

#define APP_VERSION_NUM	APP_VERSION_MAJOR*100+APP_VERSION_MINOR

// Resource compiler cannot evaluate expressions, but preprocessor can
// we use this to parse the number into two parts (hi and low words)
// we can parse numbers up to 999999

#if BUILD_NO>999999
#error BUILD_NO is too high - parsing needs to be extended
#endif

#if BUILD_NO%10==0
# define VERSION_DIG_0 0
#elif BUILD_NO%10==1
# define VERSION_DIG_0 1
#elif BUILD_NO%10==2
# define VERSION_DIG_0 2
#elif BUILD_NO%10==3
# define VERSION_DIG_0 3
#elif BUILD_NO%10==4
# define VERSION_DIG_0 4
#elif BUILD_NO%10==5
# define VERSION_DIG_0 5
#elif BUILD_NO%10==6
# define VERSION_DIG_0 6
#elif BUILD_NO%10==7
# define VERSION_DIG_0 7
#elif BUILD_NO%10==8
# define VERSION_DIG_0 8
#elif BUILD_NO%10==9
# define VERSION_DIG_0 9
#endif

#if (BUILD_NO/10)%10==0
# define VERSION_DIG_1 0
#elif (BUILD_NO/10)%10==1
# define VERSION_DIG_1 1
#elif (BUILD_NO/10)%10==2
# define VERSION_DIG_1 2
#elif (BUILD_NO/10)%10==3
# define VERSION_DIG_1 3
#elif (BUILD_NO/10)%10==4
# define VERSION_DIG_1 4
#elif (BUILD_NO/10)%10==5
# define VERSION_DIG_1 5
#elif (BUILD_NO/10)%10==6
# define VERSION_DIG_1 6
#elif (BUILD_NO/10)%10==7
# define VERSION_DIG_1 7
#elif (BUILD_NO/10)%10==8
# define VERSION_DIG_1 8
#elif (BUILD_NO/10)%10==9
# define VERSION_DIG_1 9
#endif

#if (BUILD_NO/100)%10==0
# define VERSION_DIG_2 0
#elif (BUILD_NO/100)%10==1
# define VERSION_DIG_2 1
#elif (BUILD_NO/100)%10==2
# define VERSION_DIG_2 2
#elif (BUILD_NO/100)%10==3
# define VERSION_DIG_2 3
#elif (BUILD_NO/100)%10==4
# define VERSION_DIG_2 4
#elif (BUILD_NO/100)%10==5
# define VERSION_DIG_2 5
#elif (BUILD_NO/100)%10==6
# define VERSION_DIG_2 6
#elif (BUILD_NO/100)%10==7
# define VERSION_DIG_2 7
#elif (BUILD_NO/100)%10==8
# define VERSION_DIG_2 8
#elif (BUILD_NO/100)%10==9
# define VERSION_DIG_2 9
#endif

#if (BUILD_NO/1000)%10==0
# define VERSION_DIG_3 0
#elif (BUILD_NO/1000)%10==1
# define VERSION_DIG_3 1
#elif (BUILD_NO/1000)%10==2
# define VERSION_DIG_3 2
#elif (BUILD_NO/1000)%10==3
# define VERSION_DIG_3 3
#elif (BUILD_NO/1000)%10==4
# define VERSION_DIG_3 4
#elif (BUILD_NO/1000)%10==5
# define VERSION_DIG_3 5
#elif (BUILD_NO/1000)%10==6
# define VERSION_DIG_3 6
#elif (BUILD_NO/1000)%10==7
# define VERSION_DIG_3 7
#elif (BUILD_NO/1000)%10==8
# define VERSION_DIG_3 8
#elif (BUILD_NO/1000)%10==9
# define VERSION_DIG_3 9
#endif

#if (BUILD_NO/10000)%10==0
# define VERSION_DIG_4 0
#elif (BUILD_NO/10000)%10==1
# define VERSION_DIG_4 1
#elif (BUILD_NO/10000)%10==2
# define VERSION_DIG_4 2
#elif (BUILD_NO/10000)%10==3
# define VERSION_DIG_4 3
#elif (BUILD_NO/10000)%10==4
# define VERSION_DIG_4 4
#elif (BUILD_NO/10000)%10==5
# define VERSION_DIG_4 5
#elif (BUILD_NO/10000)%10==6
# define VERSION_DIG_4 6
#elif (BUILD_NO/10000)%10==7
# define VERSION_DIG_4 7
#elif (BUILD_NO/10000)%10==8
# define VERSION_DIG_4 8
#elif (BUILD_NO/10000)%10==9
# define VERSION_DIG_4 9
#endif


#if (BUILD_NO/100000)%10==0
# define VERSION_DIG_5 0
#elif (BUILD_NO/100000)%10==1
# define VERSION_DIG_5 1
#elif (BUILD_NO/100000)%10==2
# define VERSION_DIG_5 2
#elif (BUILD_NO/100000)%10==3
# define VERSION_DIG_5 3
#elif (BUILD_NO/100000)%10==4
# define VERSION_DIG_5 4
#elif (BUILD_NO/100000)%10==5
# define VERSION_DIG_5 5
#elif (BUILD_NO/100000)%10==6
# define VERSION_DIG_5 6
#elif (BUILD_NO/100000)%10==7
# define VERSION_DIG_5 7
#elif (BUILD_NO/100000)%10==8
# define VERSION_DIG_5 8
#elif (BUILD_NO/100000)%10==9
# define VERSION_DIG_5 9
#endif

#define CAT_VER_DO(a,b,c) a##b##c
#define CAT_VER(a,b,c) CAT_VER_DO(a,b,c)

#define BUILD_NO_SPLIT_HI CAT_VER(VERSION_DIG_5,VERSION_DIG_4,VERSION_DIG_3)
#define BUILD_NO_SPLIT_LO CAT_VER(VERSION_DIG_2,VERSION_DIG_1,VERSION_DIG_0)

#define APPVER          APP_VERSION_MAJOR,APP_VERSION_MINOR,BUILD_NO_SPLIT_HI,BUILD_NO_SPLIT_LO
#define FILEVER					STRINGIFY(GET_VER(APP_VERSION_MAJOR,APP_VERSION_MINOR,0,BUILD_NO))
#define PRODUCTVER			STRINGIFY(GET_VER(APP_VERSION_MAJOR,APP_VERSION_MINOR,0,BUILD_NO))

#define STRFILEVER			APP_VERSION_TEXT "\0"
#define STRPRODUCTVER		PRODUCTVER "\0"

#define PRODUCTNAME			APP_NAME "\0"
#define INTERNALNAME		APP_NAME_SHORT "\0"

#define APPICON_RESOURCE APPICON                 ICON    DISCARDABLE ICONNAME
