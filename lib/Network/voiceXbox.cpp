/**
  @file   voiceXbox.cpp
  @brief  Voice over Net objects for Xbox SDK API.

  Copyright &copy; 2003-2004 by BIStudio (www.bistudio.com)
  @author PE
  @since  4.6.2003
  @date   15.4.2004
*/

#include "../wpch.hpp"
#include "../global.hpp"

#if VOICE_OVER_NET

#ifdef _XBOX
// global diagnostics VoN variable
char GVoNSoundDiag[4096];
#endif

#if _GAMES_FOR_WINDOWS

#include "El/Network/netpch.hpp"
#include "voiceXbox.hpp"

VoNSystemXbox::VoNSystemXbox()
{
  LogF("*** VoNSystemXbox::VoNSystemXbox not implemented");
}

VoNSystemXbox::~VoNSystemXbox()
{
  LogF("*** VoNSystemXbox::~VoNSystemXbox not implemented");
}

void VoNSystemXbox::stopClient()
{
  LogF("*** VoNSystemXbox::stopClient not implemented");
}

void VoNSystemXbox::changePort(DWORD inPort, DWORD outPort, bool disablePlayback)
{
  LogF("*** VoNSystemXbox::changePort not implemented");
  LogF("    inPort %d, outPort %d, disablePlayback %s", inPort, outPort, disablePlayback ? "true" : "false");
}

VoNResult VoNSystemXbox::setReplay(VoNChannelId chId, bool create)
{
  LogF("*** VoNSystemXbox::setReplay not implemented");
  LogF("    channel %d, create %s", chId, create ? "true" : "false");
  return VonOK;
}

VoNResult VoNSystemXbox::setSuspend(VoNChannelId chId, bool suspend)
{
  LogF("*** VoNSystemXbox::setSuspend not implemented");
  LogF("    channel %d, suspend %s", chId, suspend ? "true" : "false");
  return VonOK;
}

VoNResult VoNSystemXbox::setCapture(bool on)
{
  LogF("*** VoNSystemXbox::setCapture not implemented");
  LogF("    capture %s", on ? "true" : "false");
  return VonOK;
}

VoNResult VoNSystemXbox::startRecorder()
{
  LogF("*** VoNSystemXbox::startRecorder not implemented");
  return VonOK;
}

int VoNSystemXbox::stopRecorder(AutoArray<char,MemAllocSafe> &buf)
{
  LogF("*** VoNSystemXbox::stopRecorder not implemented");
  return VonOK;
}

VoNResult VoNSystemXbox::startReplay(AutoArray<char,MemAllocSafe> &buf, int duration)
{
  LogF("*** VoNSystemXbox::startReplay not implemented");
  LogF("    duration %d", duration);
  return VonOK;
}

VoNResult VoNSystemXbox::getReplayStatus()
{
  LogF("*** VoNSystemXbox::getReplayStatus not implemented");
  return VonOK;
}

VoNResult VoNSystemXbox::stopReplay()
{
  LogF("*** VoNSystemXbox::stopReplay not implemented");
  return VonOK;
}

RefD<VoNSoundBuffer> VoNSystemXbox::getSoundBuffer(VoNChannelId chId)
{
  LogF("*** VoNSystemXbox::getSoundBuffer not implemented");
  LogF("    channel %d", chId);
  return NULL;
}

void VoNSystemXbox::changeVoiceMask(VoiceMask &mask)
{
  LogF("*** VoNSystemXbox::changeVoiceMask not implemented");
}

unsigned VoNSystemXbox::mixData(VoNSoundBuffer *sb, unsigned pos, unsigned minSamples, unsigned maxSamples)
{
  LogF("*** VoNSystemXbox::mixData not implemented");
  return 0;
}

#elif defined _XBOX

#include "voiceXbox.hpp"
#include <xvoice.h>

//----------------------------------------------------------------------
//  Replayer buffer for Xbox Communicator Headphone:

#ifdef HEAD_SEMAPHORE

void CALLBACK dpcHeadphone ( LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus )
{
  VoNSoundBufferHead *buf = (VoNSoundBufferHead*)pStreamContext;
#if _DEBUG
  if ( buf->m_magic != HEADPHONE_MAGIC )
  {
    XSaveFloatingPointStateForDpc();        // to be sure..
    LogF("dpcHeadphone alert!");
    XRestoreFloatingPointStateForDpc();
    return;
  }
#endif
  if ( buf->m_ev ) ReleaseSemaphore(buf->m_ev,1L,NULL);
}

#endif

void headReplayerRoutine ( HANDLE object, void *data )
{
  VoNSoundBufferHead *buf = (VoNSoundBufferHead*)data;
  Assert( buf );
  if ( buf->suspended ) return;
    // check if there is at least one empty frame:
  unsigned i = buf->m_headphonePacket;
    // find first unused packet:
  while ( buf->m_status[i] == XMEDIAPACKET_STATUS_PENDING )
  {
    if ( ++i >= PACKET_NUMBER_HEAD ) i = 0;
    if ( i == buf->m_headphonePacket ) break;
  }
  if ( buf->m_status[i] == XMEDIAPACKET_STATUS_PENDING ) return; // nothing to decode
    // at least one free packet:
  unsigned j = i + 1;
  unsigned bound = i + PACKET_NUMBER_HEAD;
  while ( buf->m_status[j % PACKET_NUMBER_HEAD] != XMEDIAPACKET_STATUS_PENDING )
    if ( ++j >= bound ) break;
  j -= i;                                   // packets available
  unsigned minDec = (j < PACKET_NUMBER_HEAD - 6) ? 0 : ( (j - (PACKET_NUMBER_HEAD - 7)) * PACKET_SIZE );
  unsigned maxDec = j * PACKET_SIZE;
  buf->m_headphonePacket = i;

  i *= PACKET_SIZE;
  unsigned decoded = buf->newData(i,minDec,maxDec);
  buf->advanceSafePosition( decoded );
#ifdef NET_LOG_VOICE_VERBOSE2
  NetLog("headReplayerRoutine: next=%u, min=%u, max=%u, decoded=%u",i,minDec,maxDec,decoded);
#endif
  if ( buf->m_error )
    buf->suspendBuffer();
}

VoNSoundBufferHead::VoNSoundBufferHead ( EventToCallback *etc, DWORD port, VoNSystemXbox *von )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNSoundBufferHead::VoNSoundBufferHead");
#endif
  input = false;
  suspended = true;
  size = 0;
    // default sound format:
  format.bitsPerSample = 16;
  format.frequency     = 8000;
  format.granularity   = 1;
  format.alignment     = 1;

  Assert( etc );
  m_etc = etc;
  m_ev = NULL;
  Assert( von );
  m_von = von;
#if _DEBUG
  m_magic = HEADPHONE_MAGIC;
#endif
  m_headphonePacket = 0;                    // next packet to be processed (replayed)
  Assert( port < COMMUNICATOR_PORTS );
  m_port = port;
  m_error = false;
}

VoNSoundBufferHead::~VoNSoundBufferHead ()
{
  destroyBuffer();
}

void VoNSoundBufferHead::registerCallbacks ()
{
  Assert( m_etc );
  if ( m_ev ) unregisterCallbacks();
#ifdef HEAD_SEMAPHORE
    // register the semaphore:
  m_ev = CreateSemaphore(NULL,0,144,NULL);
  Assert( m_ev );
  m_etc->addEvent(m_ev,headReplayerRoutine,this);
#else     // timer!
    // create & register the timer:
  m_ev = CreateWaitableTimer(NULL,FALSE,NULL);
  Assert( m_ev );
  m_etc->addEvent(m_ev,headReplayerRoutine,this);
  LARGE_INTEGER start = { (int64)0 };
  SetWaitableTimer(m_ev,&start,HEADPHONE_TIMER,NULL,NULL,FALSE);
#endif
  suspended = false;
}

void VoNSoundBufferHead::unregisterCallbacks ()
{
  Assert( m_etc );
    // unregister semaphore:
  if ( m_ev )
  {
#ifndef HEAD_SEMAPHORE
    CancelWaitableTimer(m_ev);
#endif
    m_etc->removeEvent(m_ev);
    CloseHandle(m_ev);
    m_ev = NULL;
  }
}

void VoNSoundBufferHead::suspendCallbacks ()
{
  Assert( m_etc );
  if ( m_ev ) m_etc->enableEvent(m_ev,false);
}

void VoNSoundBufferHead::resumeCallbacks ()
{
  m_error = false;
  Assert( m_etc );
  if ( m_ev ) m_etc->enableEvent(m_ev,true);
}

void VoNSoundBufferHead::createBuffer ( unsigned len )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNSoundBufferHead::createBuffer(): len=%u, freq=%d, bps=%d",
         len,(int)format.frequency,(int)format.bitsPerSample);
#endif

    // Waveform format (ignoring format member):
  WAVEFORMATEX fmt;
  fmt.wFormatTag      = WAVE_FORMAT_PCM;
  fmt.nChannels       = 1;
  fmt.nSamplesPerSec  = 8000;
  fmt.wBitsPerSample  = 16;
  fmt.nBlockAlign     = fmt.wBitsPerSample >> 3;
  fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
  fmt.cbSize          = 0;

  size = 0;
  suspended = true;
  m_error = false;
  Zero(m_buffer);                           // initial silence
  HRESULT result;
#ifdef HEAD_SEMAPHORE
  result = XVoiceCreateMediaObjectEx(XDEVICE_TYPE_VOICE_HEADPHONE,m_port,PACKET_NUMBER_HEAD,&fmt,
                                     &dpcHeadphone,this,m_headPhone.Init());
#else
  result = XVoiceCreateMediaObjectEx(XDEVICE_TYPE_VOICE_HEADPHONE,m_port,PACKET_NUMBER_HEAD,&fmt,
                                     NULL,NULL,m_headPhone.Init());
#endif
  if ( result != S_OK )
    return;

  size = sizeof(m_buffer) / sizeof(m_buffer[0]);
    // all headphone media packets are free:
  int i;
  for ( i = 0; i < PACKET_NUMBER_HEAD; i++ )
    m_status[i] = XMEDIAPACKET_STATUS_SUCCESS;
}

void VoNSoundBufferHead::suspendBuffer ()
{
  if ( !m_headPhone ) return;               // nothing to do..
  suspended = true;
  suspendCallbacks();
}

void VoNSoundBufferHead::resumeBuffer ()
{
  if ( !m_headPhone ) return;               // nothing to do..
  suspended = false;
  resumeCallbacks();
}

void VoNSoundBufferHead::destroyBuffer ()
{
  if ( !m_headPhone ) return;               // nothing to do..
#ifdef NET_LOG_VOICE
  NetLog("VoNSoundBufferHead::destroyBuffer");
#endif
  suspended = true;
  unregisterCallbacks();
  size = 0;
  m_headPhone.Free();
}

unsigned VoNSoundBufferHead::newData ( unsigned &pos, unsigned minSamples, unsigned maxSamples )
{
  Assert( m_von );
    // ask slave buffers to decode & mix another bunch of data:
  return m_von->mixData(this,pos,minSamples,maxSamples);
}

void VoNSoundBufferHead::advanceSafePosition ( unsigned inc )
{
  Assert( inc <= size );
  Assert( (inc % PACKET_SIZE) == 0 );
  Assert( m_headPhone );

  inc /= PACKET_SIZE;                       // number of packets
  while ( inc-- )                           // send one packet to headphone
  {
    XMEDIAPACKET xmp;
    Zero(xmp);
    xmp.pvBuffer         = m_buffer + m_headphonePacket * PACKET_SIZE;
    xmp.dwMaxSize        = PACKET_SIZE * 2;
    xmp.pdwCompletedSize = NULL;
    xmp.pdwStatus        = m_status + m_headphonePacket;
#ifdef HEAD_SEMAPHORE
    xmp.pContext         = this;
#endif
    HRESULT result = m_headPhone->Process(&xmp,NULL);
    if ( result != DS_OK )
    {
      m_error = true;
      break;
    }
    if ( ++m_headphonePacket >= PACKET_NUMBER_HEAD ) m_headphonePacket = 0;
  }
}

void VoNSoundBufferHead::lockData ( unsigned pos, unsigned len,
                                    CircularBufferPointers &ptr )
{
  Zero(ptr);
  if ( len > size ) len = size;
  ptr.ptr1.bps16 = m_buffer + pos;
  ptr.len1 = len;
  if ( pos + len > size )
  {
    ptr.len1 = size - pos;
    ptr.ptr2.bps16 = m_buffer;
    ptr.len2 = len - ptr.len1;
  }
}

void VoNSoundBufferHead::unlockData ( unsigned processed, CircularBufferPointers &ptr )
{
}

#include "Es/Memory/normalNew.hpp"

void* VoNSoundBufferHead::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNSoundBufferHead::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNSoundBufferHead::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  Slave replayer buffer:

VoNSoundBufferSlave::VoNSoundBufferSlave ( VoNReplayer *repl )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNSoundBufferSlave::VoNSoundBufferSlave");
#endif
  input = false;
  suspended = true;
  size = 0;
    // default sound format:
  format.bitsPerSample = 16;
  format.frequency     = 8000;
  format.granularity   = 1;
  format.alignment     = 1;

  Assert( repl );
  m_repl = repl;
  m_nextPacket = m_lastPacket = 0;          // next packet to be processed (replayed)
}

VoNSoundBufferSlave::~VoNSoundBufferSlave ()
{
  destroyBuffer();
}

void VoNSoundBufferSlave::createBuffer ( unsigned len )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNSoundBufferSlave::createBuffer(): len=%u, freq=%d, bps=%d",
         len,(int)format.frequency,(int)format.bitsPerSample);
#endif
  size = sizeof(m_buffer) / sizeof(m_buffer[0]);
  suspended = true;
  m_nextPacket = m_lastPacket = 0;          // next packet to be processed (replayed)

  Zero(m_buffer);                           // initial silence
}

void VoNSoundBufferSlave::suspendBuffer ()
{
  suspended = true;
  m_nextPacket = m_lastPacket = 0;          // next packet to be processed (replayed)
}

void VoNSoundBufferSlave::resumeBuffer ()
{
  suspended = false;
}

void VoNSoundBufferSlave::destroyBuffer ()
{
#ifdef NET_LOG_VOICE
  NetLog("VoNSoundBufferSlave::destroyBuffer");
#endif
  suspended = true;
  m_nextPacket = m_lastPacket = 0;          // to be sure
  size = 0;
}

unsigned VoNSoundBufferSlave::newData ( unsigned &pos, unsigned minSamples, unsigned maxSamples )
  // I'm ignoring the "pos" parameter..
{
  unsigned ready = PACKET_SIZE * ( (m_lastPacket >= m_nextPacket) ? m_lastPacket - m_nextPacket : m_lastPacket + PACKET_NUMBER_HEAD - m_nextPacket);
  if ( ready < minSamples )                 // I need another data..
  {
    Assert( m_repl );
    unsigned newSamples = m_repl->decode(this,m_lastPacket*PACKET_SIZE,minSamples-ready,size-ready);
    ready += newSamples;
    if ( (m_lastPacket += newSamples / PACKET_SIZE) >= PACKET_NUMBER_HEAD )
      m_lastPacket -= PACKET_NUMBER_HEAD;
    Assert( ready >= minSamples );
  }
  if ( ready > maxSamples )
    ready = maxSamples;
  pos = m_nextPacket * PACKET_SIZE;
  return ready;
}

void VoNSoundBufferSlave::advanceSafePosition ( unsigned inc )
{
  Assert( inc <= size );
  Assert( (inc % PACKET_SIZE) == 0 );

  inc /= PACKET_SIZE;                       // number of packets
  if ( (m_nextPacket += inc) >= PACKET_NUMBER_HEAD )
    m_nextPacket -= PACKET_NUMBER_HEAD;
}

void VoNSoundBufferSlave::lockData ( unsigned pos, unsigned len,
                                     CircularBufferPointers &ptr )
{
  Zero(ptr);
  if ( len > size ) len = size;
  ptr.ptr1.bps16 = m_buffer + pos;
  ptr.len1 = len;
  if ( pos + len > size )
  {
    ptr.len1 = size - pos;
    ptr.ptr2.bps16 = m_buffer;
    ptr.len2 = len - ptr.len1;
  }
}

void VoNSoundBufferSlave::unlockData ( unsigned processed, CircularBufferPointers &ptr )
{
}

#include "Es/Memory/normalNew.hpp"

void* VoNSoundBufferSlave::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNSoundBufferSlave::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNSoundBufferSlave::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  Capture buffer:

void CALLBACK dpcMicrophone ( LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus )
{
  VoNCaptureBufferMic *buf = (VoNCaptureBufferMic*)pStreamContext;
#if _DEBUG
  if ( buf->m_magic != MICROPHONE_MAGIC )
  {
    XSaveFloatingPointStateForDpc();        // to be sure..
    LogF("dpcMicrophone alert!");
    XRestoreFloatingPointStateForDpc();
    return;
  }
#endif
  if ( buf->m_ev ) ReleaseSemaphore(buf->m_ev,1L,NULL);
}

void micCaptureRoutine ( HANDLE object, void *data )
{
  VoNCaptureBufferMic *buf = (VoNCaptureBufferMic*)data;
  Assert( buf );
  if ( buf->suspended ) return;
    // look for captured frames:
  unsigned i = buf->m_micPacket;
  unsigned j = i;
  unsigned bound = j + PACKET_NUMBER_MIC;
  while ( buf->m_status[j % PACKET_NUMBER_MIC] != XMEDIAPACKET_STATUS_PENDING )
    if ( ++j >= bound ) break;
  if ( j == i ) return;                     // nothing to encode
  j -= i;                                   // packets available
  unsigned minEnc = (j >= PACKET_NUMBER_MIC - 1) ? PACKET_SIZE : 0;
  unsigned maxEnc = j * PACKET_SIZE;

  i *= PACKET_SIZE;
#ifdef NET_LOG_VOICE_VERBOSE
  static Counter;
  if ( !(++Counter & 0x1f) )
    NetLog("micCaptureRoutine: next=%u, min=%u, max=%u",i,minEnc,maxEnc);
#endif
    // ask recorder object to encode another bunch of data:
  buf->advanceSafePosition( buf->newData(i,minEnc,maxEnc) );
  if ( buf->m_error )
  {
    buf->suspendBuffer();
  }
}

VoNCaptureBufferMic::VoNCaptureBufferMic ( EventToCallback *etc, VoNRecorder *rec, DWORD port )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNCaptureBufferMic::VoNCaptureBufferMic");
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
#if _DEBUG
  m_magic = MICROPHONE_MAGIC;
#endif
  m_micPacket = 0;                          // next packet to be processed (encoded)
  Assert( port < COMMUNICATOR_PORTS );
  m_port = port;
  m_error = false;
}

VoNCaptureBufferMic::~VoNCaptureBufferMic ()
{
  destroyBuffer();
}

void VoNCaptureBufferMic::setRecorder ( VoNRecorder *rec )
{
  Assert( rec );
  m_rec = rec;
}

void VoNCaptureBufferMic::registerCallbacks ()
{
  Assert( m_etc );
  if ( m_ev ) unregisterCallbacks();

    // register the semaphore:
  m_ev = CreateSemaphore(NULL,0,100,NULL);
  m_etc->addEvent(m_ev,micCaptureRoutine,this);
}

void VoNCaptureBufferMic::unregisterCallbacks ()
{
  Assert( m_etc );
    // unregister semaphore:
  if ( m_ev )
  {
    m_etc->removeEvent(m_ev);
    CloseHandle(m_ev);
    m_ev = NULL;
  }
}

void VoNCaptureBufferMic::suspendCallbacks ()
{
  Assert( m_etc );
  if ( m_ev ) m_etc->enableEvent(m_ev,false);
}

void VoNCaptureBufferMic::resumeCallbacks ()
{
  Assert( m_etc );
  m_error = false;
  if ( m_ev ) m_etc->enableEvent(m_ev,true);
}

void VoNCaptureBufferMic::createBuffer ( unsigned len )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNCaptureBufferMic::createBuffer(): len=%u, freq=%d, bps=%d",
         len,(int)format.frequency,(int)format.bitsPerSample);
#endif

    // Waveform format (ignoring format member):
  WAVEFORMATEX fmt;
  fmt.wFormatTag      = WAVE_FORMAT_PCM;
  fmt.nChannels       = 1;
  fmt.nSamplesPerSec  = 8000;
  fmt.wBitsPerSample  = 16;
  fmt.nBlockAlign     = fmt.wBitsPerSample >> 3;
  fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
  fmt.cbSize          = 0;

  size = 0;
  suspended = true;
  m_micPacket = 0;
  HRESULT result;
  result = XVoiceCreateMediaObjectEx(XDEVICE_TYPE_VOICE_MICROPHONE,m_port,PACKET_NUMBER_MIC,&fmt,
                                     &dpcMicrophone,this,m_microPhone.Init());
  if ( result != S_OK )
    return;

  size = sizeof(m_buffer) / sizeof(m_buffer[0]);
  m_error = false;
    // Seed the microphone device with all our media packets
  int i;
  for ( i = 0; i < PACKET_NUMBER_MIC; i++ )
  {
    XMEDIAPACKET xmp;
    Zero(xmp);
    xmp.dwMaxSize           = PACKET_SIZE * 2;
    xmp.pdwCompletedSize    = NULL;
    xmp.pContext            = this;
    xmp.prtTimestamp        = NULL;
    xmp.pvBuffer            = m_buffer + i * PACKET_SIZE;
    xmp.pdwStatus           = m_status + i;
    HRESULT result = m_microPhone->Process(NULL,&xmp);
    if ( result != DS_OK )
    {
      m_error = true;
      break;
    }
  }
}

void VoNCaptureBufferMic::suspendBuffer ()
{
  if ( !m_microPhone ) return;              // nothing to do..
  suspended = true;
  suspendCallbacks();
}

void VoNCaptureBufferMic::resumeBuffer ()
{
  if ( !m_microPhone ) return;              // nothing to do..
  suspended = false;
  resumeCallbacks();
}

void VoNCaptureBufferMic::destroyBuffer ()
{
  if ( !m_microPhone ) return;              // nothing to do..
#ifdef NET_LOG_VOICE
  NetLog("VoNCaptureBufferMic::destroyBuffer");
#endif
  suspended = true;
  unregisterCallbacks();
  size = 0;
  m_microPhone.Free();
}

unsigned VoNCaptureBufferMic::newData ( unsigned &pos, unsigned minSamples, unsigned maxSamples )
{
  Assert( m_rec );
  return m_rec->encode(this,pos,minSamples,maxSamples);
}

void VoNCaptureBufferMic::advanceSafePosition ( unsigned inc )
{
  Assert( inc <= size );
  Assert( (inc % PACKET_SIZE) == 0 );
  Assert( m_microPhone );

  inc /= PACKET_SIZE;                       // number of packets
  while ( inc-- )                           // resubmit one packet to microphone
  {
    XMEDIAPACKET xmp;
    Zero(xmp);
    xmp.pvBuffer         = m_buffer + m_micPacket * PACKET_SIZE;
    xmp.dwMaxSize        = PACKET_SIZE * 2;
    xmp.pdwCompletedSize = NULL;
    xmp.pdwStatus        = m_status + m_micPacket;
    xmp.pContext         = this;
    HRESULT result = m_microPhone->Process(NULL,&xmp);
    if ( result != DS_OK )
    {
      m_error = true;
      break;
    }
    if ( ++m_micPacket >= PACKET_NUMBER_MIC ) m_micPacket = 0;
  }
}

void VoNCaptureBufferMic::lockData ( unsigned pos, unsigned len,
                                     CircularBufferPointers &ptr )
{
  Zero(ptr);
  if ( len > size ) len = size;
  ptr.ptr1.bps16 = m_buffer + pos;
  ptr.len1 = len;
  if ( pos + len > size )
  {
    ptr.len1 = size - pos;
    ptr.ptr2.bps16 = m_buffer;
    ptr.len2 = len - ptr.len1;
  }
}

void VoNCaptureBufferMic::unlockData ( unsigned processed, CircularBufferPointers &ptr )
{
}

#include "Es/Memory/normalNew.hpp"

void* VoNCaptureBufferMic::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNCaptureBufferMic::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNCaptureBufferMic::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  VoN-system:

VoNChannelId ExplicitMapTraits<VoNChannelId,RefD<VoNSoundBuffer> >::keyNull = VOID_CHANNEL;
VoNChannelId ExplicitMapTraits<VoNChannelId,RefD<VoNSoundBuffer> >::zombie  = RESERVED_CHANNEL;

VoNSystemXbox::VoNSystemXbox ( IMySound *dSound, DWORD inPort, DWORD outPort )
{
  if ( dSound )
    dSound->AddRef();
  m_ds = dSound;
  m_inPort = inPort;
  m_outPort = outPort;
  m_forcedPort = false;
  _useHeadphone = (m_outPort < COMMUNICATOR_PORTS);
  _disablePlayback = false;
  // create & init the ETC object:
  m_etc = new EventToCallback;
  Assert( m_etc );
#ifdef COM_SHARED
    // Shared COM objects
  Verify( XVoiceCreateOneToOneEncoder(m_encoder.Init()) == S_OK );
  Verify( XVoiceCreateOneToOneDecoder(m_decoder.Init()) == S_OK );
#endif
}

VoNSystemXbox::~VoNSystemXbox ()
{
  stopClient();
}

void VoNSystemXbox::stopClient ()
{
    // destroy capturing side:
  m_capture = NULL;
  m_recorder = NULL;
  m_memRecorder = NULL;
    // destroy replaying side:
  enter();                                  // to protect "bufs"
  m_bufs.reset();
  leave();
  m_memBuffer = NULL;
  m_master = NULL;
  m_memReplayer = NULL;
}

void VoNSystemXbox::createMaster ()
  // The old master-buffer is destroyed automatically..
{
  m_master = new VoNSoundBufferHead(m_etc,m_outPort,this);
  Assert( m_master );
  m_master->format.frequency     = 8000;
  m_master->format.bitsPerSample = 16;
  m_master->createBuffer( PACKET_SIZE * PACKET_NUMBER_HEAD );
  m_master->registerCallbacks();            // start notification mechanism for the new buffer
  m_master->resumeBuffer();
}

void VoNSystemXbox::createNilBuffer ( VoNChannelId chId, bool suspended )
{
  Assert( m_client );
  VoNReplayer *r = (chId == VOID_CHANNEL) ? m_memReplayer.GetRef() : m_client->getReplayer(chId);
  VoNSoundBufferNil *nb = new VoNSoundBufferNil(m_etc,r);
  Assert( nb );
  RefD<VoNSoundBuffer> newB(nb);
  if ( chId == VOID_CHANNEL )
    m_memBuffer = newB;
  else
    m_bufs.put(chId,newB);

  // format and buffer size does not mater - but we do no harm by setting them
  nb->format.frequency     = 8000;
  nb->format.bitsPerSample = 16;
  
  nb->createBuffer( (3 * PACKET_SIZE) << 2 ); // one buffer tick: 3*40 = 120ms
  if ( !suspended )
    nb->resumeBuffer();
}

void VoNSystemXbox::createDSBuffer ( VoNChannelId chId, bool suspended )
{
  Assert( m_client );
  VoNReplayer *r = (chId == VOID_CHANNEL) ? m_memReplayer.GetRef() : m_client->getReplayer(chId);
  VoNSoundBufferDS *nb = new VoNSoundBufferDS(m_etc,r,m_ds);
  Assert( nb );
  RefD<VoNSoundBuffer> newB(nb);
  if ( chId == VOID_CHANNEL )
    m_memBuffer = newB;
  else
    m_bufs.put(chId,newB);

    // prepare the buffer:
  nb->format.frequency     = 8000;
  nb->format.bitsPerSample = 16;
  nb->createBuffer( (3 * PACKET_SIZE) << 2 ); // one buffer tick: 3*40 = 120ms
  nb->registerCallbacks();                  // start notification mechanism for the new buffer
  if ( !suspended )
    nb->resumeBuffer();
}

void VoNSystemXbox::createHeadBuffer ( VoNChannelId chId, bool suspended )
{
  Assert( m_client );
  VoNReplayer *r = (chId == VOID_CHANNEL) ? m_memReplayer.GetRef() : m_client->getReplayer(chId);
  VoNSoundBufferSlave *nb = new VoNSoundBufferSlave(r);
  Assert( nb );
  RefD<VoNSoundBuffer> newB(nb);
  if ( chId == VOID_CHANNEL )
    m_memBuffer = newB;
  else
    m_bufs.put(chId,newB);

    // prepare the buffer:
  nb->format.frequency     = 8000;
  nb->format.bitsPerSample = 16;
  nb->createBuffer( PACKET_SIZE * PACKET_NUMBER_HEAD );
  if ( !suspended )
    nb->resumeBuffer();
}

VoNResult VoNSystemXbox::setReplay ( VoNChannelId chId, bool create )
  // this method is called by VoNClient when the replayer is created/destroyed.
  // Create: a sound-buffer has to be established and connected to the new replayer.
  // Destroy: sound-buffer (including its notification mechanism) has to be destroyed.
{
  Assert( m_etc );
  Assert( m_client );
  RefD<VoNSoundBuffer> buf;

  enter();
  if ( create )                             // creating
  {
    VoNReplayer *r = m_client->getReplayer(chId);
    if ( !r )                               // unknown channel => ignore it
    {
      leave();
      return VonError;
    }
    if ( m_bufs.get(chId,buf) )             // buffer exists => error
    {
      leave();
      ErrF("VoNSystemXbox: Existing replayer buffer (%d)!",(int)chId);
      return VonError;
    }

#ifdef COM_SHARED
      // Set shared decoder COM object
    VoNCodec *codec = r->getCodec();
    Assert( codec );
    if ( codec )
      codec->setProperty(1,m_decoder);
#endif

    // new replayer buffer:
    if (_disablePlayback)
    {
      // use dummy buffer to discard data
      if (m_master) m_master = NULL;
      createNilBuffer(chId,false);
    }
    else if ( _useHeadphone )
    {
      // use communicator buffer
      // create master buffer for the headphone if necessary
      if ( !m_master )
        createMaster();
      createHeadBuffer(chId,false);
    }
    else
    {
      // use regular DirectSound buffer
      if (m_master) m_master = NULL;
      createDSBuffer(chId,false);
    }

#ifdef NET_LOG_VOICE
    NetLog("VoNSystemXbox: replayer object created for player=%d, %s",chId,_useHeadphone?"headphone":"TV");
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
      leave();
      ErrF("VoNSystemXbox: Unknown replayer buffer (%d)!",(int)chId);
      return VonError;
    }

#ifdef NET_LOG_VOICE
    NetLog("VoNSystemXbox: replayer object destroyed for player=%d",chId);
#endif
  }

  leave();
  return VonOK;
}

VoNResult VoNSystemXbox::setSuspend ( VoNChannelId chId, bool suspend )
  // this method is called by VoNClient when the replayer is suspended/resumed.
  // Suspend: stop the sound-buffer replay & notification mechanism.
  // Resume: restart the sound-buffer replay & notification mechanism.
{
  Assert( m_etc );
  Assert( m_client );
  RefD<VoNSoundBuffer> buf;
  enter();
  if ( !m_bufs.get(chId,buf) )
  {
    leave();
    return VonError;
  }
  leave();
#ifdef NET_LOG_VOICE
  NetLog("VoNSystemXbox: replayer object %s for player=%d",suspend?"suspended":"resumed",chId);
#endif

  if ( suspend )                            // suspend
    buf->suspendBuffer();
  else                                      // resume
    buf->resumeBuffer();

  return VonOK;
}

VoNResult VoNSystemXbox::setCapture ( bool on )
  // if called for the 1st time:
  // create VoNCaptureBufferMic & associate it with VoNRecorder
{
  Assert( m_etc );
  Assert( m_client );
#ifdef NET_LOG_VOICE
  NetLog("VoNSystemXbox::setCapture: inPort=%d, on=%d",(int)m_inPort,(int)on);
#endif

  if ( !m_capture && m_inPort < COMMUNICATOR_PORTS ) // capture buffer was not created yet
  {
    if ( IsDedicatedServer() )
    {
      return VonOK;
    }
    if ( !m_recorder )                      // voice was swithed on for the 1st time => create codec & recorder
    {
      CodecInfo info;
      Zero(info);
        // set default codec parameters:
      strcpy(info.name,"Microsoft");
      info.type = ScVoice;
      VoNCodec *codec = findCodec(info);
      Assert( codec );
#ifdef NET_LOG_VOICE
      NetLog("VoNSystemXbox::setCapture(%d)",(int)on);
#endif
#ifdef COM_SHARED
      codec->setProperty(0,m_encoder);
#endif
      codec->setVoiceMask(defaultVoiceMask);
      m_recorder = new VoNRecorder(m_client,m_client->outChannel,codec, GSoundsys?GSoundsys->GetVoNRecThreshold():VoNRecorder::DEFAULT_REC_THRESHOLD);
      m_client->setRecorder(m_recorder);
      m_recorder->setCapture(on);
    }
    Assert( m_recorder );
    m_capture = new VoNCaptureBufferMic(m_etc,m_recorder,m_inPort);
      // prepare the buffer:
    m_capture->format.frequency     = 8000;
    m_capture->format.bitsPerSample = 16;
    m_capture->createBuffer( PACKET_SIZE * PACKET_NUMBER_MIC );
    m_capture->registerCallbacks();
#ifdef NET_LOG_VOICE
    NetLog("VoNSystemXbox: capture buffer was created: %u Hz, %u bps",m_capture->format.frequency,m_capture->format.bitsPerSample);
#endif
  }

  if ( m_capture )
      // capture on/off:
    if ( on )
      m_capture->resumeBuffer();
    else
      m_capture->suspendBuffer();

  return VonOK;
}

void VoNSystemXbox::changeVoiceMask ( VoiceMask &mask )
{
  defaultVoiceMask = mask;
  if ( m_client.NotNull() &&
       m_recorder.NotNull() )               // capturing => change the actual mask..
  {
    VoNCodec *codec = m_recorder->getCodec();
    if ( codec )
      codec->setVoiceMask(defaultVoiceMask);
  }
}

VoNResult VoNSystemXbox::startRecorder ()
{
  if ( !m_client || m_inPort >= COMMUNICATOR_PORTS ) return VonError;
  if ( m_memRecorder ) return VonBusy;      // already in memory-recording mode

    // disable regular network-recording:
  m_micSuspended = true;
  if ( m_capture ) m_micSuspended = m_capture->suspended;
  setCapture(false);

    // prepare memory-based recorder:
  Assert( m_recorder );
  Assert( m_capture );
  CodecInfo info;
  Zero(info);
    // set default codec parameters:
  strcpy(info.name,"Microsoft");
  info.type = ScVoice;
  VoNCodec *codec = findCodec(info);
  Assert( codec );
#ifdef COM_SHARED
  codec->setProperty(0,m_encoder);
#endif
  codec->setVoiceMask(defaultVoiceMask);
  m_memRecorder = new VoNMemoryRecorder(codec);
  m_capture->setRecorder(m_memRecorder);

    // ..and set recording mode again:
  setCapture(true);
  return VonOK;
}

int VoNSystemXbox::stopRecorder ( AutoArray<char,MemAllocSafe> &buf )
{
  if ( !m_client || m_inPort >= COMMUNICATOR_PORTS || !m_memRecorder )
  {                                         // not in memory-recording mode
    buf.Clear();
    return 0;
  }
  Assert( m_recorder );
  Assert( m_capture );
  setCapture(false);

    // retrieve the recorded data:
  buf = m_memRecorder->getData();
  int millis = m_memRecorder->getSamples() / 8; // presume 8kHz recording rate

    // return back to previous network-recording mode:
  m_capture->setRecorder(m_recorder);
  m_memRecorder = NULL;
  setCapture(!m_micSuspended);

  return millis;
}

VoNResult VoNSystemXbox::startReplay ( AutoArray<char,MemAllocSafe> &buf, int duration )
{
  Assert( m_etc );
  if ( !m_client ) return VonError;
  stopReplay();

    // create codec instance:
  CodecInfo info;
  Zero(info);
    // set default codec parameters:
  strcpy(info.name,"Microsoft");
  info.type = ScVoice;
  VoNCodec *codec = findCodec(info);
  Assert( codec );
#ifdef COM_SHARED
  codec->setProperty(1,m_decoder);
#endif
  codec->setVoiceMask(defaultVoiceMask);

    // create replayer instance:
  m_memReplayer = new VoNMemoryReplayer(codec);
  Assert( m_memReplayer );
  m_memReplayer->setData(buf,duration);

    // new replayer buffer (force speakers):
  changePort(m_inPort,-1,false);
  m_forcedPort = true;
  createDSBuffer(VOID_CHANNEL,false);

  return VonOK;
}

VoNResult VoNSystemXbox::getReplayStatus ()
{
  if ( !m_memReplayer ) return VonError;
  return m_memReplayer->getStatus();
}

VoNResult VoNSystemXbox::stopReplay ()
{
  if ( !m_memReplayer ) return VonError;
  Assert( m_memBuffer );
  m_memBuffer->destroyBuffer();
  m_memBuffer = NULL;
  m_memReplayer = NULL;
  m_forcedPort = false;
  return VonOK;
}

void VoNSystemXbox::changePort ( DWORD inPort, DWORD outPort, bool disablePlayback )
{
  bool chHeadphone = (m_outPort != outPort) && !m_forcedPort;
  bool chMicrophone = m_inPort != inPort;
  bool chDisable = disablePlayback != _disablePlayback;
  if ( !chHeadphone && !chMicrophone && !chDisable) return;
#ifdef NET_LOG_VOICE
  NetLog("VoNSystemXbox::changePort: in(from=%d, to=%d), out(from=%d, to=%d)",
         (int)m_inPort,(int)inPort,(int)m_outPort,(int)outPort);
#endif
  bool newHeadphone = ( outPort < COMMUNICATOR_PORTS );
  if ( chMicrophone ) m_inPort = inPort;
  if ( chHeadphone )  m_outPort = outPort;
  if ( !m_client ) return;

    // capture:
  if ( chMicrophone )
  {
    if ( m_capture )                        // destroy the old capture object
    {
      m_capture->destroyBuffer();
      m_capture = NULL;
    }
    if ( m_inPort < COMMUNICATOR_PORTS )    // create new capture object
    {
      setCapture(true);
    }
  }

    // playback:
  if ( chHeadphone || chDisable)
  {
    if (chHeadphone)
    {
      if ( _useHeadphone && newHeadphone )     // I need to change master replayer-buffer only
        createMaster();
    }

    if ( _useHeadphone != newHeadphone || chDisable)     // "_useHeadphone" switch
    {
      IteratorState it;
      VoNChannelId chId;
      bool suspended;
      RefD<VoNSoundBuffer> oldB;
      enter();

      if (disablePlayback)
      {
        m_master = NULL;
        // No playback
        // set buffers to VoNSoundBufferNil-s:
        if ( m_bufs.getFirst(it,oldB,&chId) )
          do
          {
            suspended = oldB->suspended;
            oldB->suspendBuffer();          // to be sure
            createNilBuffer(chId,suspended);
          } while ( m_bufs.getNext(it,oldB,&chId) );
      }
      else if ( !newHeadphone )
      {
        m_master = NULL;
        // Direct Sound (speakers) playback
        // set buffers to VoNSoundBufferDS-s:
        if ( m_bufs.getFirst(it,oldB,&chId) )
          do
          {
            suspended = oldB->suspended;
            oldB->suspendBuffer();          // to be sure
            createDSBuffer(chId,suspended);
          } while ( m_bufs.getNext(it,oldB,&chId) );
      }
      else
      {
        // Communicator playback
        // set buffers to VoNSoundBufferSlave-s:
        if ( m_bufs.getFirst(it,oldB,&chId) )
          do
          {
            suspended = oldB->suspended;
            oldB->suspendBuffer();          // to be sure
            createHeadBuffer(chId,suspended);
          } while ( m_bufs.getNext(it,oldB,&chId) );
        createMaster();
      }

      leave();
      _useHeadphone = newHeadphone;
      _disablePlayback = disablePlayback;
    }
  }
}

unsigned VoNSystemXbox::mixData ( VoNSoundBuffer *sb, unsigned pos, unsigned minSamples, unsigned maxSamples )
{
  Assert( sb );
    // ask slave buffers to decode & mix another bunch of data:
  int active = 0;
  IteratorState it;
  RefD<VoNSoundBuffer> buf;
  m_bufs.getFirst(it,buf);
  while ( it != ITERATOR_NULL )
  {
    if ( !buf->suspended ) active++;
    m_bufs.getNext(it,buf);
  }

    // there are "active" replayers:
  unsigned prepared = 0;                    // maximum number of samples
  CircularBufferPointers acc, tmpAcc, bufPtr;
  sb->lockData(pos,maxSamples,acc);         // prepare the accumulator-buffer
  unsigned i;

  if ( active == 1 )                        // single replayer
  {
    unsigned rPos = 0;
    m_bufs.getFirst(it,buf);                // find the active replayer
    while ( it != ITERATOR_NULL && buf->suspended )
      m_bufs.getNext(it,buf);
    prepared = buf->newData(rPos,minSamples,maxSamples);
    prepared -= prepared % PACKET_SIZE;
    buf->lockData(rPos,prepared,bufPtr);
      // copy the data:
    for ( i = 0; i++ < prepared; )          // !!! TODO: new CircularBufferPointers::copy method !!!
      acc.write16(bufPtr.read16());
    buf->unlockData(prepared,acc);
    buf->advanceSafePosition(prepared);
  }

  else
  {
    tmpAcc = acc;
    generateSilence(tmpAcc,16,maxSamples);  // initialize to silence

    if ( active > 1 )                       // more active replayers => mix them together
    {
      prepared = maxSamples;
      // determine mixing coefficient:
      // mix all the channels:
      m_bufs.getFirst(it,buf);              // find active replayers
      while ( it != ITERATOR_NULL )
      {
        if ( !buf->suspended )
        {
          unsigned actPos = 0;
          unsigned actPrepared = buf->newData(actPos,minSamples,maxSamples);
          actPrepared -= actPrepared % PACKET_SIZE;
          if ( actPrepared < prepared ) prepared = actPrepared;
          // do the mixing job:
          tmpAcc = acc;
          buf->lockData(actPos,actPrepared,bufPtr);
          for ( i = 0; i++ < prepared; )
          {
            int val = *(tmpAcc.ptr16()) + bufPtr.read16();
            if ( val > 32767 )
              tmpAcc.write16(32767);
            else
            if ( val < -32768 )
              tmpAcc.write16(-32768);
            else
              tmpAcc.write16(val);
          }
          buf->unlockData(prepared,tmpAcc);
        }
        m_bufs.getNext(it,buf);
      }
        // advance all the channels:
      m_bufs.getFirst(it,buf);              // find active replayers
      while ( it != ITERATOR_NULL )
      {
        if ( !buf->suspended )
          buf->advanceSafePosition(prepared);
        m_bufs.getNext(it,buf);
      }
    }

    else
      prepared = maxSamples;

  }

  sb->unlockData(prepared,acc);
  return prepared;
}

#endif // defined _XBOX

#endif // VOICE_OVER_NET
