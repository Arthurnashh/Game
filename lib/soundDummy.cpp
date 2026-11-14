/**
    @file   soundDummy.cpp
    @brief  Dummy SoundSystem implementation for dedicated server

    Copyright &copy; 2002 by BIStudio (www.bistudio.com)
    @author PE
    @since  19.8.2002
    @date   27.9.2002
*/

#include "wpch.hpp"
#include "keyInput.hpp"
#include "global.hpp"
#include "soundDX8.hpp"
#include "paramFileExt.hpp"
#include "dikCodes.h"
#include <El/QStream/QBStream.hpp>
#include <El/Common/perfLog.hpp>

#define lenof(a) ( sizeof(a)/sizeof(*(a)) )

#define GetPos2Flags 0
#define LocHWFlags 0
#define LocSWFlags 0

void Wave8::DoConstruct()
{
    _type=Free;
    _soundSys=NULL;
    _size=0;              // Size of data.
    _frequency=0;         // sampling rate
    _nChannel=0;          // number of channels
    _sSize=0;             // number of bytes per sample
    _playing=false;       // Is this one playing?
    _loaded=false;
    _loadError=false;
    _only2DEnabled=false;
    _looping=true;
    _terminated=false;

    _volume=1.0;
    _accomodation=1.0;
    _volumeAdjust = 1;
    _enableAccomodation=true;

    _stopTreshold = FLT_MAX; // never stop

    _volumeSet=0;
    _frequencySet=0;
    _curPosition=0;
    _position=VZero;
    _velocity=VZero;
}

WaveGlobal8::~WaveGlobal8()
{
}

WaveGlobal8::WaveGlobal8( SoundSystem8 *sSys, float delay )
:Wave8(sSys,delay)
{
    _queued=false;
}

WaveGlobal8::WaveGlobal8( SoundSystem8 *sSys, RString name, bool is3D )
:Wave8(sSys,name,is3D)
{
    _queued=false;
}

DEFINE_FAST_ALLOCATOR(WaveGlobal8)

void WaveGlobal8::AdvanceQueue()
{
    if( !_queue ) return;
    if( _terminated )
    {
        if( _queue )
        {
            _queue->_queued=false;
            //_queue->Play();
            _queued=true;
            //LogF("Sound %s activated",(const char *)_queue->Name());
        }
        OnTerminateOnce();
        return;
    }
}

void WaveGlobal8::Play()
{
    if( _queued ) return;
    IsTerminated();
    AdvanceQueue();
    base::Play();
}

void WaveGlobal8::Skip( float deltaT )
{
    if( _queued ) return;
    IsTerminated();
    AdvanceQueue();
    base::Skip(deltaT);
}

void WaveGlobal8::Advance( float deltaT )
{
    if( _playing ) return;
    Skip(deltaT);
}

void WaveGlobal8::Queue( AbstractWave *wave,int repeat )
{
    // TODO: use DMusic primary segment queing
    WaveGlobal8 *after=static_cast<WaveGlobal8 *>(wave);
    //Assert( after->_repeat>0 );
    Assert( repeat>0 );
    while( after->_queue ) after=after->_queue;
    after->_queue=this;
    _queued=true;
    Repeat(repeat);
}

bool WaveGlobal8::IsTerminated()
{
    if( !base::IsTerminated() ) return false;
    for( WaveGlobal8 *next = this; next; next=next->_queue )
    {
        if( !next->_terminated ) return false;
    }
    OnTerminateOnce();
    return true;
}

#define DEFERRED 1 // TODO: use deferred mode

#define DSVerify(x) (x)
#define DSAssert(x) (x)

WaveBuffers8::WaveBuffers8()
:_position(VZero),_velocity(VZero)
{
    _3DEnabled = true;
}

WaveBuffers8::~WaveBuffers8()
{
}

void WaveBuffers8::Reset( SoundSystem8 *sys )
{
    // TODO: force setting all parameters
    _volumeSet = 0;

    _position = VZero;
    _velocity = VZero;

    _volume = 1; // volume used for 3D sound
    _accomodation = 1; // current ear accomodation setting
    _volumeAdjust = sys->_volumeAdjustEffect;

    _kind = WaveEffect;
    _3DEnabled = true;
}


void WaveBuffers8::SetVolumeDb( LONG volumeDb )
{
    if( !_playing )
    {
        _volumeSet=volumeDb;
        return;
    }
    if( volumeDb!=_volumeSet )
    {
        _volumeSet=volumeDb;
    }
}

void Wave8::Unload()
{
    if( !_loaded ) return;

    Assert( GSoundsys==_soundSys );
    Assert( _soundSys->_soundReady );

    _loaded=false;
    _stream.Free();

    _counters[_type]--;
    _type=Free;
    ReportStats();
}


void Wave8::DoDestruct()
{
    Stop();
    Unload();
}


int Wave8::_counters[NTypes];

void Wave8::AddStats( const char *name )
{
}

void Wave8::ReportStats()
{
}

int Wave8::TotalAllocated()
{
    int total=0;
    for( int i=1; i<NTypes; i++ ) total+=_counters[i];
    return total;
}

void Wave8::LoadHeader()
{
    if( _sSize>0 ) return;

    // only header is needed - whole files are cached
    Ref<WaveStream> stream = SoundLoadFile(Name());

    if( !stream )
    {
        LogF("Cannot load sound %s", (const char *)Name());
        return;
    }
    
    WAVEFORMATEX format;
    stream->GetFormat(format);
    _frequency = format.nSamplesPerSec;      // sampling rate
    _nChannel = format.nChannels;        // number of channels
    _sSize = format.nBlockAlign;           // number of bytes per sample
    _size = stream->GetUncompressedSize();
    #if DIAG_STREAM
        if (_size>StreamingBufferSize)
        {
            LogF("%s: Open stream, size %d)",(const char *)Name(),_size);
        }
    #endif
}

#define DISABLE_HW_ACCEL 0
#define ENABLE_VOICEMAN 1

void Wave8::Load()
{

    if( _loadError ) return;
    if( _loaded ) return;
    if (Name().GetLength()<=0)
    {
        _frequency = 1000; // time given in ms
    }

    // scan all channels
    // if possible make some channels free (release cache)
    _soundSys->ReserveCache(1-_only2DEnabled,_only2DEnabled);

    // search all playing waves
    _soundSys->_waves.Compact();
    // first try to reuse any cached data
    if( _soundSys->LoadFromCache(this) )
    {
        AddStats("Cache");

        _loaded=true;
        _terminated=false;
        _playing=false;
        _streamPos=0;
        _lastLoopSet = false;
        _lastLoopVerified = false;
        return;
    }

    // try to duplicate some existing sound buffer
    for( int i=0; i<_soundSys->_waves.Size(); i++ )
    {
        Wave8 *wave=_soundSys->_waves[i];
        if( !wave->Loaded() ) continue;
        if( wave==this ) continue;
        if( !strcmpi(wave->Name(),Name()) )
        {
            if( Duplicate(*wave) ) return;
        }
    }


    //LogF("Wave8 %s loaded",(const char *)Name());
    DoDestruct();

    _loadError=true;
    _loaded=true;
    _playing=false;
    
    Ref<WaveStream> stream = SoundLoadFile(Name());

    if( !stream )
    {
        RptF("Cannot load sound %s", (const char *)Name());
        _loadError = true;
        return;
    }
    
    WAVEFORMATEX format;
    stream->GetFormat(format);
    _frequency = format.nSamplesPerSec;      // sampling rate
    _nChannel = format.nChannels;        // number of channels
    _sSize = format.nBlockAlign;           // number of bytes per sample
    _size = stream->GetUncompressedSize();

    // stream if file is too large for static implementation
    bool streaming = _size>StreamingBufferSize;

    ADD_COUNTER(dsCSB,1);
    // note: at this point streaming / static implementation
    // is different
    // static decompresses whole buffer once, while streaming keeps
    // memory mapped file buffer and reads from it sequentially
    if (streaming)
    {
        // fill buffer with start of streaming data
        _stream = stream;
        _streamPos = 0;
        _lastLoopSet = false;
        _lastLoopVerified = false;
    }
    
    AddStats("Load");

    // silent sound
    _loadError=false;
}

inline int StreamBufferRoundDistance( int a, int b )
{
    int d = a-b;
    if (d<=0) d += StreamingBufferSize;
    return d;
}

void Wave8::FillStream()
{
}

#define STAT_PLAY(x)
#define STAT_TERM(x)
#define STAT_STOP(x)
#define STAT_GETCHANNELS() 0

void Wave8::Play()
{
}

void Wave8::Repeat(int repeat)
{
    _looping = (repeat >= 2);
}

void Wave8::Restart()
{
    Stop();
    _terminated=false;
    if (_playing)
    {
        LogF("%x: %s: Still playing",this,(const char *)Name());
    }
    //_playing=false;
    //STAT_STOP(this); // restarting - reset stats
    _curPosition=0;
    if (_stream)
    {
        _streamPos = 0;
    }
}

/*!
\patch 1.01 Date 15/6/2001 by Ondra.
 - Fixed: playMusic [name,t] with t>96
*/

void Wave8::Skip( float deltaT )
{
    // note: pre-start delay is used to implement speed of sound
    // this gives bad results if listener speed is high

    if (_playing)
    {
        StoreCurrentPosition();
    }

    // FIX
    int skip=toLargeInt(deltaT*_frequency)*_sSize;
    AdvanceCurrentPosition(skip);
    while( _curPosition>=(int)_size )
    {
        if (!_looping || _size<=0)
        {
            Stop();
            _terminated=true;
            _curPosition=_size;
            break;
        }
        _curPosition-=_size;
    }
    if( _playing && !_terminated )
    {
        if (_stream)
        {
            LogF("Cannot advance playing stream %s",(const char *)Name());
        }
    }
}

void Wave8::Advance( float deltaT )
{
    if( !_playing ) Skip(deltaT);
}

bool Wave8::IsStopped() {return !_playing;}
bool Wave8::IsWaiting() {return _curPosition<0;}

RString Wave8::GetDiagText()
{
    RString ret;
    if (_curPosition<0)
    {
        float delay = float(-_curPosition)/(_sSize*_frequency);
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
        return ret+RString("No buffer");
    }
}

float Wave8::GetLength() const
{
    // get wave length in sec
    if (_sSize<=0)
    {
        RptF("%s: sSize %d",(const char *)Name(),_sSize);
        return 1;
    }
    if (_frequency<=0)
    {
        RptF("%s: frequency %d",(const char *)Name(),_frequency);
        return 1;
    }
    return float(_size/_sSize)/_frequency;
}

bool Wave8::IsTerminated()
{
    if (!_terminated)
    {
        if (_playing)
        {
            STAT_TERM(this);
            _terminated=true;
        }
        else if (_loadError)
        {
            // sound that cannot be loaded is considered terminated
            _terminated = true;
        }
    }
    return _terminated;
}

void Wave8::FillStreamInit()
{
    // fill buffer with start of streaming data
    DWORD size=0;
    void *data;
    int initSize = StreamingBufferSize-2*StreamingPartSize;
    _streamPos += _stream->GetDataClipped(data,_streamPos,size);
    _writePos = size;
    if (_writePos>=StreamingBufferSize) _writePos -= StreamingBufferSize;
}

void Wave8::StoreCurrentPosition()
{
}

void Wave8::AdvanceCurrentPosition(int skip)
{
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
            _curPosition = newPos;
        }
        else
        {
            _curPosition = newPos;
        }
    }
}

void Wave8::DoStop()
{
    if( !_loaded )
    {
        if (_playing)
        {
            LogF("%x,%s: Playing, not loaded",this,(const char *)Name());
        }
        return;
    }
    if( !_playing ) return;
    _playing=false;
    STAT_STOP(this);
}

void Wave8::Stop()
{
    DoStop();
    Unload();
}

void Wave8::LastLoop()
{
    if( !_loaded ) return;
    if( !_looping ) return;
    _looping=false;
}

void Wave8::PlayUntilStopValue( float time )
{
    // play until some time
    _stopTreshold = time;
    _looping = true;
}

void Wave8::SetStopValue( float time )
{
    //LogF("%s: SetStopValue %.2f, %.2f",(const char *)Name(),time,_stopTreshold);
    if (time>_stopTreshold)
    {
        _terminated = true;
        Stop();
    }
}

// Volume is 0 to 1
// frequency is multiplied by original frequency

const int DBOffset=0;

#define DSBVOLUME_MIN     -10000
#define DSBVOLUME_MAX          0

static LONG float2db( float val )
{
    // 20 dB is 10x
    // val = 10^(dbi/20)
    // log10(val)=dbi/20
    // log10(val)*20=dbi;
    if( val<=0 ) return DSBVOLUME_MIN;
    int ret=toIntFloor(log10(val)*2000)+DBOffset;
    saturate(ret,DSBVOLUME_MIN,DSBVOLUME_MAX);
    //Log("float2db %f %d",val,ret);
    return ret;
}

void WaveBuffers8::DoSetPosition( bool immediate, bool maxDistanceFix )
{
}

bool Wave8::Get3D() const
{
    return WaveBuffers8::Get3D();
}

void Wave8::Set3D(bool is3D)
{
    if (is3D!=_3DEnabled)
    {
        //LogF("Change mode of %s to %d",(const char *)Name(),is3D);
        // stop buffer to avoid clicks during switching
        DoStop();
        WaveBuffers8::Set3D(is3D);
    }
}

void Wave8::SetPosition( Vector3Par pos, Vector3Par vel, bool immediate )
{
    _position=pos;
    _velocity=vel;
    // IDirectSound3DBuffer implementation
    _posValid = true;

    if( !_playing ) return;

    //LogF("SetPos %s",(const char *)Name());
    DoSetPosition(immediate,_soundSys->_hwEnabled);
}



bool WaveBuffers8::GetMuted() const
{
    return _volumeSet<=-10000;
}

void WaveBuffers8::UpdateVolume()
{
    if (_only2DEnabled || !_3DEnabled)
    {
        // some problem with setting parameters
        // convert _volume do Db
        float vol = _volume*_accomodation*_volumeAdjust;
        if (!_only2DEnabled) vol *= 100;
        SetVolumeDb(float2db(vol));
    }
    else
    {
        // instead of volume use 3D MinDistance factor
        SetVolumeDb(float2db(_volumeAdjust));
    }
}

void WaveBuffers8::SetAccomodation( float acc )
{
    if( !_enableAccomodation ) return;
    _accomodation=acc;

    UpdateVolume();
}

void WaveBuffers8::Set3D( bool is3D )
{
    // disable 3D processing
    _3DEnabled = is3D;
}

bool WaveBuffers8::Get3D() const
{
    return _3DEnabled;
}

void WaveBuffers8::SetKind( SoundSystem8 * sys, WaveKind kind )
{
    _kind = kind;
    float vol = sys->_volumeAdjustEffect;
    if (_kind==WaveMusic) vol = sys->_volumeAdjustMusic;
    else if (_kind==WaveSpeech) vol = sys->_volumeAdjustSpeech;
    _volumeAdjust = vol;
    UpdateVolume();
}

void WaveBuffers8::SetVolumeAdjust
(
    float volEffect, float volSpeech, float volMusic
)
{
    float vol = volEffect;
    if (_kind==WaveMusic) vol = volMusic;
    else if (_kind==WaveSpeech) vol = volSpeech;
    _volumeAdjust = vol;
    UpdateVolume();
}

void WaveBuffers8::SetVolume( float vol, bool imm )
{
    _volume=vol;;
    UpdateVolume();
}

void Wave8::SetFrequency( float freq, bool immediate )
{
    DWORD iFreq=toIntFloor(freq*_frequency);
    if( iFreq<_soundSys->_minFreq ) iFreq=_soundSys->_minFreq;
    else if( iFreq>_soundSys->_maxFreq ) iFreq=_soundSys->_maxFreq;
    if( iFreq!=_frequencySet )
    {
        _frequencySet=iFreq;
    }
}

void Wave8::SetVolume( float volume, float freq, bool immediate )
{
    // recalculate volume using master volume
    Wave8::SetFrequency(freq,immediate);
    WaveBuffers8::SetVolume(volume,immediate);
}

// Sound system

SoundSystem8::SoundSystem8()
:_soundReady(false)
{
  New((HWND)0,true);
}

SoundSystem8::SoundSystem8(HWND hwnd, bool dummy)
:_soundReady(false)
{
    New(hwnd,dummy);
}

SoundSystem8::~SoundSystem8()
{
    SaveConfig();
    Delete();
}

RString GetUserParams();

void SoundSystem8::LoadConfig()
{
    RString name = GetUserParams();

    ParamFile config;
    config.Parse(name);
    const float volFull=10;
    float cdVolume=volFull*0.5;
    float fxVolume=volFull*0.5;
    float speechVolume=volFull*0.5;
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

    _hwEnabled = false;
    _eaxEnabled = false;
    if (config.FindEntry("enableEAX"))
    {
        _eaxEnabled = config>>"enableEAX";
    }
    if (config.FindEntry("enableHW"))
    {
        _hwEnabled = config>>"enableHW";
    }

#if _FORCE_SINGLE_VOICE
    Glob.config.singleVoice = true;
#else
    Glob.config.singleVoice = false;
    ConstParamEntryPtr entry = config.FindEntry("singleVoice");
    if (entry) Glob.config.singleVoice = *entry;
#endif
}

void SoundSystem8::SaveConfig()
{
    if (!IsOutOfMemory())
    {
        RString name = GetUserParams();

        ParamFile config;
        config.Parse(name);
        config.Add("volumeCD",GetCDVolume());
        config.Add("volumeFX",GetWaveVolume());
        config.Add("volumeSpeech",GetSpeechVolume());
        config.Add("singleVoice", Glob.config.singleVoice);
        config.Add("enableEAX",_eaxEnabled);
        config.Add("enableHW",_hwEnabled);
        config.Save(name);
    }
}

bool SoundSystem8::EnableHWAccel(bool val)
{
    if (!_canHW) return false;
    if (_hwEnabled==val) return true;
    _hwEnabled = val;
    if (_hwEnabled)
    {
        _canEAX = true;
    }

    // we must restart all playing buffers
    _waves.Compact();
    for (int i=0; i<_waves.Size(); i++)
    {
        Wave8 *wave = _waves[i];
        if (!wave->_playing) continue;
        wave->Stop();
        wave->Play();
    }
    // clear cache - release all stored sounds
    _cache.Clear();
    _cacheUniqueBuffers = 0;

    return _hwEnabled;
}

bool SoundSystem8::EnableEAX(bool val)
{
    if (!_canEAX || !_hwEnabled) return false;
    if (_eaxEnabled==val) return true;
    _eaxEnabled = val;
    if (_eaxEnabled)
    {
        if (!InitEAX())
        {
            _eaxEnabled = false;
        }
    }
    else
    {
        DeinitEAX();
    }
    return _eaxEnabled;
}

bool SoundSystem8::GetEAX() const
{
    return _eaxEnabled;
}

bool SoundSystem8::GetHWAccel() const
{
    return _hwEnabled;
}

void SoundSystem8::New(HWND winHandle, bool dummy)
{
    LogF("Dedicated server => no sound");

    _soundReady=true;   
    _commited=false;
    _canHW = false;
    _canEAX = false;
    _maxChannels=31;
    _maxFreq=100000;
    _minFreq=100;
    _enablePreview = true;
    _doPreview = false;
    _hwEnabled = false;
    _eaxEnabled = false;
    SetSpeechVolume(5);
    SetWaveVolume(5);
    SetCDVolume(5);
    LoadConfig();
}

static const RString PauseName="";

Wave8::Wave8( SoundSystem8 *sSys, float delay ) // empty wave
:AbstractWave(PauseName)
{
    WaveBuffers8::Reset(sSys);
    DoConstruct();
    _soundSys=sSys;
    _only2DEnabled=true;

    // note: only wav files may be streaming
    _volumeSet=-10000;
    _frequencySet=0;
    _frequency = 1000;
    _sSize = 1;
    _curPosition=toLargeInt(delay*-1000); // delay time in ms
    _size = 0;
    _volumeAdjust = _soundSys->_volumeAdjustEffect;
    _looping = false;   
    _posValid = false;
}

Wave8::Wave8( SoundSystem8 *sSys, RString name, bool is3D )
:AbstractWave(name)
{
    WaveBuffers8::Reset(sSys);
    DoConstruct();
    _soundSys=sSys;
    _only2DEnabled=!is3D;

    _posValid=!is3D; // 3d position explicitly set

    // note: only wav files may be streaming
    _volumeSet=-10000;
    _frequencySet=0;
    _curPosition=0;
    _volumeAdjust = _soundSys->_volumeAdjustEffect;

    LoadHeader();
}

Wave8::~Wave8()
{
    DoDestruct();
}

void Wave8::CacheStore( WaveDSCache8 &store )
{
    store._name=Name();
}

WaveDSCache8::~WaveDSCache8()
{
}

void Wave8::CacheLoad( WaveDSCache8 &store )
{
}

bool Wave8::Duplicate( const Wave8 &src )
{
    if (src._stream) return false; // no duplicate possible on streaming buffers
    DoAssert( src.Loaded() );
    if( src._type!=Sw && !_soundSys->_hwDuplicateWorks ) return false;
    // try to duplicate hw buffer once
    if( src._type!=Sw )
    {
        _soundSys->_hwDuplicateWorks=false;
    }
    // note: DuplicateSoundBuffer fails on most HW accelerators
    Log("Cannot duplicate sound buffer");
    return false;
}

bool SoundSystem8::LoadFromCache( Wave8 *wave )
{
    for( int i=0; i<_cache.Size(); i++ )
    {
        WaveDSCache8 &cache=_cache[i];
        if( !strcmp(cache._name,wave->Name()) )
        {
            wave->CacheLoad(cache);
            _cache.Delete(i);
            return true;
        }
    }
    return false;
}

void SoundSystem8::LogChannels( const char *name )
{
}

void SoundSystem8::ReserveCache( int number3D, int number2D )
{
}

void SoundSystem8::StoreToCache( Wave8 *wave )
{
    if (wave->_stream)
    {
        LogF("Storing streaming buffer");
        return;
    }
    // note: it may make sense to have the same sound stored twice
    // if DuplicateBuffer does not work well, it may be much faster
    // this happend only when debugging in DX8 beta
    /**/
    for( int i=0; i<_cache.Size();  )
    {
        WaveDSCache8 &cache=_cache[i];
        if( !strcmp(cache._name,wave->Name()) )
        {
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
    }

    int index=0;
    _cache.Insert(index);
    WaveDSCache8 &cache=_cache[index];
    wave->CacheStore(cache);
}

float SoundSystem8::GetWaveDuration( const char *filename )
{
    // load file header

    // only header is needed - whole files are cached
    Ref<WaveStream> stream = SoundLoadFile(filename);
    if (!stream) return 1;

    WAVEFORMATEX format;
    stream->GetFormat(format);
    int frequency = format.nSamplesPerSec;      // sampling rate
    int sSize = format.nBlockAlign;           // number of bytes per sample
    int size = stream->GetUncompressedSize();

    return float(size/sSize)/frequency;

}

bool SoundSystem8::InitEAX()
{
    return false;
}

void SoundSystem8::DeinitEAX()
{
}

void SoundSystem8::DoSetEAXEnvironment()
{
}

void SoundSystem8::SetEnvironment( const SoundEnvironment &env )
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
}

void SoundSystem8::AddWaveToList(WaveGlobal8 *wave)
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

AbstractWave *SoundSystem8::CreateWave( const char *filename, bool is3D, bool prealloc )
{
    // TODO: use prealloc
    if( !_soundReady ) return NULL;
    // if the wave is preloaded, simply duplicate it
    char lowName[256];
    strcpy(lowName,filename);
    strlwr(lowName);
    if (!*lowName) return NULL; // empty filename

    WaveGlobal8 *wave=new WaveGlobal8(this,lowName,is3D);
    //LogF("Created %s: %x",lowName,wave);
    AddWaveToList(wave);
    return wave;
}

AbstractWave *SoundSystem8::CreateEmptyWave( float duration )
{
    // create a Wave8 with no buffers, only delay
    if( !_soundReady ) return NULL;
    WaveGlobal8 *wave=new WaveGlobal8(this,duration);
    //LogF("Created empty %x",wave);
    AddWaveToList(wave);
    return wave;
}

void SoundSystem8::Delete()
{
    if( !_soundReady ) return;

    _previewEffect.Free();
    _previewMusic.Free();
    _previewSpeech.Free();

    _cache.Clear(); 
    _cacheUniqueBuffers=0;
    _waves.Clear();

    _soundReady=false;
}

void SoundSystem8::SetListener
(
    Vector3Par pos, Vector3Par vel, Vector3Par dir, Vector3Par up
)
{
    _listenerPos=pos;
    _listenerVel=vel;
    _listenerDir=dir.Normalized();
    _listenerUp=up.Normalized();
    if (_listenerDir.SquareSize()<0.1) _listenerDir = VForward;
    if (_listenerUp.SquareSize()<0.1) _listenerUp = VUp;
    if (!_soundReady) return;
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

void SoundSystem8::Commit()
{
    if( !_soundReady ) return;

    _commited=TRUE;

    // there may be some preview active
    if (_doPreview && GlobalTickCount()>=_previewTime)
    {
        float deltaT = 0.1;
        if (!_previewSpeech )
        {
            //float volume=GetSpeechVolCoef();
            // start first sound
            ParamEntryPar cfg = Pars >> "CfgSFX" >> "Preview" >> "speech"; 
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
            ParamEntryPar cfg = Pars >> "CfgSFX" >> "Preview" >> "effect"; 
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
            ParamEntryPar cfg = Pars >> "CfgSFX" >> "Preview" >> "music"; 
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
}

inline float expVol( float vol )
{
    if (vol<=0) return 1e-10; // muted -> -200 db
    return exp((vol-10)*0.7);
}

void SoundSystem8::PreviewMixer()
{
    if (!_enablePreview) return;
    _doPreview=true;
    _previewTime = GlobalTickCount()+200;
}

void SoundSystem8::StartPreview()
{
    if (_previewMusic)
    {
        TerminatePreview();
    }
    PreviewMixer();
}

void SoundSystem8::TerminatePreview()
{
    _previewEffect.Free();
    _previewMusic.Free();
    _previewSpeech.Free();
    _doPreview = false;
}

void SoundSystem8::FlushBank(QFBank *bank)
{
    // release any cached waves from this bank
    for (int i=0; i<_cache.Size(); i++)
    {
        if (!bank->FileExists(_cache[i]._name)) continue;
        _cache.Delete(i);
        i--;
    }
    _waves.Compact();
}

float SoundSystem8::MaxVolume() const
{
    return floatMax(_musicVol,floatMax(_waveVol,_speechVol));
}

float SoundSystem8::MaxExpVolume() const
{
    return floatMax(_musicVolExp,floatMax(_waveVolExp,_speechVolExp));
}

void SoundSystem8::UpdateMixer()
{
    float maxVolExp = MaxExpVolume();
    _volumeAdjustEffect = _waveVolExp/maxVolExp;
    _volumeAdjustSpeech = _speechVolExp/maxVolExp;
    _volumeAdjustMusic = _musicVolExp/maxVolExp;

    for (int i=0; i<_waves.Size(); i++)
    {
        Wave8 *wave = _waves[i];
        if (wave) wave->SetVolumeAdjust
        (
            _volumeAdjustEffect,_volumeAdjustSpeech,_volumeAdjustMusic
        );
    }

}

float SoundSystem8::GetSpeechVolume()
{
    return _speechVol;
}

void SoundSystem8::SetSpeechVolume(float val)
{
    _speechVol=val;
    _speechVolExp=expVol(_speechVol);
    UpdateMixer();
}

float SoundSystem8::GetWaveVolume()
{
    return _waveVol;
}

void SoundSystem8::SetWaveVolume(float val)
{
    _waveVol=val;
    _waveVolExp=expVol(_waveVol);
    UpdateMixer();
}

float SoundSystem8::GetCDVolume() const
{
    return _musicVol;
}

void SoundSystem8::SetCDVolume(float val)
{
    _musicVol=val;
    _musicVolExp=expVol(_musicVol);
    UpdateMixer();
}

void SoundSystem8::Activate(bool){}
