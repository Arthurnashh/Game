#include <Es/Common/win.h>
#pragma warning(disable:4996)
#include <Es/Framework/debugLog.hpp>
#include <Es\Framework\appFrame.hpp>
#include "Assert.h"

void ImplementBreak(bool test,char const *expression,char const *file,int line,char const *desc)
{
  if (test) return;
  LogF("%s(%d) : Assertion failed '%s'",file,line,expression);
  FailHook(expression);
}

void ImplementDebugPrint(const char *pattern, va_list args)
{
#if _ENABLE_REPORT
  CurrentAppFrameFunctions->LogF(pattern, args);
#endif
}
