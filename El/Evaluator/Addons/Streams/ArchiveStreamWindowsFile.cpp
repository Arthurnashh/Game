// ArchiveStreamWindowsFile.cpp: implementation of the ArchiveStreamWindowsFile class.
//
//////////////////////////////////////////////////////////////////////

#include "ArchiveStreamWindowsFile.h"
#include <io.h>
#include <stdio.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ArchiveStreamWindowsFile::ArchiveStreamWindowsFile(HANDLE handle,bool autoclose):_handle(handle),_autoclose(autoclose)
{
  Reset();
  if (handle==NULL || handle==INVALID_HANDLE_VALUE) SetError(-1);
}

ArchiveStreamWindowsFile::~ArchiveStreamWindowsFile()
{
  if (_autoclose) CloseHandle(_handle);
}

int ArchiveStreamWindowsFile::IsError()
{
  return _err;
}

void ArchiveStreamWindowsFile::Reset()
{
  _err=0;
}

__int64 ArchiveStreamWindowsFile::Tell()
{
  if (_err) return -1;
  LONG high=0;
  DWORD low=SetFilePointer(_handle,0,&high,FILE_CURRENT);  
  return (__int64)low+((__int64)high<<32);
}

void ArchiveStreamWindowsFile::Seek(__int64 lOff, SeekOp seekop)
{  
  if (_err) return;
  LONG high=(LONG)(lOff>>32);
  DWORD low=(DWORD)(lOff & 0xFFFFFFFF);  
  switch (seekop)
  {
  case begin:low=SetFilePointer(_handle,low,&high,FILE_BEGIN);break;
  case current:low=SetFilePointer(_handle,low,&high,FILE_CURRENT);break;
  case end:low=SetFilePointer(_handle,low,&high,FILE_END);break;
  }
  if (low==0xFFFFFFFF && GetLastError()!=NO_ERROR) _err=-1;
}

int ArchiveStreamWindowsFileIn::DataExchange(void *buffer, int maxsize)
{
  if (_err) return _err;
  DWORD rd;
  if (ReadFile(_handle,buffer,maxsize,&rd,NULL)==FALSE) _err=-1;
  if (rd==0) _err=ASTRERR_EOF;
  else if (rd!=(DWORD)maxsize) ReportUncomplette(rd);
  return _err;
}


int ArchiveStreamWindowsFileOut::DataExchange(void *buffer, int maxsize)
{
  if (_err) return _err;
  DWORD rd;
  if (WriteFile(_handle,buffer,maxsize,&rd,NULL)==FALSE || rd!=(DWORD)maxsize) _err=-1;
  return _err;
}

