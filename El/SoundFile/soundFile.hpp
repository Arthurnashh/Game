// DirectX sound system
// (C) 1996, SUMA
#ifndef _SAMPLES_HPP
#define _SAMPLES_HPP

// Function declarations
#if defined _WIN32
#include <Es/Common/win.h>

#   include <mmsystem.h>
#   undef PlaySound

#else
  //#define DUMMY_SOUND_FILE
  typedef struct
  {
      WORD        wFormatTag;         /* format type */
      WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
      DWORD       nSamplesPerSec;     /* sample rate */
      DWORD       nAvgBytesPerSec;    /* for buffer estimation */
      WORD        nBlockAlign;        /* block size of data */
      WORD        wBitsPerSample;     /* number of bits per sample of mono data */
      WORD        cbSize;             /* the count in bytes of the size of */
              /* extra information (after cbSize) */
  } WAVEFORMATEX;
#endif

#include <El/QStream/fileCompress.hpp>

#include <Es/Memory/normalNew.hpp>

class QFBank;

/// buffer providing access to wave data stored in a file
class WaveBuffer
{
  Ref<IFileBuffer> _buffer;
  /// context for repeated pre-loading
  mutable FileRequestCompletionHandle _request;
  int _skip;
  int _size;
  int _deltaPack;

  // enable streaming delta-pack
  mutable int _lastWriteOffset;
  mutable int _lastValue;

  public:
  WAVEFORMATEX _format; // carry wave format with it

  public:
  WaveBuffer(IFileBuffer *buf, const WAVEFORMATEX &format, int start, int size, int deltaPack);

  IFileBuffer *GetSource() const {return _buffer;}
  int GetSourceHeaderSize() const {return _skip;}

  //char *Data() {return (char *)_buffer->GetData()+_skip;}
  //const char *Data() const {return (char *)_buffer->GetData()+_skip;}
  int PackedSize() const {return _size;}

  // how many data bytes will be made from one source byte
  int GetCompressionRatio() const {return _deltaPack==0? 1 : 16/_deltaPack;}
  int GetFormat() const {return _deltaPack;}
  void GetWaveFormat(WAVEFORMATEX &format) const {format=_format;}

  bool IsFromBank(QFBank *bank) const;

  int UnpackedSize() const
  {
    int size=_size;
    if( _deltaPack==8 ) return size*2;
    if( _deltaPack==4 ) return size*4;
    return size;
  }
  // unpack whole buffer
  void Unpack( void *dest, int destSize ) const;
  /// unpack part of the buffer
  int UnpackPart(void *dest, int offset, int size) const;

  /// request data for UnpackPart to be loaded
  bool RequestPart(int offset, int size) const;
  bool RequestRaw(FileRequestPriority prio, QFileSize offset, QFileSize size) const;
  
  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

/// interface to stream data from any source
/**
implementation of some basic functions with sound file
*/

class WaveStream: public RefCount
{
  public:
  // get some specific part of data
  // note: function should return 0 when reading after end of the data
  // it should anyway fill data with silence

  virtual int GetUncompressedSize() const = 0;
  virtual void GetFormat(WAVEFORMATEX &format) const = 0;
  /// get data from file
  virtual int GetData(void *data, int offset, int size) const = 0;
  /// request data for GetData to be loaded
  virtual bool RequestData(FileRequestPriority prior, int offset, int size) const
  {
    return true;
  }
  virtual bool IsFromBank(QFBank *bank) const = 0;

  /// check if the sound is long enough to be streamed
  virtual bool StreamingWanted(bool looped) const = 0;
  
  /// clipped data retrieval - zeroes outside of valid range
  virtual int GetDataClipped(void *data, int offset, int size) const;
  /// when end is reached, loop
  virtual int GetDataLooped(void *data, int offset, int size) const;

  /// preload data for GetDataClipped
  virtual bool RequestDataClipped(FileRequestPriority prior, int offset, int size) const;
  /// preload data for GetDataLooped
  virtual bool RequestDataLooped(FileRequestPriority prior, int offset, int size) const;
};

// Interface for Streaming Radio (OggStreamBuffer)
class IRadioStream : public RefCount
{
public:
  // decode bunch of data
  virtual int GetData(void *buffer, int size) const = 0;
  // store data into buffer (curl downloaded data). It also calls Init() when some data are downloaded...
  virtual int Write(void *buffer, int size) = 0;
  virtual void GetFormat(WAVEFORMATEX &format) const = 0;
  // data from radio are ready to be decoded and sent to audio
  virtual bool Ready() const = 0;
  // true when something went wrong
  virtual bool GetError() const = 0;
  // handle to related curl instance (or other)
  virtual void *GetHandle() const = 0;
  // name of stream (url)
  virtual const char *Name() const = 0;
};
void CreateWaveRadioStream(IRadioStream *radioStream, Ref<WaveStream> &waveStream);

#include <vorbis/vorbisfile.h>

class OggStreamBuffer : public IRadioStream
{
protected:
  static const int OggBufSize = 131072; //128kB

  // curl easy handle we are downloading data from
  void *_curl;
  RString _url;

  PoCriticalSection lock;
  inline void enter() const { lock.enter(); }
  inline void leave() const { lock.leave(); }

public:
  int  _totalSize;   // total size of current undecoded data in the buffers
  int  _readPos;     // current buffer to read data from
  char *_oggBuffer;  // writePos is _readPos+_totalSize

  //@{ vorbis file helpers
  mutable OggVorbis_File _vbfile;
  bool _vfOpen;   // Init() is called later, after some data are downloaded from internet
  WAVEFORMATEX _format;
  mutable int _bitstream;
  mutable int _offset;
  //@}

public:
  OggStreamBuffer(void *curl, const char *url);
  ~OggStreamBuffer();

  // initialize vorbisfile and callbacks
  void Init();
  // read data from buffer (orbisfile would read to decode it)
  int Read(void *buffer, int size);   
  
  //@{ IRadioStream interface
  // decode bunch of data::GetData
  int GetData(void *buffer, int size) const;
  // store data into buffer (curl downloaded data). It also calls Init() when some data are downloaded...
  int Write(void *buffer, int size);
  void GetFormat(WAVEFORMATEX &format) const;
  // data from radio are ready to be decoded and sent to audio
  bool Ready() const { return _vfOpen; }
  // true when something went wrong
  bool GetError() const 
  {
    return (_curl!=NULL); //TODO
  };
  void *GetHandle() const { return _curl; }
  // name of stream (url)
  const char *Name() const { return _url; };
  //@}
};

class InternetRadioSystem : public RefCount
{
public:
  virtual bool RequestUrl(const char *url, Ref<WaveStream> &stream) { return false; }
};

extern InternetRadioSystem* GIRadioSystem;

// file wave.cpp

WaveStream *WaveLoadFile( const char *name );
bool WSSRequestFile(const char *name, Ref<WaveStream> &stream, bool sync=false);
bool WaveRequestFile(const char *name, Ref<WaveStream> &stream, bool sync=false);

bool SoundRequestFile(const char *name, Ref<WaveStream> &stream, bool sync=false);

#endif
