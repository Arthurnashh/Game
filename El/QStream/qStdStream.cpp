#include <El/elementpch.hpp>
#include "qStdStream.hpp"

#ifdef _WIN32
#include <Es/Common/win.h>

void QStdStreamTraits<QStdDebug>::Output(const char *text, int textSize)
{
  RString debugStr(text, textSize);
  OutputDebugString(debugStr);
}
#endif

PosQIStreamBuffer QStdInStream::ReadBuffer(PosQIStreamBuffer &buf, QFileSize pos)
{
  Assert(pos==buf._start+buf._len);
  buf.Free();
  int pageSize = GFileServerFunctions->GetPageSize();

  QIStreamBuffer *page = new QIStreamBufferPage();
  void *data = page->DataLock(0,pageSize);

  // try to read some more characters
  int rd = ::read(_handle,data,pageSize);

  page->DataUnlock(0,pageSize);

  // return how much we read
  return PosQIStreamBuffer(page,pos,rd);
}
