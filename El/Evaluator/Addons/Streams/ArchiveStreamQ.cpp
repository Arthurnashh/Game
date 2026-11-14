// ArchiveStreamQ.cpp: implementation of the ArchiveStreamQ class.
//
//////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include "ArchiveStreamQ.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ArchiveStreamQIn::ArchiveStreamQIn(QIStream *stream,bool autoclose):
_stream(stream),_autoclose(autoclose)
{
  Reset();
}

ArchiveStreamQIn::~ArchiveStreamQIn()
{
  if (_autoclose) delete [] _stream;
}

__int64 ArchiveStreamQIn::Tell()
{
  if (_err) return -1;
  return _stream->tellg();
}

void ArchiveStreamQIn::Seek(__int64 lOff, SeekOp seekop)
{
  if (_err) return ;
  switch (seekop)
  {
  case begin: _stream->seekg((int)lOff,QIOS::beg);break;
  case current: _stream->seekg((int)lOff,QIOS::cur);break;
  case end: _stream->seekg((int)lOff,QIOS::end);break;
  }
  if (_stream->fail()) _err=-1;
}

int ArchiveStreamQIn::DataExchange(void *buffer, int maxsize)
{
  if (_err) return _err;
  _stream->read(buffer,maxsize);
  if (_stream->fail()) _err=-1;
  return _err;
}

ArchiveStreamQOut::ArchiveStreamQOut(QOStream *stream,bool autoclose):
_stream(stream),_autoclose(autoclose)
{
  Reset();
}

ArchiveStreamQOut::~ArchiveStreamQOut()
{
  if (_autoclose) delete [] _stream;
}

__int64 ArchiveStreamQOut::Tell()
{
  if (_err) return -1;
  return _stream->tellp();
}

void ArchiveStreamQOut::Seek(__int64 lOff, SeekOp seekop)
{
  if (_err) return ;
  switch (seekop)
  {
  case begin: _stream->seekp((int)lOff,QIOS::beg);break;
  case current: _stream->seekp((int)lOff,QIOS::cur);break;
  case end: _stream->seekp((int)lOff,QIOS::end);break;
  }
}

int ArchiveStreamQOut::DataExchange(void *buffer, int maxsize)
{
  if (_err) return _err;
  _stream->write(buffer,maxsize);
  //  if (_stream->fail()) _err=-1;
  return _err;
}
