/**
@file OpenAL sound system
*/

#define INITGUID 1 // we need to init EAX GUIDs
#include "wpch.hpp"
#include "soundOAL.hpp"
#include "keyInput.hpp"
#include "global.hpp"
#include "textbank.hpp"
#include "engine.hpp"
#include "paramFileExt.hpp"
#include "Network/network.hpp"

#include "dikCodes.h"
#include "diagModes.hpp"
#include <El/FileServer/fileServer.hpp>
#include <El/QStream/qbStream.hpp>
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>
#include <El/FreeOnDemand/memFreeReq.hpp>
#include <El/Breakpoint/breakpoint.h>
#include <Es/Containers/rStringArray.hpp>
#include <El/Evaluator/express.hpp>

#if !defined _XBOX || !_DISABLE_GUI
  #define USE_EAX 1
#endif

#if defined _WIN32 && !defined _XBOX
#include <mmsystem.h>
#include "DelayImp.h"
#endif

#define DIAG_VOL 0
#define DIAG_STREAM 0
#define DIAG_STREAM_FILL 0 // 0...50

#if defined _WIN32 && !_DISABLE_GUI
#pragma comment(lib,"OpenAL32.lib")
#endif


inline void WaveBuffersOAL::MakeContextCurrent()
{
  #if SEPARATE_2D_CONTEXT
  alcMakeContextCurrent(_context);
  #endif
}




#define DO_PERF 0
#if DO_PERF
#include "perfLog.hpp"
#endif

//#include "engine.hpp"


void WaveOAL::DoConstruct()
{
  _type=Free;
  _soundSys=NULL;
  _header._size=0;         // Size of data.
  _header._frequency=0;      // sampling rate
  _header._nChannel=0;        // number of channels
  _header._sSize=0;           // number of bytes per sample
  _playing=false;       // Is this one playing?
  _wantPlaying = false;
  _loaded=false;
  _loadedHeader = false;
  _loadError=false;
  _only2DEnabled=false;
  _looping=true;
  _terminated=false;
  _sourceOpen = false;

  _volume=1.0;
  _accomodation=1.0;
  _volumeAdjust = 1;
  _enableAccomodation=true;

  _stopTreshold = FLT_MAX; // never stop

  _offset = 0;
  _applyOffset = false;

  _obstructionSet = _occlusionSet = 0;
  _obstruction = _occlusion = 1;
  _gainSet=1;
  _freqSet=1;
  _curPosition=0;
  _skipped = 0;
  _position=VZero;
  _velocity=VZero;
}

/*
WaveGlobalOAL::WaveGlobalOAL()
{
  _queued=false;
}
*/

WaveGlobalOAL::~WaveGlobalOAL()
{
}

#define DIAG_LOAD 0

WaveGlobalOAL::WaveGlobalOAL( SoundSystemOAL *sSys, float delay )
:WaveOAL(sSys,delay)
{
  _queued=false;
}

WaveGlobalOAL::WaveGlobalOAL( SoundSystemOAL *sSys, RString name, bool is3D )
:WaveOAL(sSys,name,is3D)
{
  _queued=false;
}

DEFINE_FAST_ALLOCATOR(WaveGlobalOAL)

/*!
\patch 5160 Date 5/17/2007 by Ondra
- Fixed: Improved radio protocol fluency by binding words together.
*/
void WaveGlobalOAL::AdvanceQueue()
{
  if( !_queue ) return;
  if( _terminated )
  {
    if( _queue )
    {
      _queue->_queued=false;
      _queue->DoPlay();
      _queued=true;
      //LogF("Sound %s activated",(const char *)_queue->Name());
    }
    OnTerminateOnce();
  }
  // even before terminated we could preload and schedule the following sample
  else if (_playing && _queue->PreparePlaying())
  {
    // check how much is left from current sample to play
    // if it is less than a frame, start immediately
    // moreover, we want some overlap as well, to prevent sounding robotic
    #if _ENABLE_CHEATS
      static int maxFrameDuration = 100;
      static int overlapCoef = 8;
      const int realOverlap = 75; // how much do we really want the words to overlap
    #else
      const int maxFrameDuration = 100; // max. frame duration for estimations
      const int overlapCoef = 8; // x/16
      const int realOverlap = 75; // how much do we really want the words to overlap
    #endif
    int lastFrame = GEngine->GetLastFrameDuration();
    int avgFrame = GEngine->GetAvgFrameDuration(3);
    if (lastFrame>avgFrame*2) lastFrame = avgFrame*2;
    if (lastFrame>maxFrameDuration) lastFrame = maxFrameDuration;
    
    int overlap = ((lastFrame*overlapCoef)>>4) + realOverlap;
    
    // note: maths will not be accurate when playing 3D due to Doppler effect
    // TODO: check sample position

    // estimate when current sample will stop
    DWORD durationMs = toInt((_header._size/_header._sSize)*1000/(_header._frequency*_freqSet));
    DWORD endTime = _startTime + durationMs;
    DWORD currentTime = GlobalTickCount();
    if (currentTime>endTime-overlap)
    {
      if (!_queue->_playing)
      {
        #if 0
        if (_queue->Name().GetLength()>0)
        {
          LogF("%d:  Prestart: %s",GlobalTickCount(),cc_cast(_queue->Name()));
        }
        #endif
        _queue->DoPlay();
      }
    }
    
    
    #if 0 //def _XBOX
      // exact scheduling can help on Xbox
      // once the wave is ready, we can schedule it for playback
      //_queued->DoPlay()
      REFERENCE_TIME timestamp;
      _soundSys->DirectSound()->GetTime(&timestamp);
      
    #endif
  }
}

void WaveGlobalOAL::Play()
{
  if( _queued ) return;

  // check termination
  IsTerminated();

  AdvanceQueue();
  
  base::Play();
}

void WaveGlobalOAL::Skip( float deltaT )
{
  if( _queued ) return;

  // check termination
  IsTerminated();

  AdvanceQueue();

  base::Skip(deltaT);
}

void WaveGlobalOAL::Advance( float deltaT )
{
  // when there is no device, advance always needs to play the sounds, otherwise the sounds are stuck
  if( _playing || _wantPlaying && GetTimePosition()>=0 && _soundSys->OALDevice()) return;
  Skip(deltaT);
}

void WaveGlobalOAL::PreloadQueue()
{
  if (_queue)
  {
    _queue->PreparePlaying();
  }
}

void WaveGlobalOAL::Queue( AbstractWave *wave,int repeat )
{
  WaveGlobalOAL *after=static_cast<WaveGlobalOAL *>(wave);
  //Assert( after->_repeat>0 );
  Assert( repeat>0 );
  while( after->_queue ) after=after->_queue;
  after->_queue=this;
  /*
  LogF
  (
    "Queue %s after %s",
    (const char *)Name(),(const char *)after->Name()
  );
  */
  _queued=true;
  Repeat(repeat);
}

bool WaveGlobalOAL::IsTerminated()
{
#ifndef DUMMY_SOUND_FILE
  if( !base::IsTerminated() ) return false;
  for( WaveGlobalOAL *next = this; next; next=next->_queue )
  {
    if( !next->_terminated ) return false;
  }
  OnTerminateOnce();
#endif
  return true;
}

#define DEFERRED 1

WaveBuffersOAL::WaveBuffersOAL()
:_position(VZero),_velocity(VZero)
{
  _3DEnabled = true;
  _distance2D = 1;
  _isRadio = false;
}

WaveBuffersOAL::~WaveBuffersOAL()
{
}

void WaveBuffersOAL::Reset( SoundSystemOAL *sys )
{
  // TODO: force setting all parameters
  _gainSet = 1;

  _position = VZero;
  _velocity = VZero;

  _volume = 1; // volume used for 3D sound
  _accomodation = 1; // current ear accomodation setting
  _obstructionSet = _occlusionSet = 0;
  _obstruction = _occlusion = 1;
  _volumeAdjust = sys->_volumeAdjustEffect;

  _kind = WaveEffect;
  _3DEnabled = true;
  _isRadio = false;
  _parsTime = 0; // force parameter reset
}

static inline float DistanceRolloff(float minDistance, float distance)
{
  return minDistance/distance;
}

void WaveBuffersOAL::DoSetI3DL2Db(SoundSystemOAL *soundSys, bool immediate)
{
  // we cannot set this if there is no EAX
  Assert(soundSys->_eaxEnabled);

  int occ = _occlusionSet;
  int obs = _obstructionSet;
  int exc = 0;
  int directHF = 0;
  int direct = 0;
  float occRatio = 0.25f;
  // if sounds are accommodated, it means they are real sounds and may be obstructed
  if (_isRadio)
  {
    if (soundSys->_equalizerLoaded)
    {
      static int testOccEq = 0;
      static int testObsEq = 0;
      static int testExcEq = 0;
      static int testDHFEq = 0;
      static int testDEq = -10000;
      // play the sound as low-pass - see also RADIO
      // no direct feed, we are feeding only to the equalizer
      occ += testOccEq;
      obs += testObsEq;
      exc += testExcEq;
      directHF += testDHFEq;
      direct += testDEq;
    }
    else
    {
      static int testOcc = 0;
      static int testObs = 0;
      static int testExc = 0;
      static int testDHF = -3500;
      static int testD = 0;
      // play the sound as low-pass - see also RADIO
      //directHF = -10000;
      // we know slot 0 has 5000 set as HF cutoff for all environments
      occ += testOcc;
      obs += testObs;
      exc += testExc;
      directHF += testDHF;
      direct += testD;
    }
    saturate(occ,EAXSOURCE_MINOCCLUSION,EAXSOURCE_MAXOCCLUSION);
    saturate(obs,EAXSOURCE_MINOBSTRUCTION,EAXSOURCE_MAXOBSTRUCTION);
    saturate(exc,EAXSOURCE_MINEXCLUSION,EAXSOURCE_MAXEXCLUSION);
    saturate(direct,EAXSOURCE_MINDIRECT,EAXSOURCE_MAXDIRECT);
    saturate(directHF,EAXSOURCE_MINDIRECTHF,EAXSOURCE_MAXDIRECTHF);
  }
  if ((_only2DEnabled || !_3DEnabled) && !_accomodation)
  {
    // disable reverb for the 2D sounds, done because of radio - see also RADIO
    exc = -10000;
    // occlusion and obstruction should be both zero
    //Assert(occ==0);
    //Assert(obs==0);
  }
  ScopeLockSection lock(soundSys->GetLock());
  soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_EXCLUSION,_source,&exc,sizeof(exc));
	soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_OCCLUSION,_source,&occ,sizeof(occ));
	soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_OCCLUSIONLFRATIO,_source,&occRatio,sizeof(occRatio));
	soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_OBSTRUCTION,_source,&obs,sizeof(obs));
	soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_DIRECTHF,_source,&directHF,sizeof(directHF));
	soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_DIRECT,_source,&direct,sizeof(direct));
	// note: we might set exclusion as well, esp. for distant sounds
	// we might also set early reflection direction
}

void WaveBuffersOAL::SetI3DL2Db(SoundSystemOAL *soundSys, int obstruction, int occlusion, bool imm)
{
  if (!_playing)
  {
    _obstructionSet = obstruction;
    _occlusionSet = occlusion;
    return;
  }
  if (_sourceOpen)
  {
    _obstructionSet = obstruction;
    _occlusionSet = occlusion;
    if (soundSys->_eaxEnabled)
    {
      DoSetI3DL2Db(soundSys,imm);
    }
  }
}

#if _ENABLE_PERFLOG
  #define WAV_COUNTER(name,value) \
    ADD_COUNTER(name,value); \
    static_cast<SoundSystemOAL *>(GSoundsys)->_##name++;
  #define SND_COUNTER(name,value) \
    ADD_COUNTER(name,value); \
    _##name++;
#else
  #define WAV_COUNTER(name,value)
  #define SND_COUNTER(name,value)
#endif

void WaveBuffersOAL::SetGain(SoundSystemOAL *soundSys, float gain)
{
  if (!_playing || !_sourceOpen)
  {
    #if DIAG_VOL
    //LogF("PresetVolume %s %d",(const char *)Name(),volumeDb);
    LogF("PresetVolume %x %d",this,volumeDb);
    #endif
    _gainSet=gain;
    return;
  }
  // input is multiplicative gain
  static float minRelDiff = 1e-2f;
  if( fabs(gain-_gainSet)>minRelDiff*gain )
  {
    WAV_COUNTER(wavSV,1);
    ScopeLockSection lock(soundSys->GetLock());
    MakeContextCurrent();
    alSourcef(_source,AL_GAIN,gain);
    CHECK_AOL_ERROR("alSourcef-AL_GAIN");

    #if DIAG_VOL
    //LogF("PP: SetVolume %s %d",(const char *)Name(),volumeDb);
    LogF("PP: SetVolume %x %d",this,volumeDb);
    #endif
    _gainSet=gain;
  }
}

void WaveOAL::DoUnload()
{
  if( !_loaded ) return;

  PROFILE_SCOPE_EX(wavUL,sound);
  
  Assert( GSoundsys==_soundSys );
  Assert( _soundSys->_soundReady );

  ScopeLockSection lock(_soundSys->GetLock());
    
  #if 1
    if (_sourceOpen)
    {
      // TODO: we may store also streaming buffer
      // so that they can be reused
      if (!_stream)
      {
        _soundSys->StoreToCache(this);
      }
    }
  #endif

  #if DO_PERF
  if (_sourceOpen)
  {
    ADD_COUNTER(wavUL,1);
  }
  #endif

  // unbind the buffer
  if (_sourceOpen)
  {
    LogAL("OpenAL AL_BUFFER %d:%d - unbind",_source,_buffer.GetDebugInt());
    /*
    if (_streamBuffers.Size()>0)
    {
      LogF("Streamed unbind at %d",_source);
    }
    */
    alSourcei(_source,AL_BUFFER,0);
    CHECK_AOL_ERROR("alSourcei-AL_BUFFER");
    _buffer.Free();
  }

  if (_streamBuffers.Size()>0)
  {
    alDeleteBuffers(_streamBuffers.Size(),_streamBuffers.Data());
    CHECK_AOL_ERROR("alDeleteBuffers");
    _streamBuffers.Resize(0);
  }
  
  // if there are any buffers still queued, we should Unqueue them

  MakeContextCurrent();
  if (_sourceOpen)
  {
    alDeleteSources(1,&_source);
    CHECK_AOL_ERROR("alDeleteSources");
    LogAL("OpenAL alDeleteSources %d",_source);
    _sourceOpen = false;
  }
  _loaded=false;
  _stream.Free();

  _counters[_type]--;
  _type=Free;
  ReportStats();

  //LogF("%s unloaded",(const char *)Name());
}


void WaveOAL::DoDestruct()
{
  PROFILE_SCOPE_EX(wavDs,sound);
  // check if we are still playing
  CheckPlaying();
  // playing buffer should never be terminated, as this can be blocking (Xbox)
  //Assert(!_playing || _terminated);
  //DoStop();
  // unload must destroy the wave
  Unload(true);
  //DoUnload();
  // note: if Free is called immediately after Stop, it may be blocking
}


int WaveOAL::_counters[NTypes];
int WaveOAL::_countPlaying[NTypes];

void WaveOAL::AddStats( const char *name )
{
  if (!_sourceOpen) return;
  _type= _only2DEnabled ? Hw2D : Hw3D;

  _counters[_type]++;
  ReportStats();
}

void WaveOAL::ReportStats()
{
  /*
  char buf[256];
  strcpy(buf,"Snd");
  for( int i=1; i<NTypes; i++ )
  {
    sprintf(buf+strlen(buf)," %d",_counters[i]);
  }
  GEngine->ShowMessage(1000,buf);
  */
}

int WaveOAL::TotalAllocated()
{
  int total=0;
  for( int i=1; i<NTypes; i++ ) total+=_counters[i];
  return total;
}

void WaveOAL::ConvertSkippedToSamples()
{
  float skipSamples = _skipped*_header._frequency;
  if (skipSamples*_header._sSize>_header._size)
  {
    if (_looping)
    {
      int sampleSize = _header._size/_header._sSize;
      float newPos = fastFmod(skipSamples,sampleSize);
      _curPosition += toInt(newPos)*_header._sSize;
    }
    else
    {
      _curPosition = _header._size;
      // if the whole sound is already skipped, we want to terminate the sound
      _terminated=true;
    }
  }
  else
  {
    _curPosition += toInt(_skipped*_header._frequency)*_header._sSize;
  }
  _skipped = 0;
}

/*!
  \patch 5099 Date 12/8/2006 by Ondra
  - Fixed: Dedicated server required OpenAL32.dll
*/
bool WaveOAL::LoadHeader()
{
  if( _loadedHeader || _header._sSize>0 ) return true;
  
  /// empty sound (pause) should never be present here
  Assert(Name().GetLength()>0);

  _soundSys->_waves.RemoveNulls();
    
  // before requesting the file we should check if we are able to get the header from existing sounds
  // try to load properties from some existing sound buffer
  for( int i=0; i<_soundSys->_waves.Size(); i++ )
  {
    WaveOAL *wave=_soundSys->_waves[i];
    if (!wave->_loadedHeader) continue;
    if (wave==this) continue;
    if (!strcmp(wave->Name(),Name()))
    {
      _header = wave->_header;
      _loadedHeader = true;
      ConvertSkippedToSamples();
      return true;
    }
  }
  
  //CHECK_AOL_ERROR("probe");
  
  if (_soundSys->LoadHeaderFromCache(this))
  {
    _loadedHeader = true;
    ConvertSkippedToSamples();
    return true;
  }
  
  #if DIAG_LOAD>=2
    LogF("WaveOAL %s: Load file header",(const char *)Name());
  #endif
  Ref<WaveStream> stream;

  // only header is needed - whole files are cached
  DoAssert(RString::IsLowCase(Name()));
  if (!SoundRequestFile(Name(),stream))
  {
    //Log("Headers not loaded - %s",(const char *)Name());
    return false;
  }
  //Log("Headers loaded - %s",(const char *)Name());

  _loadedHeader = true;
  
  if( !stream )
  {
    RptF("Cannot load sound '%s'", (const char *)Name());
    _loadError = true;
    return true;
  }
  
  WAVEFORMATEX format;
  stream->GetFormat(format);
  _header._frequency = format.nSamplesPerSec;      // sampling rate
  _header._nChannel = format.nChannels;        // number of channels
  _header._sSize = format.nBlockAlign;           // number of bytes per sample
  _header._size = stream->GetUncompressedSize();
  
  ConvertSkippedToSamples();
  
  #if DIAG_STREAM
    if (IsStreaming(stream))
    {
      LogF("%s: Open stream, size %d)",(const char *)Name(),_header._size);
    }
  #endif
  return true;
}

#define DISABLE_HW_ACCEL 0
#define ENABLE_VOICEMAN 1

const int StreamingAtomPart = 16*1024;

int WaveOAL::StreamingBufferSize(const WAVEFORMATEX &format, int totalSize)
{
  int bytesPerSecond = format.nAvgBytesPerSec;
  const int keepUnder = 512*1024/StreamingAtomPart*StreamingAtomPart;
  // never buffer less that 1 second
  if (bytesPerSecond>keepUnder) return (bytesPerSecond+StreamingAtomPart-1)&~(StreamingAtomPart-1);
  // we want the buffer 2 seconds long
  const int seconds = 2;
  int minSize = bytesPerSecond*seconds;
  if (minSize>totalSize) minSize = totalSize;
  if (minSize>keepUnder) return keepUnder;
  int size=StreamingAtomPart*4;
  while (size<minSize)
  {
    size += StreamingAtomPart*4;
  }
  return size;
}
int WaveOAL::StreamingMaxInOneFill(const WAVEFORMATEX &format, int bufferSize)
{
  int bytesPerSecond = format.nAvgBytesPerSec;
  int quarterBuff = bytesPerSecond/4;
  int size = StreamingAtomPart;
  while (size<quarterBuff)
  {
    size *= 2;
  }
  if (size>StreamingAtomPart*4) return StreamingAtomPart*4;
  return size;
}

int WaveOAL::StreamingMaxInInitFill(const WAVEFORMATEX &format, int bufferSize)
{
  int bytesPerSecond = format.nAvgBytesPerSec;
  int quarterBuff = bytesPerSecond/4;
  int size = StreamingAtomPart;
  while (size<quarterBuff)
  {
    size *= 2;
  }
  if (size>StreamingAtomPart*8) return StreamingAtomPart*8;
  return size;
}

int WaveOAL::StreamingPartSize(const WAVEFORMATEX &format)
{
  return StreamingAtomPart;
}

bool WaveOAL::Load()
{
  PROFILE_SCOPE_EX(wavLo,sound);

  if( _loadError ) return true;
  if( _loaded ) return true;
  if (!_loadedHeader)
  {
    if (!LoadHeader())
    {
      return false;
    }
  }
  if (Name().GetLength()<=0)
  {
    _header._frequency = 1000; // time given in ms
  }

  // scan all channels
  // if possible make some channels free (release cache)
  {
    _soundSys->ReserveCache(1-_only2DEnabled,_only2DEnabled);

    // search all playing waves
    _soundSys->_waves.RemoveNulls();
  }
  {
    // first try to reuse any cached data
    if( _soundSys->LoadFromCache(this) )
    {
      #if DIAG_LOAD>=2
      LogF("Cache reused %s",(const char *)Name());
      #endif
      AddStats("Cache");

      _loaded=true;
      _terminated=false;
      _playing=false;
      _wantPlaying=false;
      _streamPos=0;
      _startTime = 0;
      _stopTime = 0xffffffff;
      
      return true;
    }
  }

  // try to duplicate some existing sound buffer
  for( int i=0; i<_soundSys->_waves.Size(); i++ )
  {
    WaveOAL *wave=_soundSys->_waves[i];
    if( !wave->Loaded() ) continue;
    if( wave==this ) continue;
    if( !strcmp(wave->Name(),Name()) )
    {
      if( Duplicate(*wave) )
      {
        return true;
      }
    }
  }

  Ref<WaveStream> stream;
  bool isStreaming = false;
  {
    PROFILE_SCOPE_EX(wavRq,sound);
    DoAssert(RString::IsLowCase(Name()));
    bool done = SoundRequestFile(Name(),stream);
    
    // if request is not satisfied, do not change buffer state
    if (!done)
    {
      //Log("Data not loaded - %s",(const char *)Name());
      return false;
    }
    
    if (stream)
    {
      isStreaming = IsStreaming(stream);
      if (!isStreaming)
      {
        // if not streaming, we need to read the whole file before proceeding
        bool dataReady = stream->RequestData(FileRequestPriority(10),0,stream->GetUncompressedSize());
        if (!dataReady)
        {
          return false;
        }
      }
    }
  }
  //Log("Data loaded - %s",(const char *)Name());
  
  //LogF("WaveOAL %s loaded",(const char *)Name());
  DoDestruct();

  _loadError=true;
  _loaded=true;
  _playing=false;
  _wantPlaying=false;
  _startTime = 0;
  _stopTime = 0xffffffff;
  
  if( !stream )
  {
    RptF("Cannot load sound '%s'", (const char *)Name());
    return true;
  }
  
  // this should be already done?
  WAVEFORMATEX format;
  stream->GetFormat(format);
  _header._frequency = format.nSamplesPerSec;      // sampling rate
  _header._nChannel = format.nChannels;        // number of channels
  _header._sSize = format.nBlockAlign;           // number of bytes per sample
  _header._size = stream->GetUncompressedSize();

  ALCdevice *device = _soundSys->OALDevice();

  ScopeLockSection lock(_soundSys->GetLock());
  
  {
    PROFILE_SCOPE_EX(wavCB,sound);
    if (device)
    {
      #if _ENABLE_REPORT
        if (isStreaming)
        {
//          LogF(
//            "%s: %d buffer for %d, %d BPS",
//            cc_cast(Name()),dsbd.dwBufferBytes,_header._size,_header._frequency*_header._sSize
//          );
        }
      #endif
      // TODO: OAL: simulate and debug error here
      if (!isStreaming)
      {
        ALuint buf;
        alGenBuffers(1,&buf);
        if (alGetError()==AL_NO_ERROR)
        {
          _buffer = new OALBuffer(buf);
        }
        if (_buffer.IsNull())
        {
          return true;
        }
      }
      else
      {
        int bufSize = StreamingPartSize(format);
        int totalSize = StreamingBufferSize(format,_header._size);
        Assert(totalSize%bufSize==0);
        int parts = totalSize/bufSize;
        _streamBuffers.Realloc(parts);
        _streamBuffers.Resize(parts);
        alGenBuffers(_streamBuffers.Size(),_streamBuffers.Data());
        if (alGetError()!=AL_NO_ERROR)
        {
          _streamBuffers.Resize(0);
          return true;
        }
      }
    }
  }
  #if DIAG_LOAD
    LogF("WaveOAL %s: CreateSoundBuffer",(const char *)Name());
  #endif

  {
    
    // note: at this point streaming / static implementation
    // is different
    // static decompresses whole buffer once, while streaming keeps
    // memory mapped file buffer and reads from it sequentially
    if (isStreaming)
    {
      // fill buffer with start of streaming data
      _stream = stream;
      _streamPos = 0;
      _writeBuf = 0;
      _freeBufN = 0;
      //LogF("Reset stream %s",(const char *)Name());
    }
    else
    {
      if (device)
      {
        PROFILE_SCOPE_EX(wavGC,sound);
        // temporary data copy needed
        AutoArray<char> data;
        data.Realloc(_header._size);
        data.Resize(_header._size);
        int read = stream->GetDataClipped(data.Data(),0,data.Size());
        if (read<data.Size())
        {
          LogF("GetData returned %d instead of %d",read,data.Size()-read);
          memset(data.Data()+read,0,data.Size()-read);
        }
        ALenum format = _header.GetALFormat();
        for(;;)
        {
          alBufferData(_buffer,format,data.Data(),data.Size(),_header._frequency);
          ALenum err = alGetError();
          if (err==AL_NO_ERROR) break;
          else if (err==AL_OUT_OF_MEMORY)
          {
            FreeOnDemandSystemMemoryLowLevel(data.Size());
          }
          else
          {
            alDeleteBuffers(_streamBuffers.Size(),_streamBuffers.Data());
            CHECK_AOL_ERROR("alDeleteBuffers");
            _streamBuffers.Resize(0);
            return false;
          }
        }
      }
    }
  }
  //_soundSys->LogChannels("Load");
  AddStats("Load");

  if (_soundSys->_device)
  {
    #if SEPARATE_2D_CONTEXT
    _context = DesiredContext();
    MakeContextCurrent();
    #endif

    DoAssert(!_sourceOpen);
    // we gave the buffer - now create the source
    alGenSources(1,&_source);
    if (alGetError()!=AL_NO_ERROR)
    {
      _buffer.Free();
      return true;
    }

    _sourceOpen = true;

    InitEAX();

    if (!isStreaming)
    {
      // if the sound is non-looping and pre-pause is wanted, we need to use alSourceQueueBuffers
      LogAL("OpenAL AL_BUFFER %d:%d (Load)",_source,_buffer.GetDebugInt());
      alSourcei(_source,AL_BUFFER,_buffer);
      
      CHECK_AOL_ERROR("alSourcei-AL_BUFFER");
    }
  }

  // loaded successfully - flag no error
  _loadError=false;
  return true;
}

bool WaveOAL::IsStreaming(WaveStream *stream)
{
  #if 0
  return false;
  #else
  return stream->StreamingWanted(_looping);
  #endif
}

ALenum WaveOAL::Header::GetALFormat() const
{
  Assert(_sSize==_nChannel || _sSize==_nChannel*2);
  Assert(_nChannel==1 || _nChannel==2);
  if (_nChannel==1)
  {
    return _sSize==1 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
  }
  else
  {
    return _sSize==2 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
  }
}

/*!
\patch 1.43 Date 1/31/2002 by Ondra
- Fixed: Support for looping streaming sounds.
\patch 1.85 Date 9/12/2002 by Ondra
- Fixed: Streaming sounds might loop infinitely on some hardware.
*/

void WaveOAL::UnqueueStream(int processed)
{
  if (processed<=0) return;
  // which one are those processed?
  Assert(processed<=_streamBuffers.Size()-_freeBufN);
  int firstBusy = _writeBuf+_freeBufN;
  if (firstBusy>=_streamBuffers.Size()) firstBusy -= _streamBuffers.Size();
  
  AutoArray<ALuint,MemAllocLocal<ALuint,128> > bufs;
  bufs.Resize(processed);
  for (int i=0; i<processed; i++)
  {
    bufs[i] = _streamBuffers[firstBusy];
    //LogF("Unqueing %d at %d",bufs[i],_source);
    firstBusy++;
    if (firstBusy>=_streamBuffers.Size()) firstBusy = 0;
  }
  
  ScopeLockSection lock(_soundSys->GetLock());
  // reset error
  CHECK_AOL_ERROR("Before alSourceUnqueueBuffers");
  alSourceUnqueueBuffers(_source,processed,bufs.Data());
  // check for error
#if _ENABLE_REPORT
  {
      ALenum err = alGetError();
      if (err!=AL_NO_ERROR)
      {
        LogF("alSourceUnqueueBuffers: OpenAL error %x",err);
        if (err==AL_INVALID_OPERATION)
        {
          DoAssert(alcGetCurrentContext()!=NULL);
        }
      }
    }
#endif
  _freeBufN += processed;
}

void WaveOAL::FillStream()
{
  if (_streamBuffers.Size()==0) return;
  // check how many buffers can we un-queue
  ALint processed = 0;
  {
    ScopeLockSection lock(_soundSys->GetLock());
    MakeContextCurrent();
    alGetSourcei(_source,AL_BUFFERS_PROCESSED,&processed);
    //alGetSourcei(_source,AL_BUFFERS_QUEUED,&queued);
    if (alGetError()!=AL_NO_ERROR) return;
  }
  // no special termination handling required - the source will stop itself once done
  UnqueueStream(processed);

  if (_freeBufN<=0) return;
  
#if DIAG_STREAM_FILL>40
  LogF(
    "%s: %d, free %d, pos %d",
    (const char *)Name(),GlobalTickCount(),_freeBufN,_writeBuf
  );
#endif // 0
  // time per sample in ms 1000/_header._frequency

  //write = play;

  // we may write anywhere between write and play, not between play and write

  // check what has been actually played

  WAVEFORMATEX format;
  _stream->GetFormat(format);
  int partSize = StreamingPartSize(format);
  int bufferSize = StreamingBufferSize(format,_header._size);
  int maxOneFill = StreamingMaxInOneFill(format,bufferSize)/partSize;
  
  int toWriteBufs = intMin(_freeBufN,maxOneFill);
  // TODO: OAL: log starving

  // check if data are ready
  // when continuing stream, use highest priority, as any drop will be audible
  // TODO: prioritize by the time left
  bool ready = false;
  if (_looping)
  {
    ready = _stream->RequestDataLooped(FileRequestPriority(1),_streamPos,toWriteBufs*partSize);
  }
  else
  {
    ready = _stream->RequestDataClipped(FileRequestPriority(1),_streamPos,toWriteBufs*partSize);
  }
  
  if (!ready)
  {
#if DIAG_STREAM_FILL>10
    LogF("%s: FillStream - No data, free: %d of %d",cc_cast(Name()),_freeBufN,_streamBuffers.Size());
#endif // 0
    return;
  }

  ALenum alFormat = _header.GetALFormat();
  
  AutoArray<char, MemAllocDataStack<char,StreamingAtomPart> > temp;
  temp.Resize(partSize);
  for (int i=0; i<toWriteBufs; i++)
  {
    //LogF("Streaming data %d:%d to %d",_streamPos,size,_writePos);

    int written = 0;
    #if DIAG_STREAM
    LogF("%s: Load stream %d:%d (of %d)",(const char *)Name(),_streamPos,toWriteBufs,_streamBuffers.Size());
    #endif
    if (_looping)
    {
      if (_streamPos>=(int)_header._size) _streamPos -= _header._size;
      written = _stream->GetDataLooped(temp.Data(),_streamPos,partSize);
    }
    else
    {
      // if there is nothing more to write, do not write
      if (_streamPos>=(int)_header._size) break;
      written = _stream->GetDataClipped(temp.Data(),_streamPos,partSize);
    }
    _streamPos += written;

    ALuint buf = _streamBuffers[_writeBuf];

    for(;;)
    {
      ScopeLockSection lock(_soundSys->GetLock());
      alBufferData(buf,alFormat,temp.Data(),temp.Size(),_header._frequency);
      ALenum err = alGetError();
      if (err==AL_NO_ERROR)
      {
        alSourceQueueBuffers(_source,1,&buf);
        //LogF("Queing %d at %d",buf,_source);
        _writeBuf++;
        if (_writeBuf>=_streamBuffers.Size()) _writeBuf = 0;
        _freeBufN--;
        break;
      }
      else if (err==AL_OUT_OF_MEMORY)
      {
        FreeOnDemandSystemMemoryLowLevel(temp.Size());
      }
      else
      {
        ALint type;
        alGetSourcei(_source,AL_SOURCE_TYPE,&type);
        break;
      }
    }


#if DIAG_STREAM_FILL>0
    LogF("%s: FillStream %d (%d of %d)",cc_cast(Name()),_writeBuf,i,toWriteBufs);
#endif // 0

  }

  ScopeLockSection lock(_soundSys->GetLock());
  // if there was an under-run, the source has stopped meanwhile
  // we need to resume it
  ALint state = 0;
  alGetSourcei(_source,AL_SOURCE_STATE,&state);
  if(alGetError()==AL_NO_ERROR && state!=AL_PLAYING)
  {
    LogF("Streamed sound %s starved - resuming",cc_cast(Name()));
    Assert(state==AL_STOPPED || state==AL_INITIAL);
    alSourcePlay(_source);
    CHECK_AOL_ERROR("alSourcePlay");
  }
}

#if 1

#define STAT_PLAY(x)
#define STAT_TERM(x)
#define STAT_STOP(x)
#define STAT_GETCHANNELS() 0

#else

static int NPlaying = 0;

#define STAT_PLAY(x) \
  NPlaying++; \
  GlobalShowMessage(100,"Playing %d", NPlaying); \
  LogF("%2d: Play %x:%s",NPlaying,x,(const char *)(x)->Name())

#define STAT_TERM(x) \
  LogF("   Term %x:%s",x,(const char *)(x)->Name())

#define STAT_STOP(x) \
  NPlaying--; \
  GlobalShowMessage(100,"Playing %d", NPlaying); \
  LogF("%2d: Stop %x:%s",NPlaying,x,(const char *)(x)->Name())

#define STAT_GETCHANNELS() NPlaying

#endif

bool WaveOAL::PreparePlaying()
{
  if (_playing || _terminated) return true;
  // check files needed
  bool loaded = Load();
  if (!loaded) return false;

  if (_stream)
  {
    int dummy;
    int preloadSize = InitPreloadStreamSize(dummy);
    return _stream->RequestData(FileRequestPriority(10),0,preloadSize);
  }
  
  return true;
}

void WaveOAL::Play()
{
  // set flag we want to play
  // commit will play it after listener position is set
  if( _terminated ) return;

  _wantPlaying = true;
  // avoid stop unloading
  _stopTime = 0xffffffff;
}

#if 0 //_ENABLE_REPORT
/// targeted logging
static bool InterestedIn(const char *name)
{
  return strstr(name,"ah1cobra")!=NULL;
}
#else
static inline bool InterestedIn(const char *name) {return false;}
#endif

/**
@param delay delay realized by queuing a silence buffer before the one we want to play.
*/

bool WaveOAL::DoPlay(int delayMs)
{
  if( _terminated )
  {
    return true;
  }

  if (!_soundSys->OALDevice())
  {
    // we want to load the header (background loading is enough)
    // otherwise we do not know when the sound has terminated
    bool loaded = _loadedHeader;
    if (!loaded)
    {
      loaded = LoadHeader();
    }
    // we need to report the sound as "playing", to prevent handlers stuck
    if (loaded) OnPlayOnce();
    return loaded;
  }

  if( _playing )
  {
    // check if we are playing in correct context
    #if SEPARATE_2D_CONTEXT
    if (DesiredContext()!=_context)
    {
      // if not, we need to stop and start playing again
      Stop();
    }
    else
    #endif
    {
      // if it is streaming, fill with data as necessary
      if (_stream) FillStream();
      return true;
    }
  }

  if (!_posValid && !_only2DEnabled)
  {
    // position not set yet - cannot start playing
    return false;
  }
  if( GetTimePosition()>=0 )
  {
    //LogF("Wave %s, pos %d",(const char *)Name(),_curPosition);
    bool loaded = Load();

    MakeContextCurrent();

    //PROFILE_SCOPE_EX(wavPl,sound);
    if (!loaded) return false;

    if (_applyOffset) 
    {
      int offset = (int)(_header._size * _offset);
      int tmp = offset % _header._sSize;
      if (tmp != 0) offset += tmp;
      _curPosition += offset;
      _applyOffset = false;
    }

    if( _curPosition>=(int)_header._size )
    {
      if( _looping && _header._size>0 )
      { 
        while( _curPosition>=(int)_header._size ) _curPosition-=_header._size;
      }
      else
      {
        _terminated=true;
        _curPosition=_header._size;
      }
    }
    else
    {
      {
        ScopeLockSection lock(_soundSys->GetLock());
        //PROFILE_SCOPE_EX(wavSt,sound);
        #if DIAG_VOL
        LogF("SP: SetVolume %s %g",(const char *)Name(),_gainSet);
        #endif
        //LogF("Start play %s, vol %g",(const char *)Name(),_gainSet);
        
        
        alSourcef(_source,AL_GAIN,_gainSet);
        CHECK_AOL_ERROR("alSourcei-AL_GAIN");
        
        if (InterestedIn(Name()))
        {
          LogF("%s: OO %d,%d",cc_cast(Name()),_obstructionSet,_occlusionSet);
        }
        if (_soundSys->_eaxEnabled)
        {
          DoSetI3DL2Db(_soundSys,true);
        }
        alSourcef(_source,AL_PITCH,_freqSet);
        CHECK_AOL_ERROR("alSourcef-AL_PITCH");

        
        
        //LogF("SetPos %s",(const char *)Name());
        DoSetPosition(_soundSys,true);
      }

      if (!_stream)
      {
        if (_sourceOpen)
        {
          _startTime = GlobalTickCount();
          _stopTime = 0xffffffff;
          // consider how much of the buffer has already been played back

          if (_curPosition>0)
          {
            DWORD alreadyPlayed = toInt((_curPosition/_header._sSize)*1000/(_header._frequency*_freqSet));
            _startTime -= alreadyPlayed;
          }

    
          ScopeLockSection lock(_soundSys->GetLock());
        
          MakeContextCurrent();
          //PROFILE_SCOPE_EX(wavPP,sound);
          #if 0
          if (_curPosition>0)
          {
            LogF("%s: set curpos to %d",(const char *)Name(),_curPosition);
          }
          #endif
          WAV_COUNTER(wavPl,1);
          alSourcei(_source,AL_BYTE_OFFSET,_curPosition);
          CHECK_AOL_ERROR("alSourcei-AL_BYTE_OFFSET");
          alSourcei(_source,AL_LOOPING,_looping);
          CHECK_AOL_ERROR("alSourcei-AL_LOOPING");
          alSourcePlay(_source);
          CHECK_AOL_ERROR("alSourcePlay");
        }
      }
      else
      {
        if (_streamBuffers.Size()>0)
        {
          //PROFILE_SCOPE_EX(wavPS,sound);
          // start play-back
          // perform initial fill on the stream
          Assert(_streamPos==0);
#if DIAG_STREAM_FILL>0
          LogF("%s: Set stream start to %d",cc_cast(Name()),_curPosition);
#endif

          _streamPos = _curPosition;
          _writeBuf = 0;
          _freeBufN = _streamBuffers.Size();
          if (!FillStreamInit())
          {
            _wantPlaying = true;

            return false;
          }

          ScopeLockSection lock(_soundSys->GetLock());
          
          WAV_COUNTER(wavPl,1);
          alSourcePlay(_source);
          CHECK_AOL_ERROR("alSourcePlay");
        }
        _countPlaying[_type]++;
      }
      if (_only2DEnabled)
      {
//        LogF(
//          "%d: Wave started %s, buffer %p, freq %.2f, start %d, end %d",
//          GlobalTickCount(),(const char *)Name(),_buffer.GetRef(),_freqSet,
//          _startTime,_startTime+toInt((_header._size/_header._sSize)*1000/(_header._frequency*_freqSet))
//        );
      }
    }
    _playing=true;
    STAT_PLAY(this);
    OnPlayOnce();
  }
  return true;
}

void WaveOAL::Repeat(int repeat)
{
  if (repeat<2)
  {
    _looping=false;
  }
  else
  {
    _looping = true;
    if( _playing )
    {
      if (_sourceOpen)
      {
        ScopeLockSection lock(_soundSys->GetLock());
        WAV_COUNTER(wavPl,1);
        MakeContextCurrent();
        alSourcei(_source,AL_LOOPING,true);
        CHECK_AOL_ERROR("alSourcei-AL_LOOPING");
      }
    }
  }
}
void WaveOAL::Restart()
{
  if (_playing && !_stream)
  {
    // we need to restart the sound only
    // no need to unload/load ...
    _terminated=false;
    _curPosition=0;
    _skipped = 0;
    ScopeLockSection lock(_soundSys->GetLock());
    alSourcePlay(_source);
  }
  else
  {
    bool wasPlaying = _playing;
    Stop();
    _terminated=false;
    _curPosition=0;
    _skipped = 0;
    if (_stream)
    {
      _streamPos = 0;
    }
    Play();
    if (wasPlaying)
    {
      if (DoPlay()) _wantPlaying=false;
    }
  }
}

/*!
\patch 1.01 Date 15/6/2001 by Ondra.
 - Fixed: playMusic [name,t] with t>96
*/

void WaveOAL::Skip( float deltaT )
{
  // note: pre-start delay is used to implement speed of sound
  // this gives bad results if listener speed is high
  if (!_loadedHeader)
  {
    _skipped += deltaT;
    return;
  }

  if (_playing)
  {
    StoreCurrentPosition();
  }

  // process what should be skipped
  deltaT += _skipped;
  _skipped = 0;
  Assert(_loadedHeader);
  // FIX
  int skip=toLargeInt(deltaT*_header._frequency)*_header._sSize;
  AdvanceCurrentPosition(skip);
  while( _curPosition>=(int)_header._size )
  {
    if (!_looping || _header._size<=0)
    {
      Stop();
      _terminated=true;
      _curPosition=_header._size;
      break;
    }
    _curPosition-=_header._size;
  }
  if( _playing && !_terminated )
  {
    ScopeLockSection lock(_soundSys->GetLock());
    WAV_COUNTER(wavSV,1);
    // TODO: why do we want to change offset if playing?
    MakeContextCurrent();
    alSourcei(_source,AL_BYTE_OFFSET,_curPosition);
    CHECK_AOL_ERROR("alSourcei-AL_BYTE_OFFSET");
    if (_stream)
    {
      LogF("Cannot advance playing stream %s",(const char *)Name());
    }
  }
}

void WaveOAL::Advance( float deltaT )
{
  if( !_playing ) Skip(deltaT);
}

float WaveOAL::GetTimePosition() const
{
  if (!_loadedHeader) return _skipped;
  float denom = _header._sSize*_header._frequency;
  if (denom<=0) return _skipped;
  return _skipped + float(_curPosition)/(_header._sSize*_header._frequency);
}


bool WaveOAL::IsStopped() {return !_playing && !_wantPlaying;}
bool WaveOAL::IsWaiting() {return GetTimePosition()<0;}

RString WaveOAL::GetDiagText()
{
  RString ret;
  if (GetTimePosition()<0)
  {
    float delay = -GetTimePosition();
    char buf[256];
    sprintf(buf,"Delay %.3f ",delay);
    ret = buf;
  }
  if( IsStopped() ) return ret+RString("Stopped");
  else if( IsTerminated() ) return ret+RString("Terminated");
  else if( IsWaiting() ) return ret+RString("Waiting");
  else 
  {
    if( !_loaded ) return ret+RString("Not loaded");
    if( !_sourceOpen ) return ret+RString("No buffer");
    // TODO: more diags
    if( _only2DEnabled )
    {
      char buf[512];
      sprintf
      (
        buf,
        "Playing 2D\n"
        "    vol (%.2f, adj %.2f) kind %d",
        _volume,_volumeAdjust,_kind
      );
      return ret+RString(buf);
    }
    else
    {

      char buf[512];
      sprintf
      (
        buf,
        "Playing\n"
        "    pos (%.1f,%.1f,%.1f)\n"
        "    vel (%.1f,%.1f,%.1f)\n"
        "    vol (%.3f)",
        _position[0],_position[1],_position[2],
        _velocity[0],_velocity[1],_velocity[2],
        _volume
      );
      return ret+RString(buf);
    }
  }
}

float WaveOAL::GetLength() const
{
  // get wave length in sec
  Assert(_loadedHeader);
  if (!_loadedHeader)
  {
    if (!unconst_cast(this)->LoadHeader())
    {
      LogF("Waiting for header of %s",(const char *)Name());
      do {} while(!unconst_cast(this)->LoadHeader());
    }
  }
  
  if (_header._sSize<=0)
  {
    RptF("%s: sSize %d",(const char *)Name(),_header._sSize);
    return 1;
  }
  if (_header._frequency<=0)
  {
    RptF("%s: frequency %d",(const char *)Name(),_header._frequency);
    return 1;
  }
  return float(_header._size/_header._sSize)/_header._frequency;
}

void WaveOAL::SetStartOffset(float percentage)
{ 
  _offset = percentage; 
  _offset = fastFmod(_offset, 1.0f);
  _applyOffset = true; 
}

float WaveOAL::GetCurrPosition() const
{
  if (!_loadedHeader) return 0.0f;

  if (_wantPlaying) return 0.0;

  if (_playing && !_wantPlaying)
  {
    if (_sourceOpen && _source)
    {
      int cPos = 0;
      alSourcei(_source, AL_BYTE_OFFSET, cPos);
      CHECK_AOL_ERROR("alSourcei-AL_BYTE_OFFSET");

      UINT bytesPlayed = (_offset * _header._size) + (UINT)(cPos);

      if (!_looping)
      {
        if (bytesPlayed >= _header._size) return 1.0;
      }

      return (bytesPlayed % _header._size) / ((float)_header._size); 
    }
  }

  return 1.0;
}

bool WaveOAL::IsTerminated()
{
#ifndef DUMMY_SOUND_FILE
  if (!_terminated)
  {
    if (_playing)
    {
      Assert (_sourceOpen);
      if (_sourceOpen)
      {
        if (!_buffer.IsNull())
        {
          ScopeLockSection lock(_soundSys->GetLock());
          ALint status;
          alGetSourcei(_source,AL_SOURCE_STATE,&status);
          CHECK_AOL_ERROR("alGetSourcei-AL_SOURCE_STATE");
          if( status==AL_PLAYING ) return false;
          // the buffer has terminated, we should be at the end?
          _curPosition=_header._size;
          Assert(_skipped==0);
        }
        else if (_streamBuffers.Size()>0)
        {
          // if streaming sound is looped, it will never terminate
          if (_looping) return false;
          // if it was not played whole, it did not terminate yet
          if (_streamPos<(int)_header._size) return false;
          // check status
          ScopeLockSection lock(_soundSys->GetLock());
          ALint status;
          alGetSourcei(_source,AL_SOURCE_STATE,&status);
          CHECK_AOL_ERROR("alGetSourcei-AL_SOURCE_STATE");
          if( status==AL_PLAYING ) return false;
          _curPosition=_header._size;
          Assert(_skipped==0);
          // 
        }
        if(!_looping)
        {
          STAT_TERM(this);
          _terminated=true;
          // looping wave can never be terminated
          _stopTime = GlobalTickCount();
        }
      }
    }
    else if (!_loadedHeader && _skipped>5.0f)
    {
      // if sound is skipped for quite some time, there is a good chance it has already terminated
      // we cannot be sure unless we load its header, though, because we do not know its length
      LoadHeader();
      // LoadHeader did set _terminate if appropriate
    }
    else if (_loadError)
    {
      // sound that cannot be loaded is considered terminated
      _terminated = true;
    }
  }
  return _terminated;
#else
  return true;
#endif
}

bool WaveOAL::CanBeDeleted()
{
  if (!IsTerminated())
  {
    return false;
  }
  Unload(false);
  return _buffer.IsNull();
}

int WaveOAL::InitPreloadStreamSize(int &initSize) const
{
  WAVEFORMATEX format;
  _stream->GetFormat(format);
  int bufferSize = StreamingBufferSize(format,_header._size);
  int partSize = StreamingPartSize(format);
  int maxOneFill = StreamingMaxInInitFill(format,bufferSize);

  int totalSize = _stream->GetUncompressedSize();
  initSize = bufferSize-2*partSize;
  if (initSize>maxOneFill)
  {
    initSize = maxOneFill;
  }
  
  int preloadSize = initSize*4;
  if (preloadSize>totalSize) preloadSize = totalSize;
  if (initSize>totalSize) initSize = totalSize;
  return preloadSize;
}

bool WaveOAL::FillStreamInit()
{
  MakeContextCurrent();

  int initSize;
  WAVEFORMATEX format;
  _stream->GetFormat(format);
  //int bufferSize = StreamingBufferSize(format,_header._size);
  int preloadSize = InitPreloadStreamSize(initSize);

  
  // on first frame request more data than necessary
  // to make sure next frames will be able to fill, to prevent starving
  if (!_stream->RequestData(FileRequestPriority(10),_streamPos,preloadSize))
  {
#if DIAG_STREAM_FILL>10
    LogF("%s: FillStreamInit - No data, free %d",cc_cast(Name()),_streamBuffers.Size());
#endif // 0
    return false;
  }
  #if DIAG_STREAM_FILL>20
    LogF("%s: done %d B,%d B",cc_cast(Name()),_streamPos,preloadSize);
    
  #endif

  ALenum alFormat = _header.GetALFormat();
  
  int partSize = StreamingPartSize(format);
  int initBuffers = initSize/partSize;
  AutoArray<char, MemAllocDataStack<char,StreamingAtomPart> > temp;
  temp.Resize(partSize);
  for (int i=0; i<initBuffers; i++)
  {
    #if DIAG_STREAM
    LogF
    (
      "%s: Init stream %d B:%d of %d",
      (const char *)Name(),_streamPos,i,initBuffers
    );
    #endif

#if DIAG_STREAM_FILL>0
    LogF("%s: FillStreamInit %d B (%d B)",cc_cast(Name()),_streamPos, _header._size);
#endif // 0

    if (_looping)
    {
      _streamPos += _stream->GetDataLooped(temp.Data(),_streamPos,partSize);
    }
    else
    {
      _streamPos += _stream->GetDataClipped(temp.Data(),_streamPos,partSize);
    }
    ALuint buf = _streamBuffers[_writeBuf];

    for(;;)
    {
      ScopeLockSection lock(_soundSys->GetLock());
      alBufferData(buf,alFormat,temp.Data(),temp.Size(),_header._frequency);
      ALenum err = alGetError();
      if (err==AL_NO_ERROR)
      {
        alSourceQueueBuffers(_source,1,&buf);
        CHECK_AOL_ERROR("alSourceQueueBuffers");
        break;
      }
      else if (err==AL_OUT_OF_MEMORY)
      {
        FreeOnDemandSystemMemoryLowLevel(temp.Size());
      }
      else break;
    }

    _writeBuf++;
    if (_writeBuf>=_streamBuffers.Size()) _writeBuf = 0;
    
  }
  _freeBufN = _streamBuffers.Size()-initBuffers;
  return true;
}

void WaveOAL::StoreCurrentPosition()
{
  // store current position so that is is available when playing the buffer
  if (!_sourceOpen) return;

  ALint offset = 0;
  {
    ScopeLockSection lock(_soundSys->GetLock());
    alGetSourcei(_source,AL_BYTE_OFFSET,&offset);
    CHECK_AOL_ERROR("alGetSourcei-AL_BYTE_OFFSET");
  }
  _curPosition=offset;
  Assert(_skipped==0);
  // for streaming audio store all necessary information
  if (_stream)
  {
    WAVEFORMATEX format;
    _stream->GetFormat(format);
    // check how many buffers are still waiting to be played
    int bufSize = StreamingPartSize(format);
    ALint queued = 0;
    alGetSourcei(_source,AL_BUFFERS_QUEUED,&queued);
    // offset is relative to all buffers which are already queued
    int leftInBuffer = queued*bufSize-offset;
    _curPosition = _streamPos-leftInBuffer;
    _streamPos = 0;
#if DIAG_STREAM_FILL>0
    LogF("%s: Stored stream position %d of %d (%d ahead)",cc_cast(Name()),_curPosition,_header._size,leftInBuffer);
#endif // 0
  }
}

/*!
\patch 1.27 Date 10/11/2001 by Ondra
- Fixed: speed of sound delay caused some sounds to be decayed (since 1.26).
*/
void WaveOAL::AdvanceCurrentPosition(int skip)
{
  Assert(_skipped==0);
  if (!_stream)
  {
    int oldPos = _curPosition;
    _curPosition += skip;
    if( _curPosition<0 )
    {
      Stop();
    }
    else
    {
      
      if (oldPos<0) _curPosition = 0;
    }
  }
  else
  {
    //LogF("Advance stream %s",(const char *)Name());
    if (_playing)
    {
      LogF("Cannot advance in playing stream %s",(const char *)Name());
    }
    // note: only one of _streamPos and _curPosition should be non-zero
    Assert (_streamPos==0 || _curPosition==0);
    int newPos = _streamPos + _curPosition + skip;
    if (newPos<0)
    {
      Stop();
      //LogF("Hard rewind %s to %d",(const char *)Name(),newPos);
    }
    else
    {
      //LogF("Soft rewind %s to %d",(const char *)Name(),newPos);
    }
    _curPosition = newPos;
    _streamPos = 0;
  }
}


bool WaveOAL::DoStop()
{
  _wantPlaying = false;
  if( !_loaded )
  {
    if (_playing && !_loadError)
    {
      LogF("%x,%s: Playing, not loaded",this,(const char *)Name());
    }
    return true;
  }
  if( !_playing )
  {
    if (_sourceOpen)
    {
      DWORD time = GlobalTickCount();
      if (_stopTime<time)
      {
        DWORD stopped = time-_stopTime;
        const DWORD unloadAfterStop = 2000; // in ms
        if (stopped>unloadAfterStop)
        {
          UnloadStopped(false);
        }
      }
    }
    return false;
  }
  _playing=false;
  if (_sourceOpen)
  {
    if (!_buffer.IsNull())
    {
      ScopeLockSection lock(_soundSys->GetLock());
      MakeContextCurrent();
      WAV_COUNTER(wavSt,1);
      _countPlaying[_type]--;
      StoreCurrentPosition(); // 
      alSourceStop(_source);
      CHECK_AOL_ERROR("alSourceStop");
      _writeBuf = 0;
      _stopTime = GlobalTickCount();
    }
    else if (_streamBuffers.Size()>=0)
    {
      ScopeLockSection lock(_soundSys->GetLock());
      MakeContextCurrent();
      WAV_COUNTER(wavSt,1);
      _countPlaying[_type]--;
      StoreCurrentPosition(); // 
      //LogF("Streamed stop %d",_source);
      alSourceStop(_source);
      CHECK_AOL_ERROR("alSourceStop");
      UnqueueStream(_streamBuffers.Size()-_freeBufN);
      _writeBuf = 0;
      _stopTime = GlobalTickCount();
      // all should be processed now, unqueue
    }
  }
  // if sound is stopped for a long time, we can unload it
  STAT_STOP(this);
  return true;
}

void WaveOAL::CheckPlaying()
{
  // check if wave has already terminated
  IsTerminated();
}

void WaveOAL::Unload()
{
  return Unload(false);
}

void WaveOAL::UnloadStopped(bool sync)
{
  if (_sourceOpen)
  {
    // OpenAL can always unload
    DoUnload();
  }
}

/**
\param sync when true, wave must be really stopped before function returns
*/
void WaveOAL::Unload(bool sync)
{
  DoStop();
  UnloadStopped(sync);
}

void WaveOAL::Stop()
{
  DoStop();
}
void WaveOAL::LastLoop()
{
  if( !_loaded ) return;
  if( !_looping ) return;
  if( _playing )
  {
    if (!_stream)
    {
      if (_sourceOpen)
      {
        ScopeLockSection lock(_soundSys->GetLock());
        MakeContextCurrent();
        alSourcei(_source,AL_LOOPING,false);
        CHECK_AOL_ERROR("alSourcei-AL_LOOPING");
      }
    }
  }
  _looping=false;
}

void WaveOAL::PlayUntilStopValue( float time )
{
  // play until some time
  _stopTreshold = time;
  if( _looping ) return;
  _looping = true;
  if( !_loaded ) return;
  if( _playing )
  {
    if (_sourceOpen)
    {
      ScopeLockSection lock(_soundSys->GetLock());
      WAV_COUNTER(wavPl,1);
      // if it is already playing, do not start again
      // we want to set looping only
      MakeContextCurrent();
      ALint state;
      alGetSourcei(_source,AL_SOURCE_STATE,&state);
      CHECK_AOL_ERROR("alGetSourcei-AL_SOURCE_STATE");
      Assert(state==AL_PLAYING);
      alSourcei(_source,AL_LOOPING,true);
      CHECK_AOL_ERROR("alSourcei-AL_LOOPING");
    }
  }
}
void WaveOAL::SetStopValue( float time )
{
  //LogF("%s: SetStopValue %.2f, %.2f",(const char *)Name(),time,_stopTreshold);
  if (time>_stopTreshold)
  {
    _terminated = true;
    Stop();
  }
}

float WaveBuffersOAL::CalcMinDistance() const
{
  return floatMax(sqrt(_volume)*_accomodation*30,1e-6);
}

static bool SoundParsNeedUpdate(int timeMs)
{
  // never update sound more than 20 fps
  const int minTimeUpdate = 50;
  if (timeMs<minTimeUpdate)
  {
    return false;
  }
  // TODO: OAL: cache values
  return true;
}
/*
\patch 1.47 Date 3/11/2002 by Ondra
- Workaround: Distance falloff limits do not work properly on SB Live and Audigy cards
due to driver or hardware bug. As a result, footsteps and other quiet sounds were
heard from large distances. Alternate method of distance limits handling is used
on HW accelerated cards.
*/

void WaveBuffersOAL::DoSetPosition(SoundSystemOAL *soundSys, bool immediate)
{
  if( _only2DEnabled && !immediate) return;  

  DWORD time = GlobalTickCount();
  if (immediate || SoundParsNeedUpdate(time-_parsTime))
  {
    MakeContextCurrent();
    if (_3DEnabled && !_only2DEnabled)
    {
      // TODO: consider caching
      Vector3Val pos=_position;
      Vector3Val vel=_velocity;
      float minDistance= CalcMinDistance();
      ScopeLockSection lock(soundSys->GetLock());
      // note: we invert x to convert between left handed and right handed system
      alSource3f(_source,AL_POSITION,-pos.X(),pos.Y(),pos.Z());
      CHECK_AOL_ERROR("alSourcefv-AL_POSITION");
      alSource3f(_source,AL_VELOCITY,-vel.X(),vel.Y(),vel.Z());
      CHECK_AOL_ERROR("alSourcefv-AL_VELOCITY");
      alSourcei(_source,AL_SOURCE_RELATIVE,false);
      CHECK_AOL_ERROR("alSourcefv-AL_SOURCE_RELATIVE");
      alSourcef(_source,AL_REFERENCE_DISTANCE,minDistance);
      CHECK_AOL_ERROR("alSourcefv-AL_REFERENCE_DISTANCE");
    }
    else
    {
      Vector3Val pos=VZero;
      Vector3Val vel=VZero;
      // min. distance should not matter much here
      float minDistance= CalcMinDistance();
      ScopeLockSection lock(soundSys->GetLock());
      // note: we invert x to convert between left handed and right handed system
      alSource3f(_source,AL_POSITION,-pos.X(),pos.Y(),pos.Z());
      CHECK_AOL_ERROR("alSourcefv-AL_POSITION");
      alSource3f(_source,AL_VELOCITY,-vel.X(),vel.Y(),vel.Z());
      CHECK_AOL_ERROR("alSourcefv-AL_VELOCITY");
      alSourcei(_source,AL_SOURCE_RELATIVE,true);
      CHECK_AOL_ERROR("alSourcefv-AL_SOURCE_RELATIVE");
      alSourcef(_source,AL_REFERENCE_DISTANCE,minDistance);
      CHECK_AOL_ERROR("alSourcefv-AL_REFERENCE_DISTANCE");
    }
    _parsTime = time;
  }
}

float WaveOAL::GetMinDistance() const
{
  if (_only2DEnabled)
  {
    return _distance2D;
  }
  else
  {
    return CalcMinDistance();
  }
}

/// estimate loudness of a sound before creating it
float SoundSystemOAL::GetLoudness(
  WaveKind kind, Vector3Par pos, float volume, float accomodation
) const
{
  float dist = _listenerPos.Distance(pos);
  
  float adjust = _volumeAdjustEffect;
  switch (kind)
  {
    case WaveSpeech: adjust = _volumeAdjustSpeech; break;
    case WaveMusic: adjust = _volumeAdjustMusic; break;
  }
  float minDistanceVol = floatMax(volume*adjust*accomodation, 0.0f); // Floating point trap experienced without this saturation (volume was < 0)
  float minDistance = floatMax(sqrt(minDistanceVol)*30,1e-6);

  const float maxLoud = 3;
  if( minDistance>=maxLoud*dist ) return maxLoud;
  return DistanceRolloff(minDistance,dist);
}

float WaveOAL::GetLoudness() const
{
  if (_only2DEnabled)
  {
    return GetVolume()/_distance2D;
  }
  else
  {
    if (_3DEnabled)
    {
      float dist = _3DEnabled ? _soundSys->_listenerPos.Distance(GetPosition()) : _distance2D;
      float minDistance= CalcMinDistance();
      const float maxLoud = 3;
      //if( minDistance/dist>=maxLoud ) return maxLoud;
      if( minDistance>=maxLoud*dist ) return maxLoud;
      return DistanceRolloff(minDistance,dist);
    }
    else
    {
      float volume = sqrt(_volume);
      return volume;
    }
  }
}

RString WaveOAL::GetVolumeDiag() const
{
  float gainDb = log10(_gainSet)*20;
  RString ret = Format("g %.1f",gainDb);
  if (_3DEnabled && !_only2DEnabled)
  {
    float minDistance= CalcMinDistance();
    ret = ret + Format(" a %.1f",minDistance);
  }
  return ret;
}

bool WaveOAL::Get3D() const
{
  return WaveBuffersOAL::Get3D();
}

float WaveOAL::Distance2D() const
{
  const float hwFactor = 1;
  if (_only2DEnabled) return hwFactor;
  Assert(!_3DEnabled);
  return _distance2D*hwFactor;
}

void WaveOAL::Set3D(bool is3D, float distance2D)
{
  if (distance2D<0) distance2D = 1;
  if (is3D!=_3DEnabled || distance2D!=_distance2D)
  {
    //LogF("Change mode of %s to %d",(const char *)Name(),is3D);
    // stop buffer to avoid clicks during switching
    DoStop();
    WaveBuffersOAL::Set3D(is3D,distance2D);
  }
}

void WaveOAL::SetRadio(bool isRadio)
{
  if (isRadio!=_isRadio)
  {
    DoStop();
    WaveBuffersOAL::SetRadio(isRadio);
  }
}

void WaveOAL::SetPosition( Vector3Par pos, Vector3Par vel, bool immediate )
{
  _position=pos;
  _velocity=vel;
  // IDirectSound3DBuffer implementation
  _posValid = true;

  if( !_playing ) return;

  //LogF("SetPos %s",(const char *)Name());
  DoSetPosition(_soundSys,immediate);
}



bool WaveBuffersOAL::GetMuted() const
{
  return _gainSet<=1e-20f;
}

int Float2MiliBel( float val )
{
  // for voltage 20*log10(gain) should be used
  // 20 dB is 10x
  if( val<=0 ) return -10000;
  int ret=toIntFloor(log10(val)*2000);
  saturate(ret,-10000,0);
  //Log("float2db %f %d",val,ret);
  return ret;
}

void WaveBuffersOAL::UpdateVolume(SoundSystemOAL *soundSys)
{
  // for 3D sounds and 2D sounds which are part of the environment simulate obstructions
  float volumeAdjust = _volumeAdjust; // global volume control
  if (!_only2DEnabled &&_3DEnabled || _accomodation)
  {
    // instead of volume use 3D MinDistance factor
    // we can use EAX/I3DL2 obstruction/occlusion, or we can lower total volume
    if (soundSys->_eaxEnabled)
    {
      int obstructionDb = Float2MiliBel(_obstruction);
      int occlusionDb = Float2MiliBel(_occlusion);
      SetI3DL2Db(soundSys,obstructionDb,occlusionDb,!_playing);
#if _ENABLE_REPORT
      if (InterestedIn(GetDebugName()))
      {
        LogF("%s: OO %d,%d",cc_cast(GetDebugName()),_obstructionSet,_occlusionSet);
      }
#endif
    }
    else
    {
      // no HW obstruction/occlusion
      // we use total volume to simulate it
      float obsVolume = 1-(1-_obstruction)*0.7f;
      // assume occlusion reduces the sound much more than obstruction
      float occVolume = 1-(1-_occlusion)*0.9f;
      float volumeFactor = floatMin(obsVolume,occVolume);
      volumeAdjust *= volumeFactor;
    }
  }

  if (_only2DEnabled || !_3DEnabled)
  {
    // some problem with setting parameters
    // convert _volume do Db
    float vol = _volume*_accomodation*volumeAdjust;
    if (!_only2DEnabled)
    {
      // this is used for sounds which are 3D, but we use them as 2D
      // typical example are sounds of my own vehicle
      
      // for them we want:
      // 1) never go over certain volume level
      // 2) respect ear accomodation
      // 3) respect external control
      // 2) and 3) are both controlled by _accomodation
      static float maxVolume = 0.5f;
      static float volumeCoef = 2.0f;
      float volume = sqrt(_volume)*_accomodation*volumeCoef;
      // limit volume - 1) 
      if (volume>maxVolume) volume = maxVolume;
      vol = volume*volumeAdjust;
      #if _ENABLE_REPORT
      if (InterestedIn(GetDebugName()))
      {
        LogF("%s: 2D OO %d,%d",cc_cast(GetDebugName()),_obstructionSet,_occlusionSet);
      }
      #endif
    }
    SetGain(soundSys,vol);
    #if _ENABLE_REPORT
    if (InterestedIn(GetDebugName()))
    {
      LogF("%s: 2D vol %g",cc_cast(GetDebugName()),_gainSet);
    }
    #endif
  }
  else
  {
    SetGain(soundSys,volumeAdjust);
    #if _ENABLE_REPORT
    if (InterestedIn(GetDebugName()))
    {
      LogF("%s: vol %g",cc_cast(GetDebugName()),_gainSet);
    }
    #endif
    
  }
}

void WaveBuffersOAL::SetObstruction(SoundSystemOAL *soundSys,float obstruction, float occlusion, float deltaT)
{
  // change obstruction gradually
  const float minEps = 0.01f;
  if (
    fabs(occlusion-_occlusion)<minEps*floatMin(_occlusion,occlusion) &&
    fabs(obstruction-_obstruction)<minEps*floatMin(_obstruction,obstruction)
  )
  {
    return;
  }
  
  if (deltaT<=0)
  {
    _occlusion = occlusion;
    _obstruction = obstruction;
  }
  else
  {
    float delta;
    delta = occlusion - _occlusion;
    const float maxSpeed = 1.5f;
    Limit(delta,-deltaT*maxSpeed,+deltaT*maxSpeed);
    _occlusion += delta;
    
    delta = obstruction - _obstruction;
    Limit(delta,-deltaT*maxSpeed,+deltaT*maxSpeed);
    _obstruction += delta;
  }
  UpdateVolume(soundSys);
}

void WaveBuffersOAL::SetAccomodation(SoundSystemOAL *soundSys, float acc )
{
  if( !_enableAccomodation ) return;
  if (fabs(_accomodation-acc)<0.01f)
  {
    return;
  }
  DoAssert(acc>=0);
  _accomodation=floatMax(acc,0);

  UpdateVolume(soundSys);
}

void WaveBuffersOAL::Set3D( bool is3D, float distance2D )
{
  // disable 3D processing
  Assert(!_3DEnabled || !_only2DEnabled);
  _3DEnabled = is3D;
  _distance2D = distance2D;
}

void WaveBuffersOAL::SetRadio(bool isRadio)
{
  _isRadio = isRadio;
}

bool WaveBuffersOAL::Get3D() const
{
  Assert(!_3DEnabled || !_only2DEnabled);
  return _3DEnabled && !_only2DEnabled;
}

ALCcontext *WaveOAL::DesiredContext() const
{
  #if SEPARATE_2D_CONTEXT
  if (!_3DEnabled || _only2DEnabled) return _soundSys->_context2D;
  return _soundSys->_context3D;
  #else
  return _soundSys->_context3D;
  #endif
}


void WaveBuffersOAL::SetKind( SoundSystemOAL * soundSys, WaveKind kind )
{
  _kind = kind;
  float vol = soundSys->_volumeAdjustEffect;
  if (_kind==WaveMusic) vol = soundSys->_volumeAdjustMusic;
  else if (_kind==WaveSpeech) vol = soundSys->_volumeAdjustSpeech;
  _volumeAdjust = vol;
  UpdateVolume(soundSys);
}

void WaveBuffersOAL::SetVolumeAdjust(SoundSystemOAL *soundSys, float volEffect, float volSpeech, float volMusic)
{
  float vol = volEffect;
  if (_kind==WaveMusic) vol = volMusic;
  else if (_kind==WaveSpeech) vol = volSpeech;
  _volumeAdjust = vol;
  UpdateVolume(soundSys);
}

void WaveBuffersOAL::SetVolume( SoundSystemOAL *soundSys,float vol, bool imm )
{
  _volume=vol;;
  UpdateVolume(soundSys);
}

void WaveOAL::SetFrequency( float freq, bool immediate )
{
  static float minFreqDiff = 1e-4f;
  // we can assume freq is usually something around 1
  if (fabs(freq-_freqSet)>minFreqDiff*freq)
  {
    _freqSet=freq;
    //LogF("%s: ratio %g",(const char *)Name(),ratio);
    if( _playing && _sourceOpen )
    {
      ScopeLockSection lock(_soundSys->GetLock());
      MakeContextCurrent();
      WAV_COUNTER(wavSV,1);
      alSourcef(_source,AL_PITCH,_freqSet);
      CHECK_AOL_ERROR("alSourcei-AL_PITCH");
    }
  }
}

void WaveOAL::SetVolume( float volume, float freq, bool immediate )
{
  if (freq>=0)
  {
    WaveOAL::SetFrequency(freq,immediate);
  }
  // recalculate volume using master volume
  WaveBuffersOAL::SetVolume(_soundSys,volume,immediate);
}

// Sound system

SoundSystemOAL::SoundSystemOAL()
:_soundReady(false)
{
  New((HWND)0,true);
}
SoundSystemOAL::SoundSystemOAL(HWND hwnd, bool dummy)
:_soundReady(false)
{
  RegisterFreeOnDemandSystemMemory(this);
  New(hwnd,dummy);
}

bool ParseUserParams(ParamFile &cfg, GameDataNamespace *globals);
void SaveUserParams(ParamFile &cfg);

SoundSystemOAL::~SoundSystemOAL()
{
  GameDataNamespace globals(NULL, RString(), false); // save - no global namespace

  ParamFile config;
  if (ParseUserParams(config, &globals))
  {
    SaveConfig(config);
    SaveUserParams(config);
  }
  
  Delete();
}

/*!
  \patch 5118 Date 1/17/2007 by Ondra
  - Fixed: Crash after changing Creative XFi mode from Gaming to Entertainment
  or after disabling full acceleration.
*/

void SoundSystemOAL::LoadConfig(ParamEntryPar config)
{
  float cdVolume = GetDefaultCDVolume();
  float fxVolume = GetDefaultWaveVolume();
  float speechVolume = GetDefaultSpeechVolume();
  if
  (
    config.FindEntry("volumeCD") && config.FindEntry("volumeFX")
    && config.FindEntry("volumeSpeech")
  )
  {
    cdVolume=config>>"volumeCD";
    fxVolume=config>>"volumeFX";
    speechVolume=config>>"volumeSpeech";
  }
  SetCDVolume(cdVolume);
  SetWaveVolume(fxVolume);
  SetSpeechVolume(speechVolume);
  bool oldHWWanted=_hwWanted;
  bool oldEAXWanted=_eaxWanted;
  #ifndef _XBOX
  // by default enable EAX
  _eaxEnabled = _eaxWanted = config.ReadValue("soundEnableEAX",true);
  // enable HW by default only when there is a native device available
  bool hwByDefault = _nativeSpecifier.GetLength()>0;
  _hwEnabled = _hwWanted = config.ReadValue("soundEnableHW",hwByDefault);
  #else
  _hwEnabled = true;
  _eaxEnabled = true;
  #endif

#if _FORCE_SINGLE_VOICE
  Glob.config.singleVoice = true;
#else
  Glob.config.singleVoice = false;
  ConstParamEntryPtr entry = config.FindEntry("singleVoice");
  if (entry) Glob.config.singleVoice = *entry;
#endif
  if (_commited && _device && (oldEAXWanted!=_eaxWanted || oldHWWanted!=_hwWanted))
  {
    // if the device is already working, we need to reset it
    ResetDevice();
  }
  if (!_canEAX) _eaxEnabled = false;
}

void SoundSystemOAL::SaveConfig(ParamFile &config)
{
  if (!IsOutOfMemory())
  {
    config.Add("volumeCD",GetCDVolume());
    config.Add("volumeFX",GetWaveVolume());
    config.Add("volumeSpeech",GetSpeechVolume());
    config.Add("singleVoice", Glob.config.singleVoice);
    #ifndef _XBOX
    config.Add("soundEnableEAX",_eaxWanted);
    config.Add("soundEnableHW",_hwWanted);
    #endif
    
    // config.Save(name);
  }
}

void SoundSystemOAL::ResetDevice()
{
  ScopeLockSection lock(GetLock());
  
  _waves.RemoveNulls();
  if (_active)
  {
    // we must restart all playing buffers
    // stop buffers
    for (int i=0; i<_waves.Size(); i++)
    {
      WaveOAL *wave = _waves[i];
      bool wasPlaying = wave->_playing;
      if (wasPlaying)
      {
        wave->Stop();
      }
      if (wave->_loaded)
      {
        wave->Unload();
      }
      if (wasPlaying)
      {
        wave->_wantPlaying = true;
      }
    }

    _cache.Clear();

    DestroyDevice();
    CreateDevice();
    // clear cache - release all stored sounds
    
    // start them again
    for (int i=0; i<_waves.Size(); i++)
    {
      WaveOAL *wave = _waves[i];
      if (wave->_wantPlaying && wave->DoPlay())
      {
        wave->_wantPlaying = false;
      }
    }
  }

}

bool SoundSystemOAL::CanChangeEAX() const
{
  return GetNetworkManager().GetClientState()==NCSNone;
}

bool SoundSystemOAL::CanChangeHW() const
{
  return GetNetworkManager().GetClientState()==NCSNone;
}

bool SoundSystemOAL::EnableHWAccel(bool val)
{
  if (!_canHW) return false;
  if (_hwWanted==val) return true;
  
  _hwWanted = val;
  if (!CanChangeHW()) return _hwWanted;

  _hwEnabled = _hwWanted;
  
  // we must stop and restart all playing buffers
  ResetDevice();

  return _hwWanted;
}


bool SoundSystemOAL::EnableEAX(bool val)
{
  if (!_canEAX) return false;
  if (_eaxWanted==val) return true;

  _eaxWanted = val;
  if (!CanChangeEAX()) return _eaxWanted;

  _eaxEnabled = _eaxWanted;

  // we must stop and restart all playing buffers
  ResetDevice();

  return _eaxWanted;
}

bool SoundSystemOAL::GetEAX() const
{
  return _eaxWanted;
}

bool SoundSystemOAL::GetHWAccel() const
{
  return _hwWanted;
}

bool SoundSystemOAL::GetDefaultEAX() const
{
  // by default always try to use EAX
  return true;
  //if (_canHW && _hwEnabled) return _canEAX && _nativeSpecifier.GetLength()>0;
  //return _canEAX;
}

bool SoundSystemOAL::GetDefaultHWAccel() const
{
  // by default use only native OpenAL HW devices
  return _canHW && _nativeSpecifier.GetLength()>0;
}

#if defined _WIN32
static int OpenALExcFilter(unsigned int code, struct _EXCEPTION_POINTERS *ep)
{

  LONG lDisposition = EXCEPTION_EXECUTE_HANDLER;
  PDelayLoadInfo pdli = PDelayLoadInfo(ep->ExceptionRecord->ExceptionInformation[0]);

  switch (ep->ExceptionRecord->ExceptionCode) {
   case VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND):
     RptF("Dll %s was not found", pdli->szDll);
     break;

   case VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND):
     if (pdli->dlp.fImportByName) {
       RptF("Function %s was not found in %s",
         pdli->dlp.szProcName, pdli->szDll);
     } else {
       RptF("Function ordinal %d was not found in %s",
         pdli->dlp.dwOrdinal, pdli->szDll);
     }
     break; 

   default:
     // Exception is not related to delay loading
     // still, it happened while delay loading, and we want to handle it
     break;
  }

  return lDisposition;
}
#endif

static bool CheckOpenAL()
{
#if !defined _XBOX && defined _WIN32 
# define SUPPORT_URL "http://community.bistudio.com/wiki/ArmA-crash-OpenAL"
  __try
  {
    HRESULT loadErr = __HrLoadAllImportsForDll("openal32.dll");
    if (FAILED(loadErr))
    {
      WarningMessage("Missing OpenAL32.DLL\nVisit " SUPPORT_URL " to learn more.");
      return false;
    }
  }
  __except (OpenALExcFilter(GetExceptionCode(), GetExceptionInformation()))
  {
    // Handle the error. Errors will reach here only if
    // the hook function could not fix it.
    // we still might try to ask OpenAL what version it its
    WarningMessage("Bad version of OpenAL32.DLL\nVisit " SUPPORT_URL " to learn more.");
    return false;
  }
#endif
  return true;
}


/*!
\patch_internal 1.11 Date 8/1/2001 by Ondra
- New: possibility of dummy sound system - no DirectSound objects are ever created.

\patch 5145 Date 3/23/2007 by Ondra
- Fixed: HW acceleration used only when sound card offers at least 60 simultaneous sources.
  This fixes missing sounds while playing MP.

\patch 5203 Date 12/19/2007 by Ondra
- Fixed: Possible crashes with VoN used with HW acceleration.
*/
void SoundSystemOAL::New(HWND winHandle, bool dummy)
{
  _eaxSet = NULL;
  _eaxGet = NULL;
  _context3D = NULL;
  _device = NULL;
  #if !_DISABLE_GUI
  if (dummy)
  #endif
  {
    LogF("No sound - command line switch");
    NoSound:

    _soundReady=true; 
    _commited=false;
    _canHW = false;
    _canEAX = false;
    _equalizerLoaded = false;
    _maxChannels=31;
    _maxFreq=100000;
    _minFreq=10;
    _enablePreview = true;
    _doPreview = false;
    _hwEnabled = false;
    _eaxEnabled = false;

    _speechVolWanted = GetDefaultSpeechVolume();
    _waveVolWanted = GetDefaultWaveVolume();
    _musicVolWanted = GetDefaultCDVolume();
    _active = true;

    GameDataNamespace globals(NULL, RString(), false); // save - no global namespace
    ParamFile config;
    ParseUserParams(config, &globals);
    LoadConfig(config);

    UpdateMixer();
    
    return; 
  }

  #if !_DISABLE_GUI
  _enablePreview = false; // preview disabled until sound initialized
  _doPreview = false;

  _listenerPos=VZero;
  _listenerVel=VZero;
  _listenerDir=VForward;
  _listenerUp=VUp;
  _listenerTime=0;
  
  _volumeAdjustEffect=1;
  _volumeAdjustSpeech=1;
  _volumeAdjustMusic=1;

  _waveVolWanted = GetDefaultWaveVolume();
  _musicVolWanted = GetDefaultCDVolume();
  _speechVolWanted = GetDefaultSpeechVolume();
  _active = true;

  UpdateMixer();  

  {
    GameDataNamespace globals(NULL, RString(), false); // save - no global namespace
    ParamFile config;
    ParseUserParams(config, &globals);
    LoadConfig(config);
  }

  Assert( !_soundReady );

  _eaxEnv.type = SEPlain;
  _eaxEnv.density = 0.2;
  _eaxEnv.size = 100;

  ScopeLockSection lock(GetLock());
  
  // enumerate the devices
  // if the default device is OpenAL 1.1, use it
  // otherwise prefer Generic Software (user can change to Generic Hardware if desired)

  if (!CheckOpenAL()) goto NoSound;

  const char *devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
  const char *defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
  static const char genericHWName[]="Generic Hardware";
  static const char genericSWName[]="Generic Software";
  FindArrayRStringCI deviceNames;

  // caution: when multiple sound cards are present, _native may be a different device
  // we would like to avoid using it
  // therefore we use native only when it is selected as a default, otherwise we ignore it

  for (const char *deviceName = devices; *deviceName; deviceName += strlen(deviceName) + 1)
  {
    ALCdevice *device = alcOpenDevice(deviceName);
    if (device)
    {
      const char *actualDeviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);
      // if actualDeviceName the same as deviceName?
      if (deviceNames.FindKey(actualDeviceName)<0)
      {
        ALCcontext *context = alcCreateContext(device,NULL);
        alcMakeContextCurrent(context);

        int major=0,minor=0;
        alcGetIntegerv(device, ALC_MAJOR_VERSION, sizeof(int), &major);
        alcGetIntegerv(device, ALC_MINOR_VERSION, sizeof(int), &minor);

#if _DEBUG
        alGetError();
        const ALchar *version = alGetString(AL_VERSION);
        CHECK_AOL_ERROR("alGetString(AL_VERSION)");
        const ALchar *vendor = alGetString(AL_VENDOR);
        CHECK_AOL_ERROR("alGetString(AL_VENDOR)");
        const ALchar *renderer = alGetString(AL_RENDERER);
        CHECK_AOL_ERROR("alGetString(AL_RENDERER)");

        LogF(" %s: %s by %s, renderer %s",actualDeviceName,version,vendor,renderer);
        //const ALchar *extensions = alGetString(AL_EXTENSIONS);
        //LogF(" extensions %s",extensions);
        //CHECK_AOL_ERROR("alGetString(AL_EXTENSIONS)");
#endif
        // we are searching for 1.1 device so
        if (major>1 || major==1 && minor>=1)
        {

          // we could perform other checks here to select the best device
          // we need some minimal number of sources
          // check if the device has enough sound sources
          //const int sourcesWanted = 60;
          const int sourcesNeeded = 60;
          //const int sourcesWanted = 255;
          const int sourcesWanted = 60;
          ALuint sources[sourcesWanted];
          alGetError();
          // find number of sources using binary search?
          bool usable = false;
          for (int nSources=sourcesWanted; nSources>=sourcesNeeded; nSources--)
          {
            alGenSources(nSources,sources);
            if (alGetError()==AL_NO_ERROR)
            {
              alDeleteSources(nSources,sources);
              usable = true;
              break;
            }
          }
          if (usable)
          {
            deviceNames.AddUnique(actualDeviceName);
            if (strcmp(genericHWName, deviceName) == 0) _hwSpecifier = actualDeviceName;
            else if (strcmp(genericSWName, deviceName) == 0) _swSpecifier = actualDeviceName;
            else if (strcmp(defaultDeviceName, deviceName) == 0) _nativeSpecifier = actualDeviceName;
          }
        }
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
      }
      alcCloseDevice(device);
    }
  }

  // unless proven otherwise, assume no HW acceleration
  _canHW = false;
  _canEAX = false;
  _equalizerLoaded = false;
  // we know OpenAL 1.1 runtime is installed, at least SW device must support 1.1
  if (deviceNames.Size()>0)
  {
    // when SW and HW not found, apply heuristics - SW is last, HW first
    if (_swSpecifier.GetLength()<=0) _swSpecifier = deviceNames[deviceNames.Size()-1];
    if (_hwSpecifier.GetLength()<=0 && deviceNames.Size()>=2 ) _hwSpecifier = deviceNames[0];
    // if there is HW or native, offer it as HW acceleration
    if (_hwSpecifier.GetLength()>0 || _nativeSpecifier.GetLength()>0) _canHW = true;
  }
  else
  {
    RptF("Error: No OpenAL 1.1 device found");
  }

  CreateDevice();

  // TODO: _maxChannels auto-detection
  _maxChannels=31;
  _maxFreq=100000;
  _minFreq=10;

  _enablePreview = true; // preview disabled until sound initialized
  _doPreview = false;

  #endif
}

void SoundSystemOAL::CreateDevice()
{
  // no buffers or source can exist at this point
  //_waves.RemoveNulls();
  //Assert(_waves.Size()==0);
  //Assert(_cache.Size()==0);
  
  RString spec = _swSpecifier;
  // use MMSYSTEM to avoid any DirectSound acceleration
  #if 0
  RString mmSpec = "MMSYSTEM";
  spec = mmSpec;
  #endif
  // in _hwWanted depending on _hwWanted we may force 
  if (_canHW && _hwWanted)
  {
    spec = _nativeSpecifier.GetLength()>0 ? _nativeSpecifier : _hwSpecifier;
  }

  // Initialization
  const char *specChar = spec.GetLength()>0 ? cc_cast(spec) : (const char *)NULL;
  // using default device is wrong - we should always select some during enumeration
  DoAssert(specChar)  ;
  _context3D = NULL;
  _device = alcOpenDevice(specChar); // select the "preferred device"
  #if 0
  if (!_device && _swSpecifier.GetLength()>0)
  {
    // preferred device failed, use Generic Sotfware
    _device = alcOpenDevice(_swSpecifier);
  }
  #endif

  if (_device)
  {
    ALCenum err;
    #if SEPARATE_2D_CONTEXT
      err = alcGetError(_device);
      _context2D = alcCreateContext(_device,NULL);
      alcMakeContextCurrent(_context2D);
      alDistanceModel(AL_NONE);
      err = alcGetError(_device);
    #endif
    
    _context3D = alcCreateContext(_device,NULL);
    alcMakeContextCurrent(_context3D);
    err = alcGetError(_device);
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

    int major = 0, minor = 0;
    alcGetIntegerv(_device, ALC_MAJOR_VERSION, sizeof(int), &major);
    alcGetIntegerv(_device, ALC_MINOR_VERSION, sizeof(int), &minor);
    const ALchar *specifier = alcGetString(_device,ALC_DEVICE_SPECIFIER);
    LogF("OpenAL %s, version %d.%d",specifier,major,minor);
    
    #if _DEBUG
    alGetError();

	  const ALchar *vendor = alGetString(AL_VENDOR);
	  CHECK_AOL_ERROR("alGetString(AL_VENDOR)");
	  const ALchar *renderer = alGetString(AL_RENDERER);
	  CHECK_AOL_ERROR("alGetString(AL_RENDERER)");
	  const ALchar *extensions = alGetString(AL_EXTENSIONS);
	  CHECK_AOL_ERROR("alGetString(AL_EXTENSIONS)");
    LogF(" vendor %s, renderer %s",vendor,renderer);
    if (extensions)
    {
      for (const char *ext=extensions; *ext; )
      {
        const char *end = strpbrk(ext," \n\r\t");
        RString part = end ? RString(ext,end-ext) : RString(ext);
        LogF(" ext %s",cc_cast(part));
        
        if (!end) break;
        ext=end+1;
      }
    }
    #endif
  }
  // TODO: migrate to EFX
  //if (alIsExtensionPresent("ALC_EXT_EFX"))
  if (alcIsExtensionPresent(_device,"ALC_EXT_EFX"))
  {
    LogF("  EFX extension supported");
  }
  #define EAX_VERSION "EAX4.0"
  _canEAX = alIsExtensionPresent(EAX_VERSION)!=AL_FALSE;
  if (_canEAX)
  {
    LogF("  " EAX_VERSION " detected");
  }
  if (!_canEAX)
  {
    // if native support is not here, check for emulated support
    _canEAX = alIsExtensionPresent(EAX_VERSION "Emulated")!=AL_FALSE;
    if (_canEAX)
    {
      LogF("  " EAX_VERSION "Emulated detected");
    }
  }
  if (!_canEAX)
  {
      LogF("  " EAX_VERSION " not detected");
  }
  // Generate Buffers
  alGetError(); // clear error code
  
  _soundReady=true; 
  _commited=false;
  _equalizerLoaded = false;
   // check valid freq. range
  _hwEnabled = _hwWanted && _canHW;
  _eaxEnabled = _eaxWanted && _canEAX;
  if (_eaxEnabled)
  {
    _eaxEnabled = InitEAX();
  }
}

void SoundSystemOAL::DestroyDevice()
{
  // no buffers or source can exist at this point
  #if _DEBUG
    _waves.RemoveNulls();
    for (int i=0; i<_waves.Size(); i++)
    {
      WaveOAL *wave = _waves[i];
      Assert(wave->_buffer.IsNull());
      Assert(wave->_streamBuffers.Size()==0);
    }
    Assert(_cache.Size()==0);
  #endif

  // when device will be created, it will set it again if needed
  _eaxSet = NULL;
  _eaxGet = NULL;
  _eaxEnabled = false;
  _canEAX = false;
  
  // Exit
  if (_device)
  {
    alcMakeContextCurrent(NULL);
    if (_context3D) alcDestroyContext(_context3D),_context3D=NULL;
    #if SEPARATE_2D_CONTEXT
    if (_context2D) alcDestroyContext(_context2D),_context2D=NULL;
    #endif
    alcCloseDevice(_device),_device=NULL;  
  }
}

static const RString PauseName="";

WaveOAL::WaveOAL( SoundSystemOAL *sSys, float delay ) // empty wave
:AbstractWave(PauseName)
{
  WaveBuffersOAL::Reset(sSys);
  DoConstruct();
  _soundSys=sSys;
  _only2DEnabled=true;

  _gainSet=0;
  _obstructionSet = _occlusionSet = 0;
  _obstruction = _occlusion = 1;
  _freqSet=1;
  _header._frequency = 1000;
  _header._sSize = 1;
  _header._size = 0;
  _curPosition = toLargeInt(delay*-1000); // delay time in ms
  _skipped = 0;
  _volumeAdjust = _soundSys->_volumeAdjustEffect;
  _looping = false; 
  _posValid = false;
  _loadedHeader = true;
  _loaded = true;
  _sourceOpen = false;
}

WaveOAL::WaveOAL( SoundSystemOAL *sSys, RString name, bool is3D )
:AbstractWave(name)
{
  WaveBuffersOAL::Reset(sSys);
  DoConstruct();
  _soundSys=sSys;
  _only2DEnabled=!is3D;

  _posValid=!is3D; // 3d position explicitly set

  _gainSet=0;
  _obstructionSet = _occlusionSet = 0;
  _obstruction = _occlusion = 1;
  _freqSet=1;
  _curPosition = 0;
  _skipped = 0;
  _volumeAdjust = _soundSys->_volumeAdjustEffect;

  LoadHeader();
}



WaveOAL::~WaveOAL()
{
  DoDestruct();
}

void WaveOAL::CacheStore( WaveCacheOAL &store )
{
  #if DO_PERF
    ADD_COUNTER(wavCS,1);
  #endif
  store._name=Name();
  store._buffer=_buffer;
  store._header = _header;
}

WaveCacheOAL::~WaveCacheOAL()
{
  if (!_buffer.IsNull())
  {
    #if DO_PERF
      ADD_COUNTER(wavCD,1);
    #endif

    _buffer.Free();

    #if DIAG_LOAD
      if (refC==0)
      {
        LogF("Destroy cache %s",(const char *)_name);
      }
      else
      {
        #if DIAG_LOAD>=2
        LogF("Release cache %s",(const char *)_name);
        #endif
      }
    #endif
  }
}

  
void WaveOAL::InitEAX()
{
  // send not needed - by default sound is sent to Primary FX with 0 attenuation
  if (_soundSys->_eaxEnabled && _soundSys->_device)
  {
    ScopeLockSection lock(_soundSys->GetLock());
    if (!_only2DEnabled && _3DEnabled || _isRadio)
    {
      GUID active[2];
      if (_isRadio && _soundSys->_equalizerLoaded)
      {
        // if possible, we want to use the equalizer
        active[0] = EAXPROPERTYID_EAX40_FXSlot2;
        active[1] = EAX_NULL_GUID;
        Assert(_only2DEnabled || !_3DEnabled)
      }
      else
      {
        active[0] = EAX_NULL_GUID;
        active[1] = EAXPROPERTYID_EAX40_FXSlot0;
      }
	    _soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_ACTIVEFXSLOTID,_source,&active,sizeof(active));

      if (_isRadio && _soundSys->_equalizerLoaded)
      {
        // we know slot 0 has 5000 set as HF cutoff for all environments
        EAXSOURCESENDPROPERTIES send;
	      // Equalized - feed slot 2 only
	      send.guidReceivingFXSlotID = EAXPROPERTYID_EAX40_FXSlot0;
	      send.lSend = -10000;
	      send.lSendHF = -10000;
	      _soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_SENDPARAMETERS,_source,&send,sizeof(send));
	      
	      send.guidReceivingFXSlotID = EAXPROPERTYID_EAX40_FXSlot2;
	      static int eqSendLevel = -1800;
	      static int eqSendLevelHF = 0;
	      send.lSend = eqSendLevel;
	      send.lSendHF = eqSendLevelHF;
	      _soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_SENDPARAMETERS,_source,&send,sizeof(send));
      }
      else
      {
        // we know slot 0 has 5000 set as HF cutoff for all environments
        EAXSOURCESENDPROPERTIES send;
	      // We currently use slot 0 only
	      send.guidReceivingFXSlotID = EAXPROPERTYID_EAX40_FXSlot0;
	      send.lSend = 0;
	      send.lSendHF = 0;
	      _soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_SENDPARAMETERS,_source,&send,sizeof(send));

	      send.guidReceivingFXSlotID = EAXPROPERTYID_EAX40_FXSlot2;
	      send.lSend = -10000;
	      send.lSendHF = -10000;
	      _soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_SENDPARAMETERS,_source,&send,sizeof(send));
      }
    }
    else
    {
      // 2D sound send to no slots
      GUID active[2];
      active[0] = EAX_NULL_GUID;
      active[1] = EAX_NULL_GUID;
	    _soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_ACTIVEFXSLOTID,_source,&active,sizeof(active));
    }
	  
  }
}

bool WaveOAL::CacheLoad( WaveCacheOAL &store )
{
  //Log("CacheLoad %s",(const char *)Name());
  Assert(_buffer.IsNull());
  _buffer=store._buffer;
  // no need to cache when active
  store._buffer=NULL;

  {
    ScopeLockSection lock(_soundSys->GetLock());
    // we gave the buffer - now create the source
    alGenSources(1,&_source);
    LogAL("OpenAL alGenSources %d",_source);
    if (alGetError()!=AL_NO_ERROR)
    {
      _buffer.Free();
      return true;
    }
    LogAL("OpenAL AL_BUFFER %d:%d (CacheLoad)",_source,(ALint)_buffer);
    alSourcei(_source,AL_BUFFER,_buffer);
    CHECK_AOL_ERROR("alSourcei-AL_BUFFER");
  }
  
  _sourceOpen = true;
  
  InitEAX();
  return true;
}

bool WaveOAL::Duplicate( const WaveOAL &src )
{
  if (src._stream || src._buffer.IsNull()) return false; // no duplicate possible on streaming buffers

  //Log("Duplicate %s",(const char *)src.Name());
  #if DO_PERF
  WAV_COUNTER(wavDp,1);
  #endif
  DoAssert( src.Loaded() );

  // shared buffer data
  _buffer = src._buffer;
  Assert(src._loadedHeader);
  _header=src._header;

  //_maxVolume=src._maxVolume;
  //_avgVolume=src._avgVolume;

  ScopeLockSection lock(_soundSys->GetLock());
  {
    CHECK_AOL_ERROR("Duplicate-clear");
    // we gave the buffer - now create the source
    alGenSources(1,&_source);
    if (alGetError()!=AL_NO_ERROR)
    {
      _buffer.Free();
      return false;
    }
    LogAL("OpenAL AL_BUFFER %d:%d (Duplicate)",_source,(ALint)_buffer);
    alSourcei(_source,AL_BUFFER,_buffer);
    CHECK_AOL_ERROR("alSourcei-AL_BUFFER");
  }

  _sourceOpen = true;
  _loaded=true;

  InitEAX();
  
  AddStats("Dup");
  //_soundSys->LogChannels("Duplicate");
  return true;
}

bool SoundSystemOAL::LoadHeaderFromCache( WaveOAL *wave )
{
  for( int i=0; i<_cache.Size(); i++ )
  {
    WaveCacheOAL &cache=_cache[i];
    if( !strcmp(cache._name,wave->Name()) )
    {
      wave->_header = cache._header;
      return true;
    }
  }
  return false;
}

bool SoundSystemOAL::LoadFromCache( WaveOAL *wave )
{
  for( int i=0; i<_cache.Size(); i++ )
  {
    WaveCacheOAL &cache=_cache[i];
    if( !strcmp(cache._name,wave->Name()) )
    {
      if (wave->CacheLoad(cache))
      {
        _cache.Delete(i);
        return true;
      }
    }
  }
  return false;
}

void SoundSystemOAL::LogChannels( const char *name )
{
}

void SoundSystemOAL::ReserveCache( int number3D, int number2D )
{
}

void SoundSystemOAL::StoreToCache( WaveOAL *wave )
{
  if (wave->_stream)
  {
    Fail("Storing streaming buffer");
    return;
  }
  //Log("StoreToCache %s",(const char *)wave->Name());
  #if DO_PERF
    ADD_COUNTER(wavSC,1);
  #endif
  // note: it may make sense to have the same sound stored twice
  // if DuplicateBuffer does not work well, it may be much faster
  // this happened only when debugging in DX8 beta
  /**/
  for( int i=0; i<_cache.Size();  )
  {
    WaveCacheOAL &cache=_cache[i];
    if( !strcmp(cache._name,wave->Name()) )
    {
      #if DIAG_LOAD>=2
        LogF("WaveOAL %s cache refresh",(const char *)_cache[i]._name);
      #endif
      _cache.Delete(i);
      // move to cache beginning
      _cache.Insert(0);
      wave->CacheStore(_cache[0]);
      return;
    }
    else i++;
  }
  /**/


  int size=_cache.Size();
  //int waves=_waves.Size();
  // free HW channels

  const int maxSize=128;
  while( size>=maxSize )
  {
    _cache.Delete(--size);
    //LogChannels("CacheSize");
  }

  int index=0;
  _cache.Insert(index);
  WaveCacheOAL &cache=_cache[index];
  wave->CacheStore(cache);

  #if DIAG_LOAD>=2
  LogF("WaveOAL %s inserted into the cache",(const char *)wave->Name());
  #endif
}

float SoundSystemOAL::GetWaveDuration( const char *filename )
{
  // load file header - we need to wait
  Ref<WaveStream> stream;
  DoAssert(RString::IsLowCase(filename));
  // waiting for a sound
  SoundRequestFile(filename,stream,true);

  // only header is needed - whole files are cached
  if (!stream)
  {
#ifndef DUMMY_SOUND_FILE
    RptF("Cannot find a sound file %s", filename);
#endif
    return 1;
  }

  WAVEFORMATEX format;
  stream->GetFormat(format);
  int frequency = format.nSamplesPerSec;      // sampling rate
  int sSize = format.nBlockAlign;           // number of bytes per sample
  int size = stream->GetUncompressedSize();
  
  if (frequency==0 || sSize==0)
  {
    RptF("Bad header in file %s",cc_cast(filename));
    return 1;
  }

  return float(size/sSize)/frequency;

}

/// structure used to port parameters from Xbox to PC
struct DSI3DL2LISTENER
{
    long            lRoom;                  // [-10000, 0] default: -10000 mB
    long            lRoomHF;                // [-10000, 0] default: 0 mB
    float           flRoomRolloffFactor;    // [0.0, 10.0] default: 0.0
    float           flDecayTime;            // [0.1, 20.0] default: 1.0 s
    float           flDecayHFRatio;         // [0.1, 2.0] default: 0.5
    long            lReflections;           // [-10000, 1000] default: -10000 mB
    float           flReflectionsDelay;     // [0.0, 0.3] default: 0.02 s
    long            lReverb;                // [-10000, 2000] default: -10000 mB
    float           flReverbDelay;          // [0.0, 0.1] default: 0.04 s
    float           flDiffusion;            // [0.0, 100.0] default: 100.0 %
    float           flDensity;              // [0.0, 100.0] default: 100.0 %
    float           flHFReference;          // [20.0, 20000.0] default: 5000.0 Hz
};


static void ConvertI3DL2ToEax(
  unsigned long env, float envSize, EAXREVERBPROPERTIES &eax, const DSI3DL2LISTENER &i3d
)
{
  eax.ulEnvironment = env;
  eax.flEnvironmentSize = envSize;
  eax.flEnvironmentDiffusion = 1.0;
  eax.lRoom = i3d.lRoom;
  eax.lRoomHF = i3d.lRoomHF;
  eax.lRoomLF = 0;
  eax.flDecayTime = i3d.flDecayTime;
  eax.flDecayHFRatio = i3d.flDecayHFRatio;
  eax.flDecayLFRatio = 1.0f;
  eax.lReflections = i3d.lReflections;
  eax.flReflectionsDelay = i3d.flReflectionsDelay;
  EAXVECTOR zero={0,0,0};
  eax.vReflectionsPan = zero;
  eax.lReverb = i3d.lReverb;
  eax.flReverbDelay = i3d.flReverbDelay;
  eax.vReverbPan = zero;
  eax.flEchoTime = 0.25;
  eax.flEchoDepth = 0;
  eax.flModulationTime = 0.25;
  eax.flModulationDepth = 0;

  eax.flAirAbsorptionHF = -5;
  eax.flHFReference = i3d.flHFReference;
  eax.flLFReference = 250.0f; // Hz
  eax.flRoomRolloffFactor = i3d.flRoomRolloffFactor;
  // TODO: use flags to use the room size
  eax.ulFlags = 0;
}

// avoid small room roll-off, as such sounds could be audible at extreme distances
static DSI3DL2LISTENER EnvPlain    ={-1500,-1500,0.7f,1,0.2,-850,0.06,-900,0.1,16,100,5000};
static DSI3DL2LISTENER EnvForest   ={-3000,-1000,0.7f,2.5,0.1 ,0,0.1,-900,0.5,80,100,5000};
static DSI3DL2LISTENER EnvCityOpen ={-2500,-800 ,0.4f,5  ,0.2 ,-1000,0.04,-1000,0.1,20,100,5000};
static DSI3DL2LISTENER EnvMountains={-2000,-1500,0.4f ,10 ,0.2 ,-1500,0.1,-1000,0.1,21,100,5000};
static DSI3DL2LISTENER NoReverb    ={-10000, -10000, 1.0f, 1.00f, 1.00f, -10000, 0.000f, -10000, 0.000f,   0.0f,   0.0f, 5000.0f};

bool SoundSystemOAL::InitEAX()
{
  if (!_canEAX) return false;
  _eaxSet = (EAXSet)alGetProcAddress("EAXSet");
  _eaxGet = (EAXGet)alGetProcAddress("EAXGet");
  if (_eaxSet==NULL || _eaxGet==NULL)
  {
    // for sure mark EAX as non existant
    _canEAX = false;
    return false;
  }

  GUID primary = EAXPROPERTYID_EAX40_FXSlot0;
  _eaxSet(
    &EAXPROPERTYID_EAX40_Context,EAXCONTEXT_PRIMARYFXSLOTID, 0,
    &primary, sizeof(primary)
  );

  EAXFXSLOTPROPERTIES fxSlot;
  fxSlot.guidLoadEffect = EAX_REVERB_EFFECT;
  fxSlot.lLock = EAXFXSLOT_LOCKED;
  fxSlot.lVolume = 0;
  fxSlot.ulFlags = EAXFXSLOTFLAGS_ENVIRONMENT;

  bool ret = true;
  if (_eaxSet(&EAXPROPERTYID_EAX40_FXSlot0, EAXFXSLOT_ALLPARAMETERS, 0, &fxSlot, sizeof(fxSlot))!=AL_NO_ERROR)
  {
		// Could have failed because the slot was already locked.
    // note: it is common to fail - slots 0 and 1 are locked as reverb because of legacy issues
    GUID loaded = EAX_NULL_GUID;
		_eaxGet(&EAXPROPERTYID_EAX40_FXSlot0, EAXFXSLOT_LOADEFFECT, 0, &loaded, sizeof(loaded));
		if (memcmp(&loaded,&EAX_REVERB_EFFECT,sizeof(loaded)))
		{
		  // if the first slot is locked and it is not a reverb, we cannot use EAX
		  ret = false;
		}
  }

  // if the implementation supports it, select equalizer into slot 2
  fxSlot.guidLoadEffect = EAX_EQUALIZER_EFFECT;
  fxSlot.lLock = EAXFXSLOT_LOCKED;
  fxSlot.lVolume = 0;
  fxSlot.ulFlags = 0; //EAXFXSLOTFLAGS_ENVIRONMENT;

  #if 1
  // EQUALIZER not working yet - unable to attenuate direct path
	_equalizerLoaded = true;
  if (_eaxSet(&EAXPROPERTYID_EAX40_FXSlot2, EAXFXSLOT_ALLPARAMETERS, 0, &fxSlot, sizeof(fxSlot))!=AL_NO_ERROR)
  {
		// Could have failed because the slot was already locked.
    // note: it is common to fail - slots 0 and 1 are locked as reverb because of legacy issues
    GUID loaded = EAX_NULL_GUID;
		_eaxGet(&EAXPROPERTYID_EAX40_FXSlot2, EAXFXSLOT_LOADEFFECT, 0, &loaded, sizeof(loaded));
		if (memcmp(&loaded,&EAX_EQUALIZER_EFFECT,sizeof(loaded)))
		{
		  // problem: equalized not loaded
		  _equalizerLoaded = false;
		}
  }
  
  if (_equalizerLoaded)
  {
    EAXEQUALIZERPROPERTIES eqParams;
    eqParams.lLowGain = -1800;
    eqParams.flLowCutOff = 250;
    eqParams.lMid1Gain = 1800;
    eqParams.flMid1Center = 1600;
    eqParams.flMid1Width = 0.90;
    eqParams.lMid2Gain = 1800;
    eqParams.flMid2Center = 2000;
    eqParams.flMid2Width = 0.80;
    eqParams.lHighGain = -1800;
    eqParams.flHighCutOff = 4000;

    _eaxSet(&EAXPROPERTYID_EAX40_FXSlot2, EAXEQUALIZER_ALLPARAMETERS, 0, &eqParams, sizeof(eqParams));
  }
  #endif
  
  DoSetEAXEnvironment();
  return ret;
}

void SoundSystemOAL::DeinitEAX()
{
  if (_canEAX && _eaxSet)
  {
    // we want to reset the listener so that it uses no EAX

    EAXREVERBPROPERTIES eaxPars;
    ConvertI3DL2ToEax(EAX_ENVIRONMENT_UNDEFINED,100,eaxPars,NoReverb);

    _eaxSet(&EAXPROPERTYID_EAX40_FXSlot0, EAXREVERB_ALLPARAMETERS, 0, &eaxPars, sizeof(eaxPars));
    _eaxSet(&EAXPROPERTYID_EAX40_FXSlot0, EAXREVERB_NONE, 0, &eaxPars, sizeof(eaxPars));
  }
  // no cleanup needed with OpenAL
  _eaxSet = NULL;
  _eaxGet = NULL;
}


#if _ENABLE_CHEATS
  // real-time tuning  via scripting
  #include <El/evaluator/expressImpl.hpp>

  static GameValue DebugEnvPars(const GameState *state, GameValuePar oper1, GameValuePar oper2)
  {
    RString envName = oper1;
    const GameArrayType &array = oper2;
    DSI3DL2LISTENER *env = NULL;
    if (!strcmpi(envName,"plain")) env = &EnvPlain;
    if (!strcmpi(envName,"forest")) env = &EnvForest;
    if (!strcmpi(envName,"city")) env = &EnvCityOpen;
    if (!strcmpi(envName,"mountains")) env = &EnvMountains;
    if (!env)
    {
      GlobalShowMessage(1000,"Unknown environment %s",cc_cast(envName));
      return GameValue();
    }
    if (array.Size()!=12)
    {
      state->SetError(EvalDim,array.Size(),12);
      return GameValue();
    }
    for (int i=0; i<array.Size(); i++)
    {
      if (array[i].GetType()!=GameScalar)
      {
        state->TypeError(array[i].GetType(),GameScalar);
        return GameValue();
      }
    }

    env->lRoom = toInt(array[0]);
    env->lRoomHF = toInt(array[1]);
    env->flRoomRolloffFactor = array[2];
    env->flDecayTime = array[3];
    env->flDecayHFRatio = array[4];
    env->lReflections = toInt(array[5]);
    env->flReflectionsDelay = array[6];
    env->lReverb = toInt(array[7]);
    env->flReverbDelay = array[8];
    env->flDiffusion = array[9];
    env->flDensity = array[10];
    env->flHFReference = array[11];
    
    (static_cast<SoundSystemOAL *>(GSoundsys))->DoSetEAXEnvironment();
    return GameValue();
  }

  #include <El/Modules/modules.hpp>

  static const GameOperator DbgBinary[]=
  {
    GameOperator(GameNothing,"diag_setenv",function,DebugEnvPars,GameString,GameArray TODO_OPERATOR_DOCUMENTATION),
  };

  INIT_MODULE(GameStateDbgEAX, 3)
  {
    GGameState.NewOperators(DbgBinary,lenof(DbgBinary));
    //GGameState.NewFunctions(DbgUnary,lenof(DbgUnary));
  };
  
#endif

void SoundSystemOAL::DoSetEAXEnvironment()
{
  if (!_device || !_canEAX || !_eaxEnabled) return;
  
  #if USE_EAX
    // set I3DL2 reverberation

    const DSI3DL2LISTENER *pars = &NoReverb;
    DWORD type = EAX_ENVIRONMENT_PLAIN;
    switch (_eaxEnv.type)
    {
      default:
      case SEPlain: pars = &EnvPlain, type = EAX_ENVIRONMENT_PLAIN; break;
      case SEMountains: pars = &EnvMountains, type = EAX_ENVIRONMENT_MOUNTAINS; break;
      case SEForest: pars = &EnvForest, type = EAX_ENVIRONMENT_CITY; break;
      case SERoom: pars = &EnvCityOpen, type = EAX_ENVIRONMENT_ROOM; break;
      case SECity: pars = &EnvCityOpen, type = EAX_ENVIRONMENT_CITY; break;
    } 
    float size = _eaxEnv.size;
    saturate(size,2,200);
    EAXREVERBPROPERTIES eaxPars;
    ConvertI3DL2ToEax(type,size,eaxPars,*pars);
    ScopeLockSection lock(GetLock());
    _eaxSet(&EAXPROPERTYID_EAX40_FXSlot0, EAXREVERB_ALLPARAMETERS, 0, &eaxPars, sizeof(eaxPars));

    // TODO: time to time reset equalizer properties?
  #endif
}

void SoundSystemOAL::SetEnvironment( const SoundEnvironment &env )
{
  if
  (
    env.type==_eaxEnv.type &&
    fabs(env.size-_eaxEnv.size)<1 &&
    fabs(env.density-_eaxEnv.density)<0.05
  )
  {
    // no change
    return;
  }
  _eaxEnv = env;

  DoSetEAXEnvironment();
}

/*!
\patch 1.34 Date 12/10/2001 by Ondra
- Fixed: MP: Dedicated server memory leak.
(Caused by bug in sound management with sounds disabled).
*/

void SoundSystemOAL::AddWaveToList(WaveGlobalOAL *wave)
{
  for (int i=0; i<_waves.Size(); i++)
  {
    if (!_waves[i])
    {
      _waves[i] = wave;
      return;
    }
  }
  _waves.Add(wave);
}

AbstractWave *SoundSystemOAL::CreateWave( const char *filename, bool is3D, bool prealloc )
{

  // TODO: use prealloc
  if( !_soundReady ) return NULL;
  // if the wave is preloaded, simply duplicate it
  char lowName[256];
  strcpy(lowName,filename);
  strlwr(lowName);
  if (!*lowName) return NULL; // empty filename

  WaveGlobalOAL *wave=new WaveGlobalOAL(this,lowName,is3D);
  //LogF("Created %s: %x",lowName,wave);
  AddWaveToList(wave);
  return wave;
}

AbstractWave *SoundSystemOAL::CreateEmptyWave( float duration )
{
  // create a WaveOAL with no buffers, only delay
  if( !_soundReady ) return NULL;
  WaveGlobalOAL *wave=new WaveGlobalOAL(this,duration);
  //LogF("Created empty %x",wave);
  AddWaveToList(wave);
  return wave;
}

void SoundSystemOAL::Delete()
{
  if( !_soundReady ) return;

  _previewEffect.Free();
  _previewMusic.Free();
  _previewSpeech.Free();

  #if DIAG_LOAD
  while( _cache.Size()>0 )
  {
    const WaveCacheOAL &wc = _cache[0];
    LogF("WaveOAL %s deleted from cache",(const char *)wc._name);
    _cache.Delete(0);
    //LogChannels("CacheSize");
  }
  #endif

  _cache.Clear(); 
  _waves.Clear();

  DestroyDevice();
  _soundReady=false;
}



void SoundSystemOAL::SetListener(Vector3Par pos, Vector3Par vel, Vector3Par dir, Vector3Par up)
{
  PROFILE_SCOPE_EX(snLis,sound);
  _listenerPos=pos;
  _listenerVel=vel;
  _listenerDir=dir.Normalized();
  _listenerUp=up.Normalized();
  if (_listenerDir.SquareSize()<0.1f) _listenerDir = VForward;
  if (_listenerUp.SquareSize()<0.1f) _listenerUp = VUp;
  if (!_soundReady) return;
  if (!_device) return;

  DWORD time = GlobalTickCount();
  // max 20 fps listener position update
  const DWORD minTime = 50;
  if (_listenerTime==0 || time-_listenerTime>minTime)
  {
    ScopeLockSection lock(GetLock());
    #if DEFERRED
      SND_COUNTER(wavLD,1);
    #else
      SND_COUNTER(wavLI,1);
    #endif
    // note: we invert x to convert between left handed and right handed system
    alListener3f(AL_POSITION,-pos.X(),pos.Y(),pos.Z());
    CHECK_AOL_ERROR("alListener3f-AL_POSITION");
    alListener3f(AL_VELOCITY,-vel.X(),vel.Y(),vel.Z());
    CHECK_AOL_ERROR("alListener3f-AL_VELOCITY");
    float orient[6]={-dir.X(),dir.Y(),dir.Z(),-up.X(),up.Y(),up.Z()};
    alListenerfv(AL_ORIENTATION,orient);
    CHECK_AOL_ERROR("alListenerfv-AL_ORIENTATION");
    _listenerTime = time;
  }
}

static float FloatToDb(float x)
{
  if (x<=1e-5) return -100;
  float db = 20*log10(x);
  return db;
}

static bool PreviewAdvance(AbstractWave *wave, float deltaT)
{
  if (!wave) return false;
  if (!wave->IsMuted())
  {
    wave->Play();
    //LogF("Preview %s play",(const char *)wave->Name());
  }
  else
  {
    wave->Stop();
    wave->Advance(deltaT);
    //LogF("Preview %s muted",(const char *)wave->Name());
  }
  return wave->IsTerminated();
}

/*!
\patch 1.01 Date 15/6/2001 by Ondra.
- Fixed double music volume in Audio options.
*/

void SoundSystemOAL::Commit()
{
  if( !_soundReady ) return;

  PROFILE_SCOPE_EX(snCmt,sound);
	#if _ENABLE_CHEATS 
		if (CHECK_DIAG(DESound))
		{
		  #if 0 // defined(_XBOX)
        DSOUTPUTLEVELS levels;
        _ds->GetOutputLevels(&levels,true);
        const float maxRMS = 1<<23;
        DIAG_MESSAGE(
          100,Format(
            "RMS L %+4.1f, R %+4.1f",
            FloatToDb(levels.dwAnalogLeftTotalRMS/maxRMS),
            FloatToDb(levels.dwAnalogRightTotalRMS/maxRMS)
          )
        );
        DIAG_MESSAGE(
          100,Format(
            "Max L %+4.1f, R %+4.1f",
            FloatToDb(levels.dwAnalogLeftTotalPeak/maxRMS),
            FloatToDb(levels.dwAnalogRightTotalPeak/maxRMS)
          )
        );
      #endif
/*
      DIAG_MESSAGE(100,Format(
        "Alloc: HW 3D %d, HW 2D %d, SW %d, Free %d, Cache %d",
        WaveOAL::_counters[WaveOAL::Hw3D],WaveOAL::_counters[WaveOAL::Hw2D],
        WaveOAL::_counters[WaveOAL::Sw],WaveOAL::_counters[WaveOAL::Free],
        _cache.Size()
      ));
      DIAG_MESSAGE(100,Format(
        "Play:  HW 3D %d, HW 2D %d, SW %d, Free %d",
        WaveOAL::_countPlaying[WaveOAL::Hw3D],WaveOAL::_countPlaying[WaveOAL::Hw2D],
        WaveOAL::_countPlaying[WaveOAL::Sw],WaveOAL::_countPlaying[WaveOAL::Free]
      ));
*/
		}
  #endif
  _commited=true;
  if (_active)
  {
    for( int i=0; i<_waves.Size(); i++ )
    {
      WaveOAL *wave = _waves[i];
      if (!wave) continue;
      if (wave->_wantPlaying)
      {
        if (wave->DoPlay())
        {
          wave->_wantPlaying=false;
        }
      }
    }
  }

#if _ENABLE_CHEATS 
  if( GInput.GetCheat2ToDo(DIK_W) )
  {
    // scan all playing waves
    LogF("Actual sound buffers:");
    int nPlay = 0;
    int nStop = 0;
    int nTerm = 0;
    for( int i=0; i<_waves.Size(); i++ )
    {
      WaveOAL *wave=_waves[i];
      if( !wave ) continue;
      if (wave->_playing)
      {
        nPlay++;
        if (wave->_terminated) nTerm++;
      }
      else nStop++;
      const char *name = wave->Name();
      if (wave->Name().GetLength()<=0) name = "Pause";
      LogF("  %x:%s %s",wave,name,(const char *)wave->GetDiagText());
    }
    LogF("  channels stat %d",STAT_GETCHANNELS());
    LogF("  playing %d, terminated %d",nPlay,nTerm);
    LogF("  stopped %d",nStop);
    LogF("  listener");

    LogF
    (
      "    pos (%.1f,%.1f,%.1f)\n"
      "    dir (%.1f,%.1f,%.1f)\n"
      "    vel (%.1f,%.1f,%.1f)",
      _listenerPos[0],_listenerPos[1],_listenerPos[2],
      _listenerDir[0],_listenerDir[1],_listenerDir[2],
      _listenerVel[0],_listenerVel[1],_listenerVel[2]
    );
  }
#endif

  // there may be some preview active
  if (_doPreview && GlobalTickCount()>=_previewTime)
  {
    float deltaT = 0.1;
    if (!_previewSpeech )
    {
      //float volume=GetSpeechVolCoef();
      // start first sound
      ParamEntryVal cfg = Pars >> "CfgSFX" >> "Preview" >> "speech"; 
      RStringB name = cfg[0];
      AbstractWave *wave = CreateWave(name,false,true);
      _previewSpeech = wave;
      if( wave )
      {
        wave->SetKind(WaveSpeech);
        wave->EnableAccomodation(false);
        wave->SetSticky(true);
        wave->SetVolume(cfg[1],cfg[2],true);
        wave->Repeat(1);
        wave->Play();
      }
      else
      {
        _doPreview = false; // error - no preview possible
      }
    }
    else if( PreviewAdvance(_previewSpeech,deltaT) && !_previewEffect )
    {
      // start second sound
      ParamEntryVal cfg = Pars >> "CfgSFX" >> "Preview" >> "effect"; 
      RStringB name = cfg[0];
      AbstractWave *wave = CreateWave(name,true,false);
      _previewEffect = wave;
      if( _previewEffect )
      {
        wave->EnableAccomodation(false);
        wave->SetSticky(true);
        wave->SetVolume(cfg[1],cfg[2],true);
        wave->SetPosition
        (
          _listenerPos+_listenerDir*50,_listenerVel,true
        );
        wave->Repeat(1);
        wave->Play();
      }
    }
    else if (PreviewAdvance(_previewEffect,deltaT) && !_previewMusic)
    {
      ParamEntryVal cfg = Pars >> "CfgSFX" >> "Preview" >> "music"; 
      RStringB name = cfg[0];
      AbstractWave *wave = CreateWave(name,false,true);
      _previewMusic=wave;
      if( wave )
      {
        wave->SetKind(WaveMusic);
        wave->EnableAccomodation(false);
        wave->SetSticky(true);
        // FIX
        // default volume for music is 0.5
        float vol = cfg[1];
        wave->SetVolume(vol*0.5,cfg[2],true);
        wave->Repeat(1);
        wave->Play();
      }
    }
    else if (PreviewAdvance(_previewMusic,deltaT))
    {
      _doPreview = false;
      _previewSpeech.Free();
      _previewEffect.Free();
      _previewMusic.Free();
    }
  }

  #if DEFERRED
    // it seems deferred is not working well with EAX extensions on Native devices
    static bool deferredHW = false;
    static bool deferredSW = true;
    if (_device && _context3D && (_hwEnabled ? deferredHW : deferredSW))
    {
      ScopeLockSection lock(GetLock());
      PROFILE_SCOPE_EX(snCDS,sound);
      alcProcessContext(_context3D);
      alcSuspendContext(_context3D);
    }
  #endif
}

float SoundSystemOAL::Priority() const
{
  return 0.4;
}
RString SoundSystemOAL::GetDebugName() const
{
  return "Sounds";
}
size_t SoundSystemOAL::MemoryControlled() const
{
  size_t total = 0;
  for (int i=0; i<_cache.Size(); i++)
  {
    const WaveCacheOAL &item = _cache[i];
    total += item._header._size;
  }
  return total;
}
size_t SoundSystemOAL::FreeOneItem()
{
  size_t released = 0;
  int size=_cache.Size();
  if (size==0) return released;
  WaveCacheOAL &item = _cache[size-1];
  released = item._header._size;

  _cache.Delete(size-1);
  return released;
}


static inline float expVol( float vol )
{
  if (vol<=0) return 1e-10; // muted -> -200 db
  // vol in range 0..10
  //return exp(vol*0.5-5);
  //return exp((vol*2-20)*0.5);
  // max. volume is 0
  return exp((vol-10.0f)*0.7f);
}

void SoundSystemOAL::PreviewMixer()
{
  if (!_enablePreview) return;
  _doPreview=true;
  _previewTime = GlobalTickCount()+200;
}

void SoundSystemOAL::StartPreview()
{
  if (_previewMusic)
  {
    TerminatePreview();
  }
  PreviewMixer();
}

void SoundSystemOAL::TerminatePreview()
{
  _previewEffect.Free();
  _previewMusic.Free();
  _previewSpeech.Free();
  _doPreview = false;
}

void SoundSystemOAL::FlushBank(QFBank *bank)
{
  // release any cached waves from this bank
  if (bank)
  {
    for (int i=0; i<_cache.Size(); i++)
    {
      if (!bank->Contains(_cache[i]._name)) continue;
      _cache.Delete(i);
      i--;
    }
  }
  _waves.RemoveNulls();
}

/*!
\patch 5164 Date 6/4/2007 by Ondra
- Fixed: System audio mixer settings no longer changed by ingame volume options.
*/
void SoundSystemOAL::UpdateMixer()
{
  #ifndef _XBOX
    float waveVolExp = expVol(_waveVolWanted);
    float speechVolExp = expVol(_speechVolWanted);
    float musicVolExp = expVol(_musicVolWanted);
    // when user sets volumes less than 5, we let him to get a quiet setup
    // however we want to avoid getting gains over 1, therefore we use the max of all of the in other situations
    float maxStdVol = expVol(5);
    float maxVolExp = floatMax(musicVolExp,waveVolExp,speechVolExp,maxStdVol);
    
    _volumeAdjustEffect = waveVolExp/maxVolExp;
    _volumeAdjustSpeech = speechVolExp/maxVolExp;
    _volumeAdjustMusic = musicVolExp/maxVolExp;
  #else
    float maxVol = floatMax(_musicVolWanted,_waveVolWanted,_speechVolWanted);
    float waveVol = _waveVolWanted+10-maxVol;
    float speechVol = _speechVolWanted+10-maxVol;
    float musicVol = _musicVolWanted+10-maxVol;
    static float defSpeechCoef = 0.36f;
    static float defMusicCoef = 0.36f;
    _volumeAdjustEffect = expVol(waveVol);
    _volumeAdjustSpeech = expVol(speechVol)*defSpeechCoef;
    _volumeAdjustMusic = expVol(musicVol)*defMusicCoef;
  #endif

  //LogF("Update _volumeAdjust %g",_volumeAdjust);

  for (int i=0; i<_waves.Size(); i++)
  {
    WaveOAL *wave = _waves[i];
    if (wave) wave->SetVolumeAdjust(this,_volumeAdjustEffect,_volumeAdjustSpeech,_volumeAdjustMusic);
  }

  //LogF("Update %g,%g,%g, maxVol %g",_waveVolExp,_speechVolExp,_musicVolExp,maxVol);
}

float SoundSystemOAL::GetSpeechVolume()
{
  return _speechVolWanted;
}
void SoundSystemOAL::SetSpeechVolume(float val)
{
  _speechVolWanted=val;
  UpdateMixer();
}
float SoundSystemOAL::GetWaveVolume()
{
  return _waveVolWanted;
}
void SoundSystemOAL::SetWaveVolume(float val)
{
  _waveVolWanted=val;
  UpdateMixer();
}

float SoundSystemOAL::GetCDVolume() const
{
  return _musicVolWanted;
}
void SoundSystemOAL::SetCDVolume(float val)
{
  _musicVolWanted=val;
  UpdateMixer();
}


void SoundSystemOAL::Activate(bool active)
{
  if (_active == active) return;
  _active = active;
  if (!active)
  {
    // when not active, we need to stop all buffer which are currently playing
    _waves.RemoveNulls();
    for (int i=0; i<_waves.Size(); i++)
    {
      WaveOAL *wave = _waves[i];
      if (!wave->_playing) continue;
      wave->Stop();
    }
  }
  else
  {
    // when active, there is nothing to do - all sounds call Play or regular basis
  }
}
