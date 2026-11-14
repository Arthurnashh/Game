#include <El/elementpch.hpp>
#include <El/FileServer/fileServerMT.hpp>
#include <El/FileServer/fileServerAsync.hpp>
#include "stringtableExt.hpp"
#include "engDummy.hpp"

#if defined _WIN32
#include <Es/Common/win.h>
#endif

class FileServerGameFunctions : public FileServerFunctions
{
public:
  virtual void ProcessFCPHManager(const char *name);
  virtual RString GetDamagedDiscErrorString();
  virtual void ReportFileReadError(HRESULT err, const char *name, bool terminate=true);
  virtual void ProgressRefresh();
};

#if defined(_WIN32) 
/**
Wrapped used to get va_list arguments*/
static DWORD FormatMessageWVA(
                              DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId,
                              LPWSTR lpBuffer, DWORD nSize,
                              ...
                              )
{
  va_list va;
  va_start(va, nSize);
  DWORD ret = FormatMessageW(dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,&va);
  va_end(va);
  return ret;
}
#endif

RString FileServerGameFunctions::GetDamagedDiscErrorString()
{
  return LocalizeString(IDS_DAMAGED_DISC);
}

void FileServerGameFunctions::ProcessFCPHManager(const char *name)
{
#if _USE_FCPHMANAGER
  // Try to find some open handle. If it exists, then wait for the process to finish and then continue
  FCProcessHandles *item = GFCPHManager.Find(RStringB(name));
  if (item)
  {
    // Finish process and delete item
    GFCPHManager.FinishProcessAndDeleteItem(item);
  }
#endif
}

void FileServerGameFunctions::ReportFileReadError(HRESULT err, const char *name, bool terminate)
{
  RptF("Error %x reading file '%s'",err,cc_cast(name));

  //if (terminate)
  {
    wchar_t buf[1024];
    // init as an empty string
    buf[0] = 0;
 
  #ifdef _WIN32
    int len = FormatMessageWVA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err,0,buf,lenof(buf),name,NULL);
    // docs are not very clear if string is zero terminated when buffer is full
    // to be sure we zero terminate it here
    buf[lenof(buf)-1] = 0;

    // remove trailing end of lines
    while (len>=0 && (buf[len-1]=='\n' || buf[len-1]=='\r'))
    {
      buf[--len]=0;
    }

    if (*buf)
    {
      // convert Unicode to UTF-8
      int len = WideCharToMultiByte(CP_UTF8, 0, buf, -1, NULL, 0, NULL, NULL);

      char buffer[2048];
      WideCharToMultiByte(CP_UTF8, 0, buf, -1, buffer, sizeof(buffer), NULL, NULL);
      buffer[len - 1] = 0; // make sure result is always null terminated

      if (terminate)
      {
        ErrorMessage(buffer);
      }
      else
      {
        LogF("  %s",buffer);
      }
    }
    else
  #endif
    {
      if (terminate)
      {
        ErrorMessage(GetDamagedDiscErrorString());
      }
    }
  }
}

void FileServerGameFunctions::ProgressRefresh()
{
}


static FileServerGameFunctions GFileServerGameFunctions;
FileServerFunctions *FileServer::GFileServerFunctions = &GFileServerGameFunctions;

#ifndef _WIN32
// GMultiThreadRegistration serves as the Main Thread Registration for the MultiThread library
// it is necessary (only) on Linux
MultiThread::RunningThread<MultiThread::ThreadBase> *GMultiThreadRegistration = NULL;
#endif

FileServerST SFileServer;

QIFileServerFunctions *GFileServerFunctions = &SFileServer;
//QIFileServerFunctions *GFileServerFunctions = &GAsyncFileServer;

QIFileWriteFunctions *GFileWriteFunctions = &SFileServer;
QIFileServerFunctions *GFileServerFunctionsSync = &SFileServer;

FileServer *GFileServer = &SFileServer;
