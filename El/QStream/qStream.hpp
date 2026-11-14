#ifdef _MSC_VER
#pragma once
#endif

#ifndef _QSTREAM_HPP
#define _QSTREAM_HPP

#include <Es/Memory/checkMem.hpp>
#include <Es/Strings/rString.hpp>
#include <Es/Memory/fastAlloc.hpp>
#include <Es/Types/enum_decl.hpp>
#include <Es/Framework/debugLog.hpp>
#include <El/ActiveObject/activeObject.hpp>

#ifndef DECL_ENUM_LSERROR
#define DECL_ENUM_LSERROR
DECL_ENUM(LSError)
#endif
DEFINE_ENUM_BEG(LSError)
  LSOK,
  LSFileNotFound, // no such file
  LSBadFile, // error in loaded file (CRC error...)
  LSStructure, // fire structure error - caused by programm bug
  LSUnsupportedFormat, // attempt to load other file format
  LSVersionTooNew, // attempt to load unknown version
  LSVersionTooOld, // attempt to load version that is no longer supported
  LSDiskFull, // cannot save - disk full
  LSAccessDenied, // read only, directory permiss...
  LSDiskError, // some disk error
  LSNoEntry,    // entry in ParamArchive not found
  LSNoAddOn,    // ADDED in patch 1.01 - AddOns check
  LSUnknownError,
DEFINE_ENUM_END(LSError)

typedef size_t QFileSize;
typedef __int64 QFileSize64;
//const QFileSize64 QFileSizeMax = 10000000000i64; //10GB
const QFileSize QFileSizeMax = 0xfffffffe; //cca 4GB

inline QFileSize Convert64ToQFileSize(QFileSize64 value)
{
  Assert(QFileSize(value)==value);
  return QFileSize(value);
}

const char *GetErrorName(LSError err);

class QIOS
{
  public:
  // namespace for definitions
  enum {beg,cur,end};
  enum {binary=1,text=2,in=4};
  typedef int seekdir;
  typedef int openmode;
};

// istream like simple and fast implementaion of file access
class QIStreamSimple
{
  protected:
  char *_buf;
  int _len;
  int _readFrom;
  bool _fail,_eof;
  LSError _error;

  protected:
  void Assign( const QIStreamSimple &src ); // this points to same data as from
  void Close();

  private: // disable copying
  QIStreamSimple &operator = ( const QIStreamSimple &src );
  QIStreamSimple( const QIStreamSimple &src );

  public:
  QIStreamSimple()
  :_buf(NULL),_len(0),_readFrom(0),_fail(true),_eof(false)
  {}
  QIStreamSimple( const void *buf, int len )
  :_buf((char *)buf),_len(len),_readFrom(0),_fail(false),_eof(false)
  {}
  void init( const void *buf, int len )
  {
    _buf=(char *)buf;
    _len=len;
    _readFrom=0;
    _fail=false;
    _eof=false;
  }
  /// get single character
  int get()
  {
    
    if( _readFrom>=_len ) {_eof=true;return EOF;}
    return (unsigned char)_buf[_readFrom++];
  }
  /// unget - may be called only after a get, and only once
  void unget()
  {
    if( _readFrom>0 ) _readFrom--;
  }
  // read binary data
  void read( void *buf, int n )
  {
    int left=_len-_readFrom;
    if( n>left )
    {
      if( left==0 ) _eof=true;
      _fail=true;
      return;
    }
    // note: buf and _buf may be unalligned - we cannot avoid this
    memcpy(buf,_buf+_readFrom,n);
    //for( int i=0; i<n; i++ ) buf[i]=_buf[_readFrom+i];
    _readFrom+=n;
  }

  void seekg( int pos, QIOS::seekdir dir )
  {
    int nPos;
    switch( dir )
    {
      case QIOS::beg:
        nPos=pos;
      break;
      case QIOS::end:
        nPos=_len+pos;
      break;
      default:
        nPos=_readFrom+pos;
      break;
    }
    if( nPos<0 || nPos>_len ) _fail=true;
    else _readFrom=nPos,_fail=false;
  }
  bool fail() const {return _fail;}
  bool eof() const {return _eof;}
  LSError error() const {return _error;}

  //! Reads the stream until EOLN (returns true) or EOF (returns false) is reached.
  //! Stops AFTER the EOLN.
  bool nextLine ();

  /**
      Reads the actual line into the supplied buffer.
      @param  buf External buffer.
      @param  bufLen Size of the buffer (including \0 terminator), 0 for unrestricted read.
              Too long lines will be truncated to fit in <code>buffer</code>.
      @return <code>true</code> if the line was read successfully.
  */
  bool readLine ( char *buf, int bufLen =0 );

  int tellg() const {return _readFrom;}

  // caution: following functions are not available in istream
  // use them with care
  const char *act() const {return _buf+_readFrom;}
  int rest() const {return _len-_readFrom;}
};

#include <Es/Memory/normalNew.hpp>

/// besides of counting instances, we also track all participating instances in a double linked list

template <class Type>
struct CountInstancesEx: public TLinkBidirD, public CountInstances<Type>
{
  /*
  Caution: from http://www.virtualdub.org/blog/pivot/entry.php?id=120
  There is a nasty bug with the #list and #tree statements you should be aware of: they do not work if the nodes exist
  in static storage in a module. The traversal will work OK, but the $e variable will point to an invalid offset
  instead of the actual item, and you won't be able to see any of the elements. I got burned by this when I tried
  to write a visualizer to dump the list of critical sections in the process, which turns out to reside in a static
  block in ntdll.dll, and I don't have a workaround yet. This bug does not affect the #array primitive.
  */
  static TListBidir<Type> _list;
  
  public:
  CountInstancesEx() {_list.Insert(static_cast<Type *>(this));}
  CountInstancesEx(const CountInstances<Type> &src) {_list.Insert(static_cast<Type *>(this));}
  ~CountInstancesEx(){_list.Delete(static_cast<Type *>(this));}
};

template <class Type>
TListBidir<Type> CountInstancesEx<Type>::_list;

//! one part of data loaded
/*!
This class makes sharing of buffers between various streams and caches possible
*/
class QIStreamBuffer: public RefCount
{
  friend class QIStream;
  friend class QIPreprocessedStream;

  protected:
  char *_buffer; //! data buffer
  QFileSize _bufferSize; // data buffer size

  public:
  QIStreamBuffer();
  //QIStreamBuffer(const void *data, int size);
  explicit QIStreamBuffer(QFileSize size);

  virtual ~QIStreamBuffer();

  void *DataLock(QFileSize pos, QFileSize size)
  {
    Assert(pos+size<=_bufferSize);
    //_bufLen = size;
    return _buffer+pos;
  }
  void DataUnlock(QFileSize pos, QFileSize size)
  {
    Assert(pos+size<=_bufferSize);
  }
  void FillWithData(const void *data, QFileSize size)
  {
    Assert(size<=_bufferSize);
    memcpy(_buffer,data,size);
  }
  QFileSize ReadData(void *data, QFileSize pos, QFileSize size)
  {
    Assert(pos+size<=_bufferSize);
    memcpy(data,_buffer+pos,size);
    return size;
  }
  __forceinline QFileSize GetSize() const {return _bufferSize;}
  //! change size, but copy "used" data 
  void Realloc(size_t size, size_t used);

  USE_FAST_ALLOCATOR

  private:
  //! no copy
  QIStreamBuffer(const QIStreamBuffer &src);
  void operator = (const QIStreamBuffer &src);
};

//! buffer with which we work
struct PosQIStreamBuffer: public Ref<QIStreamBuffer>
{
  QFileSize64 _start; //!< position of the buffer in source stream
  QFileSize _len; //!< position of the buffer in source stream
  
  PosQIStreamBuffer(QIStreamBuffer *ref, QFileSize64 pos, QFileSize size)
  :Ref<QIStreamBuffer>(ref),_start(pos),_len(size)
  {
  }
  PosQIStreamBuffer():Ref<QIStreamBuffer>(NULL),_start(0),_len(0){}
};

#include <Es/Types/lLinks.hpp>

/// object which can be requested for preloading
class RequestableObject: public RemoveLLinks // TODO: public SLIST_ENTRY
{
  public:
  class RequestContext
  {
  public:
    RequestContext() {}
    virtual ~RequestContext() {}
  };

  /// result of file loading request
  enum RequestResult
  {
    RRSuccess,
    RRFailed,
    RRCanceled
  };
  
  /// thread ID of the thread to process the request once done
  /** -1 means any thread, currently implemented as file server worker thread executes them */
  virtual int ProcessingThreadId() const = 0;
  /// called when loading request was finished
  virtual void RequestDone(RequestContext *context, RequestResult result) = 0;
  /// most request can be processed only in a defined callplace inside of FileServer
  virtual bool ProcessImmediately() const {return false;}
  #if DEBUG_PRELOADING
  virtual bool DebugMe() const { return false; }
  #endif
};


//! hide OS specific handle types

#if POSIX_FILES_COMMON
#define POSIX_FILES 1
#endif

// TODO: this seems to be used on Posix files too, as FileServer is full of this struct usage and it is converted to POSIX now
#if defined(POSIX_FILES) && !defined(_WIN32) && 0
typedef int FileServerHandle;
typedef int FileWriteHandle;
#else
typedef struct _fileServerHandle{} *FileServerHandle;
typedef struct _fileWriteHandle{} *FileWriteHandle;
#endif

//! provide type safety for file requests, but hide details
struct FileReqRef: public RefCount
{
};


//! provide type safety for persistent request storage, but hide details
struct FileRequestCompletionRefCount: public RefCount
{

};

typedef RefR<FileReqRef> FileRequestHandle;

typedef RefR<FileRequestCompletionRefCount> FileRequestCompletionHandle;

/// priority of the file request
/**
lower value = higher priotity
request with priority above 500 are considered non-urgent and may be reordered significantly.
*/
enum FileRequestPriority
{
};

/// timestamp type
typedef unsigned long QFileTime;

QFileTime ConvertToQFileTime(const FILETIME &filetime);
FILETIME ConvertToFILETIME(QFileTime time);

class IFileBuffer;

class QIFileWriteFunctions
{
  public:
  //! open write handle
  virtual FileWriteHandle OpenWriteHandle(const char *name) = 0;
  //! open write handle for append to existing file (or create new)
  virtual FileWriteHandle OpenWriteHandleForAppend(const char *name) = 0;
};

struct InFileLocation
{
  FileServerHandle _handle;
  QFileSize _start;
  QFileSize _size;
  
  bool Contains(FileServerHandle handle, QFileSize pos, QFileSize size) const
  {
    return handle==_handle && pos>=_start && pos+size<=_start+size;
  }
  bool Contains(const InFileLocation &loc) const
  {
    return Contains(loc._handle,loc._start,loc._size);
  }
};

//! interface with the file server
// In basic implementation, FileServerHandle is interpreted as HANDLE
class QIFileServerFunctions
{
  public:

  //! release handle - we need to write the file
  virtual bool FlushReadHandle(const char *name) = 0;
  //! request preloading
  virtual bool Preload(
    FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, FileServerHandle handle, QFileSize start, QFileSize size
  ) = 0;
  //! read single buffer
  virtual PosQIStreamBuffer Read(FileServerHandle handle, QFileSize start, QFileSize size) = 0;

  /// use Preload result if possible
  virtual PosQIStreamBuffer RequestResultAsync(FileRequestCompletionHandle &request, FileServerHandle handle, QFileSize pos) = 0;
  /// get area covered by the request
  virtual InFileLocation RequestLocationAsync(const FileRequestCompletionHandle &request) = 0;
  
  //! open read handle
  virtual FileServerHandle OpenReadHandle(const char *name) = 0;
  /// mark handle as permanent - no need to check for changed file
  virtual void MakeReadHandlePermanent(FileServerHandle handle) = 0;
  //! check filename attached with given handle
  virtual RString GetHandleName(FileServerHandle handle) const = 0;
  //! close read handle
  virtual void CloseReadHandle(FileServerHandle handle) = 0;
  //! check if handle was opened OK
  virtual bool CheckReadHandle(FileServerHandle handle) const = 0;
  //! get page size used for file buffers
  virtual int GetPageSize() = 0;
  //! file timestamp - time when file was modified
  virtual QFileTime TimeStamp(FileServerHandle handle) const = 0;
  //! file timestamp - time when file was modified - async
  virtual Future<QFileTime> RequestTimeStamp(const char *name) = 0;
  //! size of file
  virtual QFileSize GetFileSize(FileServerHandle handle) const = 0;
#ifdef _WIN32
  //! extract windows file handle from file server handle
  virtual void *GetWinHandle(FileServerHandle handle) const = 0;
#endif
  /// check if threads other than the main one can use it
  virtual bool OtherThreadsCanUse() const = 0;
};

extern QIFileServerFunctions *GFileServerFunctions;
extern QIFileWriteFunctions *GFileWriteFunctions;

#include <Es/Memory/debugNew.hpp>

enum _noBuffer {NoBuffer};

struct FileLocation
{
  RString fileName;
  int line;

  FileLocation() {line = 1;}
};

/// describe file endian (big, little)
enum QFileEndian
{
  EndianUnknown,
  EndianBig,
  EndianLittle,
};

//! istream like simple and fast implementation of file access
class QIStream: private NoCopy
{
  friend class QIPreprocessedStream;
  
  public:

  protected:
  PosQIStreamBuffer _buf;
  
  //! position of current read in the buffer - relative to the buffer start
  /*!
  must always be in range 0.._buffer.Size(), both included.
  If _bufLen is nonzero, it must also be in range 0.._bufLen (both included)
  */

  QFileSize _readFromBuf;

  bool _fail,_eof;
  LSError _error;

  protected:
  //! source can read more, but it always has to read at least one byte on pos
  virtual PosQIStreamBuffer ReadBuffer(PosQIStreamBuffer &buf, QFileSize pos) = 0; 

  //! reader can advertise it will be reading
  /*!
  \return true when data are as ready as possible
  */
  virtual bool PreloadBuffer(
    RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
  ) const = 0;
  /// reader can advertise sequential read
  virtual void PreloadSequential(QFileSize size) const {}
  //! get total data size
  virtual QFileSize GetDataSize() const = 0;

  //! forget all information from the buffer, maintain read positions
  void InvalidateBuffer()
  {
    _buf._len = 0;
    _buf.Free();
  }

  QIStream();
  QIStream(enum _noBuffer);

  /// get helper - handle complex case
  bool get_underrun()
  {
    Assert(_readFromBuf==_buf._len)
    // skip current buffer
    QFileSize actPos = Convert64ToQFileSize(_buf._start+_readFromBuf);

    _buf = ReadBuffer(_buf,actPos);

    _readFromBuf = Convert64ToQFileSize(actPos-_buf._start);

    Assert(_readFromBuf<=_buf._len);
    if( _readFromBuf>=_buf._len )
    {
      // no data to return - return error (eof)
      _eof = true;
      return false;
    }
    return true;
  }

  public:
  virtual ~QIStream();

  virtual QIStream *Clone() = 0;
  
  /// read one character - should be simple and fast
  __forceinline int get()
  {
    // get single character
    if( _readFromBuf>=_buf._len )
    {
      if (!get_underrun())
      {
        return EOF;
      }
    }
    return (unsigned char)_buf->_buffer[_readFromBuf++];
  }
  void unget()
  {
    Assert(_readFromBuf>0);
    if( _readFromBuf>0 ) _readFromBuf--;
  }
  bool PreReadObject(
    RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority=FileRequestPriority(1), QFileSize offset=0, QFileSize size=QFileSizeMax
  )
  {
    return PreloadBuffer(obj,ctx,priority,offset,size);
  }
  /// pre-read part of the file, report if data are ready
  bool PreRead( FileRequestPriority priority=FileRequestPriority(1), QFileSize offset=0, QFileSize size=QFileSizeMax )
  {
    return PreloadBuffer(NULL,NULL,priority,offset,size);
  }
  /// before reading large sequential data it may be beneficial to pre-read them
  /** This avoids performing a lot of small reads */
  void PreReadSequential(QFileSize size=QFileSizeMax)
  {
    PreloadSequential(size);
  }
  
  size_t GetNextBuffer(size_t left, size_t rd) 
  {
    // skip current buffer
    Assert(_readFromBuf==_buf._len);
    QFileSize actPos = Convert64ToQFileSize(_buf._start+_readFromBuf);

    _buf = ReadBuffer(_buf,actPos);

    _readFromBuf = Convert64ToQFileSize(actPos-_buf._start);
    Assert(_readFromBuf<=_buf._len);

    left = _buf._len-_readFromBuf;
    if (left<=0)
    {
      _eof = true;
      if (rd>0) _fail = true;
    }
    return left;
  }
  
  template <class Functor>
  size_t readCustomCopy(void *buf, size_t n, Functor copy)
  {
    size_t rd = 0;
    while (n>0)
    {
      size_t toRead = n;
      Assert(_readFromBuf<=_buf._len);
      size_t left = _buf._len-_readFromBuf;
      if (left<=0)
      {
        left = GetNextBuffer(left,rd);
        if (left<=0)
        { // avoid leaving uninitialized memory
          static const int restData[]={-1,-1,-1,-1};
          while (n>0)
          {
            size_t step = std::min(n,sizeof(restData));
            copy(buf,restData,step);
            n -= step;
            buf = (char *)buf + step;
      }
          return rd;
        }
      }
      if (toRead>left) toRead = left;
      // read what we can
      copy(buf,_buf->_buffer+_readFromBuf,toRead);
      rd += toRead;
      _readFromBuf += toRead;
      
      Assert(_readFromBuf<=_buf._len);
      
      n -= toRead;
      // we need to continue - advance all data pointers
      // advance target pointer
      buf = (char *)buf + toRead;
    }
    return rd;
  }
  size_t read( void *buf, size_t n )
  {
    size_t rd = 0;
    while(n>0)
    {
      size_t toRead = n;
      Assert(_readFromBuf<=_buf._len);
      size_t left = _buf._len-_readFromBuf;
      if (left<=0)
      {
        left = GetNextBuffer(left,rd);
        if (left<=0)
        { // avoid leaving uninitialized memory
          memset(buf,-1,n);
          return rd;
      }
      }
      if (toRead>left) toRead = left;
      // read what we can
      memcpy(buf,_buf->_buffer+_readFromBuf,toRead);
      rd += toRead;
      _readFromBuf += toRead;
      
      Assert(_readFromBuf<=_buf._len);
      
      n -= toRead;
      // we need to continue - advance all data pointers
      // advance target pointer
      buf = (char *)buf + toRead;
    }
    return rd;
  }
  
  /// read "short" data, with compile time known size
  template <size_t size>
  void readShort(void *buf)
  {
    char *cBuf = reinterpret_cast<char *>(buf);
    size_t n = size;
    size_t rd = 0;
    while (n>0)
    {
      size_t toRead = n;
      Assert(_readFromBuf<=_buf._len);
      size_t left = _buf._len-_readFromBuf;
      if (left<=0)
      {
        left = GetNextBuffer(left,rd);
        if (left<=0)
        { // avoid leaving uninitialized memory
          memset(cBuf,-1,n);
          return;
      }
      }
      if (toRead>left) toRead = left;
      // read what we can
      // simple manual implementation of memcpy, including advancing the target pointer

      {
        const char *src = _buf->_buffer+_readFromBuf;
        // compiler should convert this to rep movsd
        for (int r=(int)toRead; --r>=0; ) *cBuf++ = *src++;
      }
      _readFromBuf += toRead;
      Assert(_readFromBuf<=_buf._len);
      
      n -= toRead;
      rd += toRead;
    }
  }

  /// seek - absolute position
  /**
  Performs the same function as seekg(pos,QIOS::beg), but pos is unsigned and can be full range
  */
  void seekg(QFileSize pos)
  {
    QFileSize size = GetDataSize();
    if (pos<0) pos=0;
    if (pos>size) pos = size;
    // check if we are withing current buffer
    if (pos>=_buf._start && pos<=_buf._start+_buf._len)
    {
      // adjust position within the current buffer
      _readFromBuf = Convert64ToQFileSize(pos - _buf._start);
      Assert(_readFromBuf<=_buf._len);
    }
    else
    {
      // buffer needs to be invalidated
      // try to keep aligned to buffer size
      _buf._len = 0;
      _buf._start = pos;
      _readFromBuf = 0;
      _fail = false;
      _eof = false;
    }
  }
  /// seek - absolute or relative position
  void seekg( int pos, QIOS::seekdir dir )
  {
    switch( dir )
    {
      case QIOS::beg: seekg(pos); break;
      case QIOS::end: seekg(GetDataSize()+pos); break;
      default: seekg(Convert64ToQFileSize(_buf._start+_readFromBuf+pos)); break;
    }
  }
  bool fail() const {return _fail;}
  //it is hard to provide eof() with the same semantics as seen in the QIStreamSimple class
  bool eof() const {return _eof;}
  LSError error() const {return _error;}

  //! Reads the stream until EOLN (returns true) or EOF (returns false) is reached.
  //! Stops AFTER the EOLN.
  bool nextLine ();

  /**
      Reads the actual line into the supplied buffer.
      @param  buf External buffer.
      @param  bufLen Size of the buffer (including \0 terminator), 0 for unrestricted read.
              Too long lines will be truncated to fit in <code>buffer</code>.
      @return <code>true</code> if the line was read successfully.
  */
  bool readLine ( char *buf, int bufLen =0 );

  // read any type as binary data
  template <class Type>
  __forceinline void readBinary(Type &val)
  {
    readShort<sizeof(val)>(&val);
  }

  template <class Type>
  __forceinline Type readBinaryVal()
  {
    Type val;
    readShort<sizeof(val)>(&val);
    return val;
  }
  
  //@{ get little endian data on any platform
  /// get Intel 24b integer
  /** No raw data type available for this - always read using char */
  int geti24()
  {
    unsigned char d[3];
    readShort<sizeof(d)>(d);
    return ((unsigned)d[0])|(((unsigned)d[1])<<8)|(((unsigned)d[2])<<16);
  }

#if _M_PPC
  /// get Intel word (16b integer)
  int getiw()
  {
    unsigned char d[2];
    readShort<sizeof(d)>(d);
    return ((unsigned)d[0])|(((unsigned)d[1])<<8);
  }
  /// get Intel long (32b integer)
  long getil()
  {
    unsigned char d[4];
    readShort<sizeof(d)>(d);
    return ((unsigned)d[0])|(((unsigned)d[1])<<8)|(((unsigned)d[2])<<16)|(((unsigned)d[3])<<24);
  }
  long long getill()
  {
    // PPC is 64 platform (at least on X360), following operations are quite fast
    unsigned char d[8];
    readShort<sizeof(d)>(d);
    return (
      ((unsigned)d[0])|(((unsigned)d[1])<<8)|
      (((unsigned)d[2])<<16)|(((unsigned)d[3])<<24)|
      ((unsigned long long)d[0]<<32)|(((unsigned long long)d[1])<<40)|
      (((unsigned long long)d[2])<<48)|(((unsigned long long)d[3])<<56)
    );
  }
#else

  // on little endian platforms we can use raw data loading
  /// get Intel word (16b integer)
  __forceinline int getiw(){return readBinaryVal<unsigned short>();}  
  /// get Intel long (32b integer)
  __forceinline long getil() {return readBinaryVal<long>();}
  /// get Intel float (32b floating point)
  __forceinline float getf() {return readBinaryVal<float>();}
  /// get Intel double (64b floating point)
  __forceinline float getd() {return readBinaryVal<double>();}


# if !defined _MSVC || _MSVC>=1300
  /// get Intel long long (64b integer)
  __forceinline long long getill() {return readBinaryVal<long long>();}
# endif
#endif


  QFileSize tellg() const {return Convert64ToQFileSize(_buf._start+_readFromBuf);}
  QFileSize rest() const {return Convert64ToQFileSize(GetDataSize()-_buf._start-_readFromBuf);}
  
  /// flush the buffer currently allocated, do not change read position
  void flush()
  {
    InvalidateBuffer();
  }
  /// check if buffer is flushed
  bool IsFlushed() const
  {
    return _buf.IsNull() || _buf->GetSize()==0;
  }
  //! copy all data to anything supporting write
  template <class OStr>
  void copy(OStr &out)
  {
    char buf[4*1024];
    for(;;)
    {
      size_t rd = read(buf,sizeof(buf));
      out.write(buf,rd);
      if (eof() || fail()) break;
    }
  }

  //! pass all data to any given function
  template <class Function>
  void Process(Function f)
  {
    char buf[4*1024];
    for(;;)
    {
      size_t rd = read(buf,sizeof(buf));
      f(buf,rd);
      if (eof() || fail()) break;
    }
  }

  // helper functions for QIPreprocessedStream implementation
  virtual FileLocation GetLocation() const {return FileLocation();}
  virtual void SetLocation(RString fileName, int line) {}
};

struct WriteAutoArrayChar
{
  AutoArray<char> &tgt;

  WriteAutoArrayChar(AutoArray<char> &b):tgt(b){}
  void operator () (const void *buf, int size)
  {
    int oldSize = tgt.Size();
    tgt.Resize(oldSize+size);
    memcpy(tgt.Data()+oldSize,buf,size);
  }
};

/*!
\patch_internal 1.31 Date 11/22/2001 by Jirka
- Added: Encryption using RSA
*/
class QOStream
{
  protected:
  Ref<QIStreamBuffer> _buf;
  QFileSize _bufferOffset;
  QFileSize _bufferSize;
  QFileSize _bufferStart;
  QFileSize _written;
  
  private: // disable copying
  QOStream &operator = ( const QOStream &src );
  QOStream( const QOStream &src );

  protected:
  QOStream(enum _noBuffer):_bufferOffset(0),_bufferSize(0),_bufferStart(0),_written(0){}

  public:
  QOStream():_bufferOffset(0),_bufferSize(0),_bufferStart(0),_written(0){_buf = new QIStreamBuffer(64*1024);}

  void rewind() {_written = 0, _bufferOffset = 0, _bufferSize = 0, _bufferStart = 0;}
  void setbuffer( size_t size ) {Flush(size);}

  void put( char c )
  {
    if (_bufferOffset + 1 > (int)_buf->GetSize())
    {
      // realloc needed
      Flush(_buf->GetSize() * 2);
    }
    char *putto = (char *)_buf->DataLock(_bufferOffset, 1);
    *putto = c;
    _buf->DataUnlock(_bufferOffset, 1);
    _bufferOffset++;

    if (_bufferSize < _bufferOffset)
    {
      _bufferSize = _bufferOffset;
      Assert(_bufferSize <= _buf->GetSize());
    }
    if (_written < _bufferStart + _bufferOffset)
    {
      _written = _bufferStart + _bufferOffset;
      Assert(_written <= _bufferStart + _buf->GetSize());
    }
  }
  void write( const void *buf, size_t n )
  {
    while (n > 0)
    {
      if (_bufferOffset + n > _buf->GetSize())
      {
        QFileSize needed = _bufferOffset + n;
        QFileSize size = _buf->GetSize() * 2;
        while (size < needed)
        {
          size *= 2;
        }
        // realloc needed
        Flush(size);
      }
      Assert(_buf->GetSize() >= _bufferOffset)
      size_t m = _buf->GetSize() - _bufferOffset;
      if (n < m) m = n;
      void *putto = _buf->DataLock(_bufferOffset, m);
      memcpy(putto, buf, m);
      _buf->DataUnlock(_bufferOffset, m);
      _bufferOffset += m;
      n -= m;
      buf = (char *)buf + m;
      if (_bufferSize < _bufferOffset)
      {
        _bufferSize = _bufferOffset;
        Assert(_bufferSize <= _buf->GetSize());
      }
    }
    if (_written < _bufferStart + _bufferOffset)
    {
      _written = _bufferStart + _bufferOffset;
      Assert(_written <= _bufferStart + _buf->GetSize());
    }
  }

  QFileSize tellp() const {return _bufferStart + _bufferOffset;}
  
  void seekp( int pos, QIOS::seekdir dir )
  {
    QFileSize nPos;
    switch( dir )
    {
      case QIOS::beg:
        nPos = pos;
      break;
      case QIOS::end:
        nPos = _written + pos;
      break;
      default:
        nPos = _bufferStart + _bufferOffset + pos;
      break;
    }
    SetFilePos(nPos);
  }

  QOStream &operator <<(const char *buf)
  {
    write(buf, strlen(buf));
    return *this;
  }
  
  virtual void close() {}
  virtual bool fail() const {return false;}

protected:
  virtual void Flush(size_t size) = 0;
  virtual void SetFilePos(QFileSize pos) = 0;
};

class QOStrStream : public QOStream
{
public:
  // str and pcount: see ostrstream
  const char *str() const {return (const char *)_buf->DataLock(0,_written);} // get data
  QFileSize pcount() const {return _written;} // get data size

  //! optimal way to access data without any need to copy them
  PosQIStreamBuffer GetBuffer() const
  {
    return PosQIStreamBuffer(_buf,0,_written);
  }

protected:
  virtual void Flush(size_t size)
  {
    _buf->Realloc(size, _written);
  }
  virtual void SetFilePos(QFileSize pos)
  {
    _bufferOffset = pos - _bufferStart;
  }
};

class QOFStream: public QOStream
{
protected:
#ifdef POSIX_FILES
  int _file;
#else
  void *_file;
#endif
  bool _fail;
  LSError _error;

public:
#ifdef POSIX_FILES
  QOFStream():_file(0),_fail(false){}
  QOFStream( const char *file ):_file(0),_fail(false)
  {open(file);}
#else
  QOFStream():_file(NULL),_fail(false){}
  QOFStream( const char *file ):_file(NULL),_fail(false)
  {open(file);}
#endif
  void open( const char *file );
  void openForAppend( const char *file );
  void close();
  // void close( const void *header, int headerSize );
  bool fail() const {return _fail;}
  LSError error() const {return _error;}

  ~QOFStream(); // perform actual save

protected:
  virtual void Flush(size_t size);
  virtual void SetFilePos(QFileSize pos);
};

//#include "multiSync.hpp"

class QFBank;

//! file buffer interface
class IFileBuffer: public RefCount
{
  private:
  //! no copy allowed
  IFileBuffer( const IFileBuffer & );
  //! no copy allowed
  IFileBuffer &operator = ( const IFileBuffer & );

  public:
  IFileBuffer() {}

  virtual bool GetError() const = 0;
  //! get data
  virtual const char *GetData() const = 0;
  //! get data size
  virtual QFileSize GetSize() const = 0;
  //! read part of the file
  /** When returning a new buffer, release the old one */
  virtual PosQIStreamBuffer ReadBuffer(FileRequestCompletionHandle &request, PosQIStreamBuffer &buf, QFileSize pos);
  //! request background loading
  virtual bool PreloadBuffer(
    FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
  ) const = 0;
  //! request sequential read optimization
  virtual void PreloadSequential(QFileSize pos, QFileSize size) const
  {
    // by default there is no optimization
  }
  virtual bool CheckIfRequestContains(FileRequestCompletionHandle &request, QFileSize pos, QFileSize size) {return false;}
  //! read part of the file
  virtual QFileSize Read(void *buf, QFileSize pos, QFileSize size);
  //! check if buffer is from given bank
  virtual bool IsFromBank(QFBank *bank) const = 0;
  //! check if buffer is from given bank
  virtual FileServerHandle GetHandle() const {return NULL;}
  //! some data sources (overlapped IO) may need some time to get ready
  virtual bool IsReady() const = 0;

  static Ref<IFileBuffer> SubBuffer(IFileBuffer *whole, QFileSize offset, QFileSize size);
};

class FileBufferMemory: public IFileBuffer
{
  protected:
  Buffer<char> _data;

  public:
  FileBufferMemory() {}
  FileBufferMemory( int size ) {_data.Init(size);}

  void Realloc( int size ) {_data.Init(size);}
  const char *GetData() const {return _data.Data();}
  // non-virtual writable access
  char *GetWritableData() {return _data.Data();}
  bool PreloadBuffer(
    FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
  ) const
  {
#if _VBS2
    if(obj)
      obj->RequestDone(ctx,RequestableObject::RRSuccess);
#endif

    // data are always ready
    return true;
  }
  
  QFileSize GetSize() const {return _data.Size();}
  bool GetError() const {return false;}
  bool IsFromBank(QFBank *bank) const {return false;}
  bool IsReady() const {return true;}
};

class FileBufferError: public IFileBuffer
{
  public:
  bool GetError() const {return true;}
  //! get data
  const char *GetData() const {return NULL;}
  //! get data size
  QFileSize GetSize() const {return 0;}
  //! read part of the file
  PosQIStreamBuffer ReadBuffer(FileRequestCompletionHandle &request, PosQIStreamBuffer &buf, QFileSize pos)
  {
    buf.Free();
    return PosQIStreamBuffer(new QIStreamBuffer,0,pos);
  }
  bool PreloadBuffer(
    FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
  ) const
  {
    // data are always ready
    return true;
  }
  //! check if buffer is from given bank
  bool IsFromBank(QFBank *bank) const {return false;}
  //! some data sources (overlapped IO) may need some time to get ready
  bool IsReady() const {return true;}
};


#include "iQFBank.hpp"

//! cooperation between file buffers and file banks
class FileBufferBankFunctions
{
  static QFBankFunctions *_defaultQFBankFunctions;

  public:
  /// if possible, flush the bank handle so that the file can be deleted/written
  static bool FlushBankHandle(const char *filename)
  {
    return _defaultQFBankFunctions->FlushBankHandle(filename);
  }
};

//! QIFileFunctions - namespace would be much more logical, but we are still afraid of namespaces
/*!
Caution: if QIFStream was defined as
class QIFStream: public QIStreamDataSource, public QIFileFunctions,
compiler error in MSVC caused bad automatic copy constructor to be generated.
*/
class QIFileFunctions
{
  public:
  static QFileTime TimeStamp( const char *name); //!< get file timestamp
  static bool FileExists( const char *name ); //!< check normal file existence
  static bool FileReadOnly( const char *name ); //!< check if the file can be written or not
  static QFileSize GetFileSize(const char *name); //!< get file size
  static bool Unlink(const char *name); //!< delete file
  static bool CleanUpFile(const char *name); //!< ensure file does not exist
  static bool DirectoryExists(const char *name); //!< check directory existence
  static void Copy(const char *srcName, const char *dstName); //!< copy a file
};

#include <Es/Memory/normalNew.hpp>
class QIStreamBufferPage: public QIStreamBuffer, public CountInstancesEx<QIStreamBufferPage>
{
  public:
  explicit QIStreamBufferPage();
  ~QIStreamBufferPage();

  USE_FAST_ALLOCATOR
};
#include <Es/Memory/debugNew.hpp>

//! load data as necessary
class FileBufferLoading: public IFileBuffer
{

  //@{ define source data name
  FileServerHandle _handle;
  QFileSize _size;
  //@}

  public:
  FileBufferLoading( const char *name);
  ~FileBufferLoading();

  const char *GetData() const;
  QFileSize GetSize() const;
  PosQIStreamBuffer ReadBuffer(FileRequestCompletionHandle &request, PosQIStreamBuffer &buf, QFileSize pos);
  virtual bool PreloadBuffer(
    FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
  ) const;
  virtual void PreloadSequential(QFileSize pos, QFileSize size) const;
  virtual bool CheckIfRequestContains(FileRequestCompletionHandle &request, QFileSize pos, QFileSize size);

  bool GetError() const;
  FileServerHandle GetHandle() const {return _handle;}
  bool IsFromBank(QFBank *bank) const {return false;}
  bool IsReady() const {return true;}
};

//! sub-buffer based on whole buffer
class FileBufferSub: public IFileBuffer
{
  Ref<IFileBuffer> _whole;
  QFileSize _start,_size;

  public:
  FileBufferSub( IFileBuffer *buf, QFileSize start, QFileSize size );

  PosQIStreamBuffer ReadBuffer(FileRequestCompletionHandle &request, PosQIStreamBuffer &buf, QFileSize pos);
  bool PreloadBuffer(
    FileRequestCompletionHandle &request, RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
  ) const;
  virtual void PreloadSequential(QFileSize pos, QFileSize size) const;
  virtual bool CheckIfRequestContains(FileRequestCompletionHandle &request, QFileSize pos, QFileSize size);
  
  const char *GetData() const {return _whole->GetData()+_start;}
  QFileSize GetSize() const {return _size;}
  bool GetError() const {return _whole->GetError();}
  bool IsFromBank(QFBank *bank) const {return _whole->IsFromBank(bank);}
  bool IsReady() const {return true;}
};


class SSCompress
{
  enum {N=4096};
  enum {F=18};
  enum {THRESHOLD=2};

  unsigned char text_buf[N + F - 1];
  int match_position,match_len;
  int lsons[N+1],rsons[N+257],dads[N+1];

  void InitTree();
  void InsertNode( int p );
  void DeleteNode( int p );
  
  public:
  bool Skip(long lensb, QIStream &in );
  /// fastest possible decoding - memory to memory
  bool Decode(char *dst, long lensb, const char *src, int srclen);
  /// stream to memory decoding
  bool Decode( char *dst, long lensb, QIStream &in );
  /// stream to stream decoding
  bool Decode( QOStream &out, long lensb, QIStream &in );
  /// memory to stream encoding
  void Encode( QOStream &out, const char *dst, long lensb );
  /// stream to stream encoding
  void Encode( QOStream &out, QIStream &in);
};

#if defined(POSIX_FILES) || !defined(_WIN32)
int FileSize ( int handle );
#endif

//! fixed buffer implementation
class QIStrStream: public QIStream
{
  protected:
  virtual PosQIStreamBuffer ReadBuffer(PosQIStreamBuffer &buf, QFileSize pos)
  {
    return buf;
    // no more data to read
    //return PosQIStreamBuffer(new QIStreamBuffer,pos,0);
  }
  virtual bool PreloadBuffer(
    RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
  ) const
  {
    // data are always ready
    return true;
  }
  virtual QFileSize GetDataSize() const
  {
    return _buf._len;
  }

  public:
  QIStrStream(){}
  QIStrStream(const void *data, int size)
  {
    init(data,size);
  }
  void init(const void *data, int size);
  void init(QIStreamBuffer *buf)
  {
    _buf = PosQIStreamBuffer(buf,0,buf->GetSize());
  }
  void init(QOStrStream &stream);
  
  ~QIStrStream();

  virtual QIStrStream *Clone()
  {
    QIStrStream *stream = new QIStrStream;
    stream->_buf = _buf;
    stream->_readFromBuf = _readFromBuf;
    stream->_fail = _fail;
    stream->_eof = _eof;
    stream->_error = _error;
    return stream;
  }
};

class QIStreamDataSource: public QIStream
{
  Ref<IFileBuffer> _source;
  /// preload request - used for requests repeated in many successive frames
  mutable FileRequestCompletionHandle _request;
  
  void Copy(const QIStreamDataSource &src);

  protected:
  virtual PosQIStreamBuffer ReadBuffer(PosQIStreamBuffer &buf, QFileSize pos);
  virtual bool PreloadBuffer(
    RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
  ) const;
  virtual void PreloadSequential(QFileSize size) const;

  public:
  QIStreamDataSource();
  ~QIStreamDataSource();

  QIStreamDataSource &operator = ( const QIStreamDataSource &src );
  QIStreamDataSource( const QIStreamDataSource &src );

  void OpenBuffer(IFileBuffer *whole, QFileSize offset=0, QFileSize size=QFileSizeMax);

  virtual QFileSize GetDataSize() const {return _source->GetSize();}

  void AttachSource(IFileBuffer *src)
  {
    InvalidateBuffer();
    _source = src;
    // changing source invalidates the request, as the new source would not understand the old request
    _request.Free();
  }
  
  IFileBuffer *GetSource() const {return _source;}
  bool IsFromBank(QFBank *bank) const {return GetSource()->IsFromBank(bank);}

  QIStreamDataSource *Clone()
  {
    QIStreamDataSource *stream = new QIStreamDataSource;
    stream->Copy(*this);
    return stream;
  }
  
  void close()
  {
    InvalidateBuffer();
    AttachSource(NULL);
  }
};

class QIFStream: public QIStreamDataSource
{
  public:
  static Ref<IFileBuffer> OpenSource(const char *name, QFileSize offset, QFileSize size);
  //! open file 
  void open( const char *name, QFileSize offset=0, QFileSize size=QFileSizeMax );
};

/// Helper functor translating the input stream from Unicode to UTF-8 if needed
class HandleUnicode
{
protected:
  QIStream &_source;
  QIStrStream _converted;

public:
  HandleUnicode(QIStream &source) : _source(source) {}
  QIStream &operator() () {return *SelectStream(&_converted);}
  QIStream *CreateStream() {return SelectStream(NULL);}

protected:
  QIStream *SelectStream(QIStrStream *stream);
};

/// Encapsulation of any QIStream with automatic support for counting lines
/**
- anytime during class lifetime, GetLocation returns actual line number
*/
class QIPreprocessedStream : public QIStream
{
protected:
  /// encapsulated stream 
  SRef<QIStream> _stream;
  /// data read from source stream (only single line at time from them are in _buf)
  PosQIStreamBuffer _sourceBuf;
  
  /// position in file
  FileLocation _location;
  /// during next ReadBuffer, line number will be increased (_buf contains LF on end)
  bool _nlRequest;

public:
  QIPreprocessedStream(QIStream *stream);
  ~QIPreprocessedStream();

  virtual FileLocation GetLocation() const {return _location;}
  virtual void SetLocation(RString fileName, int line) {_location.fileName = fileName; _location.line = line;}

  virtual QIPreprocessedStream *Clone();
  
protected:
  // QIStream interface
  virtual PosQIStreamBuffer ReadBuffer(PosQIStreamBuffer &buf, QFileSize pos); 
  virtual bool PreloadBuffer(
    RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
    ) const;
  virtual QFileSize GetDataSize() const;
};

template <>
__forceinline int QIStream::readBinaryVal<int>()
{
  int val = -1;
  readShort<sizeof(int)>(&val);
  return val;
}

template <>
__forceinline short QIStream::readBinaryVal<short>()
{
  short val = -1;
  readShort<sizeof(short)>(&val);
  return val;
}

#endif

