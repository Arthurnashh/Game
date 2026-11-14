#ifndef _OGG_DECODER
#define _OGG_DECODER

#ifdef _WIN32

#include <El/QStream/qStream.hpp>
#include <theora/theoradec.h>
#include <vorbis/codec.h>

#include <Es/Containers/maps.hpp>

enum StreamType 
{
  TYPE_VORBIS,
  TYPE_THEORA,
  TYPE_UNKNOWN
};

class OggStream;

class TheoraDecode 
{
public:
  th_info         _info;
  th_comment      _comment;
  th_setup_info*  _setup;
  th_dec_ctx*     _ctx;

public:
  TheoraDecode();
  ~TheoraDecode(); 
  void initForData(OggStream* stream);
};

class VorbisDecode 
{
public:
  vorbis_info         _info;
  vorbis_comment      _comment;
  vorbis_dsp_state    _dsp;
  vorbis_block        _block;

public:
  VorbisDecode();
  ~VorbisDecode();
  void initForData(OggStream* stream);
};

class OggStream
{
public:
  int _serial;
  ogg_stream_state _state;
  StreamType _type;
  bool _active;
  TheoraDecode _theora;
  VorbisDecode _vorbis;

public:
  OggStream(int serial = -1);
  ~OggStream();
};

 
typedef ExplicitMap<int, OggStream*,true,MemAllocSafe> StreamMap;

class AbstractDecoderWave;

class OggDecoder: public RefCount
{
public:
  StreamMap          _streams;  
  ogg_int64_t        _granulepos;
  ogg_sync_state     _state;
  OggStream*         _video;
  OggStream*         _audio;
  RefR<AbstractDecoderWave> _wave;
  float             _framerate;
  Time              _lastTime;
  int               _textureWidth;
  int               _textureHeight;

  typedef AutoArray<unsigned32>::DataType VideoDataType;
  /// double buffering used so that updating and acquiring data can hapeens simultaneously
  AutoArray<VideoDataType> _data[2];
  CriticalSection _dataCS[2];

  int               _fileIdent;

  RStringB           _name;
  bool              _videoDone;
  bool              _audioDone;
  
  typedef AutoArray<short>::DataType  AudioBufferDataType;
  AutoArray<AudioBufferDataType>   _audioBuffer;
  AudioBufferDataType*             _audiobufferIndex;
  int                _totalSamples;

  bool              _stopped;
  bool              _paused;
  bool              _isRunning;
  HANDLE            _threadHandle;

  int               _loops;
  int               _loopsQueried;    // used for playback done event
  int               _initFilePos;

  bool              _stopWanted;
  Event             _pauseEvent;      // playback pause
  Event             _loopsEvent;      // wait until loops count is set
  /// which _data are ready to be acquired
  int              _frameCompleteId;

private:
  bool handleTheoraHeader(OggStream* stream, ogg_packet* packet);
  bool handleVorbisHeader(OggStream* stream, ogg_packet* packet);
  void readHeaders(ogg_sync_state* state, bool isMainThread = false);

  bool readPage(ogg_sync_state* state, ogg_page* page, bool isMainThread = false);
  bool readPacket(ogg_sync_state* state, OggStream* stream, ogg_packet* packet);
  void handleTheoraData(OggStream* stream, ogg_packet* packet, void *videoData);

  DWORD WINAPI threadProcNonStatic( LPVOID pParam );
  static DWORD WINAPI threadProc( LPVOID pParam );
  
  void  initCodec(bool isMainThread);
  bool  play();
  void  deinit();
  void AllocateBuffers();
  bool OpenVideoBuffer();
  bool InitCommon(RStringB name, int loops);

  bool CreateAudio(int channels, int frequency);
  void PauseAudio(bool pause);
  void PlayAudio();
  void StopAudio();
  bool IsBufferStopped() const;
  bool FillBuffer(const void *data, size_t nbytes);
  __int64 GetVoicePosition();

public:
  OggDecoder();
  ~OggDecoder();
  void init(RStringB name, int &texture_width, int &texture_height, bool autoplay=true, int loops = 0);
  void reinit();
  void Restart();
  bool getData(void **videoData);
  bool isReady();
  bool isPlaying();
  void pause(bool pause);
  void stop();
  void setLoops(int loops);
  void WaitUntilStopped();
  // Used for event firing, cann be called only from one place
  bool IsPlaybackDone();
  void Start();
};

#else
class OggDecoder: public RefCount
{
public:
  OggDecoder() {}
  ~OggDecoder() {}
  void init(RStringB name, int &texture_width, int &texture_height, bool autoplay=true, int loops = 0) {}
  void reinit() {}
  void Restart() {}
  bool getData(void **videoData) {return false;}
  bool isReady() {return false;}
  bool isPlaying() {return false;}
  void pause(bool pause) {}
  void stop() {}
  void setLoops(int loops) {}
  void WaitUntilStopped() {}
  // Used for event firing, cann be called only from one place
  bool IsPlaybackDone() {return false;}
  void Start() {}
};

#endif

#endif

