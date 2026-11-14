#include <Es/essencepch.hpp>
#include "appFrame.hpp"

#include <stdio.h>
#include <string.h>
#include <Es/Common/win.h>
#include <Es/Strings/bString.hpp>

#if COUNT_CLASS_INSTANCES

AllCountersLink *AllCountInstances[];
int AllCountInstancesCount;

/// limit for WalkAllInstanceCounts
static int LimitInstanceCount = 100;

/// this function can be used from Debugger to get a list of interesting (often used) objects
void WalkAllInstanceCounts()
{
  for (int i=0; i<AllCountInstancesCount; i++)
  {
    int count = AllCountInstances[i]->GetValue();
    if (count>=LimitInstanceCount)
    {
      LogF("(*AllCountInstances[%d]):%d",i,count);
    }
  }
}

#endif

bool DisableLogs = false;

#if _ENABLE_REPORT
void LogF(char const *format,...)
{
  if (DisableLogs) return;

	va_list arglist;
	va_start(arglist, format);

	CurrentAppFrameFunctions->LogF(format, arglist);

	va_end(arglist);
}

void vaLogF(const char *format, va_list argptr)
{
  if (DisableLogs) return;
	CurrentAppFrameFunctions->LogF(format, argptr);
}

#endif


void LstF(char const *format,...)
{
  if (DisableLogs) return;

	va_list arglist;
	va_start(arglist, format);

	CurrentAppFrameFunctions->LstF(format, arglist);

	va_end(arglist);
}

void vaLstF(const char *format, va_list argptr)
{
  if (DisableLogs) return;
	CurrentAppFrameFunctions->LstF(format, argptr);
}

void ErrF(char const *format,...)
{
  if (DisableLogs) return;

	va_list arglist;
	va_start(arglist, format);

	CurrentAppFrameFunctions->ErrF(format, arglist);

	va_end(arglist);
}

void vaErrF(const char *format, va_list argptr)
{
  if (DisableLogs) return;
	CurrentAppFrameFunctions->ErrF(format, argptr);
}

void LogDebugger(char const *format,...)
{
  if (DisableLogs) return;

	va_list arglist;
	va_start(arglist, format);

	CurrentAppFrameFunctions->LogDebugger(format, arglist);

	va_end(arglist);
}

void ErrorMessage(ErrorMessageLevel level, char const *format,...)
{
  if (DisableLogs) return;
	va_list arglist;
	va_start(arglist, format);

	CurrentAppFrameFunctions->ErrorMessage(level, format, arglist);

	va_end(arglist);
}

void ErrorMessage(char const *format,...)
{
  if (DisableLogs) return;
	va_list arglist;
	va_start(arglist, format);

	CurrentAppFrameFunctions->ErrorMessage(format, arglist);

	va_end(arglist);
}

void WarningMessage(char const *format,...)
{
  if (DisableLogs) return;
	va_list arglist;
	va_start(arglist, format);

	CurrentAppFrameFunctions->WarningMessage(format, arglist);

	va_end(arglist);
}

bool CheckMainThread()
{
  return CurrentAppFrameFunctions->CheckMainThread();
};
bool CheckMainThread(int id)
{
  return CurrentAppFrameFunctions->CheckMainThread(id);
};
bool CheckSameThread(int &id)
{
  return CurrentAppFrameFunctions->CheckSameThread(id);
};

void GlobalDiagMessage(int &handle, int id, int timeMs, const char *msg, ...)
{
	va_list arglist;
	va_start(arglist, msg);
	
  BString<512> buf;
  vsprintf(buf,msg,arglist);
  
	CurrentAppFrameFunctions->DiagMessage(handle,id,timeMs, buf);

	va_end(arglist);
}

void GlobalShowMessage(int timeMs, const char *msg, ...)
{
	va_list arglist;
	va_start(arglist, msg);

  BString<512> buf;
  vsprintf(buf,msg,arglist);

	CurrentAppFrameFunctions->ShowMessage(timeMs, buf);

	va_end(arglist);
}

unsigned long GlobalTickCount ()
{
	return CurrentAppFrameFunctions->TickCount();
}

void GlobalShowMessage(int timeMS, const char *text, InitVal<int,-1> *handles, int n)
{
  const int line = 80;
  int i = 0;

  // split message to multiple lines when contain \n
  const char *start = text;
  while (true)
  {
    const char *ptr = strchr(start, '\n');
    if (ptr && ptr - start <= line)
    {
      // next line
      GlobalDiagMessage(handles[i], 0, timeMS, "%s", cc_cast(RString(start, ptr - start)));
      if (i + 1 < n) i++;
      start = ptr + 1;
    }
    else
    {
      if (strlen(start) <= line)
      {
        GlobalDiagMessage(handles[i], 0, timeMS, "%s", start);
        break;
      }
      // line too long
      const char *space = start + line - 1;
      while (space > start && !isspace(*space)) space--;
      RString str;
      if (space == start)
      {
        // no space on this line
        str = RString(start, line);
        start += line;
      }
      else
      {
        // word wrap
        str = RString(start, space - start);
        start = space + 1;
      }
      GlobalDiagMessage(handles[i], 0, timeMS, "%s", cc_cast(str));
      if (i + 1 < n) i++;
    }
  }
}

// Default implementation

#if _ENABLE_REPORT

void AppFrameFunctions::LogF(const char *format, va_list argptr)
{
  if (DisableLogs) return;

	BString<512> buf;
	vsprintf(buf,format,argptr);
	strcat(buf,"\n");
#ifdef _WIN32
  #ifdef UNICODE
    WCHAR wBuf[512];
    MultiByteToWideChar(CP_ACP, 0, buf, -1, wBuf, lenof(wBuf));
    OutputDebugString(wBuf);
  #else
    OutputDebugString(buf);
  #endif // !UNICODE
#else
	fputs(buf,stderr);
#endif
}

void AppFrameFunctions::LogDebugger(const char *format, va_list argptr)
{
  LogF(format, argptr);
}

#else

void AppFrameFunctions::LogDebugger(const char *format, va_list argptr)
{
	#ifdef _WIN32
	BString<512> buf;
	vsprintf(buf,format,argptr);
	strcat(buf,"\n");
	OutputDebugString(buf);
	#endif
}

#endif

void AppFrameFunctions::LstF(const char *format, va_list argptr)
{
#if _ENABLE_REPORT
  LogF(format, argptr);
#endif
}

void AppFrameFunctions::LstFDebugOnly(const char *format, va_list argptr)
{
#if _ENABLE_REPORT
  LogF(format, argptr);
#endif
}

void AppFrameFunctions::ErrF(const char *format, va_list argptr)
{
#if _ENABLE_REPORT
  LogF(format, argptr);
#endif
}

#ifdef _WIN32
static DWORD MainThreadId = GetCurrentThreadId();

bool AppFrameFunctions::CheckMainThread(int threadId) const
{
  return threadId == MainThreadId;
}

int AppFrameFunctions::GetMainThreadId() const
{
  return MainThreadId;
}

bool AppFrameFunctions::CheckMainThread() const
{
  // if MainThreadId not initialized yet, we are unable to verify
  return MainThreadId==0 || GetCurrentThreadId() == MainThreadId;
}
bool AppFrameFunctions::CheckSameThread(int &id) const
{
  // if MainThreadId not initialized yet, we are unable to verify
  DWORD currId = GetCurrentThreadId();
  if (id==0)
  {
    // when called for the first time, initialize
    id = currId;
    return true;
  }
  else
  {
    return currId==id;
  }
}
void AppFrameFunctions::ResetMainThread()
{
  MainThreadId = GetCurrentThreadId();
}

#else

int AppFrameFunctions::GetMainThreadId() const
{
  //unconst_cast(this)->Fail("Not implemented");
  return 0;
}

bool AppFrameFunctions::CheckMainThread(int threadId) const
{
  //unconst_cast(this)->Fail("Not implemented");
  return true;
}

bool AppFrameFunctions::CheckMainThread() const
{
  return true;
}
bool AppFrameFunctions::CheckSameThread(int &id) const
{
  return true;
}
void AppFrameFunctions::ResetMainThread()
{

}

#endif

