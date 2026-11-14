#include <El/elementpch.hpp>
#include <El/FileServer/fileServerMT.hpp>
#include "qStream.hpp"

/** Basic implementation of QIFileServerFunctions
FileServerHandle is interpreted as HANDLE
all calls are forwarded to corresponding
*/

class QIFileServerFunctionsDefault: public QIFileServerFunctions
{
  public:
  virtual bool FlushReadHandle(const char *name);
  virtual bool Preload(
    FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, FileServerHandle handle, QFileSize start, QFileSize size
  );
  virtual PosQIStreamBuffer Read(FileServerHandle handle, QFileSize start, QFileSize size);
  virtual PosQIStreamBuffer RequestResultAsync(FileRequestCompletionHandle &request, FileServerHandle handle, QFileSize pos);
  virtual InFileLocation RequestLocationAsync(const FileRequestCompletionHandle &request);
  virtual bool CheckIfRequestContains( Ref<RefCount> & request, FileServerHandle handle, QFileSize pos, QFileSize size );
  virtual FileServerHandle OpenReadHandle(const char *name);
  virtual void MakeReadHandlePermanent(FileServerHandle handle);
  virtual RString GetHandleName(FileServerHandle handle) const;
  virtual void CloseReadHandle(FileServerHandle handle);
  virtual bool CheckReadHandle(FileServerHandle handle) const;
  virtual int GetPageSize();
  virtual QFileTime TimeStamp(FileServerHandle handle) const;
  virtual QFileSize GetFileSize(FileServerHandle handle) const;
  virtual IFileBuffer *NewFileBufferLoading(const char *name);
  virtual void *GetWinHandle(FileServerHandle handle) const;
  virtual Future<QFileTime> RequestTimeStamp(const char *name);
  virtual bool OtherThreadsCanUse() const;

};

PosQIStreamBuffer QIFileServerFunctionsDefault::Read(FileServerHandle handle, QFileSize start, QFileSize size)
{
  QIStreamBuffer *buffer = new QIStreamBufferPage();
  Assert(size<=buffer->GetSize());

  void *data = buffer->DataLock(0,size);
  if (!data)
  {
    //! error - out of memory
    return PosQIStreamBuffer(buffer,start,0);
  }

  HANDLE filehandle = (HANDLE)handle;
  LONG zero = NULL;
  SetFilePointer(filehandle,start,&zero,FILE_BEGIN); //so we are currently limited to 4GB
  DWORD rd = 0;
  ReadFile(filehandle,data,size,&rd,NULL);
  buffer->DataUnlock(0,rd);

  return PosQIStreamBuffer(buffer,start,rd);
}

PosQIStreamBuffer QIFileServerFunctionsDefault::RequestResultAsync(FileRequestCompletionHandle &request, FileServerHandle handle, QFileSize pos)
{
  return PosQIStreamBuffer();
}

InFileLocation QIFileServerFunctionsDefault::RequestLocationAsync(const FileRequestCompletionHandle &req)
{
  InFileLocation key;
  key._handle = NULL;
  key._start = 0;
  key._size = 0;
  
/*  TODO : FileRequestCompletionRefCount doesn't have any attributes! Don't know what to do with this.

  FileRequest *request = static_cast<FileRequest *>(req.GetRef());
  if (!request)
  {
    key._handle = NULL;
    key._start = 0;
    key._size = 0;
  }
  else
  {
    key._handle = request->_fileLogical;
    key._start = request->_neededBeg;
    key._size = request->_neededEnd-request->_neededBeg;
  }*/

  return key;
}

bool QIFileServerFunctionsDefault::CheckIfRequestContains( Ref<RefCount> & request, FileServerHandle handle, QFileSize pos, QFileSize size )
{
  return false;
}

IFileBuffer *QIFileServerFunctionsDefault::NewFileBufferLoading(const char *name)
{
  return new FileBufferLoading(name);
  //return new FileBufferLoadingAsyncable(name);
}


int QIFileServerFunctionsDefault::GetPageSize()
{
  // unless some file server is used, there any page size will do
  // ask memory manager what page size it likes
  return GetPageRecommendedSize();
}

/**
Default handling - assume all requests are satisfied and call handler immediately.
*/
bool QIFileServerFunctionsDefault::Preload(
  FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx, FileRequestPriority priority, FileServerHandle handle, QFileSize start, QFileSize size
)
{
  if (obj)
    obj->RequestDone(ctx,RequestableObject::RRSuccess);
  return true;
}

bool QIFileServerFunctionsDefault::FlushReadHandle(const char *name)
{
  return true;
}

FileServerHandle QIFileServerFunctionsDefault::OpenReadHandle(const char *name)
{
#ifdef POSIX_FILES
  LocalPath(fn,name);
  //if testing POSIX_FILES in _WIN32, must be O_BINARY, because read function replaces
  //each carriage return–linefeed (CR-LF) pair with a single linefeed character
  //In FileBufferLoading::ReadBuffer, the pageSize aligning then fails: pos &= ~(pageSize-1);
  //see "read function" msdn documentation
  #ifdef _WIN32
  return (FileServerHandle)::open(fn,O_RDONLY|O_BINARY);
  #else
  return (FileServerHandle)::open(fn,O_RDONLY);
  #endif
#else
  return (FileServerHandle)::CreateFileA
    (
    name,GENERIC_READ,FILE_SHARE_READ,
    NULL, // security
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL // template
    );
#endif
}

void QIFileServerFunctionsDefault::MakeReadHandlePermanent(FileServerHandle handle)
{
}

void QIFileServerFunctionsDefault::CloseReadHandle(FileServerHandle handle)
{
#if !POSIX_FILES_COMMON
  CloseHandle((FileServerHandle)handle);
#else
  ::close((int)handle);
#endif
}

bool QIFileServerFunctionsDefault::CheckReadHandle(FileServerHandle handle) const
{
  HANDLE winHandle = (HANDLE)handle;
  return winHandle!=NULL && winHandle!=INVALID_HANDLE_VALUE;
}

RString QIFileServerFunctionsDefault::GetHandleName(FileServerHandle handle) const
{
  // is it not possible to get a name of the file when only handle is known
  return Format("Handle %x",(int)handle);  
}

QFileSize QIFileServerFunctionsDefault::GetFileSize(FileServerHandle handle) const
{
#ifndef POSIX_FILES
  return ::GetFileSize((HANDLE)handle, NULL);
#else
  return FileSize((int)handle);
#endif
}

QFileTime QIFileServerFunctionsDefault::TimeStamp(FileServerHandle handle) const
{
#if defined POSIX_FILES
  struct stat st;
  fstat((int)handle,&st);
  return st.st_mtime;
#else
  FILETIME filetime;
  ::GetFileTime((HANDLE)handle, NULL, NULL, &filetime);
  return ConvertToQFileTime(filetime);
#endif
}

bool QIFileServerFunctionsDefault::OtherThreadsCanUse() const
{
  return false;
}

Future<QFileTime> QIFileServerFunctionsDefault::RequestTimeStamp(const char *name)
{
  FileServerHandle handle = OpenReadHandle(name);
  QFileTime ret = TimeStamp(handle);
  CloseReadHandle(handle);
  return Future<QFileTime>(ret);
}

void *QIFileServerFunctionsDefault::GetWinHandle(FileServerHandle handle) const
{
  return (void *)handle;
}

static QIFileServerFunctionsDefault SQIFileServerFunctions;

QIFileServerFunctions *GFileServerFunctions = &SQIFileServerFunctions;


class QIFileWriteFunctionsDefault: public QIFileWriteFunctions
{
  public:
  virtual FileWriteHandle OpenWriteHandle(const char *name);
  virtual FileWriteHandle OpenWriteHandleForAppend(const char *name);
};


FileWriteHandle QIFileWriteFunctionsDefault::OpenWriteHandle(const char *name)
{
#ifdef POSIX_FILES

  #ifndef _WIN32
  LocalPath(fn, (const char*)name);
  FileWriteHandle retVal = (FileServerHandle)::open(fn,O_CREAT|O_WRONLY|O_TRUNC,S_IREAD|S_IWRITE);
  if (retVal) chmod(fn,S_IREAD|S_IWRITE);
  return retVal;
  #else
  return (FileWriteHandle)::open(name,O_CREAT|O_BINARY|O_WRONLY|O_TRUNC,S_IREAD|S_IWRITE);
  #endif

#else
  return (FileWriteHandle)::CreateFileA(
    name, GENERIC_WRITE, 0,
    NULL, // security
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    NULL); // template
#endif
}

FileWriteHandle QIFileWriteFunctionsDefault::OpenWriteHandleForAppend(const char *name)
{
#ifdef POSIX_FILES

#ifndef _WIN32
  LocalPath(fn, (const char*)name);
  FileWriteHandle retVal = (FileWriteHandle)::open(fn,O_CREAT|O_APPEND|O_WRONLY,S_IREAD|S_IWRITE);
  if (retVal) chmod(fn,S_IREAD|S_IWRITE);
  return retVal;
#else
  return (FileWriteHandle)::open(name,O_CREAT|_O_APPEND|O_BINARY|O_WRONLY,S_IREAD|S_IWRITE);
#endif

#else
  HANDLE fileHandle = ::CreateFileA(
    name, GENERIC_WRITE, 0,
    NULL, // security
    OPEN_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    NULL); // template
  SetFilePointer(fileHandle, 0, 0, FILE_END);
  return (FileWriteHandle)fileHandle;
#endif
}



static QIFileWriteFunctionsDefault SQIFileWriteFunctions;

QIFileWriteFunctions *GFileWriteFunctions = &SQIFileWriteFunctions;
