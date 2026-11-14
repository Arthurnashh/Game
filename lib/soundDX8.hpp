// DirectX sound system
// (C) 1996, SUMA
#ifndef _SOUND_DX8_HPP
#define _SOUND_DX8_HPP

#include <Es/Common/win.h>
#include <El/FreeOnDemand/memFreeReq.hpp>
#include <Es/Containers/array.hpp>

#if defined _XBOX
  #define IMySoundPrimBuffer IDirectSoundBuffer8
  #define IMySoundBuffer IDirectSoundBuffer8
  #define IMySound3DBuffer IDirectSoundBuffer8
  #define IMySound IDirectSound
#elif defined _WIN32
  #include <mmsystem.h>
  #include <dsound.h>
  #include "eax.h"

  #define IMySoundPrimBuffer IDirectSoundBuffer
  #define IMySoundBuffer IDirectSoundBuffer8
  #define IMySound3DBuffer IDirectSound3DBuffer8
  #define IMySound IDirectSound8
#endif

#include "soundsys.hpp"
#include <El/SoundFile/soundFile.hpp>

#ifdef _XBOX
  struct BufferData: public Buffer<char>
  {
    explicit BufferData(int size)
    :Buffer<char>(size)
    {
    }
  };
#endif

struct WaveDSCache8;

#include <Es/Memory/normalNew.hpp>

class SoundSystem8;

/// buffers used to play the sound
class WaveBuffers8
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

#ifdef _WIN32

  /// remember buffer settings - avoid setting data which are no different
  DS3DBUFFER _pars;
  /// remember time of last buffer settings
  DWORD _parsTime;
  /// Pointer to direct sound buffer.
  ComRef<IMySoundBuffer> _buffer;
  /// time in ms when playback has started
  DWORD _startTime;
  /// time in ms when playback has stopped
  DWORD _stopTime;
#ifndef _XBOX
  /// optional pointer to DS3D interface
  ComRef<IMySound3DBuffer> _buffer3D;
  __forceinline IMySound3DBuffer *GetBuffer3D() const {return _buffer3D.GetRef();}
#else
  /// GetCurrentPosition is unreliable on Xbox
  /** we should use start time to simulate it as accuratelly as possible */
  
  Ref<BufferData> _data; // pointer to the data
  __forceinline IMySound3DBuffer *GetBuffer3D() const {return _buffer.GetRef();}
#endif


#endif

  bool _playing;       // Is this one playing?
  bool _wantPlaying;       // Is this one playing?
  bool _wantUnload;       // this one should be unloaded once stop is finished
  bool _only2DEnabled; // 2D sound
  bool _3DEnabled; // 3D may be temporarily disabled
  bool _enableAccomodation;

  int _volumeSet; // 2D volume set
  float _freqSet;
  float _distance2D;

  int _occlusionSet;
  int _obstructionSet;
  
  void SetVolumeDb( LONG volumeDb );
  void SetI3DL2Db(int obstruction, int occlusion, bool imm);
  void DoSetI3DL2Db(bool immediate);
  void DoSetPosition(bool immediate, bool hwAcceleration);

  float CalcMinDistance() const;
  
  WaveBuffers8();
  ~WaveBuffers8();

  void Reset( SoundSystem8 *sys );

  void UpdateVolume();

  void SetVolume( float vol, bool imm );

  bool GetMuted() const;
  float GetVolume() const {return _volume;}
  float GetFrequency() const {return _freqSet;}
  Vector3 GetPosition() const {return _position;}

  /// occlusion and obstruction level control
  void SetObstruction(float obstruction, float occlusion, float deltaT);
  void SetAccomodation( float accom );
  float GetAccomodation() const {return _accomodation;}

  void SetVolumeAdjust( float volEffect, float volSpeech, float volMusic );

  void Set3D( bool is3D, float distance2D=-1 );
  bool Get3D() const;

  void EnableAccomodation( bool enable )
  {
    _enableAccomodation=enable;
    if( !enable ) _accomodation=1.0;
  }
  bool AccomodationEnabled() const {return _enableAccomodation;}

  void SetKind( SoundSystem8 * sys, WaveKind kind );
  
  #if _ENABLE_REPORT
    virtual RString GetDebugName() const = 0;
  #endif
};

/// DX8 sound buffer implementation of AbstractWave
class Wave8: public AbstractWave, public WaveBuffers8
{
  friend class SoundSystem8;

  public:
  ///  describe basic properties of the sound
  struct Header
  {
    UINT _size;         // size of data in bytes.
    LONG _frequency;      // sampling rate
    int _nChannel;        // number of channels
    int _sSize;           // number of bytes per sample
  };
  protected:

  SoundSystem8 *_soundSys;

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

  DWORD _writePos; // where in the buffer we have already written data
  DWORD _endPos; // last usefull data went to this position
  bool _lastLoopSet;
  bool _lastLoopVerified;

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
  void GetBufferDesc(DSBUFFERDESC &dsbd, WAVEFORMATEX &format, bool streaming) const;
  bool IsStreaming(WaveStream *stream);

  void DoUnload();
  /// load basic information (size...)
  bool LoadHeader();

  /// once we know frequency, we want to convert time skipped into samples
  void ConvertSkippedToSamples();
  
  bool Load();

  void DoDestruct();

  public:


  //Wave8();
  Wave8( SoundSystem8 *sSys, float delay ); // empty wave
  Wave8( SoundSystem8 *sSys, RString name, bool is3D ); // real wave
  ~Wave8();

  bool Duplicate( const Wave8 &wave ); // duplicate buffer

  void CacheStore( WaveDSCache8 &store );
  bool CacheLoad( WaveDSCache8 &store );

  bool DoStop(); // stop playing immediatelly
  bool DoPlay(); // start playing
  /// check if wave is already unloaded
  bool CheckUnload();
  /// check if wave stopped playing (usefull for waves which have unload pending}
  void CheckPlaying();


  private:
  Wave8( const Wave8 &wave ); // duplicate buffer
  void operator = ( const Wave8 &wave ); // duplicate buffer

  public:

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
  void SetKind( WaveKind kind ) {WaveBuffers8::SetKind(_soundSys,kind);}

  float GetFileMaxVolume() const {return 1;}
  float GetFileAvgVolume() const {return 1;}

  DWORD CalcFrequencyDword() const;

  void SetFrequency( float freq, bool imm );

  // set sound parameters
  void SetVolume( float volume, float freq, bool immediate=false );

  void SetPosition( Vector3Par pos, Vector3Par vel, bool immediate=false );

  void SetAccomodation( float accom=1.0 )
  {
    WaveBuffers8::SetAccomodation(accom);
  }
  void SetObstruction(float obstruction, float occlusion, float deltaT=0)
  {
    WaveBuffers8::SetObstruction(obstruction,occlusion,deltaT);
  }

  float GetObstruction() const {return _obstruction;}
  virtual float GetOcclusion() const {return _occlusion;}

  float GetAccomodation() const {return WaveBuffers8::GetAccomodation();}
  void EnableAccomodation( bool enable ) {WaveBuffers8::EnableAccomodation(enable);}
  bool AccomodationEnabled() const {return WaveBuffers8::AccomodationEnabled();}

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

  bool IsMuted() const {return WaveBuffers8::GetMuted();}
  float GetVolume() const {return WaveBuffers8::GetVolume();}
	float GetFrequency() const {return WaveBuffers8::GetFrequency();}
  Vector3 GetPosition() const {return WaveBuffers8::GetPosition();}
	float GetLoudness() const;
	float GetMinDistance() const;

  private:

  void FillStream();
  bool FillStreamInit();


  // universal functions for both streaming / static sounds
  void StoreCurrentPosition();
  void AdvanceCurrentPosition(int skip);

  #if _ENABLE_REPORT
    RString GetDebugName() const {return Name();}
  #endif
};

class WaveGlobal8: public Wave8
{
  typedef Wave8 base;

  Ref<WaveGlobal8> _queue; // next wave in queue
  bool _queued; // will be started by parent channel

  public:
  //WaveGlobal8();
  ~WaveGlobal8();
  WaveGlobal8( SoundSystem8 *sSys, RString name, bool is3D );
  WaveGlobal8( SoundSystem8 *sSys, float delay );

  private:
  WaveGlobal8( const Wave8 &wave ); // duplicate buffer
  void operator = ( const Wave8 &wave ); // duplicate buffer

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
struct WaveDSCache8
{
  RString _name;

#if defined _XBOX
  Ref<BufferData> _data;
#elif defined _WIN32

  /// Pointer to direct sound buffer.
  ComRef<IMySoundBuffer> _buffer;

  /// optional pointer to DS3D interface
  ComRef<IMySound3DBuffer> _buffer3D;

  Wave8::Header _header;

#endif

  ~WaveDSCache8();
};

TypeIsMovableZeroed(WaveDSCache8)




#if defined _WIN32 && !defined _XBOX
class AudioMixer;
#endif

class SoundSystem8: public RefCount, public AbstractSoundSystem, public MemoryFreeOnDemandHelper
{
  friend class Wave8;
  friend class WaveBuffers8;
  
  private:
  #if defined _WIN32 && !defined _XBOX
  SRef<AudioMixer> _mixer;
  #endif

  LinkArray<Wave8> _waves; // store all playing waves
  RefArray<Wave8> _wavesUnload; // all waves waiting for unload opportunity
  
  AutoArray<WaveDSCache8> _cache;
  int _cacheUniqueBuffers; // count of non-duplicated buffers in cache

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

  #if defined _WIN32 && !defined _XBOX

  ComRef<IMySoundPrimBuffer> _primary;

  ComRef<IDirectSound3DListener8> _listener;  // Pointer to direct sound buffer.

  #endif

  #if defined _WIN32 && !defined _XBOX
  ComRef<IMySoundBuffer> _eaxFakeListener;
  ComRef<IKsPropertySet> _eaxListener;
  #endif
  SoundEnvironment _eaxEnv;

  //ComRef<IDirectMusicLoader8> _loader;
  //ComRef<IDirectMusicPerformance8> _performance;

  #ifdef _WIN32
  ComRef<IMySound> _ds;
  #endif

  #ifdef _XBOX
  DSEFFECTIMAGEDESC *_fxdesc;
  #endif

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
  bool _duplicateWorks;
  #ifndef _XBOX
    bool _hwWanted,_eaxWanted;
    bool _canHW,_canEAX;
  #endif
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
  SoundSystem8();
  SoundSystem8(HWND hwnd, bool dummy);
  ~SoundSystem8();

  // return state after enable
  virtual bool EnableHWAccel(bool val);
  virtual bool EnableEAX(bool val);
  virtual bool CanChangeHW() const;
  virtual bool CanChangeEAX() const;
  // get if feature is enabled
  virtual bool GetEAX() const;
  virtual bool GetHWAccel() const;
	virtual bool GetDefaultEAX() const;
  virtual bool GetDefaultHWAccel() const;

  void LoadConfig(ParamEntryPar config);
  void SaveConfig(ParamFile &config);

  bool LoadHeaderFromCache( Wave8 *wave );
  bool LoadFromCache( Wave8 *wave );
  void StoreToCache( Wave8 *wave );
  void ReserveCache( int number3D, int number2D );

  void LogChannels( const char *name );

  float GetWaveDuration( const char *filename );
  // create normal or streaming wave
  void AddWaveToList(WaveGlobal8 *wave);
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

  public:
  #ifdef _WIN32
  IMySound *DirectSound(){return _ds;} // Wave8 class uses this (voiceDX8 too)
  #endif

  #ifdef _XBOX
  DSEFFECTIMAGEDESC *GetEffectDesc() const {return _fxdesc;}
  #endif

  // implementation of MemoryFreeOnDemandHelper
  virtual float Priority() const;
  virtual RString GetDebugName() const;
  virtual size_t MemoryControlled() const;
  virtual size_t FreeOneItem();

};

#endif
