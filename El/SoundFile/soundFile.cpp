#ifdef _WIN32
#include <crtdbg.h>
#endif

/*!
\file
Loading of wav files
\patch_internal 1.04 Date 07/16/2001 by Ondra. Disable ACM wav codec support.
Reasons:
- Codecs were never actually used.
- Codec interface does not support seeking, there is no MP3 codes.
- OGG serves the purpose much better.
*/

#include <El/elementpch.hpp>
#include <Es/Common/win.h>

#include "Es/Common/langExt.hpp"
#define ENABLED_NOOPT_Bebul 1
#if ENABLED_FEATURE(NOOPT)
#if _ENABLE_CHEATS
#pragma optimize ("", off)
#endif
#endif

#if !defined _WIN32

  /* general waveform format structure (information common to all formats) */
  struct WAVEFORMAT_STRUCT {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
  } ;

/* flags for wFormatTag field of WAVEFORMAT */
# define WAVE_FORMAT_PCM     1
# define WAVEFORMAT WAVEFORMAT_STRUCT
#else
# include <mmsystem.h>
# include <mmreg.h>
#endif

#include "soundFile.hpp"
#include <El/FileServer/fileServer.hpp>
#include <El/QStream/qbStream.hpp>
#include <El/QStream/serializeBin.hpp>

#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>

#if defined(_WIN32) 
#pragma comment(lib,"winmm")
#endif

#ifndef DUMMY_SOUND_FILE

bool WaveStream::RequestDataClipped(FileRequestPriority prior, int offset, int size) const
{
  // clip offset..size to valid range
  if (offset<0)
  {
    size += offset;
    offset = 0;
  }
  if (size<0)
  {
    return true;
  }
  
  if (offset>=0)
  {
    int total = GetUncompressedSize();
    int rest = total-offset;
    if (rest>0)
    {
      if (size>rest) size = rest;
      return RequestData(prior,offset,size);
    }
  }

  return true;
}

bool WaveStream::RequestDataLooped(FileRequestPriority prior, int offset, int size) const
{
  if (offset<0)
  {
    size += offset;
    offset = 0;
  }
  if (size<=0)
  {
    return true;
  }
  // clip offset..size to valid range
  if (offset>=0)
  {
    int total = GetUncompressedSize();
    if (offset>=total)
    {
      offset = offset%total;
    }
    int rest = total-offset;
    Assert (rest>0);
    int requestSize1 = size;
    int requestSize2 = 0;
    if (size>rest)
    {
      // request both parts
      requestSize1 = rest;
      requestSize2 = size-rest;
    }
    bool ok = true;
    if (requestSize1>0)
    {
      bool ok1 = RequestData(prior,offset,requestSize1);
      if (!ok1) ok = false;
    }
    if (requestSize2>0)
    {
      bool ok2 = RequestData(prior,0,requestSize2);
      if (!ok2) ok = false;
    }
    return ok;
  }

  return true;
}

int WaveStream::GetDataClipped(void *data, int offset, int size) const
{
  if (size<0)
  {
    Fail("GetDataClipped: Negative size");
    return 0;
  }
  // clip offset..size to valid range
  int offsetDone = 0;
  if (offset<0)
  {
    int zeroData = -offset;
    saturateMin(zeroData,size);
    memset(data,0,zeroData);
    offset += zeroData;
    size -= zeroData;
    offsetDone += zeroData;

    data = (char *)data+zeroData;
  }
  
  if (offset>=0)
  {
    int total = GetUncompressedSize();
    int rest = total-offset;
    if (rest>0)
    {
      int unpack = size;
      saturateMin(unpack,rest);
      // verify valid range
      if 
      (
        offset<0 || unpack<0 || offset+unpack>total
      )
      {
        RptF
        (
          "Sound: out of range: offset %d, unpack %d, total %d",
          offset,unpack,total
        );
        RptF("  size %d, offsetDone %d",size,offsetDone);
      }
      else
      {
        int dataDone = GetData(data,offset,unpack);
        offsetDone += dataDone;
        size -= dataDone;

        data = (char *)data+dataDone;
      }
    }
  }

  if (size>0)
  {
    memset(data,0,size);
  }

  return offsetDone;

}

int WaveStream::GetDataLooped(void *data, int offset, int size) const
{
  if (size<0)
  {
    Fail("GetDataLooped: Negative size");
    return 0;
  }
  // clip offset..size to valid range
  int offsetDone = 0;
  if (offset<0)
  {
    int zeroData = -offset;
    saturateMin(zeroData,size);
    memset(data,0,zeroData);
    offset += zeroData;
    size -= zeroData;
    offsetDone += zeroData;

    data = (char *)data+zeroData;
  }
  
  if (offset>=0)
  {
    int total = GetUncompressedSize();
    if (offset>=total)
    {
      offset = offset%total;
    }
    int rest = total-offset;
    if (rest>0)
    {
      int unpack = size;
      saturateMin(unpack,rest);
      // verify valid range
      if 
      (
        offset<0 || unpack<0 || offset+unpack>total
      )
      {
        RptF
        (
          "Sound: out of range: offset %d, unpack %d, total %d",
          offset,unpack,total
        );
        RptF("  size %d, offsetDone %d",size,offsetDone);
      }
      else
      {
        int dataDone = GetData(data,offset,unpack);
        offsetDone += dataDone;
        size -= dataDone;

        data = (char *)data+dataDone;
      }
    }
    if (size>0)
    {
      // end reached, but we need some more data
      int sizeStep = size;
      saturateMin(sizeStep,total);
      int dataDone = GetData(data,0,sizeStep);
      offsetDone += dataDone;
      size -= dataDone;

      data = (char *)data+dataDone;
      
    }
  }

  if (size>0)
  {
    memset(data,0,size);
  }

  return offsetDone;

}

// WaveStreamPlain is implementation of WaveStream interface
// it is able to read from uncompressed or delta-compressed data source (WaveBuffer)

class WaveStreamPlain: public WaveStream
{
  // 
  WaveBuffer _data;

  public:
  // construction
  WaveStreamPlain(const WaveBuffer &buffer);

  // implementation of WaveStream
  virtual int GetUncompressedSize() const;
  virtual void GetFormat(WAVEFORMATEX &format) const;
  virtual int GetData(void *data, int offset, int size) const;
  virtual bool RequestData(FileRequestPriority prior, int offset, int size) const;
  virtual bool IsFromBank(QFBank *bank) const;
  virtual bool StreamingWanted(bool looped) const;
};

WaveStreamPlain::WaveStreamPlain(const WaveBuffer &buffer)
:_data(buffer)
{
}

bool WaveStreamPlain::StreamingWanted(bool looped) const
{
  // when the sound is looped, it makes less sense to play it streaming
  const int limit = looped ? 1024*1024 : 256*1024;
  return _data.UnpackedSize()>=limit;
}

void WaveStreamPlain::GetFormat(WAVEFORMATEX &format) const
{
  _data.GetWaveFormat(format);
}

int WaveStreamPlain::GetUncompressedSize() const
{
  return _data.UnpackedSize();
}

int WaveStreamPlain::GetData(void *data, int offset, int size) const
{
  PROFILE_SCOPE_EX(wssGD,sound);
  if (PROFILE_SCOPE_NAME(wssGD).IsActive())
  {
    PROFILE_SCOPE_NAME(wssGD).AddMoreInfo(Format("%d(%d)",offset,size));
  }
  // therefore we can unpack from any position
  int rest = _data.UnpackedSize()-offset;
  if (rest>0)
  {
    int unpack = size;
    saturateMin(unpack,rest);
    int unpacked = _data.UnpackPart(data,offset,unpack);

    return unpacked;
  }
  return 0;
}

bool WaveStreamPlain::RequestData(FileRequestPriority prior, int offset, int size) const
{
  PROFILE_SCOPE_EX(wssRD,sound);
  int rest = _data.UnpackedSize()-offset;
  bool ret = true;
  if (rest>0)
  {
    // to avoid seeking, round up to nearest granular size
    // when streaming, we will need the data quite soon anyway
    const int ratio = _data.GetCompressionRatio();
    const int granularity = 32*1024*ratio;
    size = ((offset+size+granularity-1)&~(granularity-1))-offset;
    if (size>rest) size = rest;
    ret = _data.RequestPart(offset,size);
  }
  if (PROFILE_SCOPE_NAME(wssRD).IsActive())
  {
    PROFILE_SCOPE_NAME(wssRD).AddMoreInfo(Format("%c %d(%d)",ret ? 'Y' : 'N',offset,size));
  }
  return ret;
}

bool WaveStreamPlain::IsFromBank(QFBank *bank) const
{
  return _data.IsFromBank(bank);
}


class WaveFile
{
  private:
  QIFStream _file;

  // note: sometimes we need to allocate more that WAVEFORMATEX
  WAVEFORMATEX *_format;
  
  DWORD _dataOffset;
  DWORD _dataSize;
  DWORD _dataRest;
  bool error;
  DWORD _samples; // actual number of samples
  

  protected:
  void New(const QIFStream &file);
  void Delete();

  void FreeFormat();
  void CreateFormat(int size);
  
  public:
  WaveFile();
  ~WaveFile(){Delete();}

  void Open(const QIFStream &file){New(file);}
  void StartDataRead();
  UINT Read( UINT cbRead, char *Dest );

  DWORD DataSize() const {return _dataSize;}
  DWORD DataOffset() const {return _dataOffset;}
  
  const WAVEFORMATEX &Format() const {return *_format;} 
  DWORD GetSamples() const {return _samples;}

  bool GetError() const {return error;}

  IFileBuffer *GetBuffer() const {return _file.GetSource();}
};

//inline void exc(){throw ios::failure(ios::failbit);}
//inline void exc( const char *file ){ErrorMessage("Error loading file %s",file);}

#define exc(file) {return;}

#define RIFF_FOURCC_C(ch0, ch1, ch2, ch3)  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))


#define RIFF_FOURCC RIFF_FOURCC_C('R','I','F','F')
#define RIFF_WAVE RIFF_FOURCC_C('W','A','V','E')
#define RIFF_fmt RIFF_FOURCC_C('f','m','t',' ')
#define RIFF_data RIFF_FOURCC_C('d','a','t','a')
#define RIFF_fact RIFF_FOURCC_C('f','a','c','t')

struct RiffChunk
{
  DWORD id;
  DWORD len;
};

WaveFile::WaveFile()
{
  _format=NULL;
}

void WaveFile :: New(const QIFStream &file)
{
  error = true;
  // Initialization...

  _samples = 0; // actual number of samples

  _file = file;
  _file.seekg(0,QIOS::beg);

  if( _file.fail() ) exc(file);

  // check file header

  RiffChunk chunk;
  _file.read(&chunk,sizeof(chunk));
  if (_file.fail() || chunk.id!=RIFF_FOURCC) exc(filename);

  DWORD fformat;
  _file.read(&fformat,sizeof(fformat));
  if (_file.fail() || fformat!=RIFF_WAVE) exc(filename);

  // process chunks
  // search for fmt chunk
    
  // Search the input file for for the 'fmt ' chunk.

  for(;;)
  {
    _file.read(&chunk,sizeof(chunk));
    if (_file.fail()) exc(filename);
    if (chunk.id!=RIFF_fmt)
    {
      // skip chunk
      _file.seekg(chunk.len,QIOS::cur);
      continue;
    }
    // check if it is valid
    if (chunk.len<sizeof(WAVEFORMATEX))
    {
      if (chunk.len<sizeof(WAVEFORMAT))
      {
        ErrF("Too short fmt chunk in PCM wave file");
        exc(filename);
      }
      else
      {
        // try to read old PCM format
        WAVEFORMAT fmt;
        _file.read(&fmt,sizeof(fmt));
        if (_file.fail()) exc(filename);
        _file.seekg(chunk.len-sizeof(fmt),QIOS::cur); // skip rest of chunk
        CreateFormat(sizeof(WAVEFORMATEX));
        _format->cbSize = 0;
        if (fmt.wFormatTag!=WAVE_FORMAT_PCM)
        {
          ErrF("Old fmt chunk in PCM wave file");
          exc(filename);
        }
        _format->wFormatTag = fmt.wFormatTag;
        _format->nAvgBytesPerSec = fmt.nAvgBytesPerSec;
        _format->nBlockAlign = fmt.nBlockAlign;
        _format->nChannels = fmt.nChannels;
        _format->nSamplesPerSec = fmt.nSamplesPerSec;
        _format->wBitsPerSample = fmt.nBlockAlign/fmt.nChannels*8;
      }
    }
    else
    {
      // read format
      CreateFormat(chunk.len);
      _file.read(_format,chunk.len);
      if (_file.fail()) exc(filename);

      if (_format->wFormatTag==WAVE_FORMAT_PCM )
      {
        if (chunk.len!=sizeof(WAVEFORMATEX))
        {
          ErrF("Extra data in fmt chunk (PCM wave)");
        }
        _format->cbSize=0;

      }
    }
    break;
  }

  
  error = false;
}

/*  This routine has to be called before WaveReadFile as it searchs for the chunk to descend into for
    reading, that is, the 'data' chunk.  For simplicity, this used to be in the open routine, but was
    taken out and moved to a separate routine so there was more control on the chunks that are before
    the data chunk, such as 'fact', etc... */

void WaveFile :: StartDataRead()
{  
  if (error) return;  
  // Do a nice little seek...
  // now we are just after the format
  // we asssume data will be somewhere ahead

  RiffChunk chunk;
  for(;;)
  {
    _file.read(&chunk,sizeof(chunk));
    if (_file.fail())
    {
      error = true;
      break;
    }
    if (chunk.id==RIFF_fact)
    {
      DWORD fact;
      _file.read(&fact,sizeof(fact));
      _file.seekg(chunk.len-sizeof(fact),QIOS::cur);
      _samples = fact;
      //LogF("Fact %d",fact);
    }
    else if (chunk.id==RIFF_data)
    {
      _dataSize = chunk.len;
      _dataRest = chunk.len;
      _dataOffset = _file.tellg();
      break;
    }
    else
    {
      // skip chunk
      _file.seekg(chunk.len,QIOS::cur);
      continue;
    }
  }

}


/*  This will read wave data from the wave file.  Makre sure we're descended into
    the data chunk, else this will fail bigtime!
    hmmioIn         - Handle to mmio.
    cbRead          - # of bytes to read.   
    pbDest          - Destination buffer to put bytes.
    cbActualRead- # of bytes actually read.
*/


UINT WaveFile :: Read( UINT cbRead, char *Dest )
{
  if (error) return 0;
  
  UINT cbDataIn = cbRead;
  if( cbDataIn>_dataRest ) cbDataIn = _dataRest;
  
  _dataRest -= cbDataIn;
  
  _file.read(Dest,cbDataIn);

  if (_file.fail())
  {
    error = true;
    return 0;
  }
  
  return cbDataIn;
}

void WaveFile::CreateFormat(int size)
{
  FreeFormat();
  _format = (WAVEFORMATEX *)new char[size];
}

void WaveFile::FreeFormat()
{
  if (_format)
  {
    delete[] (char *)_format;
    _format = NULL;
  }
}

void WaveFile :: Delete()
{
  FreeFormat();

  // no need to close file
  //_file.Close();
}

#ifdef _MSC_VER
const int WSSMagic='0SSW';
#else
const int WSSMagic=StrToInt("WSS0");
#endif

struct WSSHeader
{
  int magic;
  char deltaPack;
  char resvd[3];
};

class UnpackDelta8
{
  enum {MaxDelta=32768};
  enum {MaxDeltaInv=127};

  int _delta[MaxDeltaInv*2+1];

  public:
  UnpackDelta8();

  bool RequestUnpack(const WaveBuffer *buf, int offset, int destSize);
  int Unpack(short *dest, const WaveBuffer *buf, int offset, int destSize, int startValue);
  int Skip(const WaveBuffer *buf, int destSize, int startValue);
};
class UnpackDelta4
{
  enum {MaxDelta=8192};
  enum {MaxDeltaInv=7};

  static const int _delta[MaxDeltaInv*2+1];

  public:
  UnpackDelta4();

  bool RequestUnpack(const WaveBuffer *buf, int offset, int destSize);
  int Unpack(short *dest, const WaveBuffer *buf, int offset, int destSize, int startValue);
  int Skip(const WaveBuffer *buf, int destSize, int startValue);
};

const int UnpackDelta4::_delta[MaxDeltaInv*2+1]=
{
  -8192,-4096,-2048,-1024,-512,-256,-64,
  0, // 7
  64,256,512,1024,2048,4096,8192
};

/// branch-less max. computation
/** note: for the result to be correct, overflow must not occur while computing a-b */

static inline int BranchlessMax(int a, int b)
{
  return (a+b+abs(a-b))>>1;
}
static inline int BranchlessMin(int a, int b)
{
  return (a+b-abs(a-b))>>1;
}
static inline int Sat16S( int val )
{
  if( val>0x7fff ) return 0x7fff;
  if( val<-0x8000 ) return -0x8000;
  return val;
}

UnpackDelta8::UnpackDelta8()
{
  int i;
  double base=pow(MaxDelta,1.0f/MaxDeltaInv);
  _delta[MaxDeltaInv]=0;
  for( i=1; i<=MaxDeltaInv; i++ )
  {
    //        / -1.084618362^-x  for x<0  (-128 to -1)
    // f(x)= {   0               for x=0  (0)
    //        \  1.084618362^x   for x>0  (1 to 127)
    int v=toInt(pow(base,i));
    _delta[MaxDeltaInv+i]=+v;
    _delta[MaxDeltaInv-i]=-v;
  }
}

bool UnpackDelta8::RequestUnpack(const WaveBuffer *buf, int offset, int destSize)
{
  QFileSize strOffset = offset+buf->GetSourceHeaderSize();
  return buf->RequestRaw(FileRequestPriority(10),strOffset,destSize/2);
}

int UnpackDelta8::Unpack( short *dest, const WaveBuffer *buf, int offset, int destSize, int startValue )
{
  QIStreamDataSource str;
  str.AttachSource(buf->GetSource());
  str.seekg(offset+buf->GetSourceHeaderSize(),QIOS::beg);

  int aVal=startValue;
  
  Assert( (destSize&1)==0 );
  destSize/=2;
  while( --destSize>=0 )
  {
    int v=(signed char)str.get();
    v=_delta[v+MaxDeltaInv];
    aVal+=v;
    *dest++=Sat16S(aVal);
  }
  return aVal;
}

int UnpackDelta8::Skip(const WaveBuffer *buf, int destSize, int startValue)
{
  QIStreamDataSource str;
  str.AttachSource(buf->GetSource());
  str.seekg(buf->GetSourceHeaderSize(),QIOS::beg);

  Assert( (destSize&1)==0 );
  destSize/=2;
  int aVal=startValue;
  while( --destSize>=0 )
  {
    int v=(signed char)str.get();
    v=_delta[v+MaxDeltaInv];
    aVal+=v;
  }
  return aVal;
}

UnpackDelta4::UnpackDelta4()
{
}

bool UnpackDelta4::RequestUnpack(const WaveBuffer *buf, int offset, int destSize)
{
  QFileSize strOffset = offset+buf->GetSourceHeaderSize();
  return buf->RequestRaw(FileRequestPriority(10),strOffset,destSize/4);
}

int UnpackDelta4::Unpack( short *dest, const WaveBuffer *buf, int offset, int destSize, int startValue )
{
  QIStreamDataSource str;
  str.AttachSource(buf->GetSource());
  str.seekg(buf->GetSourceHeaderSize()+offset,QIOS::beg);

  // two samples a time
  Assert( (destSize&3)==0 );
  destSize/=2;
  int aVal=startValue;
  while( (destSize-=2)>=0 )
  {
    int v=str.get();
    int v1=(v>>4)&0xf;
    int v2=(v&0xf);
    v1=_delta[v1];
    v2=_delta[v2];
    aVal+=v1;
    *dest++=Sat16S(aVal);
    aVal+=v2;
    *dest++=Sat16S(aVal);
  }
  return aVal;
}

int UnpackDelta4::Skip( const WaveBuffer *buf, int destSize, int startValue )
{
  QIStreamDataSource str;
  str.AttachSource(buf->GetSource());
  str.seekg(buf->GetSourceHeaderSize(),QIOS::beg);

  // two samples a time
  Assert( (destSize&3)==0 );
  destSize/=2;
  int aVal=startValue;
  while( (destSize-=2)>=0 )
  {
    int v=str.get();
    int v1=(v>>4)&0xf;
    int v2=(v&0xf);
    v1=_delta[v1];
    v2=_delta[v2];
    aVal+=v1;
    aVal+=v2;
  }
  return aVal;
}

static UnpackDelta8 UnpackD8;
static UnpackDelta4 UnpackD4;

bool WSSRequestFile(const char *name, Ref<WaveStream> &stream, bool sync)
{
  QIFStreamB in;
  //Log("WSSLoadFile Open %s",name);
  in.AutoOpen(name);
  if( in.fail() || in.eof() ) return true;
  // request file header
  WSSHeader wHeader;
  WAVEFORMATEX header;
  if (!sync)
  {
    bool preloaded = in.PreRead(FileRequestPriority(1),0,sizeof(wHeader)+sizeof(header));
    if (!preloaded)
    {
      return false;
    }
  }
  in.read((char *)&wHeader,sizeof(wHeader));
  SerializeBinStream::ProcessLittleEndianData(&wHeader.magic,sizeof(wHeader.magic),sizeof(wHeader.magic));
  if( wHeader.magic!=WSSMagic )
  {
    in.seekg(0,QIOS::beg);
    wHeader.deltaPack=0;
  }
  int deltaPack=wHeader.deltaPack;
  in.read((char *)&header,sizeof(header));
  if( in.fail() || in.eof() ) return true;

  SerializeBinStream::ProcessLittleEndianData(&header.wFormatTag,sizeof(header.wFormatTag),sizeof(header.wFormatTag));
  SerializeBinStream::ProcessLittleEndianData(&header.nChannels,sizeof(header.nChannels),sizeof(header.nChannels));
  SerializeBinStream::ProcessLittleEndianData(&header.nSamplesPerSec,sizeof(header.nSamplesPerSec),sizeof(header.nSamplesPerSec));
  SerializeBinStream::ProcessLittleEndianData(&header.nAvgBytesPerSec,sizeof(header.nAvgBytesPerSec),sizeof(header.nAvgBytesPerSec));
  SerializeBinStream::ProcessLittleEndianData(&header.nBlockAlign,sizeof(header.nBlockAlign),sizeof(header.nBlockAlign));
  SerializeBinStream::ProcessLittleEndianData(&header.wBitsPerSample,sizeof(header.wBitsPerSample),sizeof(header.wBitsPerSample));
  SerializeBinStream::ProcessLittleEndianData(&header.cbSize,sizeof(header.cbSize),sizeof(header.cbSize));

  header.cbSize=0;
  IFileBuffer *fBuffer=in.GetSource();

  if( in.fail() || in.eof() ) return true;
  if( in.rest()<=0 ) return true;
  // return part of file buffer

  WaveBuffer buffer(fBuffer,header,in.tellg(),in.rest(),deltaPack);
  stream = new WaveStreamPlain(buffer);
  return true;
}

DEFINE_FAST_ALLOCATOR(WaveBuffer)

WaveBuffer::WaveBuffer
(
  IFileBuffer *buf, const WAVEFORMATEX &format, int start, int size, int deltaPack
)
:_buffer(buf),_format(format),
_skip(start),_size(size),_deltaPack(deltaPack),
_lastWriteOffset(0),_lastValue(0)
{
}

bool WaveBuffer::IsFromBank(QFBank *bank) const
{
  return _buffer->IsFromBank(bank);
}

bool WaveBuffer::RequestPart(int offset, int size) const
{
  int maxSize = UnpackedSize()-offset;
  saturateMin(size,maxSize);
  if( _deltaPack==0 )
  {
    return _buffer->PreloadBuffer(_request,NULL,NULL,FileRequestPriority(10),offset+_skip,size);
  }
  else if( _deltaPack==4 )
  {
    // check offset
    // if offset is not the same, we have to restart and rewind
    Assert(offset%4==0);
    return UnpackD4.RequestUnpack(this,offset/4,size);
  }
  else if( _deltaPack==8 )
  {
    return UnpackD8.RequestUnpack(this,offset/2,size);
  }
  else
  {
    Fail("Invalid deltaPack value");
    return true;
  }
}

int WaveBuffer::UnpackPart(void *dest, int offset, int size) const
{
  int maxSize = UnpackedSize()-offset;
  saturateMin(size,maxSize);
  if( _deltaPack==0 )
  {
    _buffer->Read(dest,offset+_skip,size);
  }
  else if( _deltaPack==4 )
  {
    // check offset
    // if offset is not the same, we have to restart and rewind
    if (offset!=_lastWriteOffset)
    {
      _lastValue = UnpackD4.Skip(this,offset,0);
    }

    Assert(offset%4==0);
    _lastValue = UnpackD4.Unpack((short *)dest,this,offset/4,size,_lastValue);
    _lastWriteOffset = offset+size;
  }
  else if( _deltaPack==8 )
  {

    if (offset!=_lastWriteOffset)
    {
      _lastValue = UnpackD8.Skip(this,offset,0);
    }

    Assert(offset%2==0);
    _lastValue = UnpackD8.Unpack((short *)dest,this,offset/2,size,_lastValue);
    _lastWriteOffset = offset+size;
  }
  else
  {
    memset(dest,0,size);
    Fail("Invalid deltaPack value");
  }
  return size;
}

void WaveBuffer::Unpack( void *dest, int destSize ) const
{
  if( _deltaPack==0 )
  {
    Assert( destSize==UnpackedSize() );
    _buffer->Read(dest,0,destSize);
  }
  else if( _deltaPack==4 )
  {
    Assert( destSize==UnpackedSize() );
    UnpackD4.Unpack((short *)dest,this,0,UnpackedSize(),0);
  }
  else if( _deltaPack==8 )
  {
    Assert( destSize==UnpackedSize() );
    UnpackD8.Unpack((short *)dest,this,0,UnpackedSize(),0);
  }
  else
  {
    Fail("Invalid deltaPack value");
  }
}

bool WaveBuffer::RequestRaw(FileRequestPriority prio, QFileSize offset, QFileSize size) const
{
  if (offset>=GetSource()->GetSize() || size==0)
    return true;
  return GetSource()->PreloadBuffer(_request,NULL,NULL,prio,offset,size);
}

WaveStream *WaveLoadFile( const char *filename )
{
  QIFStreamB file;
  file.AutoOpen(filename);

  // Check if opening the file succeeded
  if (file.fail() || file.eof())
  {
    RptF("Error: Failed to open file %s", filename);
    return NULL;
  }

  WaveFile input;
  input.Open(file);
  if (input.GetError()) return NULL;

  // check file format

  if (input.Format().wFormatTag == WAVE_FORMAT_PCM)
  {
    input.StartDataRead();
    if (input.GetError()) return NULL;

    IFileBuffer *fBuffer=input.GetBuffer();
    // return part of file buffer
    WaveBuffer buffer
    (
      fBuffer,input.Format(),
      input.DataOffset(),input.DataSize(),0
    );
    return new WaveStreamPlain(buffer);
  }
  else
  {
    ErrF("non-PCM format %d in '%s'",input.Format().wFormatTag,filename);
    return NULL;
  }
}

#ifndef _M_PPC
// TODOX360: Vorbis libraries, or use XMA

/// OGG stream implementation
class WaveStreamOGG: public WaveStream
{
  mutable QIFStream _file;
  mutable OggVorbis_File _vf;
  mutable int _bitstream;
  mutable int _offset;

  bool _vfOpen;
  WAVEFORMATEX _format;

  public:
  WaveStreamOGG(const QIFStream &file);
  ~WaveStreamOGG();

  bool GetError() const {return !_vfOpen;}
  // implement WaveStream interface
  virtual int GetUncompressedSize() const;
  virtual void GetFormat(WAVEFORMATEX &format) const;
  virtual int GetData(void *data, int offset, int size) const;
  virtual bool IsFromBank(QFBank *bank) const;
  virtual bool RequestData(FileRequestPriority prior, int offset, int size) const;
  virtual bool StreamingWanted(bool looped) const;
  
  private:
  /// seek wanted offset in the file
  void SeekOffset(int offset) const;
};

int OggStreamBuffer::Read(void *buffer, int size)
{
  enter();

  char *ptr = static_cast<char *>(buffer);
  saturateMin(size, _totalSize);
  int ret = size;

  if (size)
  {
    if (_readPos + size > OggBufSize)
    {
      int bytes = OggBufSize - _readPos;
      memcpy(ptr, _oggBuffer+_readPos, bytes);
      _readPos = 0; 
      _totalSize -= bytes; 
      size -= bytes;
      ptr += bytes;
    }
    memcpy(ptr, _oggBuffer+_readPos, size);
    _readPos += size; _totalSize -= size; 
  }

  leave();
/*
  static FILE *out = fopen("c:\\tmp\\radioSound.ogg", "wb"); // store to file for debug purposes
  fwrite(buffer, size, 1, out);
*/
  return ret;
}

int OggStreamBuffer::Write(void *buffer, int size)
{
  enter();
  char *ptr = static_cast<char *>(buffer);
  int rest = size;
  while(rest)
  {
    if (_vfOpen && _totalSize==OggBufSize) // buffers full => decode a little (sound engine do not probably replay us)
    {
      int rd = 0;
      do
      {
        const int ThrowDataSize = 1024;
        char throwAwayData[ThrowDataSize];
        rd = ov_read(&_vbfile, throwAwayData,ThrowDataSize,0,2,1,&_bitstream); //calls Read() as callback
      } while (rd>0 && _vfOpen && _totalSize==OggBufSize);
    }
    if (_totalSize==OggBufSize) 
    { // unable to free any buffer to write data in
      #if DIAG_RADIO_STREAM
      LogF("OggStreamBuffer: FAIL, unable to free any buffer to write data in");
      #endif
      leave(); 
      return size-rest; //this should never happen, moreover curl would return error, when Write returns whatever different to size
    }
    int bytes = rest;
    saturateMin(bytes, OggBufSize-_totalSize);
    int writePos = (_readPos+_totalSize) % OggBufSize;
    if (writePos+bytes > OggBufSize) bytes = OggBufSize-writePos;
    memcpy(_oggBuffer+writePos, ptr, bytes);
    ptr += bytes; rest -= bytes; _totalSize += bytes;
    if (!_vfOpen && _totalSize>16384) Init();
  }
  leave();
  return size;
}

int OggStreamBuffer::GetData(void *buffer, int size) const
{
  if (!_vfOpen) return 0;
  
  enter();
  int dataDone = 0;
  while (size>0)
  {
    int rd = ov_read(&_vbfile,(char *)buffer,size,0,2,1,&_bitstream);
    if (rd<=0)
    {
      #if DIAG_RADIO_STREAM
      LogF("Stream read error: rd=%d, dataDone=%d", rd, dataDone);
      #endif
      break;
    }

    size -= rd;
    buffer = (char *)buffer+rd;
    dataDone += rd;
    _offset += rd;
  }
  leave();
  #if DIAG_RADIO_STREAM
  LogF("OggRead: %d, %s", dataDone, Name());
  #endif
  return dataDone;
}

void OggStreamBuffer::GetFormat(WAVEFORMATEX &format) const
{
  PROFILE_SCOPE(oggGF);
  // get uncompressed format
  if (!_vfOpen)
  {
    format.cbSize = 0;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.wBitsPerSample = 16;
    format.nChannels = 1;
    format.nBlockAlign = 2;
    format.nSamplesPerSec = 22000;
    format.nAvgBytesPerSec = 44000;
    return;
  }
  //LogF("beg ov_info");
  enter();
  vorbis_info *info = ov_info(&_vbfile,-1);
  leave();
  //LogF("end ov_info");

  format.cbSize = 0;
  format.wFormatTag = WAVE_FORMAT_PCM;
  format.wBitsPerSample = 16;
  format.nChannels = info->channels;
  format.nBlockAlign = info->channels*2;
  format.nSamplesPerSec = info->rate;
  format.nAvgBytesPerSec = info->rate*2*info->channels;
}

// vorbis callback interface to OggStreamBuffer

static size_t readOggStream(void *buffer, size_t size, size_t count, void *stream)
{
  OggStreamBuffer *str = (OggStreamBuffer *)stream;
  int rd = size*count;
  return str->Read(buffer,rd);
}

static int closeOggStream(void *stream)
{
  // no close required
  return 0;
}

static int seekOggStream(void *stream, ogg_int64_t pos, int origin)
{
  return -1;
}

static long tellOggStream(void *stream)
{
  return 0;
}

#include <El/Credits/credits.hpp>
#ifndef _CONSOLE
#include <libcurl/include/curl/curl.h>
#endif

OggStreamBuffer::OggStreamBuffer(void *curl, const char *url) 
  : _curl(curl), _url(url)
{
  _oggBuffer = new char[OggBufSize];
  _readPos = _totalSize = 0;
  _vfOpen = false;
}

OggStreamBuffer::~OggStreamBuffer()
{
  enter();
  if (_vfOpen)
  {
    _vfOpen = false;
    ov_clear(&_vbfile);
  }
  leave();
  delete _oggBuffer;
}

void OggStreamBuffer::Init()
{
  enter();
  PROFILE_SCOPE(oggRad);
  ov_callbacks callbacks;
  callbacks.read_func = readOggStream;
  callbacks.seek_func = seekOggStream;  // NULL has caused crash (despite documentation)
  callbacks.close_func = closeOggStream;
  callbacks.tell_func = tellOggStream;  // NULL has caused crash

  int ok = ov_open_callbacks(this,&_vbfile,NULL,0,callbacks);

  // get uncompressed format
  if (ok>=0)
  {
    vorbis_info *info = ov_info(&_vbfile,-1);

    #if DIAG_RADIO_STREAM
    LogF("Ogg stream buffer initialized");
    #endif

    _format.cbSize = 0;
    _format.wFormatTag = WAVE_FORMAT_PCM;
    _format.wBitsPerSample = 16;
    _format.nChannels = info->channels;
    _format.nBlockAlign = info->channels*2;
    _format.nSamplesPerSec = info->rate;
    _format.nAvgBytesPerSec = info->rate*2*info->channels;
  }

  _vfOpen = ok>=0;
  _bitstream = 0;
  _offset = 0;
  leave();
}

/// Streaming Radio from internet
class WaveRadioStream: public WaveStream
{
  // no ref! The WaveXA2 is the only class containing Ref to WaveRadioStream. And WaveXA2 is referenced inside RadioStream2
  //    The _wave = NULL is set prior the RadioStream2 is destructed
  IRadioStream *_stream;  

  public:
  WaveRadioStream(IRadioStream *stream);

  bool GetError() const {return _stream ? _stream->GetError() : true;}
  // implement WaveStream interface
  virtual int GetUncompressedSize() const;
  virtual void GetFormat(WAVEFORMATEX &format) const { if (_stream) _stream->GetFormat(format); }
  virtual int GetData(void *data, int offset, int size) const;
  virtual bool RequestData(FileRequestPriority prior, int offset, int size) const;
  virtual bool StreamingWanted(bool looped) const { return true; }
  virtual bool IsFromBank(QFBank *bank) const { return false; }
};

#ifdef _WIN32
#ifndef MFC_NEW

void *_ogg_malloc(size_t size)
{
  #if !_RELEASE
  return GMemFunctions->New(size,"_ogg_malloc",0);
  #else
  return GMemFunctions->New(size);
  #endif
}

void *_ogg_calloc(size_t num, size_t size)
{
  size *= num;
  void *mem = _ogg_malloc(size);
  if (mem) memset(mem,0,size);
  return mem;
}

void *_ogg_realloc(void *memblock, size_t size)
{
  #if !_RELEASE
  return GMemFunctions->Realloc(memblock,size,"_ogg_realloc",0);
  #else
  return GMemFunctions->Realloc(memblock,size);
  #endif
  
}

void _ogg_free(void *mem)
{
  #if !_RELEASE
  GMemFunctions->Delete(mem,"_ogg_malloc",0);
  #else
  GMemFunctions->Delete(mem);
  #endif
}

#else
void *_ogg_malloc(size_t size)
{
  return malloc(size);
}

void *_ogg_calloc(size_t num, size_t size)
{
  return calloc(num,size);
}

void *_ogg_realloc(void *memblock, size_t size)
{
  return realloc(memblock,size);
}

void _ogg_free(void *mem)
{
  free(mem);
}

#endif
#endif

// provide custom allocators for Vorbis / OGG


// use vorbis file interface

/*!
\patch 1.23 Date 09/11/2001 by Ondra
- Improved: Vorbis RC2 decoder used instead Beta 4.
This could improve OGG reading compatibility.
*/

// vorbis callback interface to QIFStream

static size_t readQIFStream(void *buffer, size_t size, size_t count, void *stream)
{
  QIFStream *str = (QIFStream *)stream;
  int rd = size*count;
  //LogF("  OGG read %d:%d (%d)",str->tellg(),rd,str->rest());
  int rest = str->rest();
  if (rd>rest) rd = rest;
  str->read(buffer,rd);
  return rd;
}

static int seekQIFStream(void *stream, ogg_int64_t pos, int origin)
{
  // pretend the stream is not seekable - will improve streaming
  #if 0
    QIFStream *str = (QIFStream *)stream;
    //LogF("  OGG seek %d %d->%d",origin,str->tellg(),pos);
    return -1;
  #else
    QIFStream *str = (QIFStream *)stream;
    //LogF("  OGG seek %d %d->%d",origin,str->tellg(),pos);
    switch (origin)
    {
      case SEEK_CUR:
        str->seekg(pos,QIOS::cur);
        break;
      case SEEK_END:
        str->seekg(pos,QIOS::end);
        break;
      case SEEK_SET:
        str->seekg(pos,QIOS::beg);
        break;
    } 
    // no error
    return 0;
  #endif
}

static int closeQIFStream(void *stream)
{
  // no close required
  return 0;
}

static long tellQIFStream(void *stream)
{
  QIFStream *str = (QIFStream *)stream;
  return str->tellg();
}

WaveStreamOGG::WaveStreamOGG(const QIFStream &file)
{
  PROFILE_SCOPE(oggOP);
  _file = file;
  ov_callbacks callbacks;
  callbacks.read_func = readQIFStream;
  callbacks.seek_func = seekQIFStream;
  callbacks.close_func = closeQIFStream;
  callbacks.tell_func = tellQIFStream;

  //LogF("beg ov_open_callbacks");
  int ok = ov_open_callbacks(&_file,&_vf,NULL,0,callbacks);
  //LogF("end ov_open_callbacks");

  _vfOpen = ok>=0;
  _bitstream = 0;
  _offset = 0;

  // get uncompressed format
  if (_vfOpen)
  {
    //LogF("beg ov_info");
    vorbis_info *info = ov_info(&_vf,-1);
    //LogF("end ov_info");

    _format.cbSize = 0;
    _format.wFormatTag = WAVE_FORMAT_PCM;
    _format.wBitsPerSample = 16;
    _format.nChannels = info->channels;
    _format.nBlockAlign = info->channels*2;
    _format.nSamplesPerSec = info->rate;
    _format.nAvgBytesPerSec = info->rate*2*info->channels;
  }
}

WaveStreamOGG::~WaveStreamOGG()
{
  if (_vfOpen)
  {
    _vfOpen = false;
    ov_clear(&_vf);
  }
}

int WaveStreamOGG::GetUncompressedSize() const
{
  // stream is already open
  if (!_vfOpen) return 0;
  //LogF("beg ov_pcm_total");
  int total = ov_pcm_total(&_vf,-1);
  //LogF("end ov_pcm_total");
  //LogF("GetUncompressedSize %d",(int)total);

  return total*_format.nChannels*2;

}

bool WaveStreamOGG::StreamingWanted(bool looped) const
{
  int total = ov_pcm_total(&_vf,-1);
  //vorbis_info *info = ov_info(&_vf,-1);
  // OGG looped ? That should not happen much
  return total>64*1024;
}


void WaveStreamOGG::GetFormat(WAVEFORMATEX &format) const
{
  PROFILE_SCOPE(oggGF);
  // get uncompressed format
  if (!_vfOpen)
  {
    format.cbSize = 0;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.wBitsPerSample = 16;
    format.nChannels = 1;
    format.nBlockAlign = 2;
    format.nSamplesPerSec = 22000;
    format.nAvgBytesPerSec = 44000;
    return;
  }
  //LogF("beg ov_info");
  vorbis_info *info = ov_info(&_vf,-1);
  //LogF("end ov_info");

  format.cbSize = 0;
  format.wFormatTag = WAVE_FORMAT_PCM;
  format.wBitsPerSample = 16;
  format.nChannels = info->channels;
  format.nBlockAlign = info->channels*2;
  format.nSamplesPerSec = info->rate;
  format.nAvgBytesPerSec = info->rate*2*info->channels;
}

void WaveStreamOGG::SeekOffset(int offset) const
{
  if (_offset!=offset)
  {
    int pcm = offset / (_format.nChannels*2);
    //LogF("Seek to %d (PCM %d)",offset,pcm);
    // we need to seek in source stream
    //LogF("beg ov_pcm_seek");
    int err = ov_pcm_seek(&_vf,pcm);
    //LogF("end ov_pcm_seek");
    if (err!=0)
    {
      // ignore error, but report it
      LogF("Error %d while seeking",err);
    }
    _offset = offset;
  }
}

int WaveStreamOGG::GetData(void *data, int offset, int size) const
{
  PROFILE_SCOPE(oggGD);
  if (PROFILE_SCOPE_NAME(oggGD).IsActive())
  {
    PROFILE_SCOPE_NAME(oggGD).AddMoreInfo(Format("%d(%d)",offset,size));
  }
  if (!_vfOpen) return 0;

  SeekOffset(offset);
  
  int dataDone = 0;
  while (size>0)
  {
    int obs = _bitstream;
    //LogF("beg ov_read");
    int rd = ov_read(&_vf,(char *)data,size,0,2,1,&_bitstream);
    //LogF("end ov_read");
    if (_bitstream!=obs)
    {
      LogF("Stream %d",_bitstream);
    }
    if (rd<=0)
    {
      LogF("Stream read error");
      break;
    }

    size -= rd;
    data = (char *)data+rd;
    dataDone += rd;
    _offset += rd;
  }
  //LogF("Ogg: done %d",dataDone);
  return dataDone;
}

bool WaveStreamOGG::RequestData(FileRequestPriority prior, int offset, int size) const
{
  SeekOffset(offset);

  // we would like to preload some data, but we do not know exactly how much
  // ogg uses _file for file access, and we will assume it will read sequentially
  // assume compression ratio is 1:4
  int compressedSize = size/4;
  if (compressedSize<1) compressedSize =  1;
  compressedSize += 16*1024;
  // round up to the nearest granular boundary
  const int granularity = 32*1024;
  int roundedSize = (_file.tellg() + (compressedSize+granularity-1)&~(granularity-1))-_file.tellg();
  int rest = _file.rest();
  if (rest<=0) // when there are no data left, do not request any more preloading
    return true;
  return _file.PreRead(prior,_file.tellg(),roundedSize);
}


bool WaveStreamOGG::IsFromBank(QFBank *bank) const
{
  return _file.IsFromBank(bank);
}


bool OGGRequestFile( const char *name, Ref<WaveStream> &stream, bool sync )
{
  QIFStreamB in;
  //Log("WSSLoadFile Open %s",name);
  in.AutoOpen(name);
  // duplicate file buffer
  if( in.fail() || in.eof() ) return true;

  if( in.rest()<=0 ) return true;
  // return part of file buffer
  
  // sizes to preload
  const int begSize = 32*1024;
  const int endSize = 32*1024;
  
  //LogF("Preread beg %d:%d",0,begSize);
  if (!sync)
  {
    bool loadStart = in.PreRead(FileRequestPriority(1),0,begSize);
    if (!loadStart)
    {
      return false;
    }
    int endPreloadSize = endSize;
    int endPreloadOffset = in.rest()-endSize;
    if (endPreloadOffset<begSize)
    {
      endPreloadSize += endPreloadOffset-begSize;
      endPreloadOffset = begSize;
    }
    if (endPreloadSize>0)
    {
      //LogF("Preread end %d:%d",endPreloadOffset,endPreloadSize);
      bool loadEnd = in.PreRead(FileRequestPriority(1),endPreloadOffset,endPreloadSize);
      if (!loadEnd)
      {
        return false;
      }
    }
  }

  WaveStreamOGG *ret = new WaveStreamOGG(in);
  if (ret->GetError())
  {
    delete ret;
    return true;
  }
  stream = ret;
  return true;
}

/// WaveRadioStream (Internet Radio Streaming implementation)

WaveRadioStream::WaveRadioStream(IRadioStream *stream)
  : _stream(stream)
{}

int WaveRadioStream::GetUncompressedSize() const
{
  if (_stream) return INT_MAX; //infty
  return 0;
}

int WaveRadioStream::GetData(void *data, int offset, int size) const
{
  if (!_stream || !_stream->Ready()) return 0;
  return _stream->GetData(data,size);
}

bool WaveRadioStream::RequestData(FileRequestPriority prior, int offset, int size) const
{ 
  return _stream && _stream->Ready();
}

void CreateWaveRadioStream(IRadioStream *radioStream, Ref<WaveStream> &waveStream)
{
  if (radioStream)
    waveStream = new WaveRadioStream(radioStream);
  else
    waveStream = NULL;
}

// Global Radio System
InternetRadioSystem *GIRadioSystem = NULL;

#endif

/**
request a file, if file is cannot
*/

bool SoundRequestFile(const char *name, Ref<WaveStream> &stream, bool sync)
{
  if (!strncmp(name, "http://", 7))
  {
    return GIRadioSystem ? GIRadioSystem->RequestUrl(name, stream) : false;
  }
  const char *ext = strrchr(name,'.');
  if (!ext)
  {
    return WSSRequestFile(name,stream,sync);
  }
  // only header is needed - whole files are cached
  if (!strcmp(ext, ".wav"))
  {
    stream = WaveLoadFile(name);
    return true;
  }
  if (!strcmp(ext, ".ogg"))
  {
    #ifndef _M_PPC
    return OGGRequestFile(name,stream,sync);
    #else
    // TODOX360: Vorbis libraries, or use XMA
    LogF("OGG files not supported (%s)",name);
    stream = NULL;
    return true;
    #endif
  }
  return WSSRequestFile(name,stream,sync);
}

#if defined _WIN32
/// write RIFF WAV file using MMIO library
struct WriteWaveToFile
{
  HMMIO _hmmio;
  MMCKINFO _parent, _subchunk;

  HMMIO Open(const char *filename, const WAVEFORMATEX *format, size_t dataSize);
  bool Write(const char *waveData, DWORD dataSize);
  bool Close();
};

HMMIO WriteWaveToFile::Open(const char *filename, const WAVEFORMATEX *format, size_t dataSize)
{
  // Open the given file for writing using buffered I/O.
  if ((_hmmio = ::mmioOpen((LPSTR)filename, NULL,
    MMIO_CREATE | MMIO_WRITE | MMIO_EXCLUSIVE))
    == NULL)
    // Failed to create wave file.
    return NULL;

  // Create a 'RIFF' chunk with a 'WAVE' form type
  _parent.fccType = mmioFOURCC('W', 'A', 'V', 'E');
  if (::mmioCreateChunk(_hmmio, &_parent, MMIO_CREATERIFF) != 0)
  {
    // Failed create 'RIFF' chunk.
    ::mmioClose(_hmmio, 0);
    return NULL;
  }

  // Create the format subchunk (form type 'fmt ').
  _subchunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
  _subchunk.cksize = sizeof(*format);
  if (::mmioCreateChunk(_hmmio, &_subchunk, 0) != 0)
  {
    // Failed create the format subchunk.
    ::mmioClose(_hmmio, 0);
    return NULL; // 3;
  }

  // Write the format subchunk.
  if (::mmioWrite(_hmmio, (char *)format, sizeof(*format))
    != (LONG)sizeof(*format))
  {
    // Failed to write the format subchunk.
    ::mmioClose(_hmmio, 0);
    return NULL; // 4;
  }

  // Ascend out of the format subchunk.
  if (::mmioAscend(_hmmio, &_subchunk, 0) != 0)
  {
    // Failed to write the format subchunk.
    ::mmioClose(_hmmio, 0);
    return NULL; // 4;
  }

  // Create the data subchunk that holds the waveform samples.
  _subchunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
  _subchunk.cksize = dataSize;
  if (::mmioCreateChunk(_hmmio, &_subchunk, 0) != 0)
  {
    // Failed to create the data subchunk.
    ::mmioClose(_hmmio, 0);
    return NULL; // 5;
  }

  // open, ready to write data
  return _hmmio;
}

bool WriteWaveToFile::Write(const char *waveData, DWORD dataSize)
{
  if (waveData != NULL && dataSize != 0)
  {
    // Write the waveform data subchunk.
    if (::mmioWrite(_hmmio, waveData, (LONG)dataSize)
      != (LONG)dataSize)
    {
      // Failed to write the data subchunk.
      ::mmioClose(_hmmio, 0);
      return false;
    }
  }
  return true;
}

bool WriteWaveToFile::Close()
{
  // Ascend out of the data subchunk.
  if (::mmioAscend(_hmmio, &_subchunk, 0) != 0)
  {
    // Failed to write the data subchunk.
    ::mmioClose(_hmmio, 0);
    return false;
  }

  // Ascend out of the 'RIFF' chunk.
  if (::mmioAscend(_hmmio, &_parent, 0) != 0)
  {
    // Failed create 'RIFF' chunk.
    ::mmioClose(_hmmio, 0);
    return false;
  }

  // We're done with the file, close it.
  ::mmioClose(_hmmio, 0);

  return true;    // successful
}

bool DebugVoNSaveWav(const char* fileName, const char* data, unsigned dataSize)
{
  WriteWaveToFile wav;
  WAVEFORMATEX fmt;
  fmt.wFormatTag = WAVE_FORMAT_PCM;
  fmt.nChannels = 1;
  fmt.nSamplesPerSec = 8000;
  fmt.nBlockAlign = fmt.nChannels*2;
  fmt.nAvgBytesPerSec = fmt.nSamplesPerSec*fmt.nBlockAlign;
  fmt.wBitsPerSample = 16;
  fmt.cbSize = 0; // for PCM there is no additional info passed

  if (!wav.Open(fileName,&fmt,dataSize))
    return false;

  wav.Write(data,dataSize);
  wav.Close();
  return true;
}

#if EXAMPLE
  WriteWaveToFile wav;
  WAVEFORMATEX fmt;
  fmt.wFormatTag = WAVE_FORMAT_PCM;
  fmt.nChannels = 1;
  fmt.nSamplesPerSec = 44010;
  fmt.nBlockAlign = fmt.nChannels*2;
  fmt.nAvgBytesPerSec = fmt.nSamplesPerSec*fmt.nBlockAlign;
  fmt.wBitsPerSample = 16;
  fmt.cbSize = 0; // for PCM there is no additional info passed
  
  wav.Open("test.wav",&fmt,totalSize);
  // write all data we suspect to be leaked
  for (.... )
  {
    wav.Write(partData,partSize);
    ....
  }
  wav.Close();
#endif

#endif

#else
// necessary Dummy implementation for Linux dedicated server
bool SoundRequestFile(const char *name, Ref<WaveStream> &stream, bool sync)
{
  return false;
}
#endif
