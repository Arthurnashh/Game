/**
  @file  
  Voice over Net objects used together with DirectSound API.
*/

#include "../wpch.hpp"
#include "../global.hpp"
#include "voiceDX8.hpp"
#include "El/Speech/debugVoN.hpp"
#include "El/Speech/vonCodec.hpp"

#if VOICE_OVER_NET && _ENABLE_VON && !_GAMES_FOR_WINDOWS

// dsound.dll used - cf. DirectSoundCaptureCreate
#pragma comment(lib,"dsound")

//----------------------------------------------------------------------
//  Replayer buffer:

#undef DS_NOTIFICATION

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
  VoNSoundBufferDS *buf = (VoNSoundBufferDS*)data;
  Assert( buf );
  DWORD playPtr, writePtr;
  Assert( buf->m_buffer );
  Verify( buf->m_buffer->GetCurrentPosition(&playPtr,&writePtr) == DS_OK );
  if ( buf->format.bitsPerSample > 8 )
  {                                         // pointers must be in 'samples'
    playPtr >>= 1;
    writePtr >>= 1;
  }
  if ( writePtr < playPtr ) writePtr += buf->size;
  unsigned backup = writePtr + buf->m_ahead;// writePtr to backup must be filled-in
  if ( backup - playPtr > buf->size ) backup = playPtr + buf->size;
  unsigned nextPtr = buf->m_nextPtr;        // 1st non-valid sample index
  if ( nextPtr < playPtr ) nextPtr += buf->size;

  unsigned minDec, maxDec;                  // minimum/maximum decoded samples
  if ( nextPtr >= backup )                  // don't need new data..
    minDec = 0;
  else
    minDec = backup - nextPtr;
  maxDec = playPtr + buf->size - nextPtr;
  //minDec = max(0, min(minDec, maxDec-buf->m_repl->GetMaxFrame()));

#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
  NetLog("replayerRoutine: play=%u, write=%u, next=%u, backup=%u, min=%u, max=%u",
         playPtr,writePtr,nextPtr,backup,minDec,maxDec);
#endif

  // ask replayer object to decode another bunch of data:
  buf->advanceSafePosition( buf->newData(buf->m_nextPtr,minDec,maxDec) );


#if DEBUG_VON_DIAGS
  DWORD dummy=NULL;
  if (buf->GetCurrentPosition(&replayerCurPosition,&dummy)!=DS_OK)
    replayerCurPosition=NULL;
  if (buf->m_buffer->GetStatus(&VoNReplStatus)!=DS_OK)
    VoNReplStatus=-1;
#endif
}

VoNSoundBufferDS::VoNSoundBufferDS ( EventToCallback *etc, VoNReplayer *repl, IMySound *dSound )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNSoundBufferDS::VoNSoundBufferDS");
#endif
  input = false;
  suspended = true;
  size = 0;
    // default sound format:
  format.bitsPerSample = 8;
  format.frequency     = 8000;
  format.granularity   = 1;
  format.alignment     = 1;

  Assert( etc );
  m_etc = etc;
  Assert( repl );
  m_repl = repl;
  Assert( dSound );
  m_ds = dSound;
  m_mode3D = false;
  m_ev = NULL;
  m_d = m_ahead = 0;
  m_nextPtr = 0;
  m_lockPtr1 = m_lockPtr2 = NULL;
  m_lockLen1 = m_lockLen2 = 0;
}

VoNSoundBufferDS::~VoNSoundBufferDS ()
{
  destroyBuffer();
}
	
void VoNSoundBufferDS::registerCallbacks ()
{
  if ( !m_buffer ) return;                  // nothing to do..
  Assert( m_etc );
  if ( m_ev ) unregisterCallbacks();
  Verify( m_buffer->Stop() == DS_OK );

#ifdef DS_NOTIFICATION

#ifdef NET_LOG_VOICE_VERBOSE
  NetLog("Stop (registerCallbacks)");
#endif
    // register the event:
  m_ev = CreateEvent(NULL,FALSE,FALSE,NULL);
  Assert( m_ev );
  m_etc->addEvent(m_ev,replayerRoutine,this);

  // set sound-buffer notification:
  Assert( m_buffer );
  unsigned ptr = 0;
  unsigned delta = (format.bitsPerSample > 8) ? m_d + m_d : m_d;
  DSBPOSITIONNOTIFY posNotify[4];
  ComRef<IDirectSoundNotify> notify;
#if defined _XBOX
  notify = m_buffer;
#else
  if ( m_buffer->QueryInterface(IID_IDirectSoundNotify,(LPVOID*)notify.Init()) == DS_OK )
#endif
  {
    int i;
    for ( i = 0; i < 4; i++, ptr += delta ) // notify one event:
    {
      posNotify[i].dwOffset     = ptr;
      posNotify[i].hEventNotify = m_ev;
    }
    Verify( notify->SetNotificationPositions(4,posNotify) == DS_OK );
  }

#else   // DS_NOTIFICATION

#ifdef NET_LOG_VOICE_VERBOSE
  NetLog("Stop (registerCallbacks)");
#endif
  // create & register the timer:
  m_ev = CreateWaitableTimer(NULL,FALSE,NULL);
  Assert( m_ev );
  m_etc->addEvent(m_ev,replayerRoutine,this);
  LARGE_INTEGER start = { (int64)0 };
  SetWaitableTimer(m_ev,&start,DS_TIMER,NULL,NULL,FALSE);

#endif

  if ( !suspended )
  {
    Verify( m_buffer->Play(0,0,DSBPLAY_LOOPING) == DS_OK );
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
    NetLog("Play (registerCallbacks)");
#endif
  }
#if DEBUG_VON
  else NetLog("RegisterCallBacks: buffer suspended");
#endif
}

void VoNSoundBufferDS::unregisterCallbacks ()
{
  Assert( m_etc );

#ifdef DS_NOTIFICATION

  Verify( m_buffer->Stop() == DS_OK );
#ifdef NET_LOG_VOICE_VERBOSE
  NetLog("Stop (unregisterCallbacks)");
#endif

    // unset sound-buffer notification:
  ComRef<IDirectSoundNotify> notify;
#if defined _XBOX
  notify = m_buffer;
#else
  if ( m_buffer->QueryInterface(IID_IDirectSoundNotify,(LPVOID*)notify.Init()) == DS_OK )
#endif
  {
    Verify( notify->SetNotificationPositions(0,NULL) == DS_OK );
  }

    // unregister events:
  if ( m_ev )
  {
    m_etc->removeEvent(m_ev);
    CloseHandle(m_ev);
    m_ev = NULL;
  }

  if ( !suspended )
  {
    Verify( m_buffer->Play(0,0,DSBPLAY_LOOPING) == DS_OK );
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
    NetLog("Play (unregisterCallbacks)");
#endif
  }
#if DEBUG_VON
  else NetLog("UnRegisterCallBacks: buffer suspended");
#endif

#else   // DS_NOTIFICATION

  Verify( m_buffer->Stop() == DS_OK );
#ifdef NET_LOG_VOICE_VERBOSE
  NetLog("Stop (unregisterCallbacks)");
#endif
  if ( m_ev )
  {
    CancelWaitableTimer(m_ev);
    m_etc->removeEvent(m_ev);
    CloseHandle(m_ev);
    m_ev = NULL;
  }

#endif
}

void VoNSoundBufferDS::suspendCallbacks ()
{
  Assert( m_etc );
  Assert( m_buffer );
  if ( m_ev ) m_etc->enableEvent(m_ev,false);
}

void VoNSoundBufferDS::resumeCallbacks ()
{
  Assert( m_etc );
  Assert( m_buffer );
  if ( m_ev ) m_etc->enableEvent(m_ev,true);
}

void VoNSoundBufferDS::SetPosition ( float x, float y, float z )
{
  Assert( m_buffer3D );
  if ( !m_mode3D )
  {
    Verify( m_buffer3D->SetMode(DS3DMODE_NORMAL,DS3D_IMMEDIATE) == DS_OK );
    m_mode3D = true;
  }
  Verify( m_buffer3D->SetPosition(x,y,z,DS3D_IMMEDIATE) == DS_OK );
}

void VoNSoundBufferDS::createBuffer ( unsigned len )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNSoundBufferDS::createBuffer(): len=%u, freq=%d, bps=%d",
         len,(int)format.frequency,(int)format.bitsPerSample);
#endif
    // Waveform format:
  WAVEFORMATEX fmt;
  fmt.wFormatTag      = WAVE_FORMAT_PCM;
  fmt.nChannels       = 1;
  fmt.nSamplesPerSec  = format.frequency;
  fmt.wBitsPerSample  = (format.bitsPerSample > 8) ? 16 : 8;
  fmt.nBlockAlign     = fmt.wBitsPerSample >> 3;
  fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
  fmt.cbSize          = 0;
    // Sound-buffer description:
  DSBUFFERDESC desc;
  Zero(desc);
  desc.dwSize          = sizeof(desc);
  desc.dwFlags         = DSBCAPS_CTRL3D | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME;
  desc.dwBufferBytes   = len * fmt.nBlockAlign;
  desc.lpwfxFormat     = &fmt;
#if defined _XBOX
  desc.lpMixBins       = NULL;
  desc.dwInputMixBin   = 0;
#else
  desc.dwReserved      = 0;
  desc.guid3DAlgorithm = DS3DALG_DEFAULT;
#endif
  size = m_d = m_ahead = 0;
  suspended = true;
  m_ev = NULL;
  ComRef<IDirectSoundBuffer> temp;
  Assert( m_ds );
  Retry:
  HRESULT hr = m_ds->CreateSoundBuffer(&desc,temp.Init(),NULL);
  if (hr==DSERR_OUTOFMEMORY)
  {
    if (FreeOnDemandSystemMemoryLowLevel(desc.dwBufferBytes+4*1024)!=0)
    {
      goto Retry;
    }
  }
  if (hr==DS_OK)
  {
#if defined _XBOX
    m_buffer   = temp;
    m_buffer3D = temp;
      // set local variables:
    size = len;
#else
      // get both interfaces:
    if ( temp->QueryInterface(IID_IDirectSoundBuffer8,(LPVOID*)m_buffer.Init()) == DS_OK &&
         temp->QueryInterface(IID_IDirectSound3DBuffer8,(LPVOID*)m_buffer3D.Init()) == DS_OK )
    {
        // set local variables:
      size = len;
#if TEST_INIT_BUFFER
      // pure harmonic tone to test sound-replay:
      LONG volume;
      HRESULT hr = m_buffer->GetVolume(&volume);
      if (hr==DS_OK)  LogF("Replayer m_buffer volume = %d", volume);
      CircularBufferPointers ptr;
      lockData(0,len,ptr);
      unsigned i;
      if ( format.bitsPerSample > 8 )
        for ( i = 0; i < len; i++ )
          ptr.write16( (int16)( sin(0.3*i) * 30000.0 ) );
      else
        for ( i = 0; i < len; i++ )
          ptr.write8( (int8)( sin(0.3*i) * 100.0 ) );
      unlockData(len,ptr);
#else
      CircularBufferPointers ptr;
      lockData(0,len,ptr);
      generateSilence(ptr,format.bitsPerSample,len);
      unlockData(len,ptr);
#endif
    }
    else
    {
      m_buffer.Free();
      m_buffer3D.Free();
    }
#endif
    if ( size )
    {
      m_d = (len + 2) >> 2;                 // D = 1/4 of buffer length (in samples)
      m_ahead = (8 * m_d) >> 2;             // 8/4 of D
      m_nextPtr = 2 * m_d;                  // initial decoder latency
        // assume our buffer iz zero-initialized:
      m_mode3D = false;
      Verify( m_buffer3D->SetMode(DS3DMODE_DISABLE,DS3D_IMMEDIATE) == DS_OK );
    }
  }
  if (m_buffer.IsNull() || m_buffer3D.IsNull())
  {
    ErrorMessage("Cannot create sound buffer");
  }
}

void VoNSoundBufferDS::suspendBuffer ()
{
  if ( !m_buffer ) return;                  // nothing to do..
  Verify( m_buffer->Stop() == DS_OK );
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
  NetLog("Stop (suspendBuffer)");
#endif
  suspended = true;
  suspendCallbacks();
    // clear the buffer:
  CircularBufferPointers ptr;
  lockData(0,size,ptr);
  generateSilence(ptr,format.bitsPerSample,size);
  unlockData(size,ptr);
  Verify( m_buffer->SetCurrentPosition(0) == DS_OK );
  m_nextPtr = 2 * m_d;                      // initial decoder latency
}

void VoNSoundBufferDS::resumeBuffer ()
{
  if ( !m_buffer ) return;                  // nothing to do..
  resumeCallbacks();
  suspended = false;
  Verify( m_buffer->Play(0,0,DSBPLAY_LOOPING) == DS_OK );
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
  NetLog("Play (resumeBuffer)");
#endif
}

void VoNSoundBufferDS::destroyBuffer ()
{
  if ( !m_buffer ) return;                  // nothing to do..
#if defined(NET_LOG_VOICE) || DEBUG_VON
  NetLog("VoNSoundBufferDS::destroyBuffer: m_buffer.count=%d, m_buffer3D.count=%d",m_buffer.GetRefCount(),m_buffer3D.GetRefCount());
#endif
  suspended = true;
  unregisterCallbacks();
  size = m_d = m_ahead = 0;
  m_buffer3D.Free();
  m_buffer.Free();
}

unsigned VoNSoundBufferDS::newData ( unsigned &pos, unsigned minSamples, unsigned maxSamples )
{
  Assert( m_repl );
  return m_repl->decode(this,pos,minSamples,maxSamples);
}

void VoNSoundBufferDS::advanceSafePosition ( unsigned inc )
{
  Assert( inc < size );
  m_nextPtr += inc;
  if ( m_nextPtr >= size ) m_nextPtr -= size;
}

void VoNSoundBufferDS::lockData ( unsigned pos, unsigned len,
                                  CircularBufferPointers &ptr )
{
  Zero(ptr);
  if ( m_lockPtr1 != NULL || m_lockPtr2 != NULL ||
       !m_buffer ) return;
  if ( format.bitsPerSample > 8 )
  {
    pos += pos;
    len += len;
  }
  Verify( m_buffer->Lock(pos,len,(void**)&ptr.ptr1.bps8,(unsigned long*)&ptr.len1,(void**)&ptr.ptr2.bps8,(unsigned long*)&ptr.len2,0) == DS_OK );
  m_lockPtr1 = (void*)ptr.ptr1.bps8;
  m_lockPtr2 = (void*)ptr.ptr2.bps8;
  m_lockLen1 = ptr.len1;
  m_lockLen2 = ptr.len2;
  if ( format.bitsPerSample > 8 )
  {
    ptr.len1 >>= 1;
    ptr.len2 >>= 1;
  }
}

void VoNSoundBufferDS::unlockData ( unsigned processed, CircularBufferPointers &ptr )
{
  if ( !m_lockPtr1 || !m_buffer ) return;
  if ( format.bitsPerSample > 8 ) processed += processed;
  if ( m_lockLen1 >= processed )            // processing only in the 1st part
  {
    m_lockLen1 = processed;
    m_lockLen2 = 0;
  }
  else                                      // processing in both parts
  {
    processed -= m_lockLen1;
    Assert( processed <= m_lockLen2 );
    m_lockLen2 = processed;
  }
  Verify( m_buffer->Unlock(m_lockPtr1,m_lockLen1,m_lockPtr2,m_lockLen2) == DS_OK );
  m_lockPtr1 = m_lockPtr2 = NULL;
  m_lockLen1 = m_lockLen2 = 0;
}

#include "Es/Memory/normalNew.hpp"

void* VoNSoundBufferDS::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNSoundBufferDS::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNSoundBufferDS::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

VoNSoundBufferNil::VoNSoundBufferNil (EventToCallback *etc, VoNReplayer *repl)
:m_etc(etc),m_repl(repl)
{
}

VoNSoundBufferNil::~VoNSoundBufferNil ()
{
  destroyBuffer();
}

void VoNSoundBufferNil::createBuffer ( unsigned len )
{

  int nBlockAlign = format.bitsPerSample >> 3;
  size_t size = len * nBlockAlign;
  m_data = safeNew(size);
}

void VoNSoundBufferNil::suspendBuffer ()
{
}

void VoNSoundBufferNil::resumeBuffer ()
{
}

void VoNSoundBufferNil::destroyBuffer ()
{
  if (m_data)
  {
    safeDelete(m_data);
    m_data = NULL;
  }
}

unsigned VoNSoundBufferNil::newData ( unsigned &pos, unsigned minSamples, unsigned maxSamples )
{
  return m_repl->decode(this,pos,minSamples,maxSamples);
}

void VoNSoundBufferNil::advanceSafePosition ( unsigned inc )
{
}

void VoNSoundBufferNil::lockData(
  unsigned pos, unsigned len, CircularBufferPointers &ptr
)
{
  // we need to pretend we locked some data
  Zero(ptr);
  len *= format.bitsPerSample>>3;
  ptr.ptr1.bps16 = (int16 *)m_data;
  ptr.len1 = len;
}

void VoNSoundBufferNil::unlockData ( unsigned processed, CircularBufferPointers &ptr )
{
}


#include "Es/Memory/normalNew.hpp"

void* VoNSoundBufferNil::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNSoundBufferNil::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNSoundBufferNil::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  Capture buffer:

#ifndef _XBOX

static void captureRoutine ( HANDLE object, void *data )
{
  VoNCaptureBufferDS *buf = (VoNCaptureBufferDS*)data;
  Assert( buf );
  DWORD capturePtr, readPtr;
  Assert( buf->m_buffer );
  Verify( buf->m_buffer->GetCurrentPosition(&capturePtr,&readPtr) == DS_OK );
  if ( buf->format.bitsPerSample > 8 )      // pointers must be in 'samples'
  {
    capturePtr >>= 1;
    readPtr >>= 1;
  }
  unsigned backup = capturePtr + buf->m_ahead; // capturePtr to backup must be read
  if ( backup < readPtr ) backup += buf->size;
  unsigned nextPtr = buf->m_nextPtr;        // 1st valid sample index
  if ( nextPtr < readPtr ) nextPtr += buf->size;
  unsigned minEnc, maxEnc;                  // minimum/maximum encoded samples

  if ( nextPtr >= backup )                  // don't need to read on..
    minEnc = 0;
  else
    minEnc = backup - nextPtr;
  maxEnc = readPtr + buf->size - nextPtr;

#ifdef NET_LOG_VOICE_VERBOSE
  NetLog("captureRoutine: read=%u, capture=%u, backup=%u, next=%u, min=%u, max=%u",
         readPtr,capturePtr,backup,nextPtr,minEnc,maxEnc);
#endif

    // ask recorder object to encode another bunch of data:
  buf->advanceSafePosition( buf->newData(buf->m_nextPtr,minEnc,maxEnc) );
}

VoNCaptureBufferDS::VoNCaptureBufferDS ( EventToCallback *etc, VoNRecorder *rec )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNCaptureBufferDS::VoNCaptureBufferDS");
#endif
  input = true;
  suspended = true;
  size = 0;
    // default sound format:
  format.bitsPerSample = 16;
  format.frequency     = 8000;
  format.granularity   = 1;
  format.alignment     = 1;

  Assert( etc );
  m_etc = etc;
  Assert( rec );
  m_rec = rec;
  m_ev = NULL;
  m_d = m_ahead = 0;
  m_nextPtr = 0;
  m_lockPtr1 = m_lockPtr2 = NULL;
  m_lockLen1 = m_lockLen2 = 0;
}

VoNCaptureBufferDS::~VoNCaptureBufferDS ()
{
  destroyBuffer();
}
	
void VoNCaptureBufferDS::registerCallbacks ()
{
  if ( !m_buffer ) return;                  // nothing to do..
  Assert( m_etc );
  if ( m_ev ) unregisterCallbacks();
  Verify( m_buffer->Stop() == DS_OK );

#if defined(DS_NOTIFICATION)

  // register event:
  m_ev = CreateEvent(NULL,FALSE,FALSE,NULL);
  m_etc->addEvent(m_ev,captureRoutine,this);

  // set capture-buffer notification:
  Assert( m_buffer );
  unsigned ptr = 0;
  unsigned delta = (format.bitsPerSample > 8) ? m_d + m_d : m_d;
  DSBPOSITIONNOTIFY posNotify[4];
  ComRef<IDirectSoundNotify> notify;
  if ( m_buffer->QueryInterface(IID_IDirectSoundNotify,(LPVOID*)notify.Init()) == DS_OK )
  {
    int i;
    for ( i = 0; i < 4; i++, ptr += delta ) // notify one event:
    {
      posNotify[i].dwOffset     = ptr;
      posNotify[i].hEventNotify = m_ev;
    }
    Verify( notify->SetNotificationPositions(4,posNotify) == DS_OK );
  }

#else //DS_NOTIFICATION

  // create & register the timer:
  m_ev = CreateWaitableTimer(NULL,FALSE,NULL);
  Assert( m_ev );
  m_etc->addEvent(m_ev,captureRoutine,this);
  LARGE_INTEGER start = { (int64)0 };
  SetWaitableTimer(m_ev,&start,DS_TIMER,NULL,NULL,FALSE);

#endif

  if ( !suspended )
    Verify( m_buffer->Start(DSCBSTART_LOOPING) == DS_OK );
}

void VoNCaptureBufferDS::unregisterCallbacks ()
{
  Assert( m_etc );
  Verify( m_buffer->Stop() == DS_OK );

#if defined(DS_NOTIFICATION)

    // unset capture-buffer notification:
  ComRef<IDirectSoundNotify> notify;
  if ( m_buffer->QueryInterface(IID_IDirectSoundNotify,(LPVOID*)notify.Init()) == DS_OK )
  {
    Verify( notify->SetNotificationPositions(0,NULL) == DS_OK );
  }

    // unregister event:
  if ( m_ev )
  {
    m_etc->removeEvent(m_ev);
    CloseHandle(m_ev);
    m_ev = NULL;
  }
#else
  if ( m_ev )
  {
    CancelWaitableTimer(m_ev);
    m_etc->removeEvent(m_ev);
    CloseHandle(m_ev);
    m_ev = NULL;
  }
#endif

  if ( !suspended )
    Verify( m_buffer->Start(DSCBSTART_LOOPING) == DS_OK );
}

void VoNCaptureBufferDS::suspendCallbacks ()
{
  Assert( m_etc );
  Assert( m_buffer );
  Verify( m_buffer->Stop() == DS_OK );
  if ( m_ev ) m_etc->enableEvent(m_ev,false);
}

void VoNCaptureBufferDS::resumeCallbacks ()
{
  Assert( m_etc );
  if ( m_ev ) m_etc->enableEvent(m_ev,true);
  Assert( m_buffer );
  Verify( m_buffer->Start(DSCBSTART_LOOPING) == DS_OK );
}

void VoNCaptureBufferDS::createBuffer ( unsigned len )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNCaptureBufferDS::createBuffer(): len=%u, freq=%d, bps=%d",
         len,(int)format.frequency,(int)format.bitsPerSample);
#endif
    // Waveform format:
  WAVEFORMATEX fmt;
  fmt.wFormatTag      = WAVE_FORMAT_PCM;
  fmt.nChannels       = 1;
  fmt.nSamplesPerSec  = format.frequency;
  fmt.wBitsPerSample  = (format.bitsPerSample > 8) ? 16 : 8;
  fmt.nBlockAlign     = fmt.wBitsPerSample >> 3;
  fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
  fmt.cbSize          = 0;
    // Sound-buffer description:
  DSCBUFFERDESC desc;
  Zero(desc);
  desc.dwSize          = sizeof(desc);
  desc.dwFlags         = 0;
  desc.dwBufferBytes   = len * fmt.nBlockAlign;
  desc.lpwfxFormat     = &fmt;
  desc.dwReserved      = 0;
  size = m_d = m_ahead = 0;
  m_ev = NULL;
  #if _ENABLE_MP
  HRESULT resCap = DirectSoundCaptureCreate(NULL,m_capture.Init(),NULL);
  #else
  HRESULT resCap = E_FAIL;
  #endif
  HRESULT resBuf = DS_OK;
  if ( resCap == DS_OK &&
       (resBuf = m_capture->CreateCaptureBuffer(&desc,m_buffer.Init(),NULL)) == DS_OK )
  {
      // set local variables:
    size = len;
    m_d = (len + 2) >> 2;                   // D = 1/4 of buffer length (in samples)
    m_ahead = (5 * m_d) >> 2;               // 5/4 of D
    m_nextPtr = 0;
      // assume our buffer iz zero-initialized:
#ifdef NET_LOG_VOICE
    NetLog("VoNCaptureBufferDS::createBuffer() succeeded");
#endif
  }
  else 
  {
    m_buffer.Free();
    m_capture.Free();
#ifdef NET_LOG_VOICE
    NetLog("VoNCaptureBufferDS::createBuffer() failed: 0x%x, 0x%x",(unsigned)resCap,(unsigned)resBuf);
#endif
  }
}

void VoNCaptureBufferDS::suspendBuffer ()
{
  if ( !m_buffer ) return;                  // nothing to do..
  suspendCallbacks();
  m_nextPtr = 0;
  suspended = true;
}

void VoNCaptureBufferDS::resumeBuffer ()
{
  if ( !m_buffer ) return;                  // nothing to do..
  resumeCallbacks();
  suspended = false;
}

void VoNCaptureBufferDS::destroyBuffer ()
{
  if ( !m_buffer ) return;                  // nothing to do..
#ifdef NET_LOG_VOICE
  NetLog("VoNCaptureBufferDS::destroyBuffer: m_capture.count=%d, m_buffer.count=%d",m_capture.GetRefCount(),m_buffer.GetRefCount());
#endif
  Verify( m_buffer->Stop() == DS_OK );
  unregisterCallbacks();
  size = m_d = m_ahead = 0;
  m_buffer.Free();
  m_capture.Free();
}

unsigned VoNCaptureBufferDS::newData ( unsigned &pos, unsigned minSamples, unsigned maxSamples )
{
  Assert( m_rec );
  return m_rec->encode(this,pos,minSamples,maxSamples);
}

void VoNCaptureBufferDS::advanceSafePosition ( unsigned inc )
{
  Assert( inc <= size );
  m_nextPtr += inc;
  if ( m_nextPtr >= size ) m_nextPtr -= size;
}

void VoNCaptureBufferDS::lockData ( unsigned pos, unsigned len,
                                    CircularBufferPointers &ptr )
{
  Zero(ptr);
  if ( m_lockPtr1 != NULL || m_lockPtr2 != NULL ||
       !m_buffer ) return;
  if ( format.bitsPerSample > 8 )
  {
    pos += pos;
    len += len;
  }
  Verify( m_buffer->Lock(pos,len,(void**)&ptr.ptr1.bps8,(unsigned long*)&ptr.len1,(void**)&ptr.ptr2.bps8,(unsigned long*)&ptr.len2,0) == DS_OK );
  m_lockPtr1 = (void*)ptr.ptr1.bps8;
  m_lockPtr2 = (void*)ptr.ptr2.bps8;
  m_lockLen1 = ptr.len1;
  m_lockLen2 = ptr.len2;
  if ( format.bitsPerSample > 8 )
  {
    ptr.len1 >>= 1;
    ptr.len2 >>= 1;
  }
}

void VoNCaptureBufferDS::unlockData ( unsigned processed, CircularBufferPointers &ptr )
{
  if ( !m_lockPtr1 || !m_buffer ) return;
  if ( format.bitsPerSample > 8 ) processed += processed;
  if ( m_lockLen1 >= processed )            // processing only in the 1st part
  {
    m_lockLen1 = processed;
    m_lockLen2 = 0;
  }
  else                                      // processing in both parts
  {
    processed -= m_lockLen1;
    Assert( processed <= m_lockLen2 );
    m_lockLen2 = processed;
  }
  Verify( m_buffer->Unlock(m_lockPtr1,m_lockLen1,m_lockPtr2,m_lockLen2) == DS_OK );
  m_lockPtr1 = m_lockPtr2 = NULL;
  m_lockLen1 = m_lockLen2 = 0;
}

#include "Es/Memory/normalNew.hpp"

void* VoNCaptureBufferDS::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNCaptureBufferDS::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNCaptureBufferDS::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  VoN-system:

VoNChannelId ExplicitMapTraits<VoNChannelId,RefD<VoNSoundBufferDS> >::keyNull = VOID_CHANNEL;
VoNChannelId ExplicitMapTraits<VoNChannelId,RefD<VoNSoundBufferDS> >::zombie  = RESERVED_CHANNEL;

VoNSystemDS::VoNSystemDS ( IMySound *dSound )
{
  if ( dSound )
    dSound->AddRef();
  m_ds << dSound;
    // create & init the ETC object:
  m_etc = new EventToCallback;
}

VoNSystemDS::~VoNSystemDS ()
{
  stopClient();
}

void VoNSystemDS::stopClient()
{
  m_capture = NULL;
  m_bufs.reset();
}

VoNResult VoNSystemDS::setReplay ( VoNChannelId chId, bool create )
  // this method is called by VoNClient when the replayer is created/destroyed.
  // Create: a sound-buffer has to be established and connected to the new replayer.
  // Destroy: sound-buffer (including its notification mechanism) has to be destroyed.
{
  Assert( m_etc );
  Assert( m_client );
  RefD<VoNSoundBufferDS> buf;

  if ( create )                             // creating
  {
    if ( IsDedicatedServer() )
    {
      ErrF("VoNSystemDS: cannot create replayer-object on dedicated server!");
      return VonError;
    }
    VoNReplayer *r = m_client->getReplayer(chId);
    if ( !r ) return VonError;              // unknown channel => ignore it
    if ( m_bufs.get(chId,buf) )             // buffer exists => error
    {
      ErrF("VoNSystemDS: Existing replayer buffer (%d)!",(int)chId);
      return VonError;
    }
      // new replayer buffer:
    Assert( m_ds );
    buf = new VoNSoundBufferDS(m_etc,r,m_ds);
    m_bufs.put(chId,buf);
      // prepare the buffer:
    buf->format.frequency     = r->getFrequency();
    buf->format.bitsPerSample = outBps;
    buf->createBuffer( (unsigned)( REPLAYER_BUFFER_D * r->getFrequency() ) << 2 );
    buf->registerCallbacks();               // start notification mechanism for the new buffer
    buf->resumeBuffer();
#ifdef NET_LOG_VOICE
    NetLog("VoNSystemDS: replayer object created for player=%d",chId);
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
      ErrF("VoNSystemDS: Unknown replayer buffer (%d)!",(int)chId);
      return VonError;
    }
#ifdef NET_LOG_VOICE
    NetLog("VoNSystemDS: replayer object destroyed for player=%d",chId);
#endif
  }

  return VonOK;
}

VoNResult VoNSystemDS::setSuspend ( VoNChannelId chId, bool suspend )
  // this method is called by VoNClient when the replayer is suspended/resumed.
  // Suspend: stop the sound-buffer replay & notification mechanism.
  // Resume: restart the sound-buffer replay & notification mechanism.
{
  RefD<VoNSoundBufferDS> buf;
  if ( !m_bufs.get(chId,buf) )
    return VonError;
#if defined(NET_LOG_VOICE) || DEBUG_VON
  NetLog("VoNSystemDS: replayer object %s for player=%d",suspend?"suspended":"resumed",chId);
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

VoNResult VoNSystemDS::setCapture ( bool on, int quality )
  // if called for the 1st time:
  // create VoNCaptureBufferDS & associate it with VoNRecorder
{
  Assert( m_etc );
  Assert( m_client );

  if ( !m_capture )                         // capture buffer was not created yet
  {
    if ( IsDedicatedServer() )
    {
      ErrF("VoNSystemDS: cannot create capture-object on dedicated server!");
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
    int freq = r->getFrequency();
    m_capture = new VoNCaptureBufferDS(m_etc,r);
      // prepare the buffer:
    m_capture->format.frequency     = freq;
    m_capture->format.bitsPerSample = inBps;
    m_capture->createBuffer( (unsigned)( CAPTURE_BUFFER_D * freq ) << 2 );
    m_capture->registerCallbacks();
#ifdef NET_LOG_VOICE
    NetLog("VoNSystemDS: capture buffer was created: %u Hz, %u bps",m_capture->format.frequency,m_capture->format.bitsPerSample);
#endif
  }

    // capture on/off:
  if ( on )
    m_capture->resumeBuffer();
  else
    m_capture->suspendBuffer();

  return VonOK;
}

RefD<VoNSoundBuffer> VoNSystemDS::getSoundBuffer ( VoNChannelId chId )
{
  RefD<VoNSoundBufferDS> buf;
  m_bufs.get(chId,buf);
  return buf.GetRef();
}

#endif  // VOICE_OVER_NET

#endif
