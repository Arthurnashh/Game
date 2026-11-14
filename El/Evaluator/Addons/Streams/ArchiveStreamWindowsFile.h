// ArchiveStreamWindowsFile.h: interface for the ArchiveStreamWindowsFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ArchiveStreamWindowsFile_H__7C0A4DF7_254D_4665_9BF4_5B6513FAD754__INCLUDED_)
#define AFX_ArchiveStreamWindowsFile_H__7C0A4DF7_254D_4665_9BF4_5B6513FAD754__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ArchiveStream.h"
#include <windows.h>

class ArchiveStreamWindowsFile : public ArchiveStream  
{
protected:
  HANDLE _handle;
  int _err;
  bool _autoclose;

public:
  ArchiveStreamWindowsFile(HANDLE handle,bool autoclose);
  virtual ~ArchiveStreamWindowsFile();
  virtual int IsError();
  virtual void Reset();
  virtual __int64 Tell();
  virtual void Seek(__int64 lOff, SeekOp seekop);
  virtual void SetError(int err) {_err=err;}

};










class ArchiveStreamWindowsFileIn : public ArchiveStreamWindowsFile
{
public:
  ArchiveStreamWindowsFileIn(HANDLE handle, bool autoclose):ArchiveStreamWindowsFile(handle,autoclose) 
  {}
  virtual bool IsStoring() 
  {return false;}
  virtual int DataExchange(void *buffer, int maxsize);

};










class ArchiveStreamWindowsFileOut : public ArchiveStreamWindowsFile
{
public:
  ArchiveStreamWindowsFileOut(HANDLE handle, bool autoclose):ArchiveStreamWindowsFile(handle,autoclose) 
  {}
  virtual bool IsStoring() 
  {return true;}
  virtual int DataExchange(void *buffer, int maxsize);
  void Flush() {if (!::FlushFileBuffers(_handle)) _err=GetLastError();}

};










#endif // !defined(AFX_ARCHIVESTREAMPOSIXFILE_H__7C0A4DF7_254D_4665_9BF4_5B6513FAD754__INCLUDED_)
