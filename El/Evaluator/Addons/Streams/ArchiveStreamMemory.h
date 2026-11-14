// ArchiveStreamMemory.h: interface for the ArchiveStreamMemory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ARCHIVESTREAMMEMORY_H__BC1AB227_F8CF_4013_86BF_DDD3C844DFAA__INCLUDED_)
#define AFX_ARCHIVESTREAMMEMORY_H__BC1AB227_F8CF_4013_86BF_DDD3C844DFAA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ArchiveStream.h"

class ArchiveStreamMemory : public ArchiveStream  
{
protected:
  char *_buffer;
  unsigned long _pointer;
  unsigned long _bufuse;
  unsigned long _alloc;
  unsigned long _grow;
  int _err;
public:
	ArchiveStreamMemory();
    virtual int IsError() {return _err;}
    virtual void Reset() {_err=0;}
    virtual __int64 Tell();
    virtual void SetError(int err) {_err=err;}
    virtual void Seek(__int64 lOff, SeekOp seekop);

};

class ArchiveStreamMemoryIn: public ArchiveStreamMemory
  {
  bool _autodelete;
  public:
    ArchiveStreamMemoryIn(const char *buffer, unsigned long bufsize, bool autodelete);
      ~ArchiveStreamMemoryIn();
    virtual bool IsStoring() {return false;}
    virtual int DataExchange(void *buffer, int maxsize);
    virtual void Reserved(int bytes);
    virtual void Rewind() {_pointer=0;}

  };
class ArchiveStreamMemoryOut: public ArchiveStreamMemory
  {
    bool _stbuffer;
  public:  
    ArchiveStreamMemoryOut(char *buffer=0,unsigned long bufsize=0,unsigned long grow=0); //buffer!=NULL - static buffer, grow=0 means each buffer - grow will double buffer
    ~ArchiveStreamMemoryOut() {if (!_stbuffer) free(_buffer);}
    virtual bool IsStoring() {return true;}
    virtual int DataExchange(void *buffer, int maxsize);
    char *GetBuffer() {return _buffer;} //only gets buffer
    char *DetachBuffer() {char *ret=_buffer;_buffer=0;_alloc=_bufuse=_pointer=0;_stbuffer=false;return ret;} //detach buffer, buffer will not freeed with object destruction
    unsigned long GetBufferSize() {return _bufuse;}
    virtual void Rewind() {_pointer=0;_bufuse=0;}
    void ShiftBuffer(unsigned long n) //shift buffer about n characters. First n characters is deleted, and buffer is shifted
      {
        if (n>=GetBufferSize()) 
        {
          _bufuse=0;
          _pointer=0;
        }
        else
        {
          memcpy(_buffer,_buffer+n,GetBufferSize()-n);
          _bufuse-=n;
          _pointer-=n;
          if (_pointer<0) _pointer=0;
        }
      }

  };


#endif // !defined(AFX_ARCHIVESTREAMMEMORY_H__BC1AB227_F8CF_4013_86BF_DDD3C844DFAA__INCLUDED_)
