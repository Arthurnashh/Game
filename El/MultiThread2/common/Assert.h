    #ifndef _BREDY_LIBS_BASIC_ASSERT_H_
#define _BREDY_LIBS_BASIC_ASSERT_H_
#include <assert.h>
#include <stdarg.h>
#include "CommonDefs.h"


#ifndef NDEBUG

void ImplementBreak(bool test, const char *expression, const char *filename, int linenumber, const char *description);
void ImplementDebugPrint(const char *pattern, va_list args);

#ifndef Assert
#define Assert(x) ImplementBreak(x,#x,__FILE__,__LINE__,0)
#endif
#define AssertDesc(x,desc) ImplementBreak(x,#x,__FILE__,__LINE__,desc)
#ifndef Verify
#define Verify(x) ImplementBreak(x,#x,__FILE__,__LINE__,0)
#endif
#define VerifyDesc(x,desc) ImplementBreak(x,#x,__FILE__,__LINE__,desc)
#define Error(desc) ImplementBreak(false,"General Fail",__FILE__,__LINE__,desc)
static inline void DebugPrint(const char *pattern, ...) 
{
  va_list va;
  va_start(va,pattern);
  ImplementDebugPrint(pattern, va);
  va_end(va);
}
#define DebugPrint0(str) DebugPrint(str);
#define DebugPrint1(str,a1) DebugPrint(str,a1);
#define DebugPrint2(str,a1,a2) DebugPrint(str,a1,a2);
#define DebugPrint3(str,a1,a2,a3) DebugPrint(str,a1,a2,a3);
#define DebugPrint4(str,a1,a2,a3,a4) DebugPrint(str,a1,a2,a3,a4);
#define DebugPrint5(str,a1,a2,a3,a4,a5) DebugPrint(str,a1,a2,a3,a4,a5);
#define DebugPrint6(str,a1,a2,a3,a4,a5,a6) DebugPrint(str,a1,a2,a3,a4,a5,a6);
#define DebugPrint7(str,a1,a2,a3,a4,a5,a6,a7) DebugPrint(str,a1,a2,a3,a4,a5,a6,a7);
#define DebugPrint8(str,a1,a2,a3,a4,a5,a6,a7,a8) DebugPrint(str,a1,a2,a3,a4,a5,a6,a7,a8);
#define CompileTimeAssert(x,id) const int __ctassert_##id=1/(int)(x)

template<class T>
T ReportResultTemplate(T result, const char *format, const char *expression, const char *filename, int linenumber)
{
  DebugPrint(format,result,linenumber,expression);
  return result;
}


#define PrintResult(x,format) ReportResultTemplate(x,"%s(%d): %s = " format,#x,__FILE__,__LINE__)

#else

//#include <stdio.h>

#ifndef Assert
#define Assert(x)
#endif
//#define AssertDesc(x,desc) if (!(x)) ::printf("MTLIB Error: %s\n", desc);
#define AssertDesc(x,desc)
#ifndef Verify
#define Verify(x) x
#endif
#define VerifyDesc(x,desc) x
static inline void DebugPrint(const char *pattern, ...) {}
#define DebugPrint0(str)
#define DebugPrint1(str,a1)
#define DebugPrint2(str,a1,a2)
#define DebugPrint3(str,a1,a2,a3)
#define DebugPrint4(str,a1,a2,a3,a4)
#define DebugPrint5(str,a1,a2,a3,a4,a5)
#define DebugPrint6(str,a1,a2,a3,a4,a5,a6)
#define DebugPrint7(str,a1,a2,a3,a4,a5,a6,a7)
#define DebugPrint8(str,a1,a2,a3,a4,a5,a6,a7,a8)
#define PrintResult(x,format)
#define Error(desc)
#define CompileTimeAssert(x,id)
#endif

#ifdef assert
#undef assert
#define assert(x) Assert(x)
#endif

#ifndef ASSERT
#define ASSERT(x) Assert(x)
#endif


template<bool> class CAssert;
template<> class CAssert<true> {};

#endif

