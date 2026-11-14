// ArchiveStreamMemory.cpp: implementation of the ArchiveStreamMemory class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ArchiveStreamMemory.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ArchiveStreamMemory::ArchiveStreamMemory()
{
  Reset();
}


__int64 ArchiveStreamMemory::Tell()
{
  if (_err) return -1;
  return _pointer;
}

void ArchiveStreamMemory::Seek(__int64 lOff, SeekOp seekop)
{
  if (_err) return;
  switch (seekop)
  {
  case begin: _pointer=(unsigned long)lOff;break;
  case current:_pointer+=(unsigned long)lOff;break;
  case end: _pointer=_bufuse+(unsigned long)lOff;break;
  }
  if (_pointer<0) _pointer=0;
  if (_pointer>_bufuse) _pointer=_bufuse;
}

ArchiveStreamMemoryIn::ArchiveStreamMemoryIn(const char *buffer, unsigned long bufsize, bool autodelete)
{
  _buffer=const_cast<char *>(buffer);
  _alloc=bufsize;
  _bufuse=bufsize;
  _pointer=0;
  _grow=0;
  _autodelete=autodelete;
}

int ArchiveStreamMemoryIn::DataExchange(void *buffer, int maxsize)
{
  int rd=maxsize;
  if (_pointer+maxsize>_bufuse) rd=_bufuse-_pointer;
  if (rd==0) _err=ASTRERR_EOF;
  if (_err) return _err;
  memcpy(buffer,_buffer+_pointer,rd);
  _pointer+=rd;
  if (rd!=maxsize) ReportUncomplette(rd);
  return _err;
}

void ArchiveStreamMemoryIn::Reserved(int bytes)
{
  if (_pointer+bytes>_bufuse) _err=-1;
  if (_err) return;
  _pointer+=bytes;
}

ArchiveStreamMemoryIn::~ArchiveStreamMemoryIn()
{
  if (_autodelete) free(_buffer);
}

ArchiveStreamMemoryOut::ArchiveStreamMemoryOut(char *buffer,unsigned long bufsize,unsigned long grow)
{
  _buffer=buffer;
  _stbuffer=_buffer!=NULL;
  if (_stbuffer) _bufuse=_alloc=bufsize;
  else _bufuse=_alloc=0;
  _pointer=0;
  _grow=grow;
}

int ArchiveStreamMemoryOut::DataExchange(void *buffer, int maxsize)
{
  if (_err) return _err;
  if  (_pointer+maxsize>_alloc)
  {
    if (_stbuffer) {_err=ASTRERR_EOF;return _err;}
    else
    {
      unsigned long newsz=_alloc;
      if (_grow) newsz=newsz+__max(_grow,(unsigned long)maxsize);
      else 
      {
        while (_pointer+maxsize>newsz)
        {
          if (newsz==0) newsz=16;
          else newsz=newsz*2;
        }
        char *c=(char *)realloc(_buffer,newsz);
        if (c==NULL) {_err=-2;return _err;}
        _buffer=c;
        _alloc=newsz;
      }
    }
  }
  memcpy(_buffer+_pointer,buffer,maxsize);
  _pointer+=maxsize;
  if (_pointer>_bufuse) _bufuse=_pointer;  
  return _err;
}
