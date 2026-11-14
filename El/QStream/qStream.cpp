// Quick file stream implementation

#include <El/elementpch.hpp>
//#include "winpch.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
	#include <io.h>
	#include <stdio.h>
#else
	#include <stdio.h> 
	#include <unistd.h> 
	#define POSIX_FILES 1
#endif

#include <Es/Files/filenames.hpp>

#include "qStream.hpp"

#if POSIX_FILES_COMMON
#define POSIX_FILES 1

#endif

#if !defined INVALID_SET_FILE_POINTER && _MSC_VER<1300
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif //INVALID_SET_FILE_POINTER not defined in MSVC 6.0 headers

#ifdef POSIX_FILES
	#define NO_FILE(file) ( file<0 )
	#define NO_FILE_SET -1
#else
	#define NO_FILE(file) ( file==NULL )
	#define NO_FILE_SET NULL
#endif

const char *GetErrorName(LSError err)
{
	switch (err)
	{
	case LSOK:
		return "No error";
	case LSFileNotFound:
		return "No such file";
	case LSBadFile:
		return "Bad file (CRC, ...)";
	case LSStructure:
		return "Bad file structure";
	case LSUnsupportedFormat:
		return "Unsupported format";
	case LSVersionTooNew:
		return "Version is too new";
	case LSVersionTooOld:
		return "Version is too old";
	case LSDiskFull:
		return "No such file";
	case LSAccessDenied:
		return "Access denied";
	case LSDiskError:
		return "Disk error";
	case LSNoEntry:
		return "No entry";
	default:
		Fail("LSError");
	case LSUnknownError:
		return "Unknown error";
	}
}

void QIStreamSimple::Close()
{
	_buf=NULL;
	_len=0;
	_readFrom=0;
	_fail=true,_eof=false;
}

void QIStreamSimple::Assign( const QIStreamSimple &src )
{
	// this points to same data as from
	_buf=src._buf;
	_len=src._len;
	_readFrom=src._readFrom;
	_fail=src._fail,_eof=src._eof;
}

bool QIStreamSimple::nextLine ()
{
    int c1;
    while ( !eof() )
        if ( (c1 = get()) == 0x0D || c1 == 0x0A ) {
            if ( !eof() && c1 == 0x0D ) {
                int c2 = get();
                if ( c2 != 0x0A ) unget();
                }
            return true;
            }
    return false;
}

bool QIStreamSimple::readLine ( char *buf, int bufLen )
{
    Assert( buf );
    int left = bufLen - 1;                  // regular chars to read
    int c1;
    while ( !eof() ) {
        if ( (c1 = get()) == 0x0D || c1 == 0x0A ) {
                // EOLN:
            if ( !eof() && c1 == 0x0D ) {
                int c2 = get();
                if ( c2 != 0x0A ) unget();
                }
            *buf = (char)0;
            return true;
            }
            // regular char:
        if ( bufLen > 0 && left == 0 ) {    // buffer overflow
            *buf = (char)0;
            return nextLine();
            }
        *buf++ = c1;
        left--;
        }
    *buf = (char)0;
    return false;                           // EOF reached before EOLN
}

#ifdef POSIX_FILES
int FileSize ( int handle )
{
	struct stat buf;
	fstat(handle,&buf);
	return buf.st_size;
}
#endif

#ifndef POSIX_FILES
  #include "fileCompress.hpp"
#endif

/*!
Default implementation - assume you can access whole file.
In older implementations of IFileBuffer GetData() and GetSize() were the only way to get data
This is 
*/
PosQIStreamBuffer IFileBuffer::ReadBuffer(FileRequestCompletionHandle &request, PosQIStreamBuffer &buf, QFileSize pos)
{
  buf.Free();
  const char *wholeData = GetData();
  QFileSize wholeSize = GetSize();
  if (pos>=wholeSize) return PosQIStreamBuffer(new QIStreamBuffer,pos,0);

  // optimal buffer size is system dependent variable
  // on XBox and PC (Intel 32b) this is 4 KB
  QFileSize size = 64*1024;
  QFileSize maxSize = wholeSize-pos;
  if (size>maxSize) size=maxSize;

  QIStreamBuffer *nBuf = new QIStreamBuffer(size);

  nBuf->FillWithData(wholeData+pos,size);

  return PosQIStreamBuffer(nBuf,pos,size);
}

/*!
Read given data region using ReadBuffer
*/
QFileSize IFileBuffer::Read(void *buf, QFileSize pos, QFileSize size)
{
  QFileSize rdTotal = 0;
  char *dta = (char *)buf;
  while (size>0)
  {
    PosQIStreamBuffer dummy(NULL,0,0);
    FileRequestCompletionHandle request;
    PosQIStreamBuffer buffer = ReadBuffer(request,dummy,pos);

    QFileSize bufferPos = Convert64ToQFileSize(pos-buffer._start);
    QFileSize readSize = buffer._len-bufferPos;
    if (readSize>size) readSize = size;
    QFileSize rd = buffer->ReadData(dta,bufferPos,readSize);

    if (rd<=0) break;
    size -= rd;
    dta += rd;
    pos += rd;
    rdTotal += rd;
  }
  return rdTotal;
}

FileBufferSub::FileBufferSub( IFileBuffer *buf, QFileSize start, QFileSize size )
:_whole(buf)
{
	QFileSize bufSize = buf->GetSize();
	if (start>bufSize) start = bufSize;
  QFileSize maxSize = bufSize-start;
  if (size>maxSize) size = maxSize;
  
	_start = start;
	_size = size;
}

bool FileBufferSub::PreloadBuffer(
  FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
  FileRequestPriority priority, QFileSize pos, QFileSize size
) const
{
  if (pos>_size) pos = _size;
  QFileSize maxSize = _size-pos;
  if (size>maxSize) size=maxSize;
  return _whole->PreloadBuffer(request,obj,ctx,priority,_start+pos,size);
}

/*!
  \patch 5175 Date 10/11/2007 by Ondra
  - Fixed: Reduced loading time for large textures.
*/

void FileBufferSub::PreloadSequential(QFileSize pos, QFileSize size) const
{
  if (pos>_size) pos = _size;
  QFileSize maxSize = _size-pos;
  if (size>maxSize) size=maxSize;
  _whole->PreloadSequential(_start+pos,size);
}

bool FileBufferSub::CheckIfRequestContains(FileRequestCompletionHandle &request, QFileSize pos, QFileSize size)
{
  if (pos>_size) pos = _size;
  QFileSize maxSize = _size-pos;
  if (size>maxSize) size=maxSize;
  return _whole->CheckIfRequestContains(request,_start+pos,size);

}

PosQIStreamBuffer FileBufferSub::ReadBuffer(FileRequestCompletionHandle &request, PosQIStreamBuffer &buf, QFileSize pos)
{
  if (pos>_size) pos = _size;
  PosQIStreamBuffer nBuf = _whole->ReadBuffer(request,buf,_start+pos);
  // we may need to shrink data a little bit
  Assert(nBuf._start<=_start+_size)
  QFileSize maxEnd = Convert64ToQFileSize(_start+_size-nBuf._start);
  QFileSize end = nBuf._len;
  if (end>maxEnd)
  {
    end = maxEnd;
  }

  return PosQIStreamBuffer(nBuf,nBuf._start-_start,end);
}

Ref<IFileBuffer> IFileBuffer::SubBuffer(IFileBuffer *whole, QFileSize offset, QFileSize size)
{
  // prevent calling GetSize unless necessary - might take long
  if (offset>0 || size<QFileSizeMax && size<whole->GetSize())
  {
    return new FileBufferSub(whole,offset,size);
  }
  else
  {
    return whole;
  }
}

DEFINE_FAST_ALLOCATOR(QIStreamBufferPage)

QIStreamBufferPage::QIStreamBufferPage()
{
  // allocate one page
	int pageSize = GFileServerFunctions->GetPageSize();
  _buffer = (char *)NewPage(pageSize,pageSize);
  if (!_buffer)
  {
    _bufferSize = 0;
    // error - no memory
  }
  else
  {
    _bufferSize = pageSize;
  }
}
QIStreamBufferPage::~QIStreamBufferPage()
{
  // release one page
  if (_buffer)
  {
    DeletePage(_buffer,GFileServerFunctions->GetPageSize());
	  _buffer = NULL;
  }
  _bufferSize = 0;
}


FileBufferLoading::FileBufferLoading( const char *name)
{
  FileServerHandle handle = GFileServerFunctions->OpenReadHandle(name);
  if( GFileServerFunctions->CheckReadHandle(handle))
  {
    QFileSize fileSize = GFileServerFunctions->GetFileSize(handle);

    // adjust offset and size based on actual file size
    _handle = handle;
    _size = fileSize;
  }
  else
  {
    // do not keep invalid handle - this makes testing for error a lot easier in future
    _handle = NULL;
    _size = 0;
  } 
}

FileBufferLoading::~FileBufferLoading()
{
  if (_handle)
  {
    GFileServerFunctions->CloseReadHandle(_handle);
    _handle = NULL;
}
}

const char *FileBufferLoading::GetData() const
{
  Fail("Cannot return data");
  return NULL;
}
QFileSize FileBufferLoading::GetSize() const
{
  return _size;
}

bool FileBufferLoading::GetError() const
{
  return _handle==NULL;
}

bool FileBufferLoading::PreloadBuffer(
  FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
  FileRequestPriority priority, QFileSize pos, QFileSize size
) const
{
  if (pos>_size) pos = _size;
  QFileSize maxSize = _size - pos;
  if (size>maxSize) size = maxSize;
  return GFileServerFunctions->Preload(request,obj,ctx,priority,_handle,pos,size);
}

void FileBufferLoading::PreloadSequential(QFileSize pos, QFileSize size) const
{
  //PROFILE_SCOPE_DETAIL_EX(fbPSq,file)
  // we know actual read will follow immediately
  FileRequestPriority priority = FileRequestPriority(1);
  if (pos>_size) pos = _size;
  QFileSize maxSize = _size - pos;
  if (size>maxSize) size = maxSize;
  
  // will split the request into a lot of small ones
  // this way first part will be ready much sooner than the whole request would be
  // what granularity is best is hard to tell
  QFileSize granularity = GFileServerFunctions->GetPageSize();
  // avoid generating too small requests
  // avoid generating too many requests
  // expected request latency:
  // hard-disk: seek rate 5-30 ms, sustained data rate 12-80 MB/s
  // DVD: seek rate 100-300 ms, sustained data rate 2-15 MB/s
  // expected time for sustained reading 32 KB file from hard-disk is around 1 ms 
  
  while (granularity<32*1024) granularity = granularity+granularity;

  // we do not care about results of the requests  
  FileRequestCompletionHandle dummy;
  while (size>=granularity)
  {
    GFileServerFunctions->Preload(dummy,NULL,NULL,priority,_handle,pos,granularity);
    pos += granularity;
    size -= granularity;
  }
  if (size>0)
  {
    GFileServerFunctions->Preload(dummy,NULL,NULL,priority,_handle,pos,size);
  }
}

bool FileBufferLoading::CheckIfRequestContains(FileRequestCompletionHandle &request, QFileSize pos, QFileSize size)
{
  const InFileLocation &loc = GFileServerFunctions->RequestLocationAsync(request);
  return loc.Contains(_handle,pos,size);
}



PosQIStreamBuffer FileBufferLoading::ReadBuffer(FileRequestCompletionHandle &request, PosQIStreamBuffer &buf, QFileSize pos)
{
  buf.Free();
  if (pos>_size) pos = _size;
  

  int pageSize = GFileServerFunctions->GetPageSize();

  pos &= ~(pageSize-1); //for POSIX_FILES, reading should be O_BINARY, otherwise pos often set to zero

  Assert(_size>=pos);
  QFileSize maxSize = _size - pos;

  QFileSize size = pageSize;
  if (size>maxSize) size = maxSize;
  
  if (pos>=_size)
  {
    return PosQIStreamBuffer(new QIStreamBuffer,pos,0);
  }

  PosQIStreamBuffer ret = GFileServerFunctions->RequestResultAsync(request,_handle,pos);
  if (ret._len>0)
    return ret;
  
  return PosQIStreamBuffer(GFileServerFunctions->Read(_handle,pos,size));
}


int CmpStartStr( const char *str, const char *start )
{
	while( *start )
	{
		if( myLower(*str++)!=myLower(*start++) ) return 1;
	}
	return 0;
}

bool QIFileFunctions::FileReadOnly( const char *name )
{
	// file exists and is read only
#if defined POSIX_FILES
	LocalPath(fn,name);
	struct stat st;
	if ( stat(fn,&st) ) return false;
  return( (st.st_mode & S_IWUSR) == 0 );
#else
	DWORD attrib=::GetFileAttributesA(name);
	// check for cases where write would fail
	if( attrib&FILE_ATTRIBUTE_READONLY ) return true;
	if( attrib&FILE_ATTRIBUTE_DIRECTORY ) return true;
	if( attrib&FILE_ATTRIBUTE_SYSTEM ) return true;
	return false; // no file
#endif
}

#if !POSIX_FILES_COMMON
QFileTime ConvertToQFileTime(const FILETIME &filetime)
{
  /*
  The FILETIME structure is a 64-bit value representing
  the number of 100-nanosecond intervals since January 1, 1601 (UTC).
  
  The time function returns the number of seconds elapsed since midnight (00:00:00),
  January 1, 1970, coordinated universal time (UTC), according to the system clock.
  */

  /*
  SYSTEMTIME startTime;
  startTime.wYear = 1970; startTime.wMonth = 1; startTime.wDay = 1;
  startTime.wDayOfWeek = 0;
  startTime.wHour = 0; startTime.wMinute = 0; startTime.wSecond = 0;
  startTime.wMilliseconds = 0;
  FILETIME jan1970;
  SystemTimeToFileTime(&startTime, &jan1970);
  __int64 tJan1970 = *((__int64*)&jan1970);
  */
  
  // the code above leads to a constant like below:  
  // FILETIME of 1 Jan 1970 (base of Unix timestamps)
  ULONGLONG tJan1970 = 0x019db1ded53e8000ULL;
  ULONGLONG t100ns = 10000000ULL;
  ULARGE_INTEGER t;
  t.HighPart = filetime.dwHighDateTime;
  t.LowPart = filetime.dwLowDateTime;

  // convert 100 ns to 1 s (nano = 10e-9)
  ULONGLONG sysTime = (t.QuadPart-tJan1970)/t100ns;
  Assert(sysTime<0x100000000ULL);
  return QFileTime(sysTime);
}
FILETIME ConvertToFILETIME(QFileTime time)
{
  ULONGLONG tJan1970 = 0x019db1ded53e8000ULL;
  ULONGLONG t100ns = 10000000ULL;
  // inverse function to ConvertToQFileTime
  ULARGE_INTEGER t;
  t.QuadPart = time*t100ns+tJan1970;
  FILETIME filetime;
  filetime.dwHighDateTime = t.HighPart;
  filetime.dwLowDateTime = t.LowPart;
  Assert(ConvertToQFileTime(filetime)==time);
  return filetime;
}
#endif

QFileTime QIFileFunctions::TimeStamp(const char *name)
{
  // on X360 we require file timestamps because of shader cache
	FileServerHandle check = GFileServerFunctions->OpenReadHandle(name);
  QFileTime time = GFileServerFunctions->TimeStamp(check);
	GFileServerFunctions->CloseReadHandle(check);
  return time;
}

QFileSize QIFileFunctions::GetFileSize(const char *name)
{
  FileServerHandle check = GFileServerFunctions->OpenReadHandle(name);
  QFileSize size = GFileServerFunctions->GetFileSize(check);
  GFileServerFunctions->CloseReadHandle(check);
  if (size == 0xffffffff) return 0;
  return size;
}

bool QIFileFunctions::FileExists( const char *name )
{
	// check normal file existence
	FileServerHandle check = GFileServerFunctions->OpenReadHandle(name);
  if (!GFileServerFunctions->CheckReadHandle(check)) return false;
	GFileServerFunctions->CloseReadHandle(check);
	return true;
}

#include <Es/Strings/bString.hpp>

bool QIFileFunctions::Unlink(const char *name)
{
#ifdef POSIX_FILES
	LocalPath(fn, name);
	name = fn;
#endif
	GFileServerFunctions->FlushReadHandle(name);
  chmod(name, S_IREAD | S_IWRITE);
	return unlink(name) == 0;
}

bool QIFileFunctions::CleanUpFile(const char *name)
{
	if (!FileExists(name)) return true;
	return Unlink(name);
}

void QIFileFunctions::Copy(const char *srcName, const char *dstName)
{
  if (stricmp(srcName, dstName) == 0) return; // do not copy to itself

  QIFStream in;
  in.open(srcName);
  QOFStream out;
  out.open(dstName);
  in.copy(out);
  out.close();
  in.close();
}


bool QIFileFunctions::DirectoryExists(const char *name)
{
  RString realName;
  realName = name;
	return access(realName, 6) != -1;
}

#define WIN_DIR '\\'
#define UNIX_DIR '/'

#if __GNUC__
	#define INVAL_DIR WIN_DIR
	#define VAL_DIR UNIX_DIR
#else
	#define INVAL_DIR UNIX_DIR
	#define VAL_DIR WIN_DIR
#endif

// helper: open in file or in file bank
static RString ConvertFileName( const char *name, int inval, int valid )
{
	if( !strchr(name,inval) ) return name; // no conversion required
	// convert directory characters depending on platform
	char cname[512];
	strcpy(cname,name);
	for( char *cc=cname; *cc; cc++ )
	{
		if( *cc==inval ) *cc=valid;
	}
	return cname;
}

RString PlatformFileName( const char *name )
{
	return ConvertFileName(name,INVAL_DIR,VAL_DIR);
}

inline RString UniversalFileName( const char *name )
{ // filenames are normally stored with backslash '\\'
	return ConvertFileName(name,UNIX_DIR,WIN_DIR);
}

#ifndef POSIX_FILES
static LSError LSErrorCode( DWORD eCode )
{
  switch( eCode )
  {
  case ERROR_HANDLE_DISK_FULL: return LSDiskFull;
  case ERROR_NETWORK_ACCESS_DENIED:
  case ERROR_LOCK_VIOLATION:
  case ERROR_SHARING_VIOLATION:
  case ERROR_WRITE_PROTECT:
  case ERROR_ACCESS_DENIED: return LSAccessDenied;
  case ERROR_FILE_NOT_FOUND: return LSFileNotFound;
  case ERROR_READ_FAULT:
  case ERROR_WRITE_FAULT:
  case ERROR_CRC: return LSDiskError;
  default:
    {
      char buffer[256];
      FormatMessageA
        (
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, // source
        eCode, // requested message identifier 
        0, // languague
        buffer,sizeof(buffer),
        NULL
        );
      Log("Unknown error %d %s",eCode,buffer);
    }
    return LSUnknownError;
  }
}
#endif

void QOFStream::open( const char *file )
{
  _fail = false;
  _error = LSOK;
  
  RString name = PlatformFileName(file);

  // check if file is not open for reading
  GFileServerFunctions->FlushReadHandle(name);

#if POSIX_FILES
  _file = (int)GFileWriteFunctions->OpenWriteHandle(name);
#else
  _file = GFileWriteFunctions->OpenWriteHandle(name);

  if (_file == INVALID_HANDLE_VALUE)
  {
    _file = NULL;
    _fail = true;
    DWORD eCode = ::GetLastError();
    if (eCode) _error = LSErrorCode(eCode);
  }
#endif

  rewind();
}

void QOFStream::openForAppend( const char *file )
{
  _fail = false;
  _error = LSOK;

  RString name = PlatformFileName(file);

  // check if file is not open for reading
  GFileServerFunctions->FlushReadHandle(name);

#if POSIX_FILES
  _file = (int)GFileWriteFunctions->OpenWriteHandleForAppend(name);
#else
  _file = GFileWriteFunctions->OpenWriteHandleForAppend(name);

  if (_file == INVALID_HANDLE_VALUE)
  {
    _file = NULL;
    _fail = true;
    DWORD eCode = ::GetLastError();
    if (eCode) _error = LSErrorCode(eCode);
  }
#endif

  rewind();
}

/*!
\patch 1.85 Date 9/9/2002 by Jirka
- Fixed: Writing into big file sometimes failed with error #1450:
  "Insufficient system resources exist to complete the requested service."
*/

/*
void QOFStream::close( const void *header, int headerSize )
{
}
*/

void QOFStream::close()
{
  if (!_file) return; // no file

  Flush(0);

#ifdef POSIX_FILES
  int success = ::close(_file);
  if (success < 0)
  {
    _fail = true;
    if (_error == LSOK)
      _error = LSUnknownError;
  }
#else
  BOOL success = ::CloseHandle(_file);
  if (!success)
  {
    _fail = true;
    if (_error == LSOK)
    {
      DWORD eCode = ::GetLastError();
      _error = LSErrorCode(eCode);
    }
  }
#endif
  // forget any data
  rewind();
  _file = NULL;
}

QOFStream::~QOFStream()
{
  close();
}

void QOFStream::Flush(size_t size)
{
  if (_file && !_fail && _bufferSize > 0)
  {
    // open and preload file
#ifdef POSIX_FILES
    const char *data = (const char *)_buf->DataLock(0, _bufferSize);
    int written = ::write(_file, data, _bufferSize);
    _buf->DataUnlock(0, _bufferSize);
    if (written != _bufferSize)
    {
      _fail = true;
      _error = LSUnknownError;
    }
#else
    const char *data = (const char *)_buf->DataLock(0, _bufferSize);
    DWORD written;
    ::WriteFile(_file, data, _bufferSize, &written, NULL);
    _buf->DataUnlock(0, _bufferSize);
    if (written != _bufferSize)
    {
      _fail = true;
      DWORD eCode = ::GetLastError();
      _error=LSErrorCode(eCode);
    }
#endif

    _bufferStart += _bufferOffset;
    if (_bufferSize != _bufferOffset)
    {
      // set pointer to right place
#if POSIX_FILES_COMMON
      if (SetFilePointerPosix(_file, _bufferStart, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
#else
      if (::SetFilePointer(_file, _bufferStart, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
#endif
      {
        _fail = true;
#ifndef POSIX_FILES
        DWORD eCode = ::GetLastError();
        _error=LSErrorCode(eCode);
#endif
      }
    }
  }

  _bufferSize = 0;
  _bufferOffset = 0;
}

void QOFStream::SetFilePos(QFileSize pos)
{
  if (pos >= _bufferStart && pos <= _bufferStart + _bufferSize)
  {
    _bufferOffset = pos - _bufferStart;
  }
  else
  {
    Flush(0);
    Assert(_bufferOffset == 0)
    _bufferStart = pos;
    if (_file && !_fail)
    {
#if POSIX_FILES_COMMON
      if (SetFilePointerPosix(_file, _bufferStart, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
#else
      if (::SetFilePointer(_file, _bufferStart, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
#endif
      {
        _fail = true;
#if defined(_WIN32) && !POSIX_FILES
        DWORD eCode = ::GetLastError();
        _error=LSErrorCode(eCode);
#endif
      }
    }
  }
}

DEFINE_FAST_ALLOCATOR(QIStreamBuffer)

QIStreamBuffer::QIStreamBuffer(QFileSize size)
{
	_bufferSize = size;
  _buffer = new char[size];
}

/*
QIStreamBuffer::QIStreamBuffer(const void *data, int size)
{
	_bufferSize = size;
  _buffer = new char[size];
  memcpy(_buffer,data,size);
}
*/

void QIStreamBuffer::Realloc(size_t size, size_t used)
{
  if (_bufferSize==size) return;
  if (used>0)
  {
    Assert(_bufferSize>=(QFileSize)used);
    char *oldBuffer = _buffer;
    _buffer = new char[size];
    memcpy(_buffer,oldBuffer,used);
    delete oldBuffer;
  }
  else
  {
    if (_buffer) delete _buffer;
    _buffer = new char[size];
  }
  _bufferSize = size;
}

QIStreamBuffer::QIStreamBuffer()
{
  // create an empty buffer
  _bufferSize = 0; // default buffer size - one XBox/PCx86 system page
  _buffer = NULL;
}
QIStreamBuffer::~QIStreamBuffer()
{
  if (_buffer)
  {
    delete[] _buffer;
    _buffer = NULL;
  }
}


QIStream::QIStream()
//_buf(new QIStreamBuffer,0,0)
:_buf(NULL,0,0)
{
  // I do not see any reason why buffer should be allocated before reading starts
  // it would be discarded in ReadBuffer anyway
	_readFromBuf = 0;

	_fail = false;
  _eof = false;
	_error = LSOK;
}

QIStream::QIStream(enum _noBuffer)
:_buf(NULL,0,0)
{
  _readFromBuf = 0;

  _fail = false;
  _eof = false;
  _error = LSOK;
}

QIStream::~QIStream()
{
  // make sure buffer was released by the implementation
  Assert(_buf==NULL);
}


bool QIStream::nextLine ()
{
  for(;;)
  {
    int c1 = get();
    if (c1==EOF) return false;
    if ( c1== 0x0D || c1 == 0x0A )
    {
      if (c1 == 0x0D )
      {
        int c2 = get();
        if ( c2 != 0x0A && c2!=EOF ) unget();
      }
      return true;
    }
  }
}

bool QIStream::readLine ( char *buf, int bufLen )
{
  Assert( buf );
  int left = bufLen - 1;                  // regular chars to read
  for(;;)
  {
    int c1 = get();
    if (c1==EOF)
    {
      *buf = (char)0;
      return false;                           // EOF reached before EOLN
    }
    if ( c1== 0x0D || c1 == 0x0A )
    {
      // EOLN:
      if ( c1 == 0x0D )
      {
        int c2 = get();
        if ( c2 != 0x0A && c2!=EOF ) unget();
      }
      *buf = (char)0;
      return true;
    }
    // regular char:
    if ( bufLen > 0 && left == 0 )
    {    // buffer overflow
      *buf = (char)0;
      return nextLine();
    }
    *buf++ = c1;
    left--;
  }
}

#include <Es/Memory/normalNew.hpp>
/// a buffer style refrence to existing data
class QIStreamBufferTemp: public QIStreamBuffer
{
  public:
  QIStreamBufferTemp(const void *data, int size);
  ~QIStreamBufferTemp();

  USE_FAST_ALLOCATOR
};
#include <Es/Memory/debugNew.hpp>

DEFINE_FAST_ALLOCATOR(QIStreamBufferTemp)

QIStreamBufferTemp::QIStreamBufferTemp(const void *data, int size)
{
  _buffer = (char *)data;
  _bufferSize = size;
}
QIStreamBufferTemp::~QIStreamBufferTemp()
{
  _buffer = NULL;
  _bufferSize = 0;
}

/**
caution: data are not copied, only referenced - they must exist in proper scope
*/

void QIStrStream::init(const void *data, int size)
{
  // data are touched by ReadData, but our ReadData will not touch them
  _buf = PosQIStreamBuffer(new QIStreamBufferTemp(data,size),0,size);
  _readFromBuf = 0;
}

void QIStrStream::init(QOStrStream &stream)
{
  _buf = stream.GetBuffer();
}


QIStrStream::~QIStrStream()
{
  _buf = PosQIStreamBuffer(NULL,0,0);
}

void QIStreamDataSource::Copy(const QIStreamDataSource &src)
{
  _buf = src._buf;
  _source = src._source;

	_readFromBuf = src._readFromBuf;

	_fail = src._fail;
  _eof = src._eof;
	_error = src._error;
}

QIStreamDataSource &QIStreamDataSource::operator = ( const QIStreamDataSource &src )
{
  Copy(src);
  return *this;
}
QIStreamDataSource::QIStreamDataSource( const QIStreamDataSource &src )
{
  Copy(src);
}


QIStreamDataSource::QIStreamDataSource()
:QIStream(NoBuffer) // no need to allocate buffer until reading will start
{
}
QIStreamDataSource::~QIStreamDataSource()
{
  _buf = PosQIStreamBuffer(NULL,0,0);
}

PosQIStreamBuffer QIStreamDataSource::ReadBuffer(PosQIStreamBuffer &buf, QFileSize pos)
{
  return _source->ReadBuffer(_request,buf,pos);
}


void QIStreamDataSource::OpenBuffer(IFileBuffer *whole, QFileSize offset, QFileSize size)
{
	Assert(!GetSource());
  _error=LSUnknownError;
	InvalidateBuffer();
	_buf._start = 0;
	_readFromBuf = 0;

	if (!whole || whole->GetError())
	{
		AttachSource(new FileBufferError);
		_fail = true;
	}
	else
	{
    AttachSource( IFileBuffer::SubBuffer(whole,offset,size) );
    _fail = false;
    _error = LSOK;
	}
		}


bool QIStreamDataSource::PreloadBuffer(
  RequestableObject *obj, RequestableObject::RequestContext *ctx,
  FileRequestPriority priority, QFileSize pos, QFileSize size
) const
{
  if (!_source) return true;
  return _source->PreloadBuffer(_request,obj,ctx,priority,pos,size);
}

void QIStreamDataSource::PreloadSequential(QFileSize size) const
{
  if (!_source) return;
  QFileSize pos = tellg();
  // first of all test if the current request contains the data. If it does, we are done
  if (_source->CheckIfRequestContains(_request,pos,size))
  {
    return;
  }
  _source->PreloadSequential(pos,size);
}

void QIFStream::open(const char *dta, QFileSize offset, QFileSize size)
{
  Ref<IFileBuffer> source = OpenSource(dta,offset,size);
	OpenBuffer(source);
}

Ref<IFileBuffer> QIFStream::OpenSource(const char *name, QFileSize offset, QFileSize size)
{
	// open file handle
	IFileBuffer *buf = new FileBufferLoading(name);
	return IFileBuffer::SubBuffer(buf, offset, size);
}

/// Functor converting Unicode stream to UTF-8 stream
class FileWideCharToMultiByte
{
protected:
  QOStream &_out;
  bool _revert;

public:
  FileWideCharToMultiByte(QOStream &out, bool revert) : _out(out), _revert(revert) {}

  void operator ()(char *buf, size_t size)
  {
    if (_revert)
    {
      // fix the endians
      for (size_t i=0; i<size; i+=2) {char c = buf[i]; buf[i] = buf[i + 1]; buf[i + 1] = c;}
    }
    WCHAR *src = (WCHAR *)(buf);
    size_t srcLen = size / sizeof(WCHAR);
    // check the size of the needed buffer
    size_t dstLen = WideCharToMultiByte(CP_UTF8, 0, src, srcLen, NULL, 0, NULL, NULL);
    // allocate the buffer, convert and write to the stream
    Buffer<char> dst(dstLen);
    WideCharToMultiByte(CP_UTF8, 0, src, srcLen, dst.Data(), dstLen, NULL, NULL);
    _out.write(dst.Data(), dstLen);
  }
};

//TODO: Linux changes in archived ArmA
// [Look into IsUnicode implamentation and usage]
QIStream *HandleUnicode::SelectStream(QIStrStream *stream)
{
  int c1 = _source.get();
  if (_source.eof())
  {
    _source.seekg(0);
    return &_source; // too short source
  }
  int c2 = _source.get();
  if (_source.eof())
  {
    _source.seekg(0);
    return &_source; // too short source
  }

  if (c1 == 0xff && c2 == 0xfe)
  {
    // standard Unicode
#if _M_PPC
    bool revert = true;
#else
    bool revert = false;
#endif
    // convert file to UTF-8
    QOStrStream out;
    FileWideCharToMultiByte func(out, revert);
    _source.Process(func);

    // use the output as an input stream
    if (!stream) stream = new QIStrStream();
    stream->init(out);
    return stream;
  }
  if (c1 == 0xfe && c2 == 0xff)
  {
    // reverted Unicode
#if _M_PPC
    bool revert = false;
#else
    bool revert = true;
#endif
    // convert file to UTF-8
    QOStrStream out;
    FileWideCharToMultiByte func(out, revert);
    _source.Process(func);

    // use the output as an input stream
    if (!stream) stream = new QIStrStream();
    stream->init(out);
    return stream;
  }
  if (c1 == 0xef && c2 == 0xbb)
  {
    int c3 = _source.get();
    if (!_source.eof() && c3 == 0xbf) return &_source; // skip UTF-8 prefix
  }
  _source.seekg(0);
  return &_source; // no known prefix
}

QIPreprocessedStream::QIPreprocessedStream(QIStream *stream)
: _sourceBuf(NULL, 0, 0)
{
  _stream = stream;
  if (stream) _sourceBuf = stream->_buf;
  _nlRequest = false;
}

QIPreprocessedStream::~QIPreprocessedStream()
{
  _buf = PosQIStreamBuffer(NULL, 0, 0);
  _sourceBuf = PosQIStreamBuffer(NULL, 0, 0);
}

QIPreprocessedStream *QIPreprocessedStream::Clone()
{
  Fail("Not tested");
  QIPreprocessedStream *stream = new QIPreprocessedStream(_stream ? _stream->Clone() : NULL);  
  // can we copy source stream buffers? Clone might have created new buffers
  stream->_buf = _buf;
  stream->_sourceBuf = _sourceBuf;
  stream->_readFromBuf = _readFromBuf;
  stream->_fail = _fail;
  stream->_eof = _eof;
  stream->_error = _error;
  stream->_location = _location;
  stream->_nlRequest = _nlRequest;
  return stream;
}

PosQIStreamBuffer QIPreprocessedStream::ReadBuffer(PosQIStreamBuffer &buf, QFileSize pos)
{
  if (!_stream) return PosQIStreamBuffer(new QIStreamBuffer, pos, 0);
  if (pos != buf._start + buf._len)
  {
    // only sequential access is enabled
    return PosQIStreamBuffer(new QIStreamBuffer, pos, 0);
  }

  if (_nlRequest)
  {
    _location.line++;
    _nlRequest = false;
  }

  Assert(_sourceBuf._start + _sourceBuf._len >= pos);
  QFileSize len = Convert64ToQFileSize(_sourceBuf._start + _sourceBuf._len - pos);
  if (len <= 0)
  {
    // we are out of source buffer, read next page
    _sourceBuf = _stream->ReadBuffer(_sourceBuf, pos);
    Assert(_sourceBuf._start + _sourceBuf._len >= pos);
    len = Convert64ToQFileSize(_sourceBuf._start + _sourceBuf._len - pos);
  }


  QFileSize offset = Convert64ToQFileSize(pos - _sourceBuf._start);
  for (QFileSize i=0; i<len; i++)
  {
    if (_sourceBuf->_buffer[offset + i] == '\n')
    {
      // return single line only
      _nlRequest = true;
      return PosQIStreamBuffer(_sourceBuf, _sourceBuf._start, offset + i + 1);
    }
  }

  // return the rest of buffer
  return PosQIStreamBuffer(_sourceBuf, _sourceBuf._start, offset + len);
}

bool QIPreprocessedStream::PreloadBuffer(
  RequestableObject *obj, RequestableObject::RequestContext *ctx,
  FileRequestPriority priority, QFileSize pos, QFileSize size
  ) const
{
  if (!_stream) return true;
  return _stream->PreloadBuffer(obj, ctx, priority, pos, size);
}

QFileSize QIPreprocessedStream::GetDataSize() const
{
  if (!_stream) return 0;
  return _stream->GetDataSize();
}

void RequestableObject::RequestDone(RequestContext *context, RequestResult result)
{}

