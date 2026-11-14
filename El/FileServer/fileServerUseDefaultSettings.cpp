#include <El/elementpch.hpp>

#include <El/FileServer/fileServerMT.hpp>
#include <El/FileServer/fileServerAsync.hpp>

class FileServerDefaultFunctions : public FileServerFunctions
{
public:
  virtual void ProcessFCPHManager(const char *name);
  virtual RString GetDamagedDiscErrorString();
#ifdef _WIN32
  virtual void ReportFileReadError(HRESULT err, const char *name, bool terminate=true);
  virtual void ProgressRefresh();
#endif
};


RString FileServerDefaultFunctions::GetDamagedDiscErrorString()
{
  return RString("There's a problem with the disc you're using. It may be dirty or damaged.");
}

void FileServerDefaultFunctions::ProcessFCPHManager(const char *name)
{
}

#ifdef _WIN32
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

void FileServerDefaultFunctions::ReportFileReadError(HRESULT err, const char *name, bool terminate)
{
  RptF("Error %x reading file '%s'",err,cc_cast(name));
  //if (terminate)
  {
    wchar_t buf[1024];
    // init as an empty string
    buf[0] = 0;

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
    {
      if (terminate)
      {
        ErrorMessage(GetDamagedDiscErrorString());
      }
    }
  }
}

void FileServerDefaultFunctions::ProgressRefresh()
{
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Memory management for FileServer
// This will be implemented as another module later

// TODO: implement as module
#include <El/FreeOnDemand/memFreeReq.hpp>
void FreeOnDemandGarbageCollectSystemMemoryLowLevel(unsigned int)
{}
void RegisterFreeOnDemandLowLevelMemory(IMemoryFreeOnDemand *)
{}
///////////////////////////////////////////////////////////////////////////////

static FileServerDefaultFunctions GFileServerDefaultFunctions;
FileServerFunctions *FileServer::GFileServerFunctions = &GFileServerDefaultFunctions;

#ifdef _LINUX
// fileServer globals should be initialized inside main() function when multiThread library is used
QIFileServerFunctions *GFileServerFunctions = NULL;
  QIFileWriteFunctions *GFileWriteFunctions = NULL;
FileServer *GFileServer = NULL;
#else
static FileServerST SFileServer;
  //QIFileServerFunctions *GFileServerFunctions = &GAsyncFileServer;
QIFileServerFunctions *GFileServerFunctions = &SFileServer;
  QIFileWriteFunctions *GFileWriteFunctions = &SFileServer;

  QIFileServerFunctions *GFileServerFunctionsSync = &SFileServer;
  
FileServer *GFileServer = &SFileServer;
#endif

