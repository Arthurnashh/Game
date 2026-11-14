/**
@file OpenAL sound system
*/

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SOUND_OAL_HPP
#define _SOUND_OAL_HPP

#include <Es/Common/win.h>

#if defined _WIN32
  #if !defined _XBOX
    #include <mmsystem.h>
  #endif
  #include <al/al.h>
  #include <al/alc.h>
  #include <al/efx.h>
  #include "Eax4_0.h"
#else
  #include <al/al.h>
  #include <al/alc.h>
  #include <al/efx.h>
  #include "Eax4_0.h"
#endif

#include <El/FreeOnDemand/memFreeReq.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Threads/multisync.hpp>

#include "soundsys.hpp"
#include <El/SoundFile/soundFile.hpp>

struct WaveCacheOAL;

#include <Es/Memory/normalNew.hpp>

class SoundSystemOAL;

/// turning off 3D processing done via separate context
#define SEPARATE_2D_CONTEXT 0

//#define LogAL LogF
//#define LogAL Log

#define LogAL NoLog

#if _ENABLE_REPORT
  #define CHECK_AOL_ERROR(name) \
    do { \
      ALenum err = alGetError(); \
      if (err!=AL_NO_ERROR) \
      { \
        LogF("%s: OpenAL error %x",name,err); \
        /*Fail("OpenAL error");*/ \
      } \
    } while(false)
#else
  #define CHECK_AOL_ERROR(name) alGetError()
#endif

/// exclusive ownership of the OAL buffer
struct OALBuffer: public RefCount, private NoCopy
{
  /// which buffer
  ALuint _id;
  
  OALBuffer(ALuint id):_id(id){}
  ~OALBuffer()
  {
    alDeleteBuffers(1,&_id);
    CHECK_AOL_ERROR("alDeleteBuffers");
  }
  
  // TODO: fast alloc?
};

/// counted ownership of the OAL buffer
class OALBufferRef
{
  Ref<OALBuffer> _buffer;

  public:
  OALBufferRef(){}
  OALBufferRef(OALBuffer *src):_buffer(src){}
  
  /** will crash when used on buffer which was not properly constructed */
  operator ALuint () const {return _buffer->_id;}

  /// debugging - we want to display NULL buffers
  int GetDebugInt() const {return _buffer.NotNull() ? _buffer->_id : 0;}
  bool IsNull() const {return _buffer.IsNull();}
  void Free() {_buffer.Free();}
};

/// buffers used to play the sound
class WaveBuffersOAL
{
protected:
  Vector3 _position;
  Vector3 _velocity;

  float _volume; // volume used for 3D sound
  float _accomodation; // current ear accomodation setting

  //@{ current occlusion/obstruction levels
  float _occlusion,_obstruction;
  //@}
  float _volumeAdjust;
  WaveKind _kind;

  /// remember time of last buffer settings
  DWORD _parsTime;
  /// id of OpenAL buffer - used for caching / sharing
  OALBufferRef _buffer;
  /// id of OpenAL source (position/volume ... dependent on this)
  ALuint _source;
  /// is the source open?
  bool _sourceOpen;
  
  /// which context are we playing in
  #if SEPARATE_2D_CONTEXT
  InitPtr<ALCcontext> _context;
  #endif

  void MakeContextCurrent();
  
  /// time in ms when playback has started
  DWORD _startTime;
  /// time in ms when playback has stopped
  DWORD _stopTime;

  /// Is this one playing?
  bool _playing;
  /// request to start playing
  /*
  Playing the sound may be asynchronous. If we do not succeed immediately, we need to retry later
  */
  bool _wantPlaying;      
  
  /// 2D sound
  bool _only2DEnabled;
  
  /// even 3d sound can be temporarily played as 2D
  bool _3DEnabled;
  /// audio volume accomodation enabled
  bool _enableAccomodation;

  /// radio effects switched on
  bool _isRadio;

  float _gainSet;
  float _freqSet;
  float _distance2D;

  int _occlusionSet;
  int _obstructionSet;
  
  void SetGain(SoundSystemOAL *soundSys, float gain);
  void SetI3DL2Db(SoundSystemOAL *soundSys, int obstruction, int occlusion, bool imm);
  void DoSetI3DL2Db(SoundSystemOAL *soundSys, bool immediate);
  void DoSetPosition(SoundSystemOAL *soundSys, bool immediate);

  float CalcMinDistance() const;
  
  WaveBuffersOAL();
  ~WaveBuffersOAL();

  void Reset( SoundSystemOAL *sys );

  void UpdateVolume(SoundSystemOAL *soundSys);

  void SetVolume( SoundSystemOAL *soundSys,float vol, bool imm );

  bool GetMuted() const;
  float GetVolume() const {return _volume;}
  float GetFrequency() const {return _freqSet;}
  Vector3 GetPosition() const {return _position;}

  /// occlusion and obstruction level control
  void SetObstruction(SoundSystemOAL *soundSys, float obstruction, float occlusion, float deltaT);
  void SetAccomodation( SoundSystemOAL *soundSys,float accom );
  float GetAccomodation() const {return _accomodation;}

  void SetVolumeAdjust( SoundSystemOAL *soundSys, float volEffect, float volSpeech, float volMusic );

  void Set3D( bool is3D, float distance2D=-1 );
  bool Get3D() const;
  void SetRadio(bool isRadio);

  void EnableAccomodation( bool enable )
  {
    _enableAccomodation=enable;
    if( !enable ) _accomodation=1.0;
  }
  bool AccomodationEnabled() const {return _enableAccomodation;}

  void SetKind( SoundSystemOAL * soundSys, WaveKind kind );
  
  #if _ENABLE_REPORT
    virtual RString GetDebugName() const = 0;
  #endif
};

/// OpenAL sound buffer implementation of AbstractWave
class WaveOAL: public AbstractWave, public WaveBuffersOAL
{
  friend class SoundSystemOAL;

  public:
  ///  describe basic properties of the sound
  struct Header
  {
    UINT _size;         // size of data in bytes.
    LONG _frequency;      // sampling rate
    int _nChannel;        // number of channels
    int _sSize;           // number of bytes per sample
    
    ALenum GetALFormat() const;
  };
  protected:

  SoundSystemOAL *_soundSys;

  /// wave header  
  Header _header;

  //float _maxVolume,_avgVolume; // file properties

  bool _terminated;       // Is this one playing?
  bool _loadedHeader;        // Is this one loaded?
  bool _loaded;        // Is this one loaded?
  bool _loadError;        // Is this one loaded?
  bool _looping;
  //bool _enableAccomodation;

  float _skipped; // time skipped before headers were loaded
  int _curPosition; // current playing position
  bool _posValid; // 3d position explicitly set
  

  // streaming audio variables
  Ref<WaveStream> _stream;
  int _streamPos; // current position in stream
  /// buffers used in streaming
  AutoArray<ALuint> _streamBuffers;
  
  float _offset; // start position shift in bytes
  bool _applyOffset;

  /// which buffer we have already written data into
  int _writeBuf;
  /// count of free buffers - ready for writing
  int _freeBufN;
  
  /// last useful data went to this buffer
  int _endBuf;
  
  /// get position from both _skipped and _curPosition
  float GetTimePosition() const;
  
  public:
  enum Type {Free,Sw,Hw2D,Hw3D,NTypes};

  protected:
  Type _type; 
  static int _counters[NTypes];
  static int _countPlaying[NTypes];

  void AddStats( const char *name );
  static void ReportStats();

  public:
  static int TotalAllocated();
  static int Allocated( Type i ) {return _counters[i];}
    
  protected:

  /// check preload size for current stream
  int InitPreloadStreamSize(int &initSize) const;
  /// size of the buffer for given format
  static int StreamingBufferSize(const WAVEFORMATEX &format, int totalSize);
  /// decompressing can be slow - avoid doing too much in one frame
  static int StreamingMaxInOneFill(const WAVEFORMATEX &format, int bufferSize);
  /// init decompressing can be even slower - avoid doing too much in one frame
  static int StreamingMaxInInitFill(const WAVEFORMATEX &format, int bufferSize);
  /// size of subsection - must be power of two
  static int StreamingPartSize(const WAVEFORMATEX &format);

  void DoConstruct();
  
  bool IsStreaming(WaveStream *stream);

  void DoUnload();
  /// load basic information (size...)
  bool LoadHeader();

  /// 2D and 3D sounds require a different context
  ALCcontext *DesiredContext() const;

  /// once we know frequency, we want to convert time skipped into samples
  void ConvertSkippedToSamples();
  
  bool Load();

  void DoDestruct();

  public:


  //WaveOAL();
  WaveOAL( SoundSystemOAL *sSys, float delay ); // empty wave
  WaveOAL( SoundSystemOAL *sSys, RString name, bool is3D ); // real wave
  ~WaveOAL();

  bool Duplicate( const WaveOAL &wave ); // duplicate buffer

  void CacheStore( WaveCacheOAL &store );
  bool CacheLoad( WaveCacheOAL &store );

  void InitEAX();
  
  bool DoStop(); // stop playing immediately
  /// start playing, with an optional delay
  bool DoPlay(int delayMs=0);
  /// check if wave stopped playing (useful for waves which have unload pending}
  void CheckPlaying();


  private:
  WaveOAL( const WaveOAL &wave ); // duplicate buffer
  void operator = ( const WaveOAL &wave ); // duplicate buffer

  public:
    // set start sample, start is determined: offset * waveLength, range [0, 1]
    void SetStartOffset(float percentage);
    // return current position in percentage
    float GetCurrPosition() const;

  //void Queue(AbstractWave *wave,int repeat=1 );
  void Repeat(int repeat=1 );
  void Restart();
  
  //void AdvanceQueue();
  bool PreparePlaying(); // start playing
  void Play(); // start playing
  void Skip( float deltaT );
  void Advance( float deltaT ); // skip if stopped
  float GetLength() const; // get wave length in sec
  void Stop(); // stop playing immediately
  /// implementation of AbstractWave::Unload
  virtual void Unload();
  /// private unload - can by sync or async
  void Unload(bool sync);
  /// we know it is stopped - unload as soon as possible
  void UnloadStopped(bool sync);

  void LastLoop(); // stop playing on the end of the loop
  void PlayUntilStopValue( float time ); // play until some time
  void SetStopValue( float time ); // play until some time

  WaveKind GetKind() const {return _kind;}
  void SetKind( WaveKind kind ) {WaveBuffersOAL::SetKind(_soundSys,kind);}

  float GetFileMaxVolume() const {return 1;}
  float GetFileAvgVolume() const {return 1;}

  void SetFrequency( float freq, bool imm );

  // set sound parameters
  void SetVolume( float volume, float freq, bool immediate=false );

  void SetPosition( Vector3Par pos, Vector3Par vel, bool immediate=false );

  void SetAccomodation( float accom=1.0 )
  {
    WaveBuffersOAL::SetAccomodation(_soundSys,accom);
  }
  void SetObstruction(float obstruction, float occlusion, float deltaT=0)
  {
    WaveBuffersOAL::SetObstruction(_soundSys,obstruction,occlusion,deltaT);
  }

  float GetObstruction() const {return _obstruction;}
  virtual float GetOcclusion() const {return _occlusion;}

  float GetAccomodation() const {return WaveBuffersOAL::GetAccomodation();}
  void EnableAccomodation( bool enable ) {WaveBuffersOAL::EnableAccomodation(enable);}
  bool AccomodationEnabled() const {return WaveBuffersOAL::AccomodationEnabled();}

  bool Loaded() const {return _loaded;}
  bool IsTerminated();
  bool CanBeDeleted();
  bool IsStopped();
  bool IsWaiting();

  // equivalent distance of 2D sounds for position disabled sounds
  // this is necessary for loudness calculation
  float Distance2D() const;

  void Set3D( bool is3D, float distance2D);
  bool Get3D() const;
  virtual void SetRadio(bool isRadio);

  RString GetDiagText();

  bool IsMuted() const {return WaveBuffersOAL::GetMuted();}
  float GetVolume() const {return WaveBuffersOAL::GetVolume();}
  float GetFrequency() const {return WaveBuffersOAL::GetFrequency();}
  Vector3 GetPosition() const {return WaveBuffersOAL::GetPosition();}
  float GetLoudness() const;
  float GetMinDistance() const;
  
  virtual RString GetVolumeDiag() const;

  private:

  // un-queue oldest count buffers which are still queued
  void UnqueueStream(int count);
  
  void FillStream();
  bool FillStreamInit();


  // universal functions for both streaming / static sounds
  void StoreCurrentPosition();
  void AdvanceCurrentPosition(int skip);

  #if _ENABLE_REPORT
    RString GetDebugName() const {return Name();}
  #endif
};

class WaveGlobalOAL: public WaveOAL
{
  typedef WaveOAL base;

  Ref<WaveGlobalOAL> _queue; // next wave in queue
  bool _queued; // will be started by parent channel

  public:
  //WaveGlobalOAL();
  ~WaveGlobalOAL();
  WaveGlobalOAL( SoundSystemOAL *sSys, RString name, bool is3D );
  WaveGlobalOAL( SoundSystemOAL *sSys, float delay );

  private:
  WaveGlobalOAL( const WaveOAL &wave ); // duplicate buffer
  void operator = ( const WaveOAL &wave ); // duplicate buffer

  public:

  void AdvanceQueue();

  void Play();
  void Skip( float deltaT );
  void Advance( float deltaT );

  bool IsTerminated();

  void PreloadQueue();
  void Queue(AbstractWave *wave,int repeat=1 );

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

/// buffer data stored when buffer is not active
struct WaveCacheOAL
{
  RString _name;

  /// id of OpenAL buffer.
  OALBufferRef _buffer;

  WaveOAL::Header _header;

  ~WaveCacheOAL();
};

TypeIsMovableZeroed(WaveCacheOAL)


class SoundSystemOAL: public RefCount, public AbstractSoundSystem, public MemoryFreeOnDemandHelper
{
  friend class WaveOAL;
  friend class WaveBuffersOAL;
  friend class VoNSoundBufferOAL;
  
  private:
  LinkArray<WaveOAL> _waves; // store all playing waves
  
  AutoArray<WaveCacheOAL> _cache;

  //@{ volume settings as wanted by the user
  float _waveVolWanted;
  float _musicVolWanted;
  float _speechVolWanted;
  //@}

  //@{ relative volume adjust values - what is really applied
  float _volumeAdjustEffect;
  float _volumeAdjustSpeech;
  float _volumeAdjustMusic;
  //@}

  SoundEnvironment _eaxEnv;

  mutable CriticalSection _lock;
  
  ALCdevice *_device;  // play (capture?) device
  /// 3D context
  ALCcontext *_context3D;
  /// 2D context - no 3D processing
  #if SEPARATE_2D_CONTEXT
  ALCcontext *_context2D;
  #endif

  /// sometimes we need to suspend all sounds (like when not having focus)
  bool _active;
  
  Vector3 _listenerPos;
  Vector3 _listenerVel;
  Vector3 _listenerDir;
  Vector3 _listenerUp;
  DWORD _listenerTime;
  
  DWORD _minFreq,_maxFreq; // valid freq. range
  int _maxChannels;

  #if _ENABLE_PERFLOG
    int _wavSV;
    int _wavSD;
    int _wavSI;
    int _wavLD;
    int _wavLI;
    int _wavSt;
    int _wavPl;
  #endif
  
  bool _commited; // do not play anything until all is initialized (2nd frame)
  bool _soundReady;
  bool _hwWanted,_eaxWanted;
  bool _canHW,_canEAX;
  
  //@{ OpenAL extensions function pointers
  EAXSet _eaxSet;
  EAXGet _eaxGet;
  //@}
  /// specifier which should be used when HW acceleration is enabled
  RString _hwSpecifier;
  /// specifier which should be used when HW acceleration is disabled
  RString _swSpecifier;
  /// speficier for the native device - if present, used instead of generic HW
  RString _nativeSpecifier;
  
  bool _equalizerLoaded;
  bool _hwEnabled,_eaxEnabled;
  
  // preview state variables
  bool _doPreview;
  DWORD _previewTime;

  RefAbstractWave _previewSpeech;
  RefAbstractWave _previewEffect;
  RefAbstractWave _previewMusic;
  bool _enablePreview; // preview disabled until sound initialized

  public:
  void New(HWND win,bool dummy);
  void Delete();
  SoundSystemOAL();
  SoundSystemOAL(HWND hwnd, bool dummy);
  ~SoundSystemOAL();

  ALCdevice *OALDevice() const {return _device;}
  
  CriticalSection &GetLock() const {return _lock;}
  
  // return state after enable
  virtual bool EnableHWAccel(bool val);
  virtual bool EnableEAX(bool val);
  // get if feature is enabled
  virtual bool GetEAX() const;
  virtual bool GetHWAccel() const;
  virtual bool GetDefaultEAX() const;
  virtual bool GetDefaultHWAccel() const;

  virtual bool CanChangeEAX() const;
  virtual bool CanChangeHW() const;

  void LoadConfig(ParamEntryPar config);
  void SaveConfig(ParamFile &config);

  bool LoadHeaderFromCache( WaveOAL *wave );
  bool LoadFromCache( WaveOAL *wave );
  void StoreToCache( WaveOAL *wave );
  void ReserveCache( int number3D, int number2D );

  void LogChannels( const char *name );

  float GetWaveDuration( const char *filename );
  // create normal or streaming wave
  void AddWaveToList(WaveGlobalOAL *wave);
  AbstractWave *CreateWave(
    const char *name ,bool is3D=true ,bool prealloc=false
  );
  // create empty wave
  AbstractWave *CreateEmptyWave( float duration ); // duration in ms
  void Activate( bool active );

  /// estimate loudness of a sound before creating it
  float GetLoudness(WaveKind kind, Vector3Par pos, float volume, float accomodation) const;

  virtual void SetEnvironment( const SoundEnvironment &env );

  void DoSetEAXEnvironment();

  bool InitEAX();
  void DeinitEAX();

  void SetCDVolume(float val);
  float GetCDVolume() const;
  float GetDefaultCDVolume() const {return 5;}

  void SetWaveVolume(float val );
  float GetWaveVolume();
  float GetDefaultWaveVolume() {return 5;}

  void SetSpeechVolume(float val );
  float GetSpeechVolume();
  float GetDefaultSpeechVolume() {return 5;}

  void SetVonVolume(float val) {}
  float GetVonVolume() const {return 0;}
  float GetDefaultVoNVolume() const {return 5;}

  /// threshold fore recorded data to be considered of voice (values 0.0f - 1.0f - lower value = more data are considered as voice)
  virtual void SetVoNRecThreshold(float val) {}
  virtual float GetVoNRecThreshold() const {return 0.0f;}
  virtual float GetDefaultVoNRecThreshold() const {return 0.03f;}

  float GetVolumeAdjustEffect() const {return _volumeAdjustEffect;}
  float GetVolumeAdjustSpeech() const {return _volumeAdjustSpeech;}
  float GetVolumeAdjustMusic() const {return _volumeAdjustMusic;}
  float GetVolumeAdjustVoN() const {return 0;}

  void UpdateMixer();
  void PreviewMixer();

  void StartPreview();
  void TerminatePreview();

  void FlushBank(QFBank *bank);

  void SetListener
  (
    Vector3Par pos, Vector3Par vel,
    Vector3Par dir, Vector3Par up
  );
  Vector3 GetListenerPosition() const {return _listenerPos;}
  void Commit();

  private:
  void CreateDevice();
  void DestroyDevice();

  // unload all waves, destroy and create a device, load waves again
  void ResetDevice();
  

  public:

  // implementation of MemoryFreeOnDemandHelper
  virtual float Priority() const;
  virtual RString GetDebugName() const;
  virtual size_t MemoryControlled() const;
  virtual size_t FreeOneItem();

};

/// Conversion of "gain" to MiliBel (used also in VoiceOAL)
int Float2MiliBel( float val );

#endif
