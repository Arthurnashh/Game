#ifdef _MSC_VER
#pragma once
#endif

#ifndef __APP_FRAME_EXT_HPP
#define __APP_FRAME_EXT_HPP

#include <Es/Framework/appFrame.hpp>

class OFPFrameFunctions : public AppFrameFunctions
{
protected:

public:

#if _ENABLE_REPORT
  virtual void LogF(const char *format, va_list argptr);
#endif
  virtual void LstF(const char *format, va_list argptr);
  virtual void LstFDebugOnly(const char *format, va_list argptr);
  virtual void ErrF(const char *format, va_list argptr);

  virtual void ErrorMessage(const char *format, va_list argptr);
  virtual void ErrorMessage(ErrorMessageLevel level, const char *format, va_list argptr);
  virtual void WarningMessage(const char *format, va_list argptr);

  virtual void ShowMessage(int timeMs, const char *msg);
  virtual void DiagMessage(int &handle, int id, int timeMs, const char *msg, ...);
  virtual DWORD TickCount();
  virtual RString GetAppCommandLine() const;
  virtual int ProfileBeginGraphScope(unsigned int color,const char *name) const;
  virtual int ProfileEndGraphScope() const;

  OFPFrameFunctions();
  ~OFPFrameFunctions();
};

#endif
