/**
@file  
Voice over Net objects used together with DirectSound API.
*/

#include "../wpch.hpp"
#include "../global.hpp"
#include "voiceOAL.hpp"
#include "El/Speech/debugVoN.hpp"
#include "El/Speech/vonCodec.hpp"
#include <Es/Containers/staticArray.hpp>
#include <Es/Algorithms/qsort.hpp>

#if VOICE_OVER_NET && _ENABLE_VON && !_GAMES_FOR_WINDOWS

//----------------------------------------------------------------------
//  Replayer buffer:

#ifdef _XBOX
#undef DS_NOTIFICATION
#else
#undef DS_NOTIFICATION
#endif

/**
1/4 of replayer buffer (in seconds). Larger D increases latency but reduces VoN overhead.
Effective replayer latency = 2*D.
*/
const float REPLAYER_BUFFER_D = 0.125;

/**
1/4 of capture buffer (in seconds). Larger D increases latency but reduces VoN overhead.
Effective capture latency = D.
*/

const float CAPTURE_BUFFER_D = 0.125;

/// Direct-sound timer interval in milliseconds.
const long DS_TIMER            = 50L;


#if DEBUG_VON_DIAGS
extern DWORD replayerCurPosition;
extern DWORD VoNReplStatus;
#endif
static void replayerRoutine ( HANDLE object, void *data )
{
  VoNSoundBufferOAL *buf = (VoNSoundBufferOAL*)data;
  Assert( buf );
  // update what required the main thread through SetPosition, SetObstruction, UpdateVolumeAndAccomodation, ...
  buf->UpdateSoundPars();
  // stop/start control
  buf->doCtrlAction();
  // after all is updated, we can get the currentBuffers
  int write,free;
  buf->GetCurrentBuffers(free,write);

  if (free>0)
  {
    unsigned minDec = free>0 ? buf->BuffersToSamples(1) : 0;
    // we need to give some freedom to the buffer size
    // there is some granularity
    unsigned maxDec = minDec*2;
    // note: pos is actually never used
    unsigned pos = buf->BuffersToSamples(write);

#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
    NetLog("replayerRoutine: write=%u, free=%u",write,free);
#endif
#if DEBUG_VON_DIAGS
    replayerCurPosition = write;
#endif
    // for purpose of the VoNSoundBuffer interface we need to convert data to samples
    // ask replayer object to decode another bunch of data:
    buf->advanceSafePosition( buf->newData(pos,minDec,maxDec) );
  }
}

VOID CALLBACK ReplayerTimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
  VoNSoundBufferOAL *buf = (VoNSoundBufferOAL*)lpParam;
  Assert(buf);
  if (!buf->suspended)
    replayerRoutine(NULL, lpParam);
}

/*!
\patch 5240 Date 3/13/2008 by Bebul
- Fixed: More than 64 players can use VoN now.
*/

#if _USE_TIMER_QUEUE
VoNSoundBufferOAL::VoNSoundBufferOAL ( HANDLE qtimer, VoNReplayer *repl, ALCdevice *device )
#else
VoNSoundBufferOAL::VoNSoundBufferOAL ( EventToCallback *etc, VoNReplayer *repl, ALCdevice *device )
#endif
{
#ifdef NET_LOG_VOICE
  NetLog("VoNSoundBufferOAL::VoNSoundBufferOAL");
#endif
  input = false;
  suspended = true;
  size = 0;
  // default sound format:
  format.bitsPerSample = 8;
  format.frequency     = 8000;
  format.granularity   = 1;
  format.alignment     = 1;

#if _USE_TIMER_QUEUE
  Assert(qtimer);
  m_timerQueue = qtimer;
#else
  Assert( etc );
  m_etc = etc;
#endif
  m_ev = NULL;
  Assert( repl );
  m_repl = repl;
  Assert( device );
  m_device = device;
  m_mode3D = false;
  m_d = m_ahead = 0;
  // until buffers are created, assume zero size
  m_write = 0;
  m_free = 0;
}

VoNSoundBufferOAL::~VoNSoundBufferOAL ()
{
  destroyBuffer();
}

#define GSoundSysOAL ( static_cast<SoundSystemOAL *>(GSoundsys) )

void VoNSoundBufferOAL::registerCallbacks ()
{
  if ( m_buffers.Size()<=0 ) 
  {
    //(callback should work even when buffers are not created)
    // return; // nothing to do..
  }
#if _USE_TIMER_QUEUE
  Assert( m_timerQueue ); 
#else
  Assert( m_etc );
#endif

  if ( m_ev ) unregisterCallbacks();

#ifdef NET_LOG_VOICE_VERBOSE
  NetLog("Stop (registerCallbacks)");
#endif
#if _USE_TIMER_QUEUE
  BOOL err = CreateTimerQueueTimer(&m_ev, m_timerQueue, ReplayerTimerRoutine, this, DS_TIMER, DS_TIMER, WT_EXECUTEINTIMERTHREAD);
  Assert( err );
  (void) err;
#else
  // create & register the timer:
  m_ev = CreateWaitableTimer(NULL,FALSE,NULL);
  Assert( m_ev );
  m_etc->addEvent(m_ev,replayerRoutine,this);
  LARGE_INTEGER start = { (int64)0 };
  SetWaitableTimer(m_ev,&start,DS_TIMER,NULL,NULL,FALSE);
#endif

  if ( !suspended )
  {
    // no need to start playing - will be done once queued
    //alSourcePlay(m_source);
    //Verify(alGetError()==AL_NO_ERROR);
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
    NetLog("Play (registerCallbacks)");
#endif
  }
  else
  {
    ScopeLockSection lock(GSoundSysOAL->GetLock());
    
    alGetError(); // clear Error status
    if (m_sourceOpen) alSourceStop(m_source);
    if (m_source3DOpen) alSourceStop(m_source3D);
    Verify(alGetError()==AL_NO_ERROR);
    UnqueueStream(m_buffers.Size()-m_free);
  }
}

void VoNSoundBufferOAL::unregisterCallbacks ()
{
#if _USE_TIMER_QUEUE
  Assert( m_timerQueue );
#else
  Assert( m_etc );
#endif

  if (m_sourceOpen) 
  {
    ScopeLockSection lock(GSoundSysOAL->GetLock());
    alGetError(); // clear Error status
    alSourceStop(m_source);
    ALCenum err = alGetError();
    if (err!=AL_NO_ERROR) 
      RptF("VoN: Error %x while stopping source %x",err, m_source);
  }
  if (m_source3DOpen) 
  {
    ScopeLockSection lock(GSoundSysOAL->GetLock());
    alGetError(); // clear Error status
    alSourceStop(m_source3D);
    ALCenum err = alGetError();
    if (err!=AL_NO_ERROR) 
      RptF("VoN: Error %x while stopping 3D source %x",err, m_source3D);
  }
  UnqueueStream(m_buffers.Size()-m_free);

  if ( m_ev )
  {
#if _USE_TIMER_QUEUE
    DeleteTimerQueueTimer(m_timerQueue, m_ev, INVALID_HANDLE_VALUE); //we want to wait to routine completion
#else
    CancelWaitableTimer(m_ev);
    m_etc->removeEvent(m_ev);
    CloseHandle(m_ev);
#endif
    m_ev = NULL;
  }

#ifdef NET_LOG_VOICE_VERBOSE
  NetLog("Stop (unregisterCallbacks)");
#endif
}

void VoNSoundBufferOAL::suspendCallbacks ()
{
#if !_USE_TIMER_QUEUE
  Assert( m_etc );
  //Assert( m_buffer );
  if ( m_ev ) m_etc->enableEvent(m_ev,false);
#endif
}

void VoNSoundBufferOAL::resumeCallbacks ()
{
#if !_USE_TIMER_QUEUE
  Assert( m_etc );
  //Assert( m_buffer );
  if ( m_ev ) m_etc->enableEvent(m_ev,true);
#endif
}

// TODO: add velocity
void VoNSoundBufferOAL::SetPosition ( float x, float y, float z )
{
  ScopeLockSection lock(GSoundSysOAL->GetLock());
  alGetError(); // clear Error status
  if ( !m_mode3D )
  {
    if (m_sourceOpen) alSourcei(m_source,AL_SOURCE_RELATIVE,false);
    Verify(alGetError()==AL_NO_ERROR);
    m_mode3D = true;
  }
  if (m_sourceOpen) alSource3f(m_source,AL_POSITION,x,y,z);
  Verify(alGetError()==AL_NO_ERROR);
}

void VoNSoundBufferOAL::UpdateSoundPars()
{
  // Set3DPosition
  DWORD time = GlobalTickCount();
  if (m_voiceState.set3DPos)
  {
    if (m_source3DOpen) 
    {
      ScopeLockSection lock(GSoundSysOAL->GetLock());
      alGetError(); // clear Error status
      // note: we invert x to convert between left handed and right handed system
      const Vector3 &pos = m_voiceState.position;
      const Vector3 &speed = m_voiceState.speed;
      alSource3f(m_source3D,AL_POSITION,-pos.X(),pos.Y(),pos.Z());
      alSource3f(m_source3D,AL_VELOCITY,-speed.X(),speed.Y(),speed.Z());
      Verify(alGetError()==AL_NO_ERROR);
      m_voiceState.set3DPos = false;
    }
    // parsTime means the 3D position is set (at least in the m_voiceState) so the direct speak can start working
    m_voiceState.parsTime = time;
  }
  // UpdateVolumeAndAccomodation
  // SetObstruction
  UpdateVolume();
}

void VoNSoundBufferOAL::Set3DPosition ( Vector3Par pos, Vector3Par speed )
{ // Called from main thread!
  m_voiceState.position = pos;
  m_voiceState.speed = speed;
  m_voiceState.set3DPos = true;
}

void VoNSoundBufferOAL::UpdateVolumeAndAccomodation(float volume, float accomodation)
{ // Called from main thread!
  m_voiceState.volume = volume;
  m_voiceState.accomodation = accomodation;
}

VoNChatChannel VoNSoundBufferOAL::CheckChatChannel(bool &audible2D, bool &audible3D) const
{ // Called from main thread!
  if (!m_repl)
  {
    audible2D = audible3D = false;
    return 0;
  }
  audible2D = m_repl->GetPlay2D();
  audible3D = m_repl->GetPlay3D();
  return m_repl->GetChatChannel();
}

void VoNSoundBufferOAL::SetObstruction(float obstruction, float occlusion, float deltaT)
{ // Called from main thread!
  // change obstruction gradually
  const float minEps = 0.01f;
  if (
    fabs(occlusion-m_voiceState.occlusion)<minEps*floatMin(m_voiceState.occlusion,occlusion) &&
    fabs(obstruction-m_voiceState.obstruction)<minEps*floatMin(m_voiceState.obstruction,obstruction)
    )
  {
    return;
  }

  if (deltaT<=0)
  {
    m_voiceState.occlusion = occlusion;
    m_voiceState.obstruction = obstruction;
  }
  else
  {
    float delta;
    delta = occlusion - m_voiceState.occlusion;
    const float maxSpeed = 1.5f;
    Limit(delta,-deltaT*maxSpeed,+deltaT*maxSpeed);
    m_voiceState.occlusion += delta;

    delta = obstruction - m_voiceState.obstruction;
    Limit(delta,-deltaT*maxSpeed,+deltaT*maxSpeed);
    m_voiceState.obstruction += delta;
  }
}

static inline float DistanceRolloff(float minDistance, float distance)
{
  return minDistance/distance;
}

void VoNSoundBufferOAL::UpdateVolume()
{
  // for 3D voice (these are part of the environment) simulate obstructions
  float volumeAdjust = (static_cast<SoundSystemOAL *>(GSoundsys))->_volumeAdjustEffect;
  const bool is3DEnabled = true; //this method is called only for 3D sounds
  if (is3DEnabled || m_voiceState.accomodation)
  {
    // instead of volume use 3D MinDistance factor
    // we can use EAX/I3DL2 obstruction/occlusion, or we can lower total volume
    if ((static_cast<SoundSystemOAL *>(GSoundsys))->_eaxEnabled)
    {
      int obstructionDb = Float2MiliBel(m_voiceState.obstruction);
      int occlusionDb = Float2MiliBel(m_voiceState.occlusion);
      SetI3DL2Db(obstructionDb,occlusionDb);
    }
    else
    {
      // no HW obstruction/occlusion
      // we use total volume to simulate it
      float obsVolume = 1-(1-m_voiceState.obstruction)*0.7f;
      // assume occlusion reduces the sound much more than obstruction
      float occVolume = 1-(1-m_voiceState.occlusion)*0.9f;
      float volumeFactor = floatMin(obsVolume,occVolume);
      volumeAdjust *= volumeFactor;
    }
  }

  if (!is3DEnabled)
  {
    // some problem with setting parameters
    // convert _volume do Db
    float vol = m_voiceState.volume*m_voiceState.accomodation*volumeAdjust;
    static bool Only2DEnabled = false;
    if (!Only2DEnabled)
    {
      // simulate DirectSound attenuation for _distance2D
      // calculate volume based on distance2D
      float minDistance = CalcMinDistance();
      static float Distance2D = 1.0f;
      float atten = Distance2D<minDistance ? 1 : DistanceRolloff(minDistance,Distance2D);
      vol = atten*volumeAdjust;
    }
    SetGain(vol);
  }
  else
  {
    SetGain(volumeAdjust);
  }
}

float VoNSoundBufferOAL::CalcMinDistance() const
{
  static float MinDistCoef = 1.0f; //30.0f; 
  return floatMax(sqrt(m_voiceState.volume)*m_voiceState.accomodation*MinDistCoef,1e-6);
}

void VoNSoundBufferOAL::SetGain(float gain)
{
  // input is multiplicative gain
  m_voiceState.current3DGain = gain;
  UpdateGain();
}

void VoNSoundBufferOAL::DoSetI3DL2Db()
{
  // we cannot set this if there is no EAX
  SoundSystemOAL *soundSys = static_cast<SoundSystemOAL *>(GSoundsys);

  ScopeLockSection lock(soundSys->GetLock());
  Assert(soundSys->_eaxEnabled);

  int occ = m_voiceState.occlusionSet;
  int obs = m_voiceState.obstructionSet;
  int exc = 0;
  int directHF = 0;
  int direct = 0;
  float occRatio = 0.25f;
  /*
  static bool radioOnly = true;
  // if sounds are accommodated, it means they are real sounds and may be obstructed
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
  */
  saturate(occ,EAXSOURCE_MINOCCLUSION,EAXSOURCE_MAXOCCLUSION);
  saturate(obs,EAXSOURCE_MINOBSTRUCTION,EAXSOURCE_MAXOBSTRUCTION);
  saturate(exc,EAXSOURCE_MINEXCLUSION,EAXSOURCE_MAXEXCLUSION);
  saturate(direct,EAXSOURCE_MINDIRECT,EAXSOURCE_MAXDIRECT);
  saturate(directHF,EAXSOURCE_MINDIRECTHF,EAXSOURCE_MAXDIRECTHF);
  static bool Is3DEnabled = true;
  if (!Is3DEnabled && !m_voiceState.accomodation)
  {
    // disable reverb for the 2D sounds, done because of radio - see also RADIO
    exc = -10000;
    // occlusion and obstruction should be both zero
    //Assert(occ==0);
    //Assert(obs==0);
  }
  if (m_source3DOpen)
  {
    ScopeLockSection lock(soundSys->GetLock());
    soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_EXCLUSION,m_source3D,&exc,sizeof(exc));
    soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_OCCLUSION,m_source3D,&occ,sizeof(occ));
    soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_OCCLUSIONLFRATIO,m_source3D,&occRatio,sizeof(occRatio));
    soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_OBSTRUCTION,m_source3D,&obs,sizeof(obs));
    soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_DIRECTHF,m_source3D,&directHF,sizeof(directHF));
    soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_DIRECT,m_source3D,&direct,sizeof(direct));
  }
  // note: we might set exclusion as well, esp. for distant sounds
  // we might also set early reflection direction
}

void VoNSoundBufferOAL::SetI3DL2Db(int obstruction, int occlusion)
{
  /*
  ALint state;
  alGetSourcei(m_source,AL_SOURCE_STATE,&state);
  if (state!=AL_PLAYING)
  {
  m_voiceState.obstructionSet = obstruction;
  m_voiceState.occlusionSet = occlusion;
  return;
  }
  */
  m_voiceState.obstructionSet = obstruction;
  m_voiceState.occlusionSet = occlusion;
  if (static_cast<SoundSystemOAL *>(GSoundsys)->_eaxEnabled)
  {
    DoSetI3DL2Db();
  }
}

/*!
\patch 5128 Date 2/14/2007 by Bebul
- Fixed: When OpenAL is unable to create sound buffer for VoN, the game continues without VoN.
\patch 5153 Date 4/18/2007 by Bebul
- Fixed: VoN sources management to prevent some sounds not audible in multiplayer
\patch 5161 Date 5/31/2007 by Bebul
- Fixed: Bugs in VoN OpenAL source management.
\patch 5162 Date 6/1/2007 by Bebul
- Fixed: No VoN voice played sometimes even when player mouth moving.
*/
void VoNSoundBufferOAL::createBuffer ( unsigned len )
{
  Assert(m_ev==NULL);

#ifdef NET_LOG_VOICE
  NetLog("VoNSoundBufferOAL::createBuffer(): len=%u, freq=%d, bps=%d",
    len,(int)format.frequency,(int)format.bitsPerSample);
#endif
  size = m_d = m_ahead = 0;
  suspended = true;

  // create buffers and source
  // TODO: better buffer granularity calculations?

  // use
  m_bufferSize = ((len>>2)+format.granularity-1)/format.granularity*format.granularity;

  int bufferCount = (len+m_bufferSize-1)/m_bufferSize;
  len = bufferCount*m_bufferSize;

#if VON_USE_CREATE_BUFFER
  {
    ScopeLockSection lock(soundSys->GetLock());
    alGetError(); // clear Error status
    alGenSources(1,&m_source);
    ALenum err = alGetError();
    if (err==AL_NO_ERROR) m_sourceOpen = true;
    else 
    {
      RptF("createBuffer: cannot create source, error %x", err);
      m_source = 0; m_sourceOpen = false;
    }
    alGetError(); // clear Error status
    alGenSources(1,&m_source3D);
    err = alGetError();
    if (err==AL_NO_ERROR) m_source3DOpen = true;
    else
    {
      RptF("createBuffer: cannot create 3D source, error %x", err);
      m_source3D = 0; m_source3DOpen = false;
    }
  }
#else
  m_source = m_source3D = 0;
  m_sourceOpen = m_source3DOpen = false;
#endif
  { // sound buffers can be created always (even when source creation failed)
#if VON_USE_CREATE_BUFFER
    m_buffers.Realloc(bufferCount);
    m_buffers.Resize(bufferCount);
#endif
    m_buffers2D3D.Realloc(bufferCount);
    m_buffers2D3D.Resize(bufferCount);
    m_buffersLen.Realloc(bufferCount);
    m_buffersLen.Resize(bufferCount);
#if VON_USE_CREATE_BUFFER
Retry:
    ScopeLockSection lock(soundSys->GetLock());
    alGenBuffers(bufferCount,m_buffers.Data());
    ALenum err = alGetError();
    if (err==AL_OUT_OF_MEMORY)
    {
      if (FreeOnDemandSystemMemoryLowLevel(len+4*1024)!=0)
      {
        goto Retry;
      }
    }
    else if (err==AL_NO_ERROR)
    {
      size = len;
      m_free = bufferCount;
      m_write = 0;
      // no need to provide any data - buffer will not start until we queue something
      if ( size )
      {
        m_d = (len + 2) >> 2;                 // D = 1/4 of buffer length (in samples)
        m_ahead = (8 * m_d) >> 2;             // 8/4 of D
        m_mode3D = false;
        if (m_sourceOpen) alSourcei(m_source,AL_SOURCE_RELATIVE,true);
        if (m_source3DOpen) alSourcei(m_source3D,AL_SOURCE_RELATIVE,false);
        Verify(alGetError()==AL_NO_ERROR);
      }
    }
    else
    {
      if (m_sourceOpen) { alDeleteSources(1,&m_source); m_source = 0; m_sourceOpen = false; }
      if (m_source3DOpen) { alDeleteSources(1,&m_source3D); m_source3D = 0; m_source3DOpen = false; }
      m_buffers.Clear();
    }
#else
    size = len;
    m_free = bufferCount;
    m_write = 0;
    // no need to provide any data - buffer will not start until we queue something
    if ( size )
    {
      m_d = (len + 2) >> 2;                 // D = 1/4 of buffer length (in samples)
      m_ahead = (8 * m_d) >> 2;             // 8/4 of D
      m_mode3D = false;
    }
#endif
  }
#if VON_USE_CREATE_BUFFER
  if (m_buffers.Size()<=0)
  {
    // we should keep time running even in this case
    m_repl->ForceVoNMessagesDiscarding();
    RptF("CreateBuffer: VoN sound buffer was not created!");
  }
  else
  {
    InitEAX();
  }
#else
  DWORD time = GlobalTickCount();
  m_activeBufferStartTime = time; //initialize in order to make time running during muted
#endif
}

void VoNSoundBufferOAL::GetCurrentBuffers(int &free, int &write)
{
  if (m_buffers.Size()<=0)
  {
    //we should make time running (throw muted buffers continuously)
    if (!m_buffers2D3D.Size()) return;
    DWORD time = GlobalTickCount();
    int bufTimeElapsed = time-m_activeBufferStartTime;
    int busyIx = (m_write + m_free) % m_buffers2D3D.Size();
    while ( bufTimeElapsed >= m_buffersLen[busyIx] && m_free<m_buffers2D3D.Size() )
    {
      m_free++; //"unqueued"
      bufTimeElapsed -= m_buffersLen[busyIx];
      busyIx = (busyIx+1) % m_buffers2D3D.Size();
    }
    m_activeBufferStartTime = time-bufTimeElapsed; //next buffer will start now
    write=m_write; free = m_free;
    return;
  }
  ScopeLockSection lock(GSoundSysOAL->GetLock());
  // 2D source buffering
  ALint queued = 0, processed = m_buffers.Size();
  if (m_sourceOpen)
  {
    alGetSourcei(m_source,AL_BUFFERS_PROCESSED,&processed);
    alGetSourcei(m_source,AL_BUFFERS_QUEUED,&queued);
    if (!queued && !processed) processed = m_buffers.Size()-m_free;
  }
  // 3D source buffering
  ALint queued3D = 0, processed3D = m_buffers.Size();
  if (m_source3DOpen) 
  {
    alGetSourcei(m_source3D,AL_BUFFERS_PROCESSED,&processed3D);
    alGetSourcei(m_source3D,AL_BUFFERS_QUEUED,&queued3D);
    if (!queued3D && !processed3D) processed3D = m_buffers.Size()-m_free;
  }
  UnqueueStream(std::min(processed,processed3D));
  if (alGetError()!=AL_NO_ERROR) return;
  UpdateGain();
  write=m_write; free = m_free;
}

void VoNSoundBufferOAL::suspendBuffer ()
{
  if ( m_buffers.Size()<=0 ) 
  {
    suspended = true;
    suspendCallbacks();
    return; // nothing else to do..
  }

  ScopeLockSection lock(GSoundSysOAL->GetLock());
  // TODO: OAL - check how much buffers was already processed and unqueue them
  alGetError(); // clear Error status
  if (m_sourceOpen) alSourceStop(m_source);
  if (m_source3DOpen) alSourceStop(m_source3D);
  Verify(alGetError()==AL_NO_ERROR);
  UnqueueStream(m_buffers.Size()-m_free);

#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
  NetLog("Stop (suspendBuffer)");
#endif
  suspended = true;
  suspendCallbacks();
  // once we stop providing data, the OpenAL playback will stop once queue buffers are done
}

void VoNSoundBufferOAL::resumeBuffer ()
{
  if ( m_buffers.Size()<=0 ) 
  { 
#if _USE_TIMER_QUEUE
    suspended=false; 
#endif
    return; 
  }                 // nothing to do..
  resumeCallbacks();
  suspended = false;

  // no need to start playing - will be done once queued
  /*
  // if there is still something to play, play it
  if (m_free<m_buffers.Size())
  {
  alGetError(); // clear Error status
  alSourcePlay(m_source);
  Verify(alGetError()==AL_NO_ERROR);
  }
  */

#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
  NetLog("Play (resumeBuffer)");
#endif
}

void VoNSoundBufferOAL::destroyBuffer ()
{
#if defined(NET_LOG_VOICE) || DEBUG_VON
  NetLog("VoNSoundBufferOAL::destroyBuffer");
#endif
  suspended = true;
  unregisterCallbacks();
  size = m_d = m_ahead = 0;
  if ( m_buffers.Size()<=0 ) return; // nothing else to do..
  ScopeLockSection lock(GSoundSysOAL->GetLock());
  alDeleteBuffers(m_buffers.Size(),m_buffers.Data());
  m_buffers.Clear();
  if (m_sourceOpen) { alDeleteSources(1,&m_source); m_source = 0; m_sourceOpen = false;}
  if (m_source3DOpen) { alDeleteSources(1,&m_source3D); m_source3D = 0; m_source3DOpen = false;}
}

unsigned VoNSoundBufferOAL::newData ( unsigned &pos, unsigned minSamples, unsigned maxSamples )
{
  Assert( m_repl );
  return m_repl->decode(this,pos,minSamples,maxSamples);
}

void VoNSoundBufferOAL::advanceSafePosition ( unsigned inc )
{
  // we do not really care - buffers are updated when queued
}

void VoNSoundBufferOAL::lockData (unsigned pos, unsigned len, CircularBufferPointers &ptr)
{

  Zero(ptr);
  // we need to process data even when muted
  // if (m_buffers.Size()<=0) return;

  // lock max. possible
  // is less will be used, we will queue less
  ptr.len1 = len;

  // convert to bytes
  if ( format.bitsPerSample > 8 )
  {
    pos *= 2;
    len *= 2;
  }
  // position is ignored
  // we use CircularBufferPointers to allocate a temporary memory
  // TODO: use one memory buffer and recycle it if possible
  void *mem = safeNew(len);
  // note: because of union used in CircularBufferPointers following two branches generate the same code
  if (format.bitsPerSample > 8)
  {
    ptr.ptr1.bps16 = (int16 *)mem;
  }
  else
  {
    ptr.ptr1.bps8 = (int8 *)mem;
  }
}

/*!
\patch 5123 Date 1/24/2007 by Ondra
- Fixed: Bad voice buffer unqueuing, could cause memory leak when using voice over net.
*/

void VoNSoundBufferOAL::UnqueueStream(int processed)
{
  if (processed<=0) return;
  // which one are those processed?
  Assert(processed<=m_buffers.Size()-m_free);
  int firstBusy = m_write+m_free;
  if (firstBusy>=m_buffers.Size()) firstBusy -= m_buffers.Size();

  ScopeLockSection lock(GSoundSysOAL->GetLock());
  
  for (int i=0; i<processed; i++)
  {
    int ix = (firstBusy+i)%m_buffers.Size();
    ALuint buf = m_buffers[ix];
    if ((m_buffers2D3D[ix]&TTBuf2DQueued) && m_sourceOpen) 
      alSourceUnqueueBuffers(m_source,1,&buf);
    if ((m_buffers2D3D[ix]&TTBuf3DQueued) && m_source3DOpen) 
      alSourceUnqueueBuffers(m_source3D,1,&buf);
  }

  m_free += processed;
}

/*!
\patch 5160 Date 5/18/2007 by Bebul
- New: VoN 2D voice volume is now controlled by radio volume.
*/

/// Assume there are no processed buffers
/// i.e. VoNSoundBufferOAL::UpdateGain must be called immediatelly after alUnQueueBuffers
void VoNSoundBufferOAL::UpdateGain()
{
  if (m_buffers2D3D.Size()<=0) return;
  ScopeLockSection lock(GSoundSysOAL->GetLock());
  alGetError();
  ALint state=0, state3D=0;
  if (m_sourceOpen) alGetSourcei(m_source,AL_SOURCE_STATE,&state);
  if (m_source3DOpen) alGetSourcei(m_source3D,AL_SOURCE_STATE,&state3D);
  if (state==AL_PLAYING || state3D==AL_PLAYING)
  {
    int currentBufId = (m_write + m_free)%m_buffers2D3D.Size();
    if (state==AL_PLAYING)
    {
      // for 2D voice use radio volume
      float radioVolume = (static_cast<SoundSystemOAL *>(GSoundsys))->_volumeAdjustSpeech;
      float gain = (m_buffers2D3D[currentBufId]&TTChan2D) ? radioVolume : 0.001f;
      UPDATE_OAL_SOURCE_F(m_source, AL_GAIN, gain, m_voiceState.gain2D)
    }
    if (state3D==AL_PLAYING)
    {
      float gain = (m_buffers2D3D[currentBufId]&TTChan3D) ? m_voiceState.current3DGain : 0.001f;
      if (!m_voiceState.parsTime) gain = 0.001f; //no 3D sound until position and other params are set (from Man::UpdateSpeechPosition)
      UPDATE_OAL_SOURCE_F(m_source3D, AL_GAIN, gain, m_voiceState.gain3D)
        float refDistance = (( m_buffers2D3D[currentBufId] & TTDirectChan ) ? 10.0f : 1.0f ) * CalcMinDistance();
      UPDATE_OAL_SOURCE_F(m_source3D, AL_REFERENCE_DISTANCE, refDistance, m_voiceState.referenceDistance)
    }
  }
}

void VoNSoundBufferOAL::unlockData ( unsigned processed, CircularBufferPointers &ptr )
{
  if (m_buffers.Size()<=0) 
  {
    // we should make time flowing even when VoN voice is muted
    int freq = format.frequency ? format.frequency : 1;
    m_buffersLen[m_write] = (1000 * processed) / freq;
    m_write++; m_free--; //simulate as if new VoN data were queued
    if (m_write>=m_buffers2D3D.Size()) m_write = 0;
    if ( format.bitsPerSample > 8 )
    {
      safeDelete(ptr.ptr1.bps16);
      ptr.ptr1.bps16 = 0;
    }
    else
    {
      safeDelete(ptr.ptr1.bps8);
      ptr.ptr1.bps8 = 0;
    }
    ptr.len1 = 0;
    return;
  }

  int freq = format.frequency ? format.frequency : 1;
  m_buffersLen[m_write] = (1000 * processed) / freq;
  // un-queue buffers which are already done
  ScopeLockSection lock(GSoundSysOAL->GetLock());
  alGetError();
  // when unlocking, queue the data
  ALuint buf = m_buffers[m_write];
  ALenum alFormat = format.bitsPerSample>8 ? AL_FORMAT_MONO16 : AL_FORMAT_MONO8;
  ALvoid *data = format.bitsPerSample > 8 ? (ALvoid *)ptr.ptr1.bps16 : (ALvoid *)ptr.ptr1.bps8;
  Assert(processed<=ptr.len1);
  if (format.bitsPerSample > 8) processed *= 2;

  alBufferData(buf,alFormat,data,processed,format.frequency);

  ALint state=0, state3D=0;
  if (m_sourceOpen) alGetSourcei(m_source,AL_SOURCE_STATE,&state);
  if (m_source3DOpen) alGetSourcei(m_source3D,AL_SOURCE_STATE,&state3D);
  if (m_buffers2D3D[m_write] & TTChan2D)
  {
    if (m_sourceOpen)
    {
      if (state!=AL_PLAYING && state3D==AL_PLAYING)
      { //we should generate silence in 2D buffer to be synchronized with 3D buffer
        int write,free;
        GetCurrentBuffers(write, free); //only to update state
        int alByteOffset;
        alGetSourcei(m_source3D, AL_BYTE_OFFSET, &alByteOffset);
        int bufSize = m_buffers.Size();
        int firstBusy = m_write + m_free;
        for (int i=0, num = bufSize-m_free; i<num; i++)
        {
          int ix = (firstBusy+i)%bufSize;
          alSourceQueueBuffers(m_source, 1, &m_buffers[ix]);
          m_buffers2D3D[ix] |= TTBuf2DQueued;
        }
        alSourcei(m_source, AL_BYTE_OFFSET, alByteOffset); //synchronize sources
        alSourcef(m_source, AL_GAIN, 0.001f);
      }
      alSourceQueueBuffers(m_source,1,&buf);
      m_buffers2D3D[m_write] |= TTBuf2DQueued;
    }
  }
  if (m_buffers2D3D[m_write] & TTChan3D)
  {
    if (m_source3DOpen)
    {
      if (state3D!=AL_PLAYING && state==AL_PLAYING)
      { //we should generate silence in 3D buffer to be synchronized with 2D buffer
        int write,free;
        GetCurrentBuffers(write, free); //only to update state
        int alByteOffset;
        alGetSourcei(m_source3D, AL_BYTE_OFFSET, &alByteOffset);
        int bufSize = m_buffers.Size();
        int firstBusy = m_write + m_free;
        for (int i=0, num = bufSize-m_free; i<num; i++)
        {
          int ix = (firstBusy+i)%bufSize;
          alSourceQueueBuffers(m_source3D, 1, &m_buffers[(firstBusy+i)%bufSize]);
          m_buffers2D3D[ix] |= TTBuf3DQueued;
        }
        alSourcei(m_source3D, AL_BYTE_OFFSET, alByteOffset); //synchronize sources
        alSourcef(m_source3D, AL_GAIN, 0.001f);
      }
      alSourceQueueBuffers(m_source3D,1,&buf);
      m_buffers2D3D[m_write] |= TTBuf3DQueued;
    }
  }

  Assert(m_free>0);
  int oldWrite = m_write++;
  m_free--;
  if (m_write>=m_buffers.Size()) m_write = 0;

  // make sure the buffers start playing

  if (m_sourceOpen && state!=AL_PLAYING && (m_buffers2D3D[oldWrite] & TTChan2D))
  {
    alSourcePlay(m_source);
  }
  if (m_source3DOpen && state3D!=AL_PLAYING && (m_buffers2D3D[oldWrite] & TTChan3D))
  {
    alSourcePlay(m_source3D);
  }

  if ( format.bitsPerSample > 8 )
  {
    safeDelete(ptr.ptr1.bps16);
    ptr.ptr1.bps16 = 0;
  }
  else
  {
    safeDelete(ptr.ptr1.bps8);
    ptr.ptr1.bps8 = 0;
  }
  ptr.len1 = 0;
  ALenum alError = alGetError();
  Verify(alError == AL_NO_ERROR);
}

void VoNSoundBufferOAL::update2D3D(int play2D3D)
{
  if (m_buffers2D3D.Size()<=0) return;
  m_buffers2D3D[m_write] = play2D3D;
}

void VoNSoundBufferOAL::startVoice(int voice2D3D)
{
  if (!m_buffers2D3D.Size()) return; //unable to start it. createBuffer was unsuccessful before
  bool invalidateCache = false;
  bool srcOpen = m_sourceOpen;
  bool src3DOpen = m_source3DOpen;
  ScopeLockSection lock(GSoundSysOAL->GetLock());
  if ( !m_sourceOpen && (voice2D3D & TTChan2D) ) 
  {
    alGetError(); // clear Error status
    alGenSources(1,&m_source);
    ALenum err = alGetError();
    if (err==AL_NO_ERROR)
    {
      alSourcei(m_source,AL_SOURCE_RELATIVE,true);
      invalidateCache = true;
      m_sourceOpen = true;
    }
    else
    {
      RptF("startVoice: cannot create source, error %x", err);
    }
  }
  if ( !m_source3DOpen && (voice2D3D & TTChan3D) ) 
  {
    alGetError(); // clear Error status
    alGenSources(1,&m_source3D);
    ALenum err = alGetError();
    if (err==AL_NO_ERROR)
    {
      alSourcei(m_source3D,AL_SOURCE_RELATIVE,false);
      invalidateCache = true;
      m_source3DOpen = true;
    }
    else
    {
      RptF("startVoice: cannot create source, error %x", err);
    }
  }
  //createBuffer if it does not exist
  if ( !srcOpen && !src3DOpen )
  {
    Verify(m_buffers.Size()==0);
    int bufferCount = m_buffers2D3D.Size();
    int len = bufferCount*m_bufferSize;
    m_buffers.Realloc(bufferCount);
    m_buffers.Resize(bufferCount);
    alGetError(); // clear Error status
Retry:
    alGenBuffers(bufferCount,m_buffers.Data());
    ALenum err = alGetError();
    if (err==AL_OUT_OF_MEMORY)
    {
      if (FreeOnDemandSystemMemoryLowLevel(len+4*1024)!=0)
      {
        goto Retry;
      }
    }
    else if (err==AL_NO_ERROR)
    {
      size = len;
      //m_free = bufferCount;
      //m_write = 0;
      // no need to provide any data - buffer will not start until we queue something
      if ( size )
      {
        m_d = (len + 2) >> 2;                 // D = 1/4 of buffer length (in samples)
        m_ahead = (8 * m_d) >> 2;             // 8/4 of D
        m_mode3D = false;
        if (m_sourceOpen) alSourcei(m_source,AL_SOURCE_RELATIVE,true);
        if (m_source3DOpen) alSourcei(m_source3D,AL_SOURCE_RELATIVE,false);
        Verify(alGetError()==AL_NO_ERROR);
      }
    }
    else
    {
      if (m_sourceOpen) { alDeleteSources(1,&m_source); m_source = 0; m_sourceOpen = false; }
      if (m_source3DOpen) { alDeleteSources(1,&m_source3D); m_source3D = 0; m_source3DOpen = false; }
      m_buffers.Clear();
    }
    if (m_buffers.Size()<=0)
    {
      // we should keep time running even in this case
      m_repl->ForceVoNMessagesDiscarding();
      RptF("StartVoice: VoN sound buffer was not created!");
    }
  }
  if (invalidateCache) m_voiceState.InvalidateCache(voice2D3D);

  if (!src3DOpen) 
  {
    InitEAX();
    UpdateVolume();
  }
}

void VoNSoundBufferOAL::stopVoice(int voice2D3D)
{
  if (!m_sourceOpen && !m_source3DOpen) return; //already stopped
  bool invalidateCache = false;

  ScopeLockSection lock(GSoundSysOAL->GetLock());
  // 1. stop given sources
  if ((voice2D3D&TTChan2D) && m_sourceOpen)
  {
    alGetError(); // clear Error status
    alSourceStop(m_source);
    Verify(alGetError()==AL_NO_ERROR);
  }
  if ((voice2D3D&TTChan3D) && m_source3DOpen)
  {
    alGetError(); // clear Error status
    alSourceStop(m_source3D);
    Verify(alGetError()==AL_NO_ERROR);
  }

  // 2. unqueue stream (if no source remains open after this call)
  if (
    (!m_sourceOpen || (voice2D3D&TTChan2D))      //2D not playing or will be stopped now
    && (!m_source3DOpen || (voice2D3D&TTChan3D)) //3D not playing or will be stopped now
    && m_buffers.Size()>0  // something was playing
  )
  {
    UnqueueStream(m_buffers.Size()-m_free);
  }

  // 3. unbind the buffer from the given sources
  if ((voice2D3D&TTChan2D) && m_sourceOpen)
  {
    alGetError(); // clear Error status
    alSourcei(m_source,AL_BUFFER,0);
    Verify(alGetError()==AL_NO_ERROR);
  }
  if ((voice2D3D&TTChan3D) && m_source3DOpen)
  {
    alGetError(); // clear Error status
    alSourcei(m_source3D,AL_BUFFER,0);
    Verify(alGetError()==AL_NO_ERROR);
  }

  // 4. delete the buffer (only when no source uses it)
  if (
    (!m_sourceOpen || (voice2D3D&TTChan2D))      //2D not playing or will be stopped now
    && (!m_source3DOpen || (voice2D3D&TTChan3D)) //3D not playing or will be stopped now
    && m_buffers.Size()>0  // something was playing
  )
  {
    alGetError(); // clear Error status
    alDeleteBuffers(m_buffers.Size(),m_buffers.Data());
    Verify(alGetError()==AL_NO_ERROR);
    m_buffers.Resize(0);
    DWORD time = GlobalTickCount();
    m_activeBufferStartTime = time; //initialize in order to make time running during muted
  }

  // 5. delete sources
  if ((voice2D3D&TTChan2D) && m_sourceOpen)
  {
    alGetError(); // clear Error status
    alDeleteSources(1,&m_source);
    Verify(alGetError()==AL_NO_ERROR);
    m_source = 0;
    m_sourceOpen = false;
    invalidateCache = true;
  }
  if ((voice2D3D&TTChan3D) && m_source3DOpen)
  {
    alGetError(); // clear Error status
    alDeleteSources(1,&m_source3D);
    Verify(alGetError()==AL_NO_ERROR);
    m_source3D = 0;
    m_source3DOpen = false;
    invalidateCache = true;
  }

  if (invalidateCache) m_voiceState.InvalidateCache(voice2D3D);
}

// Only schedule startVoice (it is called from another thread, it should be MT safe)
void VoNSoundBufferOAL::setStartVoice(int voice2D3D)
{
  enterSoundBuffer();
  bool done = false;
  for (int i=0; i<m_ctrlActions.Size(); i++)
  {
    if (m_ctrlActions[i].action==VON_BUF_START) { m_ctrlActions[i].voice2D3D |= voice2D3D; done=true; break; }
  }
  if (!done) m_ctrlActions.Add(VoNCtrlAction(VON_BUF_START, voice2D3D));
  leaveSoundBuffer();
}

// Only schedule stopVoice (it is called from another thread, it should be MT safe)
void VoNSoundBufferOAL::setStopVoice(int voice2D3D)
{
  enterSoundBuffer();
  bool done = false;
  for (int i=0; i<m_ctrlActions.Size(); i++)
  {
    if (m_ctrlActions[i].action==VON_BUF_STOP) { m_ctrlActions[i].voice2D3D |= voice2D3D; done=true; break; }
  }
  if (!done) m_ctrlActions.Add(VoNCtrlAction(VON_BUF_STOP, voice2D3D));
  leaveSoundBuffer();
}

// Process scheduled control action (stop/start voice)
void VoNSoundBufferOAL::doCtrlAction()
{
  enterSoundBuffer();
  AutoArray<VoNCtrlAction,MemAllocLocal<VoNCtrlAction,16> > actions(m_ctrlActions);
  m_ctrlActions.Clear(); //reset
  leaveSoundBuffer();
  for (int i=0; i<actions.Size(); i++)
  {
    switch (actions[i].action)
    {
    case VON_BUF_START: startVoice(actions[i].voice2D3D); break;
    case VON_BUF_STOP: stopVoice(actions[i].voice2D3D); break;
    case VON_BUF_NONE:
    default: break;
    }
  }
}

void VoNSoundBufferOAL::InitEAX()
{
  if (!m_source3DOpen) return;
  SoundSystemOAL *soundSys = static_cast<SoundSystemOAL *>(GSoundsys);
  if (!soundSys) return;
  if (soundSys->_eaxEnabled && m_device)
  {
    ScopeLockSection lock(soundSys->GetLock());
    static bool Is3DEnabled = true;
    if (Is3DEnabled)
    {
      GUID active[2];
      active[0] = EAX_NULL_GUID;
      active[1] = EAXPROPERTYID_EAX40_FXSlot0;
      soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_ACTIVEFXSLOTID,m_source3D,&active,sizeof(active));

      // we know slot 0 has 5000 set as HF cutoff for all environments
      EAXSOURCESENDPROPERTIES send;
      // We currently use slot 0 only
      send.guidReceivingFXSlotID = EAXPROPERTYID_EAX40_FXSlot0;
      send.lSend = 0;
      send.lSendHF = 0;
      soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_SENDPARAMETERS,m_source3D,&send,sizeof(send));

      send.guidReceivingFXSlotID = EAXPROPERTYID_EAX40_FXSlot2;
      send.lSend = -10000;
      send.lSendHF = -10000;
      soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_SENDPARAMETERS,m_source3D,&send,sizeof(send));
    }
    else
    {
      // 2D sound send to no slots
      GUID active[2];
      active[0] = EAX_NULL_GUID;
      active[1] = EAX_NULL_GUID;
      soundSys->_eaxSet(&EAXPROPERTYID_EAX40_Source,EAXSOURCE_ACTIVEFXSLOTID,m_source3D,&active,sizeof(active));
    }
  }
}

#include "Es/Memory/normalNew.hpp"

void* VoNSoundBufferOAL::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNSoundBufferOAL::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNSoundBufferOAL::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  Capture buffer:

#ifndef _XBOX

/*!
\patch 5110 Date 12/21/2006 by Ondra
- Fixed: Voice recording did not work with some sound-cards/drivers.
*/
static void captureRoutine ( HANDLE object, void *data )
{
  VoNCaptureBufferOAL *buf = (VoNCaptureBufferOAL*)data;
  Assert( buf );
  if (!buf->m_device) return;
  Assert( buf->m_data.Data() );
  // we want to capture more data

  {
    ScopeLockSection lock(GSoundSysOAL->GetLock());
    ALCint samples = 0;
    alcGetIntegerv(buf->m_device, ALC_CAPTURE_SAMPLES, 1, &samples);

    // there are samples and we are able to capture them  
    if (samples>0 && buf->m_dataUsed<buf->m_data.Size())
    {
      Assert(buf->m_dataUsed>=0);
      //alcGetError(buf->m_device);
      int toCapture = intMin(samples,buf->m_data.Size()-buf->m_dataUsed);
      alcCaptureSamples(buf->m_device,buf->m_data.Data()+buf->m_dataUsed,toCapture);

      //ALCenum err = alcGetError(buf->m_device);
      //if (err==ALC_NO_ERROR)
      {
        buf->m_dataUsed += toCapture;
      }
      //else
      {
        //LogF("Capture error %x",err);
      }
    }
  }

  int minEnc = 0;
  int maxEnc = buf->m_dataUsed;

#ifdef NET_LOG_VOICE_VERBOSE
  NetLog("captureRoutine: samples=%u, dataUsed=%u, min=%u, max=%u",
    samples,m_dataUsed,minEnc,maxEnc);
#endif

  // ask recorder object to encode another bunch of data:
  buf->advanceSafePosition( buf->newData(buf->m_nextPtr,minEnc,maxEnc) );
}

VOID CALLBACK CaptureTimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
  VoNSoundBufferOAL *buf = (VoNSoundBufferOAL*)lpParam;
  Assert(buf);
  if (!buf->suspended)
    captureRoutine(NULL, lpParam);
}

#if _USE_TIMER_QUEUE
VoNCaptureBufferOAL::VoNCaptureBufferOAL ( HANDLE qtimer, VoNRecorder *rec )
#else
VoNCaptureBufferOAL::VoNCaptureBufferOAL ( EventToCallback *etc, VoNRecorder *rec )
#endif
{
#ifdef NET_LOG_VOICE
  NetLog("VoNCaptureBufferOAL::VoNCaptureBufferOAL");
#endif
  input = true;
  suspended = true;
  m_ev = NULL;
  size = 0;
  // default sound format:
  format.bitsPerSample = 16;
  format.frequency     = 8000;
  format.granularity   = 1;
  format.alignment     = 1;

#if _USE_TIMER_QUEUE
  Assert( qtimer );
  m_timerQueue = qtimer;
#else
  Assert( etc );
  m_etc = etc;
#endif
  Assert( rec );
  m_rec = rec;
  m_ev = NULL;
  m_d = m_ahead = 0;
  m_nextPtr = 0;
}

VoNCaptureBufferOAL::~VoNCaptureBufferOAL ()
{
  destroyBuffer();
}

void VoNCaptureBufferOAL::startCapture()
{
  ScopeLockSection lock(GSoundSysOAL->GetLock());
  alcGetError(m_device);
  alcCaptureStart(m_device);
  ALCenum err = alcGetError(m_device);
  (void)err;
  Log("Started capture device %p, err %x",m_device,err);
}
void VoNCaptureBufferOAL::stopCapture()
{
  ScopeLockSection lock(GSoundSysOAL->GetLock());
  alcGetError(m_device);
  alcCaptureStop(m_device);
  ALCenum err = alcGetError(m_device);
  (void)err;
  Log("Stopped capture device %p, err %x",m_device,err);
}

void VoNCaptureBufferOAL::registerCallbacks ()
{
  if ( !m_device ) return;                  // nothing to do..
#if _USE_TIMER_QUEUE
  Assert( m_timerQueue );
#else
  Assert( m_etc );
#endif
  if ( m_ev ) unregisterCallbacks();
  stopCapture();

#if _USE_TIMER_QUEUE
  // create & register the timer:
  BOOL err = CreateTimerQueueTimer(&m_ev, m_timerQueue, CaptureTimerRoutine, this, DS_TIMER, DS_TIMER, WT_EXECUTEINTIMERTHREAD);
  Assert(err);
  (void) err;
#else
  // create & register the timer:
  m_ev = CreateWaitableTimer(NULL,FALSE,NULL);
  Assert( m_ev );
  m_etc->addEvent(m_ev,captureRoutine,this);
  LARGE_INTEGER start = { (int64)0 };
  SetWaitableTimer(m_ev,&start,DS_TIMER,NULL,NULL,FALSE);
#endif

  if ( !suspended )
  {
    startCapture();
  }
}

void VoNCaptureBufferOAL::unregisterCallbacks ()
{
#if _USE_TIMER_QUEUE
  Assert( m_timerQueue );
#else
  Assert( m_etc );
#endif
  stopCapture();

  if ( m_ev )
  {
#if _USE_TIMER_QUEUE
    DeleteTimerQueueTimer(m_timerQueue, m_ev, INVALID_HANDLE_VALUE); //we want to wait to routine completion 
#else
    CancelWaitableTimer(m_ev);
    m_etc->removeEvent(m_ev);
    CloseHandle(m_ev);
#endif
    m_ev = NULL;
  }

  if ( !suspended )
  {
    startCapture();
  }
}

void VoNCaptureBufferOAL::suspendCallbacks ()
{
  Assert( m_device );
  stopCapture();
#if !_USE_TIMER_QUEUE
  Assert( m_etc );
  if ( m_ev ) m_etc->enableEvent(m_ev,false);
#endif
}

void VoNCaptureBufferOAL::resumeCallbacks ()
{
#if !_USE_TIMER_QUEUE
  Assert( m_etc );
  if ( m_ev ) m_etc->enableEvent(m_ev,true);
#endif
  Assert( m_device );
  startCapture();
}

void VoNCaptureBufferOAL::createBuffer ( unsigned len )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNCaptureBufferOAL::createBuffer(): len=%u, freq=%d, bps=%d",
    len,(int)format.frequency,(int)format.bitsPerSample);
#endif
  size = m_d = m_ahead = 0;
  m_ev = NULL;
  m_device = NULL;
#if _ENABLE_MP
  ScopeLockSection lock(GSoundSysOAL->GetLock());
  // check if there is a support for capture - for this we need valid device, though
  //alcIsExtensionPresent(NULL,"ALC_EXT_CAPTURE");
  const char *defaultCapture = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
  m_device = alcCaptureOpenDevice(defaultCapture, format.frequency, AL_FORMAT_MONO16, len);
#endif

  if (m_device)
  {
    ALCenum err = alcGetError(m_device);
    (void)err;
#if _ENABLE_MP    
    Log("Created capture device %s - %p, err %x",defaultCapture,m_device,err);
#endif
    m_data.Realloc(len);
    m_data.Resize(len);

    // set local variables:
    size = len;
    m_d = (len + 2) >> 2;                   // D = 1/4 of buffer length (in samples)
    m_ahead = (5 * m_d) >> 2;               // 5/4 of D
    m_nextPtr = 0;
    m_dataUsed = 0;
    // assume our buffer iz zero-initialized:
#ifdef NET_LOG_VOICE
    NetLog("VoNCaptureBufferOAL::createBuffer() succeeded");
#endif
  }
  else 
  {
    m_data.Clear();
#ifdef NET_LOG_VOICE
    NetLog("VoNCaptureBufferOAL::createBuffer() failed: 0x%x, 0x%x",(unsigned)resCap,(unsigned)resBuf);
#endif
  }
}

void VoNCaptureBufferOAL::suspendBuffer ()
{
  if ( !m_device ) return;                  // nothing to do..
  suspendCallbacks();
  stopCapture();
  m_nextPtr = 0;
  suspended = true;
}

void VoNCaptureBufferOAL::resumeBuffer ()
{
  if ( !m_device ) return;                  // nothing to do..
  resumeCallbacks();
  startCapture();
  suspended = false;
}

void VoNCaptureBufferOAL::destroyBuffer ()
{
  if ( !m_device ) return;                  // nothing to do..
#ifdef NET_LOG_VOICE
  NetLog("VoNCaptureBufferOAL::destroyBuffer: m_capture.count=%d, m_buffer.count=%d",m_capture.GetRefCount(),m_buffer.GetRefCount());
#endif
  stopCapture();
  unregisterCallbacks();
  ScopeLockSection lock(GSoundSysOAL->GetLock());
  size = m_d = m_ahead = 0;
  alcCaptureCloseDevice(m_device);
  Log("Closed capture device %p",m_device);
  m_device = NULL;
  m_data.Clear();
}

unsigned VoNCaptureBufferOAL::newData ( unsigned &pos, unsigned minSamples, unsigned maxSamples )
{
  Assert( m_rec );
  return m_rec->encode(this,pos,minSamples,maxSamples);
}

void VoNCaptureBufferOAL::advanceSafePosition ( unsigned inc )
{
  // we may delete inc from the data intended for encoding
  Assert((int)inc<=m_dataUsed);
  int size = m_data.Size();
  m_data.Delete(0,inc);
  m_dataUsed -= inc;
  // we want to resize the buffer to the original size again
  m_data.Resize(size);
}

void VoNCaptureBufferOAL::lockData ( unsigned pos, unsigned len, CircularBufferPointers &ptr )
{
  Zero(ptr);
  Assert((int)len<=m_data.Size());
  // return pointer into the temporary buffer
  ptr.ptr1.bps16 = m_data.Data();
  ptr.len1 = len;
}

void VoNCaptureBufferOAL::unlockData ( unsigned processed, CircularBufferPointers &ptr )
{
}

#include "Es/Memory/normalNew.hpp"

void* VoNCaptureBufferOAL::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNCaptureBufferOAL::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNCaptureBufferOAL::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  VoN-system:

VoNChannelId ExplicitMapTraits<VoNChannelId,RefD<VoNSoundBufferOAL> >::keyNull = VOID_CHANNEL;
VoNChannelId ExplicitMapTraits<VoNChannelId,RefD<VoNSoundBufferOAL> >::zombie  = RESERVED_CHANNEL;

VoNSystemOAL::VoNSystemOAL ( ALCdevice *device )
{
  m_device = device;
#if _USE_TIMER_QUEUE
  m_timerQueue = CreateTimerQueue();
#else
  // create & init the ETC object:
  m_etc = new EventToCallback;
#endif
}

VoNSystemOAL::~VoNSystemOAL ()
{
  stopClient();
#if _USE_TIMER_QUEUE
  DeleteTimerQueue(m_timerQueue);
#endif
}

void VoNSystemOAL::stopClient()
{
  m_capture = NULL;
  m_bufs.reset();
}

VoNResult VoNSystemOAL::setReplay ( VoNChannelId chId, bool create )
// this method is called by VoNClient when the replayer is created/destroyed.
// Create: a sound-buffer has to be established and connected to the new replayer.
// Destroy: sound-buffer (including its notification mechanism) has to be destroyed.
{
#if !_USE_TIMER_QUEUE
  Assert( m_etc );
#endif
  Assert( m_client );
  RefD<VoNSoundBufferOAL> buf;

  if ( create )                             // creating
  {
    if ( IsDedicatedServer() )
    {
      ErrF("VoNSystemOAL: cannot create replayer-object on dedicated server!");
      return VonError;
    }
    VoNReplayer *r = m_client->getReplayer(chId);
    if ( !r ) return VonError;              // unknown channel => ignore it
    if ( m_bufs.get(chId,buf) )             // buffer exists => error
    {
      ErrF("VoNSystemOAL: Existing replayer buffer (%d)!",(int)chId);
      return VonError;
    }
    // new replayer buffer:
    Assert( m_device );
#if _USE_TIMER_QUEUE
    buf = new VoNSoundBufferOAL(m_timerQueue,r,m_device);
#else
    buf = new VoNSoundBufferOAL(m_etc,r,m_device);
#endif
    m_bufs.put(chId,buf);
    // prepare the buffer:
    buf->format.frequency     = r->getFrequency();
    buf->format.bitsPerSample = outBps;
    buf->format.granularity = r->getCodec()->estFrameLen();
    buf->createBuffer( (unsigned)( REPLAYER_BUFFER_D * r->getFrequency() ) << 2 );
    buf->registerCallbacks();               // start notification mechanism for the new buffer
    buf->resumeBuffer();
#ifdef NET_LOG_VOICE
    NetLog("VoNSystemOAL: replayer object created for player=%d",chId);
#endif
  }

  else                                      // destroying
  {
    if ( m_bufs.get(chId,buf) )             // buffer exists => destroy it
    {
      buf->destroyBuffer();
      m_bufs.removeKey(chId);
    }
    else
    {
      ErrF("VoNSystemOAL: Unknown replayer buffer (%d)!",(int)chId);
      return VonError;
    }
#ifdef NET_LOG_VOICE
    NetLog("VoNSystemOAL: replayer object destroyed for player=%d",chId);
#endif
  }

  return VonOK;
}

VoNResult VoNSystemOAL::setSuspend ( VoNChannelId chId, bool suspend )
// this method is called by VoNClient when the replayer is suspended/resumed.
// Suspend: stop the sound-buffer replay & notification mechanism.
// Resume: restart the sound-buffer replay & notification mechanism.
{
  RefD<VoNSoundBufferOAL> buf;
  if ( !m_bufs.get(chId,buf) )
    return VonError;
#if defined(NET_LOG_VOICE) || DEBUG_VON
  NetLog("VoNSystemOAL: replayer object %s for player=%d",suspend?"suspended":"resumed",chId);
#endif

  if ( suspend )                            // suspend
    buf->suspendBuffer();
  else                                      // resume
    buf->resumeBuffer();

  return VonOK;
}

static int GetVoiceFrequency(int quality, int &codecQuality)
{
  DoAssert(quality>0);
  if (quality<=10)
  {
    codecQuality = quality;
    return 8000;
  }
  if (quality<=20)
  {
    codecQuality = quality-10;
    return 16000;
  }
  codecQuality = quality-20;
  return 32000;
}

VoNResult VoNSystemOAL::setCapture ( bool on, int quality )
// if called for the 1st time:
// create VoNCaptureBufferOAL & associate it with VoNRecorder
{
#if !_USE_TIMER_QUEUE
  Assert( m_etc );
#endif
  Assert( m_client );

  if ( !m_capture )                         // capture buffer was not created yet
  {
    if ( IsDedicatedServer() )
    {
      ErrF("VoNSystemOAL: cannot create capture-object on dedicated server!");
      return VonError;
    }
    VoNRecorder *r = m_client->getRecorder();
    if ( !r )                               // voice was swithed on for the 1st time => create codec & recorder
    {
      CodecInfo info;
      Zero(info);
      // set default codec parameters:
#if 1
      strcpy(info.name,"Speex");
      info.type = ScVoice;
      int codecQuality=3;
      info.nominalFrequency = GetVoiceFrequency(quality,codecQuality);
#else
      strcpy(info.name,"LPC");
      info.type = ScVoice;
      strcpy(info.name,"PCM");
      info.type = ScUncompressed;
      strcpy(info.name,"DPCM");
      info.type = ScUniversal;
#endif
      VoNCodec *codec = findCodec(info);
      Assert( codec );
      codec->setVoiceMask(defaultVoiceMask);
      r = new VoNRecorder(m_client,m_client->outChannel,codec, GSoundsys?GSoundsys->GetVoNRecThreshold():VoNRecorder::DEFAULT_REC_THRESHOLD);
      r->SetCodecQuality(codecQuality);
      m_client->setRecorder(r);
    }
    Assert( r );
#if _USE_TIMER_QUEUE
    m_capture = new VoNCaptureBufferOAL(m_timerQueue,r);
#else
    m_capture = new VoNCaptureBufferOAL(m_etc,r);
#endif
    // prepare the buffer:
    int freq = r->getFrequency();
    m_capture->format.frequency     = freq;
    m_capture->format.bitsPerSample = inBps;
    m_capture->createBuffer( (unsigned)( CAPTURE_BUFFER_D * freq ) << 2 );
    m_capture->registerCallbacks();
#ifdef NET_LOG_VOICE
    NetLog("VoNSystemOAL: capture buffer was created: %u Hz, %u bps",m_capture->format.frequency,m_capture->format.bitsPerSample);
#endif
  }

  // capture on/off:
  if ( on )
    m_capture->resumeBuffer();
  else
    m_capture->suspendBuffer();

  return VonOK;
}

RefD<VoNSoundBuffer> VoNSystemOAL::getSoundBuffer ( VoNChannelId chId )
{
  RefD<VoNSoundBufferOAL> buf;
  m_bufs.get(chId,buf);
  return buf.GetRef();
}

/// Set position of player (in OAL it affects source)
void VoNSystemOAL::setPosition(int dpnid, Vector3Par pos, Vector3Par speed)
{
  RefD<VoNSoundBufferOAL> buf;
  m_bufs.get(dpnid, buf);
  if (buf) buf->Set3DPosition(pos, speed);
}

void VoNSystemOAL::WhatUnitsVoNDirect(AutoArray<int> &units, Vector3Par pos, float maxDist)
{
  IteratorState iterator;
  RefD<VoNSoundBufferOAL> buf;
  for (bool avail=m_bufs.getFirst(iterator, buf); avail; avail=m_bufs.getNext(iterator, buf))
  {
    const VoiceOALStateEAX &state = buf->GetVoiceState();
    if (state.parsTime)
    { //position was set already
      float dist2 = state.position.Distance2(pos);
      if (dist2 <= Square(maxDist)) 
      {
        units.Add(buf->GetChannel());
      }
    }
  }
}

/// Set position of player (in OAL it affects source)
void VoNSystemOAL::setObstruction(int dpnid, float obstruction,float occlusion,float deltaT)
{
  RefD<VoNSoundBufferOAL> buf;
  m_bufs.get(dpnid, buf);
  if (buf) buf->SetObstruction(obstruction,occlusion,deltaT);
}

/// Set volume and ear accomodation of player (in OAL it affects source)
void VoNSystemOAL::updateVolumeAndAccomodation(int dpnid, float volume,float accomodation)
{
  RefD<VoNSoundBufferOAL> buf;
  m_bufs.get(dpnid, buf);
  if (buf) buf->UpdateVolumeAndAccomodation(volume, accomodation);
}

VoNChatChannel VoNSystemOAL::checkChatChannel(int dpnid, bool &audible2D, bool &audible3D)
{
  audible2D = audible2D = false;
  RefD<VoNSoundBufferOAL> buf;
  m_bufs.get(dpnid, buf);
  if (buf)
  {
    return buf->CheckChatChannel(audible2D,audible3D);
  }
  return 0;
}

void VoNSystemOAL::get2D3DPlaying(int dpnid, bool &audible2D, bool &audible3D)
{
  audible2D = audible2D = false;
  RefD<VoNSoundBufferOAL> buf;
  m_bufs.get(dpnid, buf);
  if (buf)
  {
    return buf->get2D3DPlaying(audible2D,audible3D);
  }
}

//////////////////////////////////////////////////////////////////////////
/// VoiceManager
/// Only some M most loud voices can get the OAL source
//////////////////////////////////////////////////////////////////////////

#include "../textbank.hpp"
#include "../ui/chat.hpp"

//! Interface for 2D or 3D voice sound (in OAL affects sources)
struct SortVoice
{
  RefD<VoNSoundBufferOAL> _voice;
  int   _voice2D3D;
  float _loudness;

  SortVoice(){}
  explicit SortVoice(RefD<VoNSoundBufferOAL> voice, int chan2D3D);

  float Loudness() const {return _loudness;}
};

TypeIsMovable(SortVoice);

// OAL buffer has its 2D and 3D parts. SortVoice need to distinguish between these two.
SortVoice::SortVoice(RefD<VoNSoundBufferOAL> voice, int chan2D3D)
:_voice(voice), _voice2D3D(chan2D3D)
{
  _loudness=_voice->GetLoudness(chan2D3D);
}

// Coefficients to get some stable measure to estimate "relative volume" of incomming voices
// Channel coefficients
const float VoiceCoefCCDirect  = 0.3f;
const float VoiceCoefCCGlobal = 0.3f;
const float VoiceCoefCCVehicle = 0.6f;
// Other coefficients
const float VoiceCoefIsPlaying = 0.1f;
const float MinVoN2DDistance = 0.5f;

//#define DEBUG_VON_SORTING 1
float VoNSoundBufferOAL::GetLoudness(int voice2D3D)
{
  float volume = 0;
  if ( m_voiceState.parsTime || (voice2D3D&TTChan2D) ) //3D position was already set OR it is 2D voice
  {
    extern ChatChannel VoNChatChannel2ChatChannel(int channel);
    ChatChannel bufChan = VoNChatChannel2ChatChannel(m_repl->GetChatChannel());
    switch (bufChan)
    {
    case CCGlobal: volume += VoiceCoefCCGlobal; break;
    case CCSystem: volume += VoiceCoefCCGlobal; break;
    case CCVehicle: volume += VoiceCoefCCVehicle; break;
    case CCDirect: volume += VoiceCoefCCDirect; break;
    }
    if ( voice2D3D&TTChan2D )
    {
      // add some bonus for voices currently playing (in order to add some Hysteresis)
      if (m_sourceOpen) volume += VoiceCoefIsPlaying;
      if (!m_repl->GetPlay2D()) return 0; // only 3D can be heard
      float distance = GetListenerPos().Distance(m_voiceState.position);
      if (distance < MinVoN2DDistance) distance=MinVoN2DDistance;
      float distVol = 2/distance;
      volume += distVol;
      // combine with the radio volume set in Audio options
      float radioVolume = (static_cast<SoundSystemOAL *>(GSoundsys))->_volumeAdjustSpeech;
      volume *= radioVolume;
#if DEBUG_VON_SORTING
      LogF("2DVoice: distVol=%.2f finalVolume=%.2f", distVol, volume);
#endif
    }
    else //3D
    {
      //TODO: is DirectChannel louder than Global? take into account it should be...
      // add some bonus for voices currently playing (in order to add some Hysteresis)
      if (!m_repl->GetPlay3D()) return 0; // only 2D can be heard
      if (m_source3DOpen) volume += VoiceCoefIsPlaying;
      if (bufChan!=CCDirect) volume *= 0.5f; // direct sound going with 2D speaking is 1/2 attenuated
      float volumeAdjust = (static_cast<SoundSystemOAL *>(GSoundsys))->_volumeAdjustEffect;
      float volumeAdjustAll = volumeAdjust;
      float obsVolume = 1-(1-m_voiceState.obstruction)*0.7f;
      // assume occlusion reduces the sound much more than obstruction
      float occVolume = 1-(1-m_voiceState.occlusion)*0.9f;
      float volumeFactor = floatMin(obsVolume,occVolume);
      volumeAdjust *= volumeFactor;
      // calculate volume based on distance
      float minDistance = CalcMinDistance();
      float distance = GetListenerPos().Distance(m_voiceState.position);
      float atten = distance<minDistance ? 1 : DistanceRolloff(minDistance,distance);
      float vol = atten*volumeAdjust;
      volume += vol;
      // combine with the effects volume set in Audio options
      volume *= volumeAdjustAll;
#if DEBUG_VON_SORTING
      LogF("3DVoice: vol=%.2f finalVolume=%.2f", vol, volume);
#endif
    }
  }
  if (m_repl->getPlaying() <= 0) //no speech
  {
    DWORD time = ::GetTickCount();
    if (time>m_lastPlayingTime)
    {
      float coef = Interpolativ((time-m_lastPlayingTime)/1000.0f, 0,2,1,0);
      volume *= coef;
    }
    else m_lastPlayingTime = ::GetTickCount(); //probably not set yet
  }
  else //reset the timestamp
  {
    m_lastPlayingTime = ::GetTickCount();
  }
#if DEBUG_VON_SORTING
  if ( voice2D3D&TTChan2D ) LogF("    2DVoice %.2f", volume);
  else                      LogF("    3DVoice %.2f", volume);
#endif
  return volume;
}

static int CompareVoices( const SortVoice *v0, const SortVoice *v1 )
{
  float diff=v0->_loudness-v1->_loudness;
  if( diff>0 ) return -1; //reverse order - louder voices are less then silence, so louder are QSorted to the front of array
  if( diff<0 ) return +1;
  return 0;
}

#if _ENABLE_CHEATS
char GVoNSoundDiag1[4096];
#endif
void VoNSystemOAL::advanceAll(float deltaT, bool paused, bool quiet)
{
#ifdef TEST_START_STOP_VOICE
  static bool doIt = false;
  static bool stopIt = true;
  if (doIt) //mute All
  {
    IteratorState iterator;
    RefD<VoNSoundBufferOAL> buf;
    for (bool avail=m_bufs.getFirst(iterator, buf); avail; avail=m_bufs.getNext(iterator, buf))
    {
      if (stopIt)
      {
        int whatToStop = TTChan2D | TTChan3D;
        buf->setStopVoice(whatToStop);
      }
      else
      {
        int whatToStart = TTChan2D | TTChan3D;
        buf->setStartVoice(whatToStart);
      }
    }
  }
#else //Voice Manager implementation
  DWORD time = ::GetTickCount();
  int diff = time-m_listenerPosSetTime;
  if (diff<0 || diff>2000) // voices cannot be updated when listener position is not being set
  {
    m_listenerPos = Vector3(-100000.0f,-100000.0f,-100000.0f); //far far away, but no QNANs
  }
  AUTO_STATIC_ARRAY(SortVoice,sort,128);
  // sort voices by intensity (as heard by the listener)
  IteratorState iterator;
  RefD<VoNSoundBufferOAL> buf;
  const float LimitMuteMinimal = 0.001f; //TODO TODO TODO
  for (bool avail=m_bufs.getFirst(iterator, buf); avail; avail=m_bufs.getNext(iterator, buf))
  {
    buf->SetListenerPos(m_listenerPos);
    SortVoice sv2D(buf, TTChan2D);
    SortVoice sv3D(buf, TTChan3D);
#define MUTE_3D_WHEN_BOTH_PRESENT 1
#ifdef MUTE_3D_WHEN_BOTH_PRESENT
    // Now we would prefer 2D voice over 3D if both are present (mute 3D)
    if (sv2D.Loudness()>LimitMuteMinimal && sv3D.Loudness()>LimitMuteMinimal)
    {
      //take maximum of loudness for this voice
      if ( sv3D.Loudness() > sv2D.Loudness() ) sv2D._loudness = sv3D.Loudness();
      sv3D._loudness = 0.0f; // Mute 3D
    }
#endif
    //voices must be added even when muted, as we should stop its possible playing
    sort.Add(sv2D);
    sort.Add(sv3D);
  }
  QSort(sort.Data(),sort.Size(),CompareVoices);

#if DEBUG_VON_SORTING
  LogF("... QSort voices: %d", sort.Size());
#endif
  // There can be limited number of OAL sources for VoN sounds
  int played=sort.Size();
  if (played>Glob.config.maxVoNSounds) played=Glob.config.maxVoNSounds;
  // first stop all voices that should be muted
  for( int i=played; i<sort.Size(); i++ )
  {
    SortVoice &voice=sort[i];
    // if the voice has active OAL source, it will be deleted
    voice._voice->setStopVoice(voice._voice2D3D);
  }
#if _ENABLE_CHEATS
  *GVoNSoundDiag1 = 0; //terminate
  static DWORD lastTimeStamp = GetTickCount();
  DWORD curTimeStamp = GetTickCount();
  bool updateDiag = false;
  bool firstEntry = true;
  if (lastTimeStamp + 500 < curTimeStamp)
  {
    lastTimeStamp = curTimeStamp;
    updateDiag = true;
  }
#endif
  // Play all voices that have been chosen
  for( int i=0; i<played; i++ )
  {
    SortVoice &voice=sort[i];
    if (voice.Loudness()>=LimitMuteMinimal)
    {
      // if the voice was inactive, OAL source will be assigned to it
      voice._voice->setStartVoice(voice._voice2D3D);
#if _ENABLE_CHEATS
      if (updateDiag)
      {
        char newDiag[1024];
        const VoiceOALStateEAX &state = voice._voice->GetVoiceState();
        sprintf(newDiag, "%sVoN %x (%s) - %.1f m (%.1f m), dB %.1f, obs %.1f/%.1f dB", 
          (firstEntry ? "" : "\n"),
          voice._voice->GetChatChannel(),
          ((voice._voice2D3D==TTChan2D) ? "2D" : "3D"),
          state.referenceDistance,
          voice._voice->GetListenerPos().Distance(state.position),
          log10(voice._loudness)*20,
          log10(state.obstruction)*20, log10(state.occlusion)*20
          );
        strcat(GVoNSoundDiag1, newDiag);
        firstEntry = false;
      }
#endif
    }
    else
    {
      // if the voice has active OAL source, it will be deleted
      voice._voice->setStopVoice(voice._voice2D3D);
    }
  }
#endif
}

#endif  // VOICE_OVER_NET

#endif
