#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SOUNDSYS_HPP
#define _SOUNDSYS_HPP

#include "types.hpp"

#include <Es/Strings/rString.hpp>
#include <El/ParamFile/paramFileDecl.hpp>

enum WaveKind {WaveEffect,WaveSpeech,WaveMusic,WaveSpeechEx};

enum FilterTypeEx
{ 
  NoFilter = 0,
  GenericLowPFilter,
  InternalFireLowPFilter,
  SubmixLowPFilter,
  RadioBandPFilter,
  HalfInternalBandFilter
};

typedef void WaveCallback( AbstractWave *wave, RefCount *context );

class AbstractWave: public RefCountWithLinks
{
	RString _name;
	bool _sticky;
	WaveCallback *_onTerminate;
	Ref<RefCount> _onTerminateContext;

	WaveCallback *_onPlay;
	Ref<RefCount> _onPlayContext;

	protected:
	float _stopTreshold; // sound should be stopped if Glob.time>_stopTime
  // enable channels distribution recalculation, used for explosions - no handle(Wave*) to wave for update
  bool _autoUpdateEnable; 

	// playing one sample
	public:

  bool debugMe;

	RString Name() const {return _name;}
	explicit AbstractWave( RString name );
	void SetName( RString name ) {_name=name;}
	
	virtual ~AbstractWave();
	
	/// preload next wave in the queue
  virtual void PreloadQueue();
  /// queue playing
	virtual void Queue( AbstractWave *wave, int repeat=1 ) = 0;
	virtual void Repeat( int repeat=1 ) = 0; // set repeat count (1 or infinity)

	virtual void Play() = 0; // start playing
	virtual void Stop() = 0; // stop playing immediatelly
	/// prepare all files for playing, return true once ready
	virtual bool PreparePlaying() = 0;
	virtual void Unload() {} // unload, prepare to be freed
	virtual void LastLoop() = 0; // stop playing at the end of the loop

	virtual void PlayUntilStopValue( float time ) = 0; // play until some time
	virtual void SetStopValue( float time ) = 0; // play until some time

  virtual bool LoadError() const { return false; }

	void OnTerminateOnce();
	void OnPlayOnce();

	void SetOnTerminate( WaveCallback *function, RefCount *context )
	{
		_onTerminate = function;
		_onTerminateContext = context;
	}
	void SetOnPlay( WaveCallback *function, RefCount *context )
	{
		_onPlay = function;
		_onPlayContext = context;
	}
	
	virtual bool IsWaiting() = 0;
	virtual void Skip(float deltaT) = 0; // advance time
	virtual void Advance( float deltaT ) = 0; // skip if stopped
	virtual float GetLength() const = 0; // get wave length in sec

  //! check if buffer can be deleted with no performance penalty
	virtual bool CanBeDeleted() {return true;}
	virtual bool IsStopped() = 0; // query for Stop status
	virtual bool IsTerminated() = 0; // query for termination status
	virtual void Restart() = 0; // start playing again
	
	// set sound parameters
	virtual bool IsMuted() const = 0; // check if wave is muted
	virtual float GetVolume() const = 0;
	virtual float GetFrequency() const = 0;
	virtual void SetVolume( float volume, float freq=-1, bool immediate=false ) = 0;

  // distance where is sound audible
  virtual void SetMaxDistance(float distance) {}
  virtual float GetMaxDistance() const {return 0;}

  // set whistling params
  virtual void SetEarWhistling(float volume, bool immediate = true) {}
  virtual bool CanCauseWhistling() const { return false; }
  virtual void SetEWPars(bool whistling, const Vector3 &pos, float maxDist) {}
  virtual Vector3 GetWhPos() { return VZero; }
  virtual float WhistlingDist() const { return 0; }

	// get volume scanned in file
	virtual float GetFileMaxVolume() const = 0;
	virtual float GetFileAvgVolume() const = 0;

  //@{ obstruction/occlusion level - 1 means not obstructed
	virtual void SetObstruction(float obstruction, float occlusion, float deltaT=0) = 0;
  virtual float GetObstruction() const = 0;
  virtual float GetOcclusion() const = 0;
  //@}
  
	virtual void SetAccomodation(float accom=1.0) = 0;
	virtual float GetAccomodation() const = 0;
	virtual void EnableAccomodation( bool enable ) = 0;
	virtual bool AccomodationEnabled() const = 0;
  virtual void EnableFilter(bool enable, FilterTypeEx fType) { }

	virtual void SetKind( WaveKind kind ) = 0;
	virtual WaveKind GetKind() const = 0;
	
	void SetSticky( bool sticky ){_sticky=sticky;}
	bool GetSticky() const {return _sticky;}

  __forceinline void EnableAutoUpdate(bool autoUpdateEnable) {_autoUpdateEnable = autoUpdateEnable;}
  __forceinline bool AutoUpdateEnabled() const {return _autoUpdateEnable;}
  virtual void UpdatePosition() {}
	virtual void SetPosition
	(
		Vector3Par pos, Vector3Par vel, bool immediate=false
	) = 0;
	virtual Vector3 GetPosition() const = 0;
	// perceived loudness based on volume and position
	virtual float GetLoudness() const {return 1.0;}
  virtual float GetMinDistance() const {return 1.0;}

  virtual RString GetVolumeDiag() const {return RString();}
  
	// set if sound should be 3D processed
	// if not, volume is computed based distance argument
	virtual void Set3D( bool is3D, float distance2D=-1 ) = 0;
	virtual bool Get3D() const = 0;
	/// simulate radio comm voice distortion
	virtual void SetRadio(bool isRadio) = 0;
	//virtual void DisablePosition( float volume, bool immediate=false ) = 0;
	//virtual void EnablePosition( float volume, bool immediate=false ) = 0;

	//virtual bool PositionEnabled() const = 0;
	// equvalent distance of 2D sounds for position disabled sounds
	// this is necessary for loudness calculation
	virtual float Distance2D() const = 0;

  // set start sample, start is determined: offset * waveLength, range [0, 1]
  virtual void SetStartOffset(float offset) {}
  // return current position in percentage
  virtual float GetCurrPosition() const { return 0;}
};

#define AUDIO_BUFFER_SIZE       131072

class AbstractDecoderWave: public RefCount
{
public:
  virtual bool InitSourceVoice(int channels, int rate) = 0;
  virtual bool FillBuffer(const void *data, size_t nbytes) = 0;
  virtual __int64 GetVoicePosition() const = 0;
  virtual bool IsBuffersStopped() const = 0;
  virtual void Pause(bool pause) = 0;
  virtual void Play() = 0;
  virtual void Stop() = 0;
};

//! Ref to AbstractWave that takes care of unloading before destruction

class RefAbstractWave: public Ref<AbstractWave>
{
  typedef Ref<AbstractWave> base;
  
  public:
  __forceinline void Unload()
  {
    AbstractWave *wave = GetRef();
    if (wave)
    {
      wave->Unload();
    }
  }
  RefAbstractWave(){}
  RefAbstractWave(AbstractWave *wave):base(wave){}
  void operator = (AbstractWave *wave)
  {
    if (wave==GetRef()) return;
    Unload();
    base::operator =(wave);
  }
  void Free()
  {
    Unload();
    base::Free();
  }
  
  ~RefAbstractWave(){Unload();}
};

TypeIsMovableZeroed(RefAbstractWave)

enum SoundEnvironmentType
{
	SEPlain, // normal
	SECity, // houses around
	SEForest, // in forest
	SEMountains, // mountains
	SERoom, // inside building
};

struct SoundEnvironment
{
	SoundEnvironmentType type;
	float size; // how large (controls mostly echo time)
	float density; // how dense (echo intensity)
};

class QFBank;

enum SubmixOutput
{ 
  DefaultOutput = 0,
  CustomFilterOutput
};

class AbstractSubmix: public RemoveLinks
{
public:
  virtual void Unload() {}
  virtual void Include(AbstractWave *wave) {}
  virtual void Exclude(AbstractWave *wave) {}
  virtual float Calculate3D(bool enable3D, const Vector3 &position, const Vector3 &speed, float distance, float vol) { return 0; }
  virtual FilterTypeEx SetFilter(FilterTypeEx filter) { return NoFilter; }
  virtual void SetOrientation(Vector3Val top, Vector3Val front) {}
  virtual void EnableDirectionalSound(bool enable) = 0;
  virtual void SetDirSoundConePars(float innerAngle, float outerAngle, float innerVolume, float outerVolume) = 0;
  virtual void SetOutputVoiceSound(SubmixOutput sOut) {}
  virtual int WaveCount() const { return 0; }
  virtual void *GetHandle() {return NULL; }
};

class AbstractSoundSystem // whole playing system
{
	public:

	virtual float GetWaveDuration( const char *filename ) = 0;

	virtual AbstractWave *CreateEmptyWave( float duration ) = 0; // duration in ms
  virtual AbstractSubmix *CreateSubmix() { return NULL; }
  virtual AbstractDecoderWave *CreateDecoderWave() { return NULL; }
  /// estimate loudness of a sound before creating it
	virtual float GetLoudness(WaveKind kind, Vector3Par pos, float volume, float accomodation) const = 0;
	
	virtual AbstractWave *CreateWave(
		const char *filename, bool is3D=true, bool prealloc=false
	) = 0;
	virtual void SetListener(
		Vector3Par pos, Vector3Par vel,
		Vector3Par dir, Vector3Par up
	) = 0;
	virtual Vector3 GetListenerPosition() const = 0;
	virtual void Commit() = 0;
	virtual void Activate( bool active ) = 0;
	virtual void SetEnvironment( const SoundEnvironment &env ) {}
	virtual void GetEnvironment( SoundEnvironment &env ) {}
	virtual ~AbstractSoundSystem(){}

	virtual void SetCDVolume( float vol ) = 0;
  virtual float GetCDVolume() const = 0;
  virtual float GetDefaultCDVolume() const = 0;

	virtual void SetWaveVolume( float vol ) = 0;
	virtual float GetWaveVolume() = 0;
  virtual float GetDefaultWaveVolume() = 0;

	virtual void SetSpeechVolume( float vol ) = 0;
	virtual float GetSpeechVolume() = 0;
  virtual float GetDefaultSpeechVolume() = 0;

  virtual void SetVonVolume(float val) = 0;
  virtual float GetVonVolume() const = 0;
  virtual float GetDefaultVoNVolume() const = 0;

  /// threshold fore recorded data to be considered of voice (values 0.0f - 1.0f - lower value = more data are considered as voice)
  virtual void SetVoNRecThreshold(float val) = 0;
  virtual float GetVoNRecThreshold() const = 0;
  virtual float GetDefaultVoNRecThreshold() const = 0;

  virtual float GetVolumeAdjustEffect() const = 0;
  virtual float GetVolumeAdjustSpeech() const = 0;
  virtual float GetVolumeAdjustMusic() const = 0;
  virtual float GetVolumeAdjustVoN() const = 0;

	virtual void StartPreview() = 0;
	virtual void TerminatePreview() = 0;

	virtual void FlushBank(QFBank *bank) {}

	// return state after enable
	virtual bool EnableHWAccel(bool val) = 0;
	virtual bool EnableEAX(bool val) = 0;
	// get if feature is enabled
	virtual bool GetEAX() const = 0;
	virtual bool GetHWAccel() const = 0;

  //@{ check if restart is needed because of changes
  virtual bool CanChangeHW() const = 0;
  virtual bool CanChangeEAX() const = 0;
  //@}

	virtual bool GetDefaultEAX() const = 0;
	virtual bool GetDefaultHWAccel() const = 0;

	virtual void LoadConfig(ParamEntryPar config) = 0;
	virtual void SaveConfig(ParamFile &config) = 0;

  //@{ samples count
  virtual int GetSamplesCountMin() const { return 8; }
  virtual int GetSamplesCountMax() const { return 128; }
  virtual int GetSamplesCount() const { return 16; }
  virtual int GetSamplesCountDefault() const { return 16; }
  virtual void SetSamplesCount(int val) {}
  //@}

  virtual void SimulateMasterVoice(float deltaT, bool inside) {}
  virtual void UpdateRadioSubmixes(bool simEnabled) {}
  virtual void UpdateGlobalVolume(float volume) {}
  virtual void UpdateGlobalSoundVolume(float volume) {}
  virtual void UpdateGlobalRadioVolume(float volume) {}
  virtual void UpdateGlobalSpeechExVolume(float volume) {}

  // microphone sensitivity adjustments
  virtual void StartCapture(float time) { }
  virtual void StopCapture() { }
  virtual bool CaptureRunning() const { return false; }
  virtual float GetThreasholdWanted() const { return 0.0f; }

  virtual void SetCustomVoiceFilter(bool set) {}
};

extern AbstractSoundSystem *GSoundsys;

const float SpeedOfSound=330;
const float InvSpeedOfSound=1/SpeedOfSound;

#endif


