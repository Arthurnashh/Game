#ifndef _OPTIMA2MFC_H_
#define _OPTIMA2MFC_H_
/*
#define WMsgBSOk MB_OK
#define WMBButtonOk MB_OK
#define WMBLevelInfo MB_ICONINFORMATION 
#define WMBLevelPlain 0
#define WMBLevelWarning MB_ICONQUESTION
#define WMBLevelError MB_ICONSTOP

#define WMBButtonOkCancel MB_OKCANCEL
#define WMBButtonAbortRetryIgnore MB_ABORTRETRYIGNORE
#define WMBButtonYesNoCancel MB_YESNOCANCEL
#define WMBButtonYesNo MB_YESNO
#define WMBButtonRetryCancel MB_RETRYCANCEL
*/
//void SetProgress(int);
//#include <windows.h>
#define ISMASS(f) (f>1e12 && f<1e14 ) 
/*
class WMessageBox
  {
  public:
    static int Message(HWND wnd, int id, const char *title, const char *text)
      {
      if (wnd==NULL) wnd=::GetActiveWindow();
      return MessageBox(wnd,text,title,id);
      }
    static int Messagef(HWND wnd, int id, const char *title, const char *text, va_list list)
      {
      int size=1024+strlen(text)*2;
      char *szBuff=new char[size];
      _vsnprintf(szBuff,size,text,list);
      if (wnd==NULL) wnd=::GetActiveWindow();
      int ret=MessageBox(wnd,szBuff,title,id);
      delete [] szBuff;
      return ret;
      }
    static int Messagef(HWND wnd, int style,
    const char * caption,
    const char * format, ... )
      {
      va_list list;va_start(list,format);
      return Messagef(wnd,style,caption,format,list);
      }
    static int Messagef(HWND wnd, int style,int button,
    const char * caption,
    const char * format, ... )
      {
      va_list list;va_start(list,format);
      return Messagef(wnd,style|button,caption,format,list);
      }
    
    
  };
*/
//--------------------------------------------------
#include <atlstr.h>
class WString: public CString
  {
  public:
    const char *GetText() 
      {return *this;}
    WString& operator=(const CString& other) 
      {CString::operator=(other);return *this;}
    WString& operator=(const char *other) 
      {CString::operator=(other);return *this;}
    WString& operator=(char other) 
      {CString::operator=(other);return *this;}
    //	WString(const char *str): CString(str) {}
    WString(const char *str,...): CString() 
      {
      va_list list;
      va_start(list,str);
      FormatV(str,list);
      }
    WString(const CString& str): CString(str) 
      {}
    WString():CString() 
      {}
    WString(int ids):CString() 
      {LoadString(ids);}
    WString(long ids,...):CString() 
      {
      va_list list;
      va_start(list,ids);
      CString s;s.LoadString(ids);
      FormatV(s,list);
      }
    void Sprintf(const char *format, ...)
      {va_list list;va_start(list,format);FormatV(format,list);}
    
  };

//--------------------------------------------------

class WDebug
  {
  public:
    static void BreakIntoDebugger(const char *msg, bool debugstart)
      {
      ::MessageBox(NULL,"BreakIntoDebugger",msg,MB_OK|MB_ICONERROR|MB_TASKMODAL);
      __asm int 03h;
      }
    static void VPrintf(const char *msg, va_list list)
      {
      CString text; text.FormatV(msg,list);
      //TRACE0(text);
      }
    static void Printf(const char *msg, ...)
      {
      va_list list; va_start(list,msg);
      VPrintf(msg,list);
      }
  };

//--------------------------------------------------

#include "wFilePath.h"
#pragma warning (disable : 4244)
#pragma warning (disable : 4305)

//WString LODResolToName( float resol );
//float LODNameToResol( WString name );

struct WaterMark
  {
  void *self;
  int magic1a,magic2a;
  int value1,value2;
  int magic1b,magic2b;
  void *self2;
  };

//--------------------------------------------------

extern WaterMark MyWaterMark;

class ObjectData;
//void miscCreateBonePrimitive(ObjectData &obj,float *direction, float length);

#endif

