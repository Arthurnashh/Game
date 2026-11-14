#ifdef _MSC_VER
#pragma once
#endif

#ifndef _QSTDSTREAM_HPP
#define _QSTDSTREAM_HPP

#include <io.h>
#include <stddef.h>
#include <El/QStream/qStream.hpp>

enum
{
  QStdDebug = -1,
  QStdIn = 0,
  QStdOut = 1, QStdErr =2
};


/// stdout & stderr connected stream  - output encapsulation
template <int handle>
struct QStdStreamTraits
{
  static void Output(const char *text, int textSize)
  {
    // by default write to given handle
    write(handle,text,textSize);
  }
};

/// define template specialization 
template <>
struct QStdStreamTraits<QStdDebug>
{
  static void Output(const char *text, int textSize);
};

/// stdout & stderr connected stream
template<class Traits=QStdStreamTraits<QStdOut> >
class QStdStream : public QOStream
{
public:
  
  ~QStdStream() { close(); }
  void open() {}
  void close() { Flush(); _buf.Free(); } 
  void SetFilePos(int pos) {} //_error=LSUnknownError; _fail=true; //these are not members of QOStream

  virtual void Flush(int size=0);
};

template<class Traits>
void QStdStream<Traits>::Flush(int size)
{
  const char *data = cc_cast(_buf->DataLock(0, _bufferSize));
  Traits::Output(data, _bufferSize);
  _buf->DataUnlock(0, _bufferSize);
  _bufferStart += _bufferOffset;
  _bufferSize = 0;
  _bufferOffset = 0;
}

typedef QStdStream< QStdStreamTraits<QStdOut> > QStdOutStream;
typedef QStdStream< QStdStreamTraits<QStdErr> > QStdErrStream;
typedef QStdStream< QStdStreamTraits<QStdDebug> > QStdDebugStream;

/// stdin connected stream
class QStdInStream: public QIStream
{
  int _handle;

  //! source can read more, but it always has to read at least on byte on pos
  virtual PosQIStreamBuffer ReadBuffer(PosQIStreamBuffer &buf, QFileSize pos); 

  virtual bool PreloadBuffer(
    RequestableObject *obj, RequestableObject::RequestContext *ctx,
    FileRequestPriority priority, QFileSize pos, QFileSize size
    ) const
  {
    // no pre loading on stdin possible
    return true;
  }
  virtual QFileSize GetDataSize() const
  {
    // pretend stdin is infinite, handle eof using ReadBuffer
    return QFileSizeMax;
  }
public:
  QStdInStream(int handle=QStdIn)
    :_handle(handle)
  {
  }
  ~QStdInStream()
  {
    _buf.Free();
  }
};

#endif
