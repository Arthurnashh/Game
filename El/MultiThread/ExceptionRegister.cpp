#include "common_Win.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <malloc.h>
#include "ExceptionRegister.h"
#define WIN32_LEAN_AND_MEAN




#ifdef _MT
#include "tls.h"
static TLS(ExceptReg::ExceptionRegister) root=0;

#else

static ExceptReg::ExceptionRegister *root=0;

#endif

namespace ExceptReg
{
  ExceptionRegister::ExceptionRegister(void):_nextRegister(root)
  {
    root=this;
  }

  ExceptReg::ExceptionRegister *ExceptionRegister::GetCurrent()
  {
    return root;
  }

  ExceptionRegister::~ExceptionRegister(void)
  {
    if (root==this)  
      root=_nextRegister;
    else
    {
      ExceptionRegister *p=root;
      if (p == 0) return;
      while (p->_nextRegister!=this) p=p->_nextRegister;
      p->_nextRegister=_nextRegister;
    }
  }

  int ExceptionRegister::RegExcpt(const IException &exception)
  {
    if (root) return root->Catch(exception);
    else return DefaultHandler(exception);
  }

  static DWORD WINAPI MsgBoxThread(LPVOID text)
  {
    _TCHAR buff[2048];
    GetModuleFileName(0,buff,sizeof(buff)/sizeof(_TCHAR));
    const _TCHAR *name=_tcsrchr(buff,'\\');
    if (name==0) name=buff;else name++;
    int i=MessageBox(0,(LPCTSTR)text,name,MB_ICONEXCLAMATION|MB_ABORTRETRYIGNORE|MB_SYSTEMMODAL);
    return i;
  }

  static int UnhandledHandler(const IException &exception)
  {
    _TCHAR buff[2048];
    _TCHAR paramStr[1024];
    va_list parms=ExceptionRegister::CreateListOfParams(exception,alloca(exception.GetParamCount()*sizeof(double)));
    _vsntprintf(paramStr,sizeof(paramStr)/sizeof(_TCHAR),exception.GetDesc(),parms);
    paramStr[sizeof(paramStr)/sizeof(_TCHAR)-1]=0;
    _sntprintf(buff,sizeof(buff)/sizeof(_TCHAR),_T("Unhandled exception:\r\n\r\nModule: %s\r\nType: %s\r\nDescription: %s\r\n\r\n(Note: this is a default handler. Sender can ignore your decision.)"),
      exception.GetModule(),exception.GetType(),paramStr);
    
    ///HALT the calling thread to prevent re-entering to this section.
    DWORD thrdid;
    HANDLE thr=CreateThread(0,0,&MsgBoxThread,buff,0,&thrdid);
    WaitForSingleObject(thr,INFINITE);
    DWORD i;
    GetExitCodeThread(thr,&i);
    

    if (i==IDRETRY) return ExceptionRegister::fbRetry;
    if (i==IDIGNORE) return ExceptionRegister::fbIgnore;
    return ExceptionRegister::fbAbort;
  }

  int ExceptionRegister::DefaultHandler(const IException &exception)
  {
    if (RegExcpt_DefaultHandler==0) 
    {
      return UnhandledHandler(exception);    
    }
    else
      return RegExcpt_DefaultHandler(exception);
  }

  va_list ExceptionRegister::CreateListOfParams(const IException &exception, void *buffer)
  {
    va_list list=reinterpret_cast<va_list>(buffer);
    va_list ptr=list;

    for (unsigned int i=0,sz=exception.GetParamCount();i<sz;i++)
    {
      if (exception.GetParam(i,*(int *)ptr)) va_arg(ptr,int);
      else if (exception.GetParam(i,*(unsigned int *)ptr)) va_arg(ptr,unsigned int);
      else if (exception.GetParam(i,*(float *)ptr)) va_arg(ptr,float);
      else if (exception.GetParam(i,*(double *)ptr)) va_arg(ptr,double);
      else if (exception.GetParam(i,*(const char **)ptr)) va_arg(ptr,const char *);
      else if (exception.GetParam(i,*(void **)ptr)) va_arg(ptr,void *);
      else {*(int *)ptr=0;va_arg(ptr,int);}
    }
    return list;
  }

  RegExcpt_DefaultHandler_Type RegExcpt_DefaultHandler;
}

