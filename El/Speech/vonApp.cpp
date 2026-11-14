/**
  @file   vonApp.cpp
  @brief  VoN application interface.

  Communication between application and VoN system.
  Application is responsible for sound recording/playback,
    VoN system does everything else..
*/

#include <El/Speech/vonPch.hpp>
#include <Es/Strings/bString.hpp>
#include <Es/Algorithms/qsort.hpp>
#include <El/Graph/clique.hpp>
#include <malloc.h> // _alloca is used 
#include "El/Speech/debugVoN.hpp"
#if DEBUG_VON
#include "Es/Framework/appFrame.hpp"
#endif

#if _GAMES_FOR_WINDOWS
#   include <mmsystem.h>
#   include <dsound.h>
# include <xhv2.h>
#endif

#define VON_PREFIX "Vo"
//#define VON_PREFIX ""

//----------------------------------------------------------------------
//  Support (templates, traits):

/// Define VON_CTRL_VIM if contol-messages must be VIM.
#define VON_CTRL_VIM

#if 1
template <> VoNChannelId ExplicitMapTraits<VoNChannelId,RefD<VoNChannel> >::keyNull = VOID_CHANNEL;
template <> VoNChannelId ExplicitMapTraits<VoNChannelId,RefD<VoNChannel> >::zombie  = RESERVED_CHANNEL;

template <> VoNChannelId ExplicitMapTraits<VoNChannelId,RefD<VoNReplayer> >::keyNull = VOID_CHANNEL;
template <> VoNChannelId ExplicitMapTraits<VoNChannelId,RefD<VoNReplayer> >::zombie  = RESERVED_CHANNEL;
#endif

unsigned vonIdentityKey ( VoNIdentity &id )
{
  return (unsigned)id;
}

VoNChannelId netMessageToVoiceChannel (const RefD<NetMessage> &msg)
{
  const VoNMagicPacket *vonHdr = (const VoNMagicPacket *)msg->getData();
  return( vonHdr->channel );
}

VoNChannelId identityChannel ( VoNChannelId &id )
{
  return id;
}

void VoiceMask::init ()
{
  pitchScale       = -1.0f;
  roboticValue     = -1.0f;
  specEnergyWeight = -1.0f;
  whisperValue     = -1.0f;
}

/// Special VoN packets (peerToPeer is set true only after response to probe packet comes)
/// KeepAlive packets are sent but no response is answered (but their flow is monitored)
const unsigned64 OriginKeepAliveValue = 0;
const unsigned64 OriginProbeValue = 1;
const unsigned64 OriginResponseValue = 2;
const unsigned64 OriginStartSamples = 3;

#if _ENABLE_VON

#include <El/Speech/Speex/speex.hpp>
static const int SignalFrequency = 8000;
void VoNClient::VoNSay(int dpnid, int channel, int frequency, int seconds)
{
  LogF("((Enter VoNSay");
  enter();
  RefD<VoNReplayer> replayer;
  if ( !m_repl.get(dpnid,replayer) ) // channel does not already exist yet => create one
  {
    // try to restore codec from transmitted binary info:
    Assert( m_von );
    VoNCodec *codec = new SpeexCodec(SignalFrequency);   // 8000 is narrow band
    if ( !codec )                             // codec restore has failed!
    {
      leave();
      return;
    }
    replayer = new VoNReplayer(this,dpnid,codec);

    m_repl.put(dpnid,replayer);
    m_von->setReplay(dpnid,true);           // application-level response
  }
  // VoNReplayer and VoNSoundBuffer should be ready now to beep our frequency
  LogF(" ... enter EncodeBeep");
  EncodeBeep(dpnid, channel, frequency, seconds, replayer);

  leave();
  LogF("))Leave VoNSay");
}

void VoNClient::EncodeBeep(int dpnid, int channel, int frequency, int seconds, VoNReplayer *replayer)
{
  VoNCodec *codec = replayer->getCodec();

  Temp<unsigned8> encData;
  const int encBufSize = 8192;
  encData.Realloc(encBufSize); //temporary space for encoded beep
  if (codec)
  {
    int samplesToEncode = seconds*SignalFrequency; 
    int samplesInFrame = SignalFrequency/2; //half of second
    for (int shift=0, samples=0; samples<samplesToEncode; samples+=samplesInFrame)
    {
      int encDataSize = encBufSize;
      int encSamples = samplesInFrame;
      if (encSamples > (samplesToEncode-samples) ) encSamples=(samplesToEncode-samples);
      codec->encodeBeep(const_cast<unsigned8*>(encData.Data()), (unsigned int &)encDataSize, frequency, encSamples, shift);
      /* convert data into new von data message */
      // Prepare new message
      codec->groupStart();
      RefD<NetMessage> msg = (NetMessage*)NetMessagePool::pool()->newMessage(m_netCh->maxMessageData(),m_netCh);
      Assert( msg );
      msg->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG);
      VoNDataPacket *vonData = (VoNDataPacket*)msg->getData();
      vonData->channel = dpnid;        // dpnid of the speaking AI  
      vonData->chatChannel = channel;  // vonChannel of the speaker
      vonData->origin  = replayer->m_toEncode;  // we have no recorder, but need some simple recording management
      vonData->samples = 0;                 // will be set at completion-time
      vonData->size    = 0;                 // - - - - - - "" - - - - - -
      // available bytes (excluding trailing 0x0000 and packet header)
      int msgRoom = m_netCh->maxMessageData() - VON_DATA_OFFSET - 4;
      // Pointer to next unencoded segment
      unsigned16 *codePtr = (unsigned16*)(((unsigned8*)vonData) + VON_DATA_OFFSET);

      // copy encoded data from temp space into msg (it was possible to encode straightforward, but this is safer)
      if (encDataSize<=msgRoom)
      {
        memcpy(codePtr, encData.Data(), encDataSize);
        // do not append the length, codec has done it itself
        // *codePtr++ = (unsigned16)encDataSize;
        // complete the message:
        codePtr = (unsigned16*)(((unsigned8*)codePtr) + encDataSize);
        *codePtr++ = 0;
        unsigned len = ((unsigned8*)codePtr) - ((unsigned8*)vonData);
        vonData->samples = (unsigned16)( encSamples );
        vonData->size    = (unsigned16)( len - VON_DATA_OFFSET );
        msg->setLength(len);
        // And "send" the message at last (copy it to the VoNReplayer message heap}
        replayer->newDataMessage(msg);
        replayer->m_toEncode += encSamples;
      }

      shift += encSamples; // the sinusoid of harmonic tone should not crack at the end of every encoded part
    }
  }
}

//----------------------------------------------------------------------
//  Abstract sound-buffer (VoNSoundBuffer):

void generateSilence ( CircularBufferPointers &ptr, int bps, unsigned size )
{
  Assert( ptr.len1 + ptr.len2 >= size );
  unsigned batch = ptr.len1;                // 1st batch
  if ( batch > size ) batch = size;
  if ( bps > 8 )                            // 16 bits per sample
  {
    memset(ptr.ptr1.bps16,0,2*batch);
    ptr.ptr1.bps16 += batch;
    ptr.len1 -= batch;
    if ( (size -= batch) == 0 ) return;
    batch = ptr.len2;
    if ( batch > size ) batch = size;
    memset(ptr.ptr2.bps16,0,2*batch);
    ptr.ptr2.bps16 += batch;
    ptr.len2 -= batch;
  }
  else                                      // 8 bits per sample
  {
    memset(ptr.ptr1.bps8,0,batch);
    ptr.ptr1.bps8 += batch;
    ptr.len1 -= batch;
    if ( (size -= batch) == 0 ) return;
    batch = ptr.len2;
    if ( batch > size ) batch = size;
    memset(ptr.ptr2.bps8,0,batch);
    ptr.ptr2.bps8 += batch;
    ptr.len2 -= batch;
  }
}

//----------------------------------------------------------------------
//  Recorder (sound-capture):

/// If maximum amplitude is less than this, force "silence" state!
const float VoNRecorder::DEFAULT_REC_THRESHOLD = 0.03f;

VoNRecorder::VoNRecorder ( VoNClient *client, VoNChannelId ch, VoNCodec *codec, float recThreshold ) : m_codec(codec)
{
  Assert( client || ch == VOID_CHANNEL );
  m_client = client;
  Assert( codec );
  m_channel = ch;
  m_res = VonOK;
  m_isCapturing = false;
  m_isSilence = true;
  m_sound = 0;
  m_floatingMax = 0.0f;
    // acquire sound frequency & frame from codec info:
  CodecInfo info;
  codec->getInfo(info);
  m_frequency = info.nominalFrequency;
  m_bps       = info.nominalBps;
  m_minFrame  = info.minFrame;
  m_maxFrame  = info.maxFrame;
    // set codec quality
  codec->SetQuality(m_client->CodecQuality());
    // maxPacketSamples:
  m_maxPacketSamples = m_frequency / 4;     // default: 1/4 sec
  if ( m_maxPacketSamples < m_maxFrame )
      m_maxPacketSamples = m_maxFrame;
    // encoding state:
  m_toEncode = OriginStartSamples;          // sample index (next frame to encode)
  m_nonSilence = m_toEncode;
  m_codePtr = NULL;                         // pointer to the next code-segment
  m_lastHeartBeat = getSystemTime() - MAX_VON_HEART_BEAT;
                                            // force VON_CMD_CREATE at the very beginning
#ifdef NET_LOG_VOICE
  NetLog("VoNRecorder::VoNRecorder: chId=%d",ch);
#endif
  DCsum = 0; DCcount = 0;
  PWavg = 0; PWcount = 0;
  ClipFactor = 0; ClipCount = 0;
  m_recThreshold = recThreshold;
  m_transmitterFreq = 0;
}

VoNRecorder::~VoNRecorder ()
{
  setCapture(false);
}

#include "Es/Memory/normalNew.hpp"

void* VoNRecorder::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNRecorder::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNRecorder::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

/// If maximum amplitude is less than "RELATIVE_SILENCE_THR * m_floatingMax", set "silence" state
const float RELATIVE_SILENCE_THR = 0.08f;

/// Fading coefficient for "m_floatingMax" (in sounding-frames).
const float AMPLITUDE_FADE_SOUND = 0.98f;

/// Fading coefficient for "m_floatingMax" (in silent-frames).
const float AMPLITUDE_FADE_SILENCE = 0.995f;

bool VoNRecorder::silenceAnalyzer ( const CircularBufferPointers &cb, unsigned size, bool remember )
{
  if ( !size ) return true;

  Assert( size <= cb.len1 + cb.len2 );
  int DC = DCsum/DCcount;

    // compute maximal amplitude:
  int amplint = 0;
  unsigned ready = cb.len1;
  if ( m_bps > 8 )                          // 16-bit samples
  {
    int16 *p = cb.ptr1.bps16;
    if (!p) return true;
    if ( !ready )
    {
      ready = cb.len2;
      p = cb.ptr2.bps16;
    }
    while ( size-- )                        // process one sample
    {
      int act = (*p++ - DC);
      if (act<0) act = -act;
      if ( act > amplint ) amplint = act;
      if ( !--ready )
      {
        ready = cb.len2;
        p = cb.ptr2.bps16;
      }
    }
  }
  else                                      // 8-bit samples
  {
    int8 *p = cb.ptr1.bps8;
    if (!p) return true;
    if ( !ready )
    {
      ready = cb.len2;
      p = cb.ptr2.bps8;
    }
    while ( size-- )                        // process one sample
    {
      int act = (*p++ - DC);
      if (act < 0) act = -act;
      if ( act > amplint ) amplint = act;
      if ( !--ready )
      {
        ready = cb.len2;
        p = cb.ptr2.bps8;
      }
    }
  }
  float amplitude = (m_bps > 8) ? amplint/32768.0f : amplint/128.0f;

  // determine the "silence" state:
  //static float silenceThrCoef = 0.2f; //does not help much
  //float RelativeSilenceThr = RELATIVE_SILENCE_THR;// * silenceThrCoef;
  float HardSilenceThr = m_recThreshold;// * silenceThrCoef;
  bool isSilence = amplitude < HardSilenceThr;
  //bool isSilence = (amplitude < HardSilenceThr ||
  //                  amplitude < relative );

    // update floatingMax:
  if ( remember )
    if ( amplitude > m_floatingMax )
      m_floatingMax = amplitude;
    else
      if ( isSilence )
        m_floatingMax = m_floatingMax * AMPLITUDE_FADE_SILENCE + amplitude * (1.0f - AMPLITUDE_FADE_SILENCE);
      else
        m_floatingMax = m_floatingMax * AMPLITUDE_FADE_SOUND   + amplitude * (1.0f - AMPLITUDE_FADE_SOUND);

    // update "sound":
  if ( isSilence )
  {
    if ( m_sound ) m_sound--;
  }
  else
    m_sound = 32;

#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
  NetLog("VoNRecorder::silenceAnalyzer: %s, ampl=%5.1f %%, rel=%5.1f %%, floating=%5.1f %%",
    m_sound?"voice":"silence",100.0*amplitude,100.0*relative,100.0*m_floatingMax);
#endif
  if (m_codec->VADEnabled()) return false;
  return !m_sound;
}

void VoNRecorder::SetCodecQuality(int quality)
{
  if (m_codec) m_codec->SetQuality(quality);
}

static bool ComputeAmplitude ( const CircularBufferPointers &cb, unsigned size, float &ampl, int m_bps, float recThreshold )
{
  if ( !size ) { ampl=0; return true; };
  Assert( size <= cb.len1 + cb.len2 );

  // compute maximal amplitude:
  float amplitude = 0.0f;
  unsigned ready = cb.len1;
  if ( m_bps > 8 )                          // 16-bit samples
  {
    int16 *p = cb.ptr1.bps16;
    if (!p) return true;
    if ( !ready )
    {
      ready = cb.len2;
      p = cb.ptr2.bps16;
    }
    while ( size-- )                        // process one sample
    {
      float act = (float)fabs( *p++ / 32768.0 );
      if ( act > amplitude )
        amplitude = act;
      if ( !--ready )
      {
        ready = cb.len2;
        p = cb.ptr2.bps16;
      }
    }
  }
  else                                      // 8-bit samples
  {
    int8 *p = cb.ptr1.bps8;
    if (!p) return true;
    if ( !ready )
    {
      ready = cb.len2;
      p = cb.ptr2.bps8;
    }
    while ( size-- )                        // process one sample
    {
      float act = (float)fabs( *p++ / 128.0 );
      if ( act > amplitude )
        amplitude = act;
      if ( !--ready )
      {
        ready = cb.len2;
        p = cb.ptr2.bps8;
      }
    }
  }

  ampl=amplitude; //fill in the output parameter
  // determine the "silence" state:
  return (amplitude < recThreshold);
}

bool VoNReplayer::GetAmplitude ( const CircularBufferPointers &cb, unsigned size, float &ampl )
{
  return ::ComputeAmplitude(cb, size, ampl, m_bps, VoNRecorder::DEFAULT_REC_THRESHOLD);
}

int VoNRecorder::GetDC ( const CircularBufferPointers &cb, unsigned size )
{
  if ( !size ) { return 0; };
  Assert( size <= cb.len1 + cb.len2 );

  // compute sum of PCM values
  int sum = 0;
  unsigned cnt = size; //backup
  unsigned ready = cb.len1;
  if ( m_bps > 8 )                          // 16-bit samples
  {
    int16 *p = cb.ptr1.bps16;
    if (!p) return 0;
    if ( !ready )
    {
      ready = cb.len2;
      p = cb.ptr2.bps16;
    }
    while ( size-- )                        // process one sample
    {
      //float act = (float)( *p++ / 32768.0 );
      sum += *p++;
      if ( !--ready )
      {
        ready = cb.len2;
        p = cb.ptr2.bps16;
      }
    }
  }
  else                                      // 8-bit samples
  {
    int8 *p = cb.ptr1.bps8;
    if (!p) return true;
    if ( !ready )
    {
      ready = cb.len2;
      p = cb.ptr2.bps8;
    }
    while ( size-- )                        // process one sample
    {
      //float act = (float)( *p++ / 128.0 );
      sum += *p++;
      if ( !--ready )
      {
        ready = cb.len2;
        p = cb.ptr2.bps8;
      }
    }
  }

  return sum/(signed)cnt;
}

/*!
\patch 5122 Date 1/24/2007 by Bebul
- New: Voice over Net - the recorded signal level is adjusted before sending out to recipients
*/
float VoNRecorder::GetPower ( const CircularBufferPointers &cb, unsigned size )
{
  if ( !size ) { return 0; };
  Assert( size <= cb.len1 + cb.len2 );

  // compute sum of PCM values
  float sum = 0;
  unsigned cnt = size; //backup
  unsigned ready = cb.len1;
  if ( m_bps > 8 )                          // 16-bit samples
  {
    int16 *p = cb.ptr1.bps16;
    if (!p) return 0;
    if ( !ready )
    {
      ready = cb.len2;
      p = cb.ptr2.bps16;
    }
    while ( size-- )                        // process one sample
    {
      //float act = (float)( *p++ / 32768.0 );
      int val = *p++;
      sum += val*val;
      if ( !--ready )
      {
        ready = cb.len2;
        p = cb.ptr2.bps16;
      }
    }
  }
  else                                      // 8-bit samples
  {
    int8 *p = cb.ptr1.bps8;
    if (!p) return true;
    if ( !ready )
    {
      ready = cb.len2;
      p = cb.ptr2.bps8;
    }
    while ( size-- )                        // process one sample
    {
      //float act = (float)( *p++ / 128.0 );
      int val = *p++;
      sum += val*val;
      if ( !--ready )
      {
        ready = cb.len2;
        p = cb.ptr2.bps8;
      }
    }
  }

  int dc = DCsum/DCcount;
  float rawval = sum/(signed)cnt - dc*dc;
  if (rawval<1.0f) rawval = 1.0f;
  if (m_bps > 8) return rawval / (32768.0f*32768.0f);
  else return rawval / (128.0f*128.0f);
}

void VoNRecorder::UpdateDC (int curDC)
{
  if (DCcount<200) 
  {
    DCsum = DCsum+curDC;
    DCcount++;
  }
  else DCsum += curDC-DCsum/DCcount;
}

void VoNRecorder::UpdatePower (float curPW, unsigned sampleCount)
{
  /* maxPWSeconds is empiric value 
  // temporary shouting or something should not be boosted to the same volume
  // but when speaking in different loudness for longer time, 
  // it should boost it to the "normalized" volume
  */
  static int maxPWSeconds = 80; //number of seconds to average
  int maxPWCount = maxPWSeconds * m_frequency;
  PWavg = (PWavg*PWcount+curPW*sampleCount)/(PWcount + sampleCount);
  if (PWcount<maxPWCount) PWcount += sampleCount;
  else PWcount = maxPWCount;
}

void VoNRecorder::UpdateAmlitudeClipper (float maxAmpl, int sampleCount)
{
  const float ClippingResetFactor=0.5f;
  const int maxClipSeconds = 5; //number of seconds to average
  const int clipInfluenceSampleCount = m_frequency/2; // half second
  int maxClipCount = maxClipSeconds * m_frequency;
  if (maxAmpl <= 1)
    ClipFactor = (ClipFactor*ClipCount + sampleCount*ClippingResetFactor) / (ClipCount + sampleCount*ClippingResetFactor);
  else
  {
    float clipReduction = 1/maxAmpl;
    ClipFactor = (ClipFactor*ClipCount + clipReduction*clipInfluenceSampleCount) / (ClipCount + clipInfluenceSampleCount);
  }
  saturate(ClipFactor, 0.75f, 1.0f);
  if (ClipCount < maxClipCount) ClipCount += sampleCount;
  else ClipCount = maxClipCount;
  //LogF("ClipFactor = %.2f", ClipFactor);
}

#endif //_ENABLE_VON

struct VoNClient::IndirectInfo
{
  int  dpid;
  BYTE extInfo;
};


/// notify channel we have transmitted data
void VoiceDataSent(int pid, int len);

/*!
\patch 5154 Date 5/2/2007 by Bebul
- Fixed: VoN connection established by retranslation through server Game socket when NAT negotiation fails
\patch 5161 Date 5/31/2007 by Bebul
- Fixed: Dedicated server crashes when retranslating VoN packets through Game Socket.
*/

/// send message using direct peerToPeer channel or, when this is not available, send it using game socket
void VoNClient::SendPeerToPeerMessage(
  int directId, const sockaddr_in &ia, const void *data, int size, int encryptedSize,
  IndirectInfo *indirect, int nIndirect
)
{
  TransmitTargetExt *tg = FindTransmitTarget(directId);
  bool peerToPeer = false;
  if (tg) peerToPeer = tg->peerToPeer;
#define FORCE_VOICE_ENCAPSULATE 0
#if FORCE_VOICE_ENCAPSULATE
  if (true) //server retranslate forced (through the game socket)
#else
  if (!peerToPeer || ia.sin_addr.s_addr==0) //server retranslate needed (through the game socket)
#endif
  {
    bool clientServerEdge = (directId==EXT_BOT_CLIENT || _botClient);
    if (!clientServerEdge) 
    {
#if !_SUPER_RELEASE
      RptF("Warning: [VoN, SendPeerToPeerMessageCheckServer] zero IP address used to send message!");
#endif
      return;
    }
    // optimize special case - only one direct recipient
    if (nIndirect==1 && indirect[0].dpid==directId) 
    {
      nIndirect = 0;
      ((VoNDataPacket*)data)->chatChannel = indirect->extInfo;
    }
    size_t indirectSize = sizeof(*indirect)*nIndirect;
    // we need to allocate a temporary storage so that we can append data
#ifdef _WIN32
    unsigned8 *mem = (unsigned8 *)_alloca(size+indirectSize+sizeof(char));
#else
    unsigned8 *mem = (unsigned8 *)malloc(size+indirectSize+sizeof(char));
#endif
    unsigned8 *mem0 = mem;

    memcpy(mem,data,size);
    mem += size;

    // append list of indirect-s first
    memcpy(mem,indirect,indirectSize);
    mem += indirectSize;
    size += indirectSize;
    // append count of indirects
    *mem = nIndirect;
    size++;

#ifdef DEBUG_VON_RETRANSLATE
    LogF("VoN - retranslate through GameSocket! (%d,%d)", directId, nIndirect);
#endif

    // encapsulated voice when being sent from Server to recipient is done by method call, 
    // so some other Critical Sections can be entered (such as server->enterUsr)
    // When both are needed always should be this order: 1. server->enterUsr 2. VoNClient::enter()
    // For this reason, StoreCtrlToSend is used instead of direct call to sendCtrlTo
    StoreCtrlToSend(directId, VON_CMD_ENCAPSULATE_VOICE, mem0, size, false); //no VIM
#ifndef _WIN32
    free(mem0);
#endif
  }
  else SendPeerToPeerMessageDirect(directId, ia, data, size, encryptedSize, indirect, nIndirect);
}

void VoNClient::SendPeerToPeerMessageDirect(
  int directId, const sockaddr_in &ia, const void *data, int sizeIn, int encryptedSize,
  IndirectInfo *indirect, int nIndirect
)
{
/* //still usefull to debug NAT traversal, where the VoN message was send at last (if retranslated)
#if DEBUG_IDENTITIES
  LogF("SendPeerToPeerMessage: to IP: %d.%d.%d.%d:%d",
    ia.sin_addr.S_un.S_un_b.s_b1, ia.sin_addr.S_un.S_un_b.s_b2, 
    ia.sin_addr.S_un.S_un_b.s_b3, ia.sin_addr.S_un.S_un_b.s_b4,
    ntohs(ia.sin_port)
  );
#endif
*/
  // optimize special case - only one direct recipient
  if (nIndirect==1 && indirect[0].dpid==directId) 
  {
    nIndirect = 0;
    ((VoNDataPacket*)data)->chatChannel = indirect->extInfo;
  }
  size_t indirectSize = sizeof(*indirect)*nIndirect;
  // we need to allocate a temporary storage so that we can append data
  #ifdef _WIN32
  unsigned8 *mem = (unsigned8 *)_alloca(sizeIn+indirectSize+1+sizeof(VON_PREFIX)-1);
  #else
  unsigned8 *mem = (unsigned8 *)malloc(sizeIn+indirectSize+1+sizeof(VON_PREFIX)-1);
  #endif
  unsigned8 *mem0 = mem;
  size_t size = 0;

  //if (withPrefix) // direct sent always with a prefix
  {
    memcpy(mem,VON_PREFIX,sizeof(VON_PREFIX)-1);
    mem += sizeof(VON_PREFIX)-1;
    size += sizeof(VON_PREFIX)-1;

  }
  memcpy(mem,data,sizeIn);
  size += sizeIn;
  mem += sizeIn;
  
  // append list of indirect-s first
  memcpy(mem,indirect,indirectSize);
  mem += indirectSize;
  size += indirectSize;
  // append count of indirects
  *mem = nIndirect;
  size++;
  
#ifdef DEBUG_VON_RETRANSLATE
  LogF("VoN - send direct (%d,%d)", directId, nIndirect);
#endif

  SendPeerToPeerData(directId,ia,mem0,size,sizeof(VoNDataPacket));

  #ifndef _WIN32
  free(mem0);
  #endif

  #if _ENABLE_REPORT
    OnDataSent(size);
  #endif
}

/*!
\patch 5226 Date 2/19/2008 by Bebul
- Fixed: VoN - Unable to hear other players issue.
*/

/**
Decide which targets will be retranslated and how.
Note: can be called only inside or targets list lock, uses FindTransmitTarget
Note: only VoNDataPackets can be sent
*/
void VoNClient::SendPeerToPeerMessage(
  const void *data, int size, int encryptedSize,
  const TransmitTarget *targets, int nTargets
)
{
  Assert(nTargets<maxVoicePlayers)

  VoNChatChannel chatChannel = ((VoNDataPacket*)data)->chatChannel;
  
  if (nTargets<0 || nTargets>maxVoicePlayers) return;

  // #debug von diagnostics
  for (int i=0; i<nTargets; i++)
  {
    if ( (targets[i].extInfo & TTChannelMask)==0 )
    {
      GDebugVoNBank.SetFeature(DebugVoNBank::VoNFeature_SendEmptyVoice);
      break;
    }
  }
  
  //enterNoDeadLockDetector(); //only to get to know whether also different VoNClient.enter can cause a deadlock
  enter();
  
  {
    // separate targets by clique
    // remove targets from the array one by one
    bool rest[maxVoicePlayers];
    
    /// track how many targets are still left for transmission
    int restCount = nTargets;
    for (int tgIx=0; tgIx<nTargets; tgIx++)
    {
      rest[tgIx] = true;
    }
    
    IndirectInfo indirect[maxVoicePlayers+1];
    sockaddr_in indirectAddr[maxVoicePlayers+1];
    // first handle those which we can reach directly
    while (restCount>0)
    {
      // check which clique can be best used for broadcasting
      
      int bestCount = 0;
      int bestClique = -1;
      int bestCliqueEntry = -1;
      for (int clqIx=0; clqIx<m_cliques.Size(); clqIx++)
      {
        const FindArray<int> &clique = m_cliques[clqIx];
        // we can use only cliques we are in of we have some connection into it
        // note: we should have a connection into each clique, as server is there
        // first we want to use cliques with direct connection
        int found = 0;
        int entry = -1;
        
        // check how many targets are in this clique
        for (int tgIx=0; tgIx<nTargets; tgIx++)
        {
          // ignore targets which are already served
          if (!rest[tgIx]) continue;
          if (clique.Find(targets[tgIx].dpid)>=0) found++;
        }

        if (found>0)
        {
          if (clique.Find(outChannel)>=0)
          {
            // strongly prefer cliques we are in
            found += 1000;
            // entry can be anyone
            entry = outChannel;
          }
          else
          {
            for (int ix=0; ix<clique.Size(); ix++)
            {
              int eid = clique[ix];
              const TransmitTargetExt *etgt = FindTransmitTarget(eid);
              bool clientServerEdge = ((etgt && etgt->dpid==EXT_BOT_CLIENT) || _botClient);
              if (!etgt || (!etgt->peerToPeer && !clientServerEdge) ) continue;
              // connection found - use it
              entry = eid;
              break;
            }
            if (entry<0)
            {
              // no entry - we cannot use this clique
              found = -1;
            }
          }
        }

        if (found>bestCount)
        {
          bestCount = found;
          bestClique = clqIx;
          bestCliqueEntry = entry;
        }
      }
      
      // send into this clique
#if FORCE_VOICE_ENCAPSULATE
      if (false)
#else
      if (bestClique>=0)
#endif
      {
        const FindArray<int> &clique = m_cliques[bestClique];
        // retranslate into the clique
        int nIndirect = 0;
        for (int tgIx=0; tgIx<nTargets; tgIx++)
        {
          // ignore targets which are already served
          if (!rest[tgIx]) continue;
          if (clique.Find(targets[tgIx].dpid)>=0)
          {
            indirect[nIndirect].dpid = targets[tgIx].dpid;
            indirect[nIndirect].extInfo = targets[tgIx].extInfo | chatChannel;
            indirectAddr[nIndirect] = targets[tgIx].addr;
            nIndirect++;
            rest[tgIx]=false;
            restCount--;
          }
        }

        Assert(nIndirect>0);
        if (nIndirect>0)
        {
          if (bestCliqueEntry==outChannel)
          {
            // no mediator needed - distribute as desired
            // if there are many recipients, we need re-translation
            // this should be checked based on current bandwidth
            // TODO: some global optimization would be handy
            // currently we count each clique separately
            const int MaxDirectTargets = 6; //maxTargets is dynamic now
            int maxTargets = (nIndirect>36) ? toInt(sqrt(nIndirect)) : MaxDirectTargets;
            int perDirect = (nIndirect+maxTargets-1)/maxTargets;
            Assert(perDirect>=1);
            if (perDirect<1) perDirect = 1;
            for (int start=0; start<nIndirect; start+=perDirect)
            {
              int batch = nIndirect-start;
              if (batch>perDirect) batch = perDirect;
              //VoNDataPacket *vonPacket = (VoNDataPacket*)data;
              // channel inside vonPacket is always correct. 
              // moreover extInfo contains 2D3D flags along with the VoNChatChannel already
              SendPeerToPeerMessage(
                indirect[start].dpid,indirectAddr[start],data,size,encryptedSize,
                indirect+start,batch
              );
            }
          }
          else
          {
            // we need a mediator - he needs to handle retranslation on his own
            int tgtId = bestCliqueEntry;
            const TransmitTargetExt *tgt = FindTransmitTarget(tgtId);
            sockaddr_in tgtAddr = tgt->addr;
            //VoNDataPacket *vonPacket = (VoNDataPacket*)data;
            // channel inside vonPacket is always correct. 
            // moreover extInfo contains 2D3D flags along with the VoNChatChannel already
            SendPeerToPeerMessage(
              tgtId,tgtAddr,data,size,encryptedSize,
              indirect,nIndirect
            );
          }
        }
      }
      else
      {
        // if there is no clique, use server retranslation
        // this can happen before clique message is received
        
        // note: server might receive the same message anyway, as a part of a clique
        // we do not care much - it will not affect his outgoing traffic
        int nIndirect = 0;
        for (int tgIx=0; tgIx<nTargets; tgIx++)
        {
          if (rest[tgIx])
          {
            indirect[nIndirect].dpid = targets[tgIx].dpid;
            indirect[nIndirect].extInfo = targets[tgIx].extInfo | chatChannel;
            nIndirect++;
            rest[tgIx]=false;
            restCount--;
          }
        }
        Assert(nIndirect>0);
        if (nIndirect>0)
        {
          TransmitTargetExt *host = FindTransmitTarget(EXT_BOT_CLIENT);
          if (host)
          {
            // server may be not in the list
            VoNDataPacket *vonPacket = (VoNDataPacket*)data;
            vonPacket; //just so that compiler did not show warning
            // channel inside vonPacket is always correct. 
            // moreover extInfo contains 2D3D flags along with the VoNChatChannel already
            SendPeerToPeerMessage(
              EXT_BOT_CLIENT,host->addr,data,size,encryptedSize,
              indirect,nIndirect
            );
          }
        }
        Assert(restCount==0);
        // if something is left, it is a bug - there is no more we can do
        break;
      }
    }
  }
  
  //leaveNoDeadLockDetector();
  leave();
  
}

void VoNClient::SendPeerToPeerData(int dpid, const sockaddr_in &addr, const void *data, size_t size, size_t encryptedSize)
{
  RefD<VoNServer> server = getServer();
  // on a server, we need to use the channel to a respective client
  RefD<NetChannel> netCh = server ? server->netChannelFromIdentity(dpid) : getChannel();
  if (netCh)
  {
    netCh->sendRaw(addr,data,size,encryptedSize);
    netCh->dataSentAck(size);
  }
}

void VoNClient::SendPeerToPeerSpecial(int srcChannel, int value, TransmitTargetExt &target)
{
  char prefixedPacket[sizeof(VoNDataPacket)+sizeof(VON_PREFIX)-1];
  int prefixPos = 0;
  //if (target.needsPrefix)
  {
    memcpy(prefixedPacket,VON_PREFIX,sizeof(VON_PREFIX)-1);
    prefixPos = sizeof(VON_PREFIX)-1;
  }
  VoNDataPacket &packet = *reinterpret_cast<VoNDataPacket *>(prefixedPacket+prefixPos);
  packet.channel = srcChannel;  
  packet.origin = value; // zero means probe
  packet.samples = 0; // no samples - special message
  packet.size = 0; // uninitialized otherwise
  packet.chatChannel = 0; // uninitialized otherwise
#ifdef DEBUG_VON_KEEP_ALIVE
  char *packetType = "unknown";
  switch (packet.origin)
  {
  case OriginProbeValue: packetType = "probe"; break;
  case OriginResponseValue: packetType = "response"; break;
  case OriginKeepAliveValue: packetType = "keep alive"; break;
  }
  const sockaddr_in &ta = target.addr;
  LogF(
    "VoNLog, sendPeerToPeerSpecial: %s to %d (%d.%d.%d.%d:%d)", packetType, target.dpid,
    ta.sin_addr.S_un.S_un_b.s_b1, ta.sin_addr.S_un.S_un_b.s_b2, ta.sin_addr.S_un.S_un_b.s_b3, ta.sin_addr.S_un.S_un_b.s_b4, ntohs(ta.sin_port)
  );
#endif

  SendPeerToPeerData(target.dpid,target.addr,&prefixedPacket,sizeof(prefixedPacket),sizeof(prefixedPacket));
}

/// send a control message to the server
void VoNClient::sendCtrl(unsigned command, const void *data, int size)
{
  RefD<NetChannel> netCh = getChannel();
  
  unsigned len = VON_CONTROL_OFFSET + size;
  RefD<NetMessage> msg = NetMessagePool::pool()->newMessage(len,netCh);
  if ( msg )
  {
#ifdef VON_CTRL_VIM
    msg->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG|MSG_VIM_FLAG);
#else
    msg->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG);
#endif
    msg->setLength(len);
    VoNControlPacket *vonHdr = (VoNControlPacket*)msg->getData();
    vonHdr->magic   = MAGIC_VON_CTRL;
    vonHdr->channel = outChannel;
    vonHdr->command = command;
    vonHdr->size    = size;

    memcpy(vonHdr->data,data,size);
    
    msg->send(true);
#if _ENABLE_REPORT
    OnCtrlSent(msg->getLength()+MSG_HEADER_LEN);
#endif
  }
}

/// send a control message to the given recipient (through game socket)
/// if on server - it is send directly to the recipient
/// if not on server - it is send to server (and server should retranslate it)
///    the recipient should be included inside data member (as 'id' is not packed into packet)
void VoNClient::sendCtrlTo(VoNChannelId id, unsigned command, const void *data, int size, bool useVimFlag)
{
  RefD<NetChannel> netCh;
  RefD<VoNServer> server = getServer();
  if (!server || id==EXT_TO_SERVER) 
    netCh = getChannel(); // send it to server first
  else 
  {
    // uses server->enterUsr inside!!!
    netCh = server->netChannelFromIdentity(id);  // send it directly to the recipient
  }

  unsigned len = VON_CONTROL_OFFSET + size;
  RefD<NetMessage> msg = NetMessagePool::pool()->newMessage(len,netCh);
  if ( msg )
  {
#ifdef VON_CTRL_VIM
    if (useVimFlag)
      msg->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG|MSG_VIM_FLAG);
    else
      msg->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG);
#else
    msg->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG);
#endif
    msg->setLength(len);
    VoNControlPacket *vonHdr = (VoNControlPacket*)msg->getData();
    vonHdr->magic   = MAGIC_VON_CTRL;
    vonHdr->channel = outChannel;
    vonHdr->command = command;
    vonHdr->size    = size;

    memcpy(vonHdr->data,data,size);

    msg->send(true);
#if _ENABLE_REPORT
    OnCtrlSent(msg->getLength()+MSG_HEADER_LEN);
#endif
  }
}

#if _ENABLE_VON

bool VoNRecorder::SendMsg()
{
  Assert(!m_msg.IsNull());
  if (m_msg.IsNull())
    return false;

  Assert(m_codePtr);
  if (!m_codePtr)
    return false;

  Assert(m_client);
  if (!m_client)
    return false;

  // complete & send the message:
  *m_codePtr++ = 0;
  VoNDataPacket *vonData = (VoNDataPacket*)m_msg->getData();
  unsigned len = ((unsigned8*)m_codePtr) - ((unsigned8*)vonData);
  vonData->samples = (unsigned16)( m_toEncode - m_msgStart );
  vonData->size    = (unsigned16)( len - VON_DATA_OFFSET );
  m_msg->setLength(len);

  RefD<NetPeerToPeerChannel> peerToPeer = m_client->getPeerToPeer();

  if (peerToPeer)
  {
    // get recipients
    m_client->enterTransmitTargets();
    const AutoArray<TransmitTarget> &targets = m_client->getTransmitTargets();
    m_client->StoreVoiceToSend(m_msg->getData(),len,sizeof(VoNDataPacket),
      targets.Data(),targets.Size()
      );
    m_client->leaveTransmitTargets();
  }
  else
  {
#if _ENABLE_REPORT
    m_client->OnDataSent(m_msg->getLength()+MSG_HEADER_LEN);
#endif
    m_msg->send(true);
  }
  m_msg = NULL;
#ifdef NET_LOG_VOICE_VERBOSE
  NetLog("  voice-packet sent: origin=%u, samples=%u, code=%u",(unsigned)m_msgStart,(unsigned)(m_toEncode-m_msgStart),len);
#endif

#if DEBUG_VON_SAVE_BUFFERS
  GDebugVoNSaveBuffers.GetSendBuf().AddMessage(Format("Sending message: origin=%u, samples=%u, code=%u, targets=%u, transmissionEnabled=%u",
    (unsigned)m_msgStart,(unsigned)(m_toEncode-m_msgStart),len, (unsigned)m_client->getTransmitTargets().Size(), (unsigned)m_client->IsTransmitionEnabled()));
#endif

  return true;
}
/*!
\patch 5126 Date 1/31/2007 by Bebul
- Fixed: Voice over IP direct speak communication
\patch 5160 Date 5/17/2007 by Bebul
- Fixed: Voice over IP direct speak is made louder now.
\patch 5161 Date 5/30/2007 by Bebul
- Fixed: VoN voice clipping is reduced now.
*/


unsigned VoNRecorder::encode ( VoNSoundBuffer *sb,
                               unsigned pos, unsigned minEnc, unsigned maxEnc )
{
#if DEBUG_VON_SILENCE_RATIO
  static unsigned dbgTotal = 0;
  static unsigned dbgSilent = 0;
#endif

  Assert( m_client );
  RefD<NetChannel> netCh = m_client->getChannel();
  Assert( netCh );
  Assert( m_codec );

  m_res = VonOK;

#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
  NetLog("VoNRecorder::encode: ch=%u, pos=%u, minEnc=%u, maxEnc=%u",
         netCh->getChannelId(),pos,minEnc,maxEnc);
#endif

  // heart-beat is used especially once the channel is created
  // heart-beat check:
  unsigned64 now = getSystemTime();
  if ( now >= m_lastHeartBeat + MAX_VON_HEART_BEAT ) // sent heart-beat
  {
    unsigned16 saveSize = m_codec->saveInfo();
    unsigned len = VON_CONTROL_OFFSET + saveSize;
    char buf[1024]; //close to max packet size (should be enough)
    VoNControlPacket *vonHdr = (VoNControlPacket *)buf;
    vonHdr->magic   = MAGIC_VON_CTRL;
    vonHdr->channel = m_channel;
    vonHdr->command = VON_CMD_CREATE;
    vonHdr->size    = m_codec->saveInfo(vonHdr->data);
    m_client->StoreVoiceToSend(vonHdr, len, 0, NULL, 0, true); //Heart Beat
#if defined NET_LOG_VOICE_VERBOSE || NET_LOG_VOICE_P2P>10
    NetLog("  heart-beat stored to send after %.0f ms: %u bytes ",1.e-3*(now-m_lastHeartBeat),len);
#endif
    m_lastHeartBeat = now;
  }

  if ( maxEnc < m_minFrame ) 
  {
    // not enough sound data to encode

    // send message if needed
    if (!m_msg.IsNull() && sb->suspended)
    {
      SendMsg();
#if DEBUG_VON_SAVE_BUFFERS
      GDebugVoNSaveBuffers.GetSendBuf().Save();
#endif
    }

    return 0;      
  }

    // something will be encoded:
  Assert( sb );
  CircularBufferPointers cb;
  sb->lockData(pos,maxEnc,cb);              // prepare "CircularBufferPointers"
  CircularBufferPointers cbLock = cb;

    // encode the sound:
  unsigned encoded = 0;
  unsigned enc;                             // bunch-size in samples
  bool finish = false;                      // finish processing

  /// maximal message room
  int maxMsgRoom = netCh->maxMessageData() - VON_DATA_OFFSET - 4;
  
  /// temporary buffer for storing encoded data
  if (m_tmpBuf.Size() < maxMsgRoom * 2 )
    m_tmpBuf.Realloc(maxMsgRoom * 2);

  if (cb.len1>0) while ( !finish &&
          encoded + m_minFrame <= maxEnc )  // got something to encode..
  {
    unsigned ready = maxEnc - encoded;
    Assert( ready <= cb.len1 + cb.len2 );

    enc=m_minFrame;

    int curDC = GetDC(cb, ready);
    UpdateDC(curDC);
    // we want to detect silence only when user is NOT using push to talk button (voice toggle on is enabled)

    // remember if there was silence
    bool wasSilence = m_isSilence;

    // encoded size
    unsigned codeSize = 0;
    // is we should skip silence in CB
    bool skipSilenceInCB = true;

#if DEBUG_VON_SILENCE_RATIO
    if (m_client->IsTransmitionEnabled())
      dbgTotal++;
    RptF("VoN silence ratio: %f", dbgTotal>0? (float)dbgSilent/dbgTotal: 0);
#endif

    m_isSilence = !m_client->IsTransmitionEnabled() || (m_client->IsVoiceToggleOn() && silenceAnalyzer(cb,enc,true));

    if (!m_isSilence)
    {
      if (m_msg.IsNull())
      {
        // message does not exist

        // start group
        m_codec->groupStart();
        m_msgRoom = maxMsgRoom;
      }

      // encode data
      codeSize = m_msgRoom;
      enc = m_minFrame;
      float curPower = GetPower(cb, ready);
      UpdatePower(curPower, ready);
      float boost = GetBoost();
      float amplitude = 0;
      (void) ComputeAmplitude(cb, ready, amplitude, m_bps, m_recThreshold);
      amplitude *= boost;
      UpdateAmlitudeClipper(amplitude, ready);
      boost *= ClipFactor;
      // NetLog("  curPw=%f, avgPw=%f, boost=%.1f",curPower, PWavg, boost);
      VoNResult encResult = m_codec->encode(cb,enc,m_tmpBuf.Data(),codeSize,DCsum/DCcount,boost);
      encResult; //so that compiler does not show warnings
#if DIAGNOSE_SPEEX_ESTFRAMES
      extern int gCodecQuality;
      RptF("Q: %d S: %d", gCodecQuality, codeSize);
#endif

#if SPEEX_ENABLE_VAD_DTX || SPEEX_USE_PREPROCESOR
      if (VonSilence == encResult || 2 == codeSize) // speex VAD detected silence
      {
        skipSilenceInCB = false; //we dont want to skip silence in CB, because it is already skipped (by feeding it to encode)
        m_isSilence = true;
      }
#endif
    }

    if (m_msg.IsNull())
    {
      // message does not exist yet

      if (m_isSilence && wasSilence)
      {
        // msg does not exist, there is silence and was also silence before
        // do not send any packets as long as there is only silence
        if (skipSilenceInCB)
        {
        if ( m_bps > 8 )
          cb.skip16(enc);
        else
          cb.skip8(enc);
        }
        encoded += enc;
#if DEBUG_VON_SAVE_BUFFERS
        GDebugVoNSaveBuffers.GetSendBuf().Save();
#endif
#if DEBUG_VON_SILENCE_RATIO
        if (m_client->IsTransmitionEnabled())
          dbgSilent++;
#endif
        continue;
      }

      // create new message
      m_msg = (NetMessage*)NetMessagePool::pool()->newMessage(netCh->maxMessageData(),netCh);
      Assert( m_msg );
      m_msg->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG);
      VoNDataPacket *vonData = (VoNDataPacket*)m_msg->getData();
      vonData->channel = m_channel;
      vonData->chatChannel = m_client->GetChatChannel();
      vonData->origin  = m_msgStart = m_toEncode;
      vonData->samples = 0;                 // will be set at completion-time
      vonData->size    = 0;                 // - - - - - - "" - - - - - -
      // available bytes (excluding trailing 0x0000 and packet header)
      m_msgRoom = netCh->maxMessageData() - VON_DATA_OFFSET - 4;
      m_codePtr = (unsigned16*)(((unsigned8*)vonData) + VON_DATA_OFFSET);

      if ((vonData->chatChannel&~TTChannelMask) == 2 /*CCItemTransmitter*/)
      { // encode current transmitter frequency
        float *freq = (float *)m_codePtr;
        *freq = m_transmitterFreq;
        m_codePtr = (unsigned16*)(((unsigned8*)m_codePtr) + sizeof(float));
      }
    }

    if (m_isSilence)
    {
          // encode "enc" silent samples:
#ifdef NET_LOG_VOICE_VERBOSE
      NetLog("  silent samples=%u, origin=%u",enc,(unsigned)m_msgStart);
#endif
      *m_codePtr++ = 2;                 // "silence" flag
      *m_codePtr++ = (unsigned16)enc;
      m_msgRoom -= 4;
      if (skipSilenceInCB)
      {
      if ( m_bps > 8 )
        cb.skip16(enc);
      else
        cb.skip8(enc);
    }
    }
    else
    {
      // send encoded data
      if ( enc )
      {
        Assert( codeSize );
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
        NetLog("  voice samples=%u, code=%u bytes, origin=%u",enc,codeSize,(unsigned)m_msgStart);
#endif
        *m_codePtr++ = (unsigned16)codeSize;
        memcpy((unsigned8*)m_codePtr, m_tmpBuf.Data(), codeSize);
        m_codePtr = (unsigned16*)(((unsigned8*)m_codePtr) + codeSize);
        m_msgRoom -= 2 + codeSize;
      }
      else                                  // enc == 0 => data-bunch is too small to encode
      {
        if ( encoded < minEnc )             // I have to encode at least "minEnc" samples.. :C
        {
          enc = minEnc - encoded;
          *m_codePtr++ = 2;                 // "silence" flag
          *m_codePtr++ = (unsigned16)enc;
          m_msgRoom -= 4;
          if ( m_bps > 8 )
            cb.skip16(enc);
          else
            cb.skip8(enc);
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
          NetLog("  encoding failed: generating silence=%u",enc);
#endif
        }
        finish = true;              // finish processing of this data-batch
      }
    }

      // 4. check for msg overflow:
    m_toEncode += enc;
    encoded  += enc;
    bool chatChannelChanged = ((VoNDataPacket*)m_msg->getData())->chatChannel != m_client->GetChatChannel();
    if ( m_toEncode > m_msgStart + m_maxPacketSamples || // too much samples
         m_msgRoom < (int)m_codec->estFrameCodeLen() || // too much code data
         chatChannelChanged )
    {
        // complete & send the message:
      SendMsg();
      }

  }

  sb->unlockData(encoded,cbLock);
#if defined (NET_LOG_VOICE_VERBOSE) || DEBUG_VON
  NetLog("  encoded samples=%u",encoded);
#endif
  return encoded;
}

bool VoNRecorder::getRecording ()
{
  return( m_isCapturing && !m_isSilence);
}

void VoNRecorder::setCapture ( bool on )
{
  if ( on == m_isCapturing ) return;
  m_isCapturing = on;
}

void VoNRecorder::setMaxPacketTime ( double maxPacketTime )
{
  m_maxPacketSamples = (unsigned)( maxPacketTime * m_frequency );
  if ( m_maxPacketSamples < m_maxFrame )
    m_maxPacketSamples = m_maxFrame;
}

float VoNRecorder::GetBoost()
{
  //empiric value: normalizeVolumVal - higher value means normalization to louder volume
  const float normalizeVolumeVal = 0.02f; //former value was 0.005f; but we want boost 25% amplitudes to 50%
  float boost = 1.0f;
  //if (PWavg < normalizeVolumeVal)
  {
    boost = sqrt(normalizeVolumeVal / PWavg);
    saturate(boost, 0.1f, 10.0f); //maximal boost is 100x in power (10x in amplitude)
  }
  return boost;
}

//----------------------------------------------------------------------
//  Replayer (sound-player):

VoNReplayer::VoNReplayer ( VoNClient *client, VoNChannelId ch, VoNCodec *codec ) 
  : m_codec(codec), m_discardVoNMessages(false), m_transmitterFreq(0)
{
  Assert( codec );
  Assert( client || ch == VOID_CHANNEL );
  m_client = client;
  m_channel = ch;
  m_res = VonOK;
  // origins (sample index, system-time): need to be updated?
  m_toDecode = m_newest = 0;
  m_timeOrigin = getSystemTime();
  m_codePtr = NULL;
  m_suspended = false;
  resetDropout(false);
  m_playingCounter = 0;
    // acquire sound frequency & frame from codec info:
  CodecInfo info;
  codec->getInfo(info);
  m_frequency = info.nominalFrequency;
  m_bps       = info.nominalBps;
  m_minFrame  = info.minFrame;
  m_maxFrame  = info.maxFrame;
  codec->groupStart();
  m_toEncode = OriginStartSamples; //only for VoNSay purposes
  m_noMsgCount = 0;
  m_waitingForBuffersCount = 0;
  m_newVoice = false;
}

VoNReplayer::~VoNReplayer ()
{
}

#include "Es/Memory/normalNew.hpp"

void* VoNReplayer::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNReplayer::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNReplayer::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

//RandomJames vonRnd;                         // used in dropout noise generator.

/// Maximal relative amplitude of dropout noise.
const float MAX_DROPOUT_AMPLITUDE = 0.10f;

/// Slope of linear-up dropout phase.
const float LINEAR_UP_DELTA = 0.00001f;

/// Coefficient of exp-down dropout phase.
const float EXP_DOWN_COEF = 0.99992f;

#ifdef SIMPLE_DROPOUT

void VoNReplayer::resetDropout ( bool sounding )
{
  m_dropoutAmp = MAX_DROPOUT_AMPLITUDE;
  setSound(sounding);
}

#define IncDropout

#else

void VoNReplayer::resetDropout ( bool sounding )
{
  m_dropoutState = 0;
  m_dropoutAmp = 0.0f;
  setSound(sounding);
}

#define IncDropout if ( m_dropoutState ) m_dropoutAmp *= EXP_DOWN_COEF; \
                   else if ( (m_dropoutAmp += LINEAR_UP_DELTA) > MAX_DROPOUT_AMPLITUDE ) { \
                     m_dropoutAmp = MAX_DROPOUT_AMPLITUDE; m_dropoutState = 1; }

#endif

void VoNReplayer::generateDropoutSilence ( CircularBufferPointers &ptr, unsigned size )
{
  Assert( ptr.len1 + ptr.len2 >= size );
  unsigned i;
  if ( m_bps > 8 )                          // 16 bits per sample
    for ( i = 0; i++ < size; )
    {
      ptr.write16( 0 );
    }
  else                                      // 8 bits per sample
    for ( i = 0; i++ < size; )
    {
      ptr.write8( 0 );
    }
  setSound(false);
}

void VoNReplayer::generateDropout ( CircularBufferPointers &ptr, unsigned size )
{
#if 0
  Assert( ptr.len1 + ptr.len2 >= size );
  unsigned i;
  if ( m_bps > 8 )                          // 16 bits per sample
    for ( i = 0; i++ < size; )
    {
      ptr.write16( (int16)( (65535.0f * m_dropoutAmp) * vonRnd.uniformNumber() - 32768.0f ) );
      IncDropout;
    }
  else                                      // 8 bits per sample
    for ( i = 0; i++ < size; )
    {
      ptr.write8( (int8)( (255.0f * m_dropoutAmp) * vonRnd.uniformNumber() - 128.0f ) );
      IncDropout;
    }
#endif
  setSound(false);
}

#undef IncDropout

#if DEBUG_VON_DIAGS
DWORD diagAmplTimeStamp=0;
float diagLastAmpl=0, diagMaxAmpl=0;
int diagChatChan=0;
#endif

unsigned VoNReplayer::decode ( VoNSoundBuffer *sb,
                               unsigned pos, unsigned minDec, unsigned maxDec )
{
  Assert(sb);
  if (!sb)
    return 0;

  /*
  // only testing the DeadLockDetector and its *.dld.bidmp dumping
  static bool doit=true;
  if (doit)
  {
    doit = false;
    GDeadLockDetector.TestDLD();
  }
  */

  enter();

  // message from heap
  RefD<NetMessage> msg;
  // packet from message
  VoNDataPacket *packet = NULL;
  // the oldest data ready (in samples)
  unsigned64 ready = INFINITY_U64;         

  // get message from heap
  if ( m_dataMessages.HeapGetFirst(msg) )   // the oldest relevant message (i.e. next to be decoded)
  {
    packet = (VoNDataPacket*)msg->getData();
    Assert(packet);

    CheckChatChannel(packet);
    ready = packet->origin;

    if (m_noMsgCount > 10)
      m_newVoice = true; //there was longer time between messages - assume that this is new voice data

    // reset count for no messages
    m_noMsgCount = 0;

#if DEBUG_VON_SAVE_BUFFERS
    GDebugVoNSaveBuffers.GetReceiveBuf().EnableMessages(true);
    GDebugVoNSaveBuffers.GetReplayBuf().EnableMessages(true);
    GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("Current message on heap: origin=%u, samples=%u, curpos:%u",(unsigned)packet->origin, (unsigned)packet->samples, (unsigned)m_toDecode));
#endif
  }
  else
  {
#if DEBUG_VON_SAVE_BUFFERS
    {
      if ( m_noMsgCount == 0)
      {
        GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("No message received, minDec = %u, queued soundbufs = %d",(unsigned)minDec,  sb->GetQueuedBuffersCount()));
  }
      if ( m_noMsgCount > 5)
      {
        GDebugVoNSaveBuffers.GetReceiveBuf().EnableMessages(false);
        GDebugVoNSaveBuffers.GetReplayBuf().EnableMessages(false);
      }
      GDebugVoNSaveBuffers.GetReplayBuf().SaveTimed(1000);
      GDebugVoNSaveBuffers.GetReceiveBuf().SaveTimed(1000);
    }
#endif
    // increase count for no messages
    m_noMsgCount++;
  }

  // result
  m_res = VonOK;

  if (msg.NotNull())
  {
    // there is some message waiting - indicate that we are playing
    m_playingCounter = MAX_PLAYING_DELAY;
  }

  if ( ready > m_toDecode && minDec == 0 )  // data aren't ready but I need not decode at all
  {
#if DEBUG_VON_SAVE_BUFFERS
    {
      if (GDebugVoNSaveBuffers.GetReceiveBuf().MessagesEnabled())
        GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("Data not ready, but no need to decode - minDec = 0"));
    }
#endif
    leave();
    return 0;
  }

  if (!sb->AreBuffersReady())
  {
    // there is some messages waiting, but audio buffers are not ready
    // we have to wait, when buffers will be prepared (possibly in the main thread)
    // but we don't want to wait too long - we use m_waitingForBuffersCount counter - we wait max 2 decodes, then just continue
    if (msg.NotNull() && m_waitingForBuffersCount < 2)
    {
#if DEBUG_VON_SAVE_BUFFERS
      GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("Waiting for audio buffers to be ready."));
#endif

      m_waitingForBuffersCount++;
      m_newVoice = true;
      leave();
      return 0;
    }

  }
  else
  {
    // reset counter
    m_waitingForBuffersCount = 0;
  }

    // something will be written into *sb:
  CircularBufferPointers cb;
  sb->lockData(pos,maxDec,cb);              // prepare "CircularBufferPointers"
  // for unlockData we need to have the state provided by lockData
  CircularBufferPointers cbLock = cb;
  unsigned decLen = 0;
  unsigned decoded = 0;                     // total count of decoded samples
  int heapSize = m_dataMessages.Size();     // how many messages are waiting?
  unsigned forward = heapSize ? (unsigned)(m_newest - m_toDecode) : 0; // prepared-queue size in samples (ignoring gaps)
  
  // decode at least minDec
  unsigned decodeCnt = minDec;
  if (ready == m_toDecode)
  {
    // we are exactly where we should be (msg in the heap is starting where we finished decoding last time)
    // if there are data available (forward) decode maxDec
    if (maxDec > minDec && forward > minDec)
      decodeCnt = std::min(maxDec, forward);
  }
  
  if (msg.NotNull() && m_newVoice)
  {
    // new voice is starting
    // generate silence (we have to wait a moment so that other messages can arrive and voice can be played fluently)
    decLen = m_frequency >> 3;
    if (decLen > decodeCnt)
      decLen = decodeCnt;

#if DEBUG_VON_SAVE_BUFFERS
    if (GDebugVoNSaveBuffers.GetReceiveBuf().MessagesEnabled())
      GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("Generating initial silence - %d", decLen));
#endif

    generateSilence(cb,m_bps,decLen);
    decoded += decLen;

    m_newVoice = false;
  }

  while ( decoded < decodeCnt )             // some data are needed (do not be lazy: decode more than needed, if data available)
  {
    if ( ready > m_toDecode )               // data aren't ready => generate dropout/silence sound
    {
      if ( decoded >= minDec ) break; // we have already decoded more than minDec, so we don't need to generate any silence

      decLen = minDec - decoded;       
      if ( !heapSize )
      {
      // no messages are waiting - generate dropout silence

        generateDropoutSilence(cb,decLen);
        m_res = VonDropout;               // at least some dropout => return vonDropout

#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
        NetLog("  dropout samples=%u, origin=%u, toDecode=%u",
                (ready==INFINITY_U64)?(decLen>>1):decLen,
                (ready==INFINITY_U64)?0:(unsigned)ready,(unsigned)m_toDecode);
#else
#  ifdef NET_LOG_VOICE_TUNING
        NetLog("Von:drop(%u,%u,%u)",
                (ready==INFINITY_U64)?(decLen>>1):decLen,
                (ready==INFINITY_U64)?0:(unsigned)ready,(unsigned)m_toDecode);
#  endif
#endif

#if DEBUG_VON_SAVE_BUFFERS
        {
        if (GDebugVoNSaveBuffers.GetReceiveBuf().MessagesEnabled())
          GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("  dropout samples=%u, origin=%u, toDecode=%u",
          (ready==INFINITY_U64)?(decLen>>1):decLen,
          (ready==INFINITY_U64)?0:(unsigned)ready,(unsigned)m_toDecode));
        }
#endif
      }
      else
      {
        // there are some messages waiting - just skip the silence

#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
        NetLog("  ignoring dropout: |heap|=%d, ready=%u, skip=%u, origin=%u, toDecode=%u",
          heapSize,forward,decLen,(unsigned)ready,(unsigned)m_toDecode);
#else
#  ifdef NET_LOG_VOICE_TUNING
        NetLog("Von:ignDrop(%d,%u,%u,%u,%u)",
          heapSize,forward,decLen,(unsigned)ready,(unsigned)m_toDecode);
#  endif
#endif
#if DEBUG_VON_SAVE_BUFFERS
        {
          if (GDebugVoNSaveBuffers.GetReceiveBuf().MessagesEnabled())
            GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("ignoring dropout: |heap|=%d, ready=%u, skip=%u, origin=%u, toDecode=%u",
            heapSize,forward,decLen,(unsigned)ready,(unsigned)m_toDecode));
        }
#  endif

        resetDropout(false);
        m_toDecode = ready;
        forward = (unsigned)(m_newest - m_toDecode);
        decLen = 0;
      }

      decoded  += decLen;
    } 
    else                  // more data to decode?
    {
      Assert( m_toDecode == ready );        // must have at least one data frame ready..

      if (m_minFrame > decodeCnt - decoded)
        break; //not enough space in buffer

      if ( !m_codePtr )                     // starting a new message:
      {
        m_codePtr = packet->data;

        //@{ extract possible special data (ItemTransmitter frequency)
        if ((packet->chatChannel&~TTChannelMask) == 2/*CCItemTransmitter*/)
        {
          m_transmitterFreq = *reinterpret_cast<float *>(m_codePtr);
          m_codePtr = (unsigned16*)(((unsigned8*)m_codePtr) + sizeof(float)); //skip
        }
        //@}

#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
        NetLog("  starting message (ch=%d, origin=%u, samples=%u, code=%u, |heap|=%d, ready=%u)",
               (int)packet->channel,(unsigned)packet->origin,(unsigned)packet->samples,(unsigned)packet->size,heapSize,forward);
#endif
#if DEBUG_VON_SAVE_BUFFERS
        {
          if (GDebugVoNSaveBuffers.GetReceiveBuf().MessagesEnabled())
            GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("  starting message (ch=%d, origin=%u, samples=%u, code=%u, |heap|=%d, ready=%u)",
            (int)packet->channel,(unsigned)packet->origin,(unsigned)packet->samples,(unsigned)packet->size,heapSize,forward));
        }
#endif

      }
      unsigned16 packetSize = *m_codePtr++;
      Assert( packetSize >= 2 );            // code packet must not be empty

        // decode one code packet (see "vonNet.hpp"):
      if ( packetSize == 2 )                // silent packet
      {
        resetDropout(false);
        decLen = *m_codePtr++;              // number of "silent" samples
        if (decLen > decodeCnt-decoded) decLen = decodeCnt-decoded;

          // skip over part of silent packet if too much data is ready:
        if ( forward > m_frequency )
        {
          unsigned skipLen = (decLen * forward) / (m_frequency << 2);
                                            // 1 sec .. 0.25, 4 sec .. 1.0
          if ( skipLen > decLen ) skipLen = decLen;
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
          NetLog("  ignoring silence: |heap|=%d, ready=%u, skip=%u (from %u)",heapSize,forward,skipLen,decLen);
#else
#  ifdef NET_LOG_VOICE_TUNING
          NetLog("Von:ignSil(%d,%u,%u/%u)",heapSize,forward,skipLen,decLen);
#  endif
#endif
#if DEBUG_VON_SAVE_BUFFERS
          {
            if (GDebugVoNSaveBuffers.GetReceiveBuf().MessagesEnabled())
              GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("  ignoring silence: |heap|=%d, ready=%u, skip=%u (from %u)",heapSize,forward,skipLen,decLen));
          }
#endif

          decoded -= skipLen;
          if ( skipLen < decLen )
          {
#if DEBUG_VON_SAVE_BUFFERS
            {
              if (GDebugVoNSaveBuffers.GetReceiveBuf().MessagesEnabled())
                GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("  generating silence: %d",decLen-skipLen));
            }
#endif
            generateSilence(cb,m_bps,decLen-skipLen);
            m_res = VonSilence;             // at least some silence => return vonSilence
          }
        }

        else                                // regular silent packet
        {
          generateSilence(cb,m_bps,decLen);
          m_res = VonSilence;               // at least some silence => return vonSilence
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
          NetLog("  silent samples=%u, origin=%u",decLen,(unsigned)m_toDecode);
#else
#  ifdef NET_LOG_VOICE_TUNING
          NetLog("Von:sil(%u,%u)",decLen,(unsigned)m_toDecode);
#  endif
#endif
#if DEBUG_VON_SAVE_BUFFERS
          {
            if (GDebugVoNSaveBuffers.GetReceiveBuf().MessagesEnabled())
              GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("  silent samples=%u, origin=%u",decLen,(unsigned)m_toDecode));
          }
#endif
        }
      }

      else                                  // "sounding" packet
      {
        //  resetDropout(true); is no longer used, for these reasons:
        //  1. first word of speech was not replayed often
        //  2. the speech was choppy sometimes HEllo WOrld (only Capital letters were replayed)
        // BUT: this can be possibly caused by wrong usage of VoNReplayer::isPlaying outside vonApp.cpp, namely VoNSoundBufferOAL (sorting voices, getting Loudness)
        m_playingCounter = MAX_PLAYING_DELAY;
        unsigned inSize = packetSize;
        decLen = m_minFrame;

        CircularBufferPointers bak = cb;
        VoNResult r = m_codec->decode(cb,decLen,(unsigned8*)m_codePtr,inSize);
        Assert( inSize == packetSize );
        // Assert( inSize <= packetSize );  // more tolerant check
        m_codePtr = (unsigned16*)((unsigned8*)m_codePtr + packetSize);
        if ( r != VonOK ) m_res = r;
        if ( forward > (m_frequency << 1) ) // at least partially ignore the sounding packet:
        {                                   // 2 sec .. 0.25, 8 sec .. 1.0
          unsigned skipLen = (decLen * forward) / (m_frequency << 3);
          if ( skipLen > decLen ) skipLen = decLen;
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
          NetLog("  ignoring voice: |heap|=%d, ready=%u, skip=%u (from %u)",heapSize,forward,skipLen,decLen);
#else
#  ifdef NET_LOG_VOICE_TUNING
          NetLog("Von:ignVoi(%d,%u,%u/%u)",heapSize,forward,skipLen,decLen);
#  endif
#endif
#if DEBUG_VON_SAVE_BUFFERS
          {
            if (GDebugVoNSaveBuffers.GetReceiveBuf().MessagesEnabled())
              GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("  ignoring voice: |heap|=%d, ready=%u, skip=%u (from %u)",heapSize,forward,skipLen,decLen));
          }
#endif

          cb = bak;
          if ( skipLen )
          {
            decoded -= skipLen;
            if ( m_bps > 8 )
              cb.skip16(skipLen);
            else
              cb.skip8(skipLen);
          }
        }
        else
        {
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON || DEBUG_VON_DIAGS
          float amplitude=0;
          bool amplitudeIsSilence = GetAmplitude(bak, decLen, amplitude);
#         if DEBUG_VON || defined(NET_LOG_VOICE_VERBOSE)
            LogF("  voice samples=%u, origin=%u ampl=%.1f %% (%s)",
              decLen,(unsigned)m_toDecode, amplitude*100.0f, amplitudeIsSilence ? "silence" : "notSilence");
#         endif
#if DEBUG_VON_DIAGS
          if (diagMaxAmpl < amplitude) diagMaxAmpl = amplitude;
          DWORD sysTime = ::GetTickCount();
          if (sysTime-diagAmplTimeStamp>=1000)
          {
            diagAmplTimeStamp = sysTime;
            diagLastAmpl = diagMaxAmpl;
            diagChatChan = m_playerInfo.m_chatChannel;
            if (m_playerInfo.m_play2D) diagChatChan |= TTChan2D;
            if (m_playerInfo.m_play3D) diagChatChan |= TTChan3D;
            if (!m_playerInfo.m_chatChannel) diagChatChan |= TTDirectChan;
            diagMaxAmpl = 0;
          }
#endif
#else
#  ifdef NET_LOG_VOICE_TUNING
          NetLog("Von:voi(%u,%u)",decLen,(unsigned)m_toDecode);
#  endif
#endif
#if DEBUG_VON_SAVE_BUFFERS
          {
            if (GDebugVoNSaveBuffers.GetReceiveBuf().MessagesEnabled())
              GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("  voice samples=%u, origin=%u ampl=%.1f %% (%s)",
              decLen,(unsigned)m_toDecode, amplitude*100.0f, amplitudeIsSilence ? "silence" : "notSilence"));
          }
#endif
        }
      }

      Assert( (m_codePtr - packet->data) * 2 < (int)(VON_DATA_OFFSET + packet->size) );

      packet->origin = (m_toDecode += decLen);
      decoded += decLen;
      forward -= decLen;

          // looking for the next code packet:
      if ( !*m_codePtr )                    // end of the message
      {
        m_dataMessages.HeapRemoveFirst();
        heapSize--;
        m_codec->groupStart();
        m_codePtr = NULL;                   // new message => reset code pointer
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
        NetLog("  finishing message at=%u",(unsigned)m_toDecode);
#endif
#if DEBUG_VON_SAVE_BUFFERS
        {
          if (GDebugVoNSaveBuffers.GetReceiveBuf().MessagesEnabled())
            GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("  finishing message at=%u",(unsigned)m_toDecode));
        }
#endif

        if ( m_dataMessages.HeapGetFirst(msg) )
        {
          packet = (VoNDataPacket*)msg->getData();
          CheckChatChannel(packet);
          Assert( packet->data[0] );
          ready = packet->origin;
        }
        else
          ready = INFINITY_U64;
      }
      else
        ready += decLen;                    // the current message continues..
    }

  }
  Assert( decoded <= maxDec );
#if DEBUG_VON
  //this tests VoN replayer by feeding harmonic tone every couple of seconds
  static bool harmonicToneTest = false;
  static int counter = 0, countMax = 50;
  static int decodedSum = 0;
  decodedSum += decoded;
  if (counter++ > countMax)
  {
    counter = 0;
    LogF("*** HARMONIC TONE FEED: decodedSum=%d", decodedSum);
    decodedSum = decoded;
    if (harmonicToneTest)
    {
      CircularBufferPointers cbTest = cbLock;
      int len = cbTest.len1+cbTest.len2;
      int i;
      if ( m_bps > 8 )
        for ( i = 0; i < len; i++ )
          cbTest.write16( (int16)( sin(0.3*i) * 30000.0 ) );
      else
        for ( i = 0; i < len; i++ )
          cbTest.write8( (int8)( sin(0.3*i) * 100.0 ) );
      decoded = (len>0 ? (len-1) : len);
    }
  }
#endif

  if (decoded > 0)
  {
    int play2D3Dflag = ((m_playerInfo.m_play2D ? TTChan2D : 0) | (m_playerInfo.m_play3D ? TTChan3D : 0));
    if (!m_playerInfo.m_chatChannel) play2D3Dflag |= TTDirectChan;
    if (!play2D3Dflag) play2D3Dflag = TTChan2D; //there are data, so somewhat must be played (dropout silence was probably generated)
    sb->update2D3D(play2D3Dflag);
    sb->unlockData(decoded,cbLock);
  }
  leave();
#if defined(NET_LOG_VOICE_VERBOSE) || DEBUG_VON
  NetLog("  decoded samples=%u",decoded);
#endif
  return decoded;
}

void VoNReplayer::newDataMessage ( NetMessage *msg )
{
  Assert( msg );
  VoNDataPacket *packet = (VoNDataPacket*)msg->getData();
  Assert( packet->channel == m_channel );
  Assert( packet->size + VON_DATA_OFFSET <= msg->getLength() );
  enter();

#if DEBUG_VON_SAVE_BUFFERS
  GDebugVoNSaveBuffers.GetReceiveBuf().AddMessage(Format("New message: origin=%u, curpos:%u, suspended=%d",
    (unsigned)packet->origin, (unsigned)m_toDecode, (int)m_suspended));
#endif

  if ( m_suspended )
  {
    leave();
    return;
  }
  if ( packet->origin >= m_toDecode )       // ignore obsolete messages
  {
    if (!m_discardVoNMessages)
    {
      RefD<NetMessage> rmsg = msg;
      m_dataMessages.HeapInsert(rmsg);
      // update timeOrigin:
      unsigned64 newOrigin = getSystemTime() - (1000000L / m_frequency) * packet->origin;
      if ( newOrigin < m_timeOrigin ) m_timeOrigin = newOrigin; //m_timeOrigin is only set, never used for any other purpose
      // update newest:
      unsigned64 newNewest = packet->origin + packet->samples;
      if ( newNewest > m_newest ) m_newest = newNewest;
#ifdef NET_LOG_VOICE_VERBOSE
      NetLog("VoNReplayer::newDataMessage: accepting message (origin=%u, advance=%u, |heap|=%d)",
        (unsigned)packet->origin,(unsigned)(packet->origin + packet->samples - m_toDecode),m_dataMessages.Size());
#else
#  ifdef NET_LOG_VOICE_TUNING
      NetLog("Von:new(%u,%u,%u,%d)",
        (unsigned)packet->origin,(unsigned)packet->samples,
        (unsigned)(packet->origin + packet->samples - m_toDecode),m_dataMessages.Size());
#  endif
#endif
    }
#if defined NET_LOG_VOICE_VERBOSE || NET_LOG_VOICE_P2P>10
    else
      NetLog("VoNReplayer::newDataMessage: ignoring message as SoundBuffer creation failed (origin=%u, toDecode=%u)",
      (unsigned)packet->origin,(unsigned)m_toDecode);
#endif
  }
#if defined NET_LOG_VOICE_VERBOSE || NET_LOG_VOICE_P2P>10
  else
    NetLog("VoNReplayer::newDataMessage: ignoring obsolete message (origin=%u, toDecode=%u)",
           (unsigned)packet->origin,(unsigned)m_toDecode);
#else
#  ifdef NET_LOG_VOICE_TUNING
  else
    NetLog("Von:ign(%u,%u)",
           (unsigned)packet->origin,(unsigned)m_toDecode);
#  endif
#endif
  leave();
}

void VoNReplayer::suspend ( bool on )
{
  enter();
  if ( m_suspended == on )
  {
    leave();
    return;
  }
  if ( (m_suspended = on) )                 // enter the "suspended" state
  {
    m_dataMessages.Clear();
  }
  else                                      // resume the replayer..
  {
    m_toDecode = m_newest = 0;              // the "init" mode will be started again..
    m_timeOrigin = getSystemTime();
    m_codePtr = NULL;
    resetDropout(false);
  }
  m_playingCounter = 0;                     // reset playing-delay in both cases
  leave();
}

void VoNReplayer::setSound ( bool sounding )
{
  if ( sounding )
  {
    if ( ++m_playingCounter > MAX_PLAYING_DELAY )
      m_playingCounter = MAX_PLAYING_DELAY;
  }
  else
  {
    if ( --m_playingCounter < -MAX_PLAYING_DELAY )
      m_playingCounter = -MAX_PLAYING_DELAY;
  }
}

float VoNReplayer::getPlaying ()
{
  int counter = m_playingCounter;           // weak form of MT-safety
  if ( counter <= 0 ) return 0.0f;
  if ( counter >= MAX_PLAYING_DELAY ) return 1.0f;
  return( (counter + 0.0f) / MAX_PLAYING_DELAY );
}
#endif //_ENABLE_VON

//----------------------------------------------------------------------
//  VoNChannel (lives inside of VoNServer):

VoNChannel::VoNChannel ( VoNChannelId i_id )
{
  id = i_id;
}

#include "Es/Memory/normalNew.hpp"

void* VoNChannel::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNChannel::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNChannel::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  VoN server (VoNServer class):

NetStatus vonServerReceive ( NetMessage *msg, NetStatus event, void *data )
{
  if ( !msg ) return nsNoMoreCallbacks;     // empty message
  unsigned len = msg->getLength();
  unsigned flags = msg->getFlags();
  if ( !(flags & MSG_VOICE_FLAG) ||
       len < sizeof(VoNMagicPacket) )
    return nsNoMoreCallbacks;               // bad message format

  VoNMagicPacket *vonHdr = (VoNMagicPacket*)msg->getData();
  unsigned32 magic  = vonHdr->magic;
  VoNChannelId chId = vonHdr->channel;
  if ( magic != MAGIC_VON_CTRL )
    return nsNoMoreCallbacks;               // unknown magic number -> drop the message

  VoNServer *server = (VoNServer*)data;
  Assert( server );

#if _ENABLE_REPORT
  if (magic == MAGIC_VON_CTRL)
    server->OnCtrlReceived(len+MSG_HEADER_LEN);
  else
    server->OnDataReceived(len+MSG_HEADER_LEN);
#endif

  // this channel is for control messages only
  if ( magic != MAGIC_VON_CTRL )
  {
    LogF("VoN server: Unexpected non-control message");
    return nsNoMoreCallbacks;
  }
  
  // check the voice-channel (source):
  RefD<VoNChannel> act;

    // check (and remember) VON_CTRL_CREATE message:
  int command = ((VoNControlPacket*)vonHdr)->command;
  if ( command == VON_CMD_CREATE )
          // remember this message:
    server->m_createMessages.put(msg);

  if (command==VON_CMD_REMOVE)
  {
    server->enter();
    if ( server->m_ch.get(chId,act) )         // known voice-channel -> delete it
    {
      server->m_ch.removeKey(chId);
    }
    server->m_peerCliquesDirty = true;
    server->leave();
  }
  if (command == VON_CMD_PEER_INFO)
  {
    server->enter();
    server->m_peerInfo.put(msg);
    server->m_peerCliquesDirty = true;
    server->leave();
  }
  else if (command == VON_CMD_CREATE || command==VON_CMD_REMOVE)
  {
    // it seems we need to resend create messages
    // it is not very clear why, as they should be resent as needed
    if ( server->m_ch.get(chId,act) )         // known voice-channel -> routing the message
    {
  #ifdef NET_LOG_VOICE_VERBOSE
      NetLog("vonServerReceive(ctrl): ch=%d",(int)chId);
  #endif
      IteratorState it;
      VoNIdentity dest;
      act->ids.getFirst(it,dest);
      while ( it != ITERATOR_NULL )           // route message to one destination:
      {
        RefD<NetChannel> channel =            // destination net-channel
          server->netChannelFromIdentity(dest);
        if ( channel )                        // valid net-channel
        {
            // copy the message:
          Ref<NetMessage> out = NetMessagePool::pool()->newMessage(len,channel);
          if ( out )
          {
  #ifdef VON_CTRL_VIM
            out->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG | ((magic==MAGIC_VON_CTRL) ? MSG_VIM_FLAG : 0) );
  #else
            out->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG);
  #endif
            out->setData((unsigned8*)vonHdr,len);
            out->send(true);
  #if _ENABLE_REPORT
            if (magic == MAGIC_VON_CTRL)
              server->OnCtrlSent(len);
            else
              server->OnDataSent(len);
  #endif
          }
        }
        act->ids.getNext(it,dest);
      }
    }
  }
  else if (command == VON_CMD_ENCAPSULATE_VOICE)
  {
    struct sockaddr_in distantAddr;
    msg->getDistant(distantAddr);
    RefD<VoNClient> client = server->getClient();

#ifdef DEBUG_VON_RETRANSLATE
    LogF("vonServerReceive - ENCAPSULATE_VOICE");
#endif

    if (client)
    {
      VoNControlPacket* vonCtrl = (VoNControlPacket*)vonHdr;
      client->PeerToPeerReceive(vonCtrl->data, vonCtrl->size, &distantAddr,false);
    }
  }
  else if (command == VON_CMD_RESET_CONNECTION)
  {
    VoNControlPacket* vonCtrl = (VoNControlPacket*)vonHdr;
    PeerToPeerEdge *edge = reinterpret_cast<PeerToPeerEdge *>(vonCtrl->data);
    RefD<VoNClient> client = server->getClient();
    if (client)
    { // send it only to dpnidFrom (she will reset her transmit target dpnidTo)
      client->sendCtrlTo(edge->dpnidFrom, VON_CMD_RESET_CONNECTION, edge, sizeof(PeerToPeerEdge));
    }
  }
  else
  {
    LogF("VoN server: Unexpected control message type %d",command);
  }
  

  return nsNoMoreCallbacks;
}

VoNServer::VoNServer ( VoNSystem *von )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNServer::VoNServer");
#endif
  Assert( von );
  m_von = von;
  m_peerCliquesDirty = false;
  m_peerCliques.Init(32);
  m_matrixIds.Init(32);
#if _ENABLE_REPORT
  ResetStats();
#endif
}

void VoNServer::sendCreate ( VoNChannelId id, VoNIdentity dest )
{
  RefD<NetMessage> cr;
  if ( m_createMessages.get(id,cr) )
  {
    RefD<NetChannel> channel =              // destination net-channel
      netChannelFromIdentity(dest);
    if ( channel )                          // valid net-channel
    {
        // copy the message:
      unsigned len = cr->getLength();
#if defined NET_LOG_VOICE || NET_LOG_VOICE_P2P>10
      NetLog("VoNServer::sendCreate: from=%d, to=%d, len=%u",(int)id,(int)dest,len);
#endif
      Ref<NetMessage> out = NetMessagePool::pool()->newMessage(len,channel);
      if ( out )
      {
#ifdef VON_CTRL_VIM
        out->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG|MSG_VIM_FLAG);
#else
        out->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG);
#endif
        out->setData((unsigned8*)cr->getData(),len);
        out->send(true);
#if _ENABLE_REPORT
        OnCtrlSent(len);
#endif
      }
    }
  }
  else
  {
    // no create message ready. This can happen if there is no encoder on the channel.
  }
}

/// client id  comparison used for QSort
/** We use reversed order to make sure host is listed last */
static inline int CompareHostId(const int *a1, const int *a2)
{
  return *a2-*a1;
}

/// integer comparison used for QSort
static inline int CompareInt(const int *a1, const int *a2)
{
  return *a1-*a2;
}

/// compare if two cliques are the same

static bool CliqueEqual(const AutoArray<int> &c1, const AutoArray<int> &c2)
{
  if (c1.Size()!=c2.Size()) return false;
  for (int i=0; i<c1.Size(); i++)
  {
    if (c1[i]!=c2[i]) return false;
  }
  return true;
}
/**
Called from main thread - may allocate memory as needed
*/
void VoNServer::updateTransmitTargets()
{
  // if we did not receive any new info, no need to update
  if (!m_peerCliquesDirty) return;
  
  enter();

  {  
    m_peerCliquesDirty = false;
    
    // build index to channel conversion
    m_matrixIds.Resize(0);
    IteratorState it;
    RefD<VoNChannel> ch;
    VoNChannelId dpid;
    m_ch.getFirst(it,ch,&dpid);
    while ( it != ITERATOR_NULL )
    {
      m_matrixIds.Add(dpid);
      //to.Append() = i;
      m_ch.getNext(it,ch,&dpid);
    }

    // server (bot-client) is always here
    m_matrixIds.AddUnique(EXT_BOT_CLIENT);

    int channels = m_matrixIds.Size();
    
    // keep matrix ids sorted so that cliques are sorted as well
    QSort(m_matrixIds,CompareHostId);
    
    int server = m_matrixIds.Find(EXT_BOT_CLIENT);

    // initialize matrix, clear all edges
    m_matrix.Resize(channels);
    for (int row=0; row<channels; row++) for (int col=0; col<channels; col++)
      m_matrix(row,col) = 0;
    
    // scan all peer info messages waiting, and build adjacency matrix
    for (int i=0; i<m_matrixIds.Size(); i++)
    {
      int dpid = m_matrixIds[i];
      RefD<NetMessage> msg;
      if (m_peerInfo.get(dpid,msg) && msg.NotNull())
      {
    
        // build the peer matrix
        // read the peer info indices
        //const char *msgData = (const char *)msg->getData();
        const VoNControlPacket *header = (const VoNControlPacket *)msg->getData();
        // TODO: check message size here
        const int *intData = (const int *)header->data;
        int fromIndex = i;
        int nPeers = *intData++;
        //LogF("PeerToPeer %d",header->channel);
        for (int j=0; j<nPeers; j++)
        {
          int toId = *intData++;
          // index known - update matrix
          int toIndex = m_matrixIds.Find(toId);
          if (toIndex>=0)
          {
            //LogF("  to %d",toId);
            m_matrix(fromIndex,toIndex)=true;
            //we cannot add m_matrix(toIndex,fromIndex)=true;
            //as P2P connection is not necessarily bidirectional (it should be, but who knows?!)
          }
        }
      }
    }
    //tempMatrix is used to convert m_matrix into symmetric one
    AdjacencyMatrix tempMatrix;
    tempMatrix.Resize(channels);
    for (int row=0; row<channels; row++) for (int col=0; col<row; col++)
    {
      if ( m_matrix(row,col)!=m_matrix(col,row) )
      { //matrix should be symmetric. Store zero = unconnected.
        tempMatrix(row,col)=tempMatrix(col,row)=0; //there is no connection in one direction, clear both
      }
      else
      { //both values are the same, copy it
        tempMatrix(row,col)=tempMatrix(col,row)=m_matrix(row,col);
      }
    }
    for (int row=0; row<channels; row++)
    {
      // everyone is talking to himself
      // everyone is talking to the server
      tempMatrix(row,row)=tempMatrix(row,server)=tempMatrix(server,row)=true;
    }
    
    // we have the matrix - build cliques now
    m_peerCliques.Resize(0);
  
    if (tempMatrix.Size()>0)
    {
      tempMatrix.FindCliques(m_peerCliques);
    }
    
    // normalize cliques
    for (int i=0; i<m_peerCliques.Size(); i++)
    {
      QSort(m_peerCliques[i],CompareInt);
    }
    
    // remove identical cliques
    if (m_peerCliques.Size()>1)
    {
      int d=1;
      for (int s=1; s<m_peerCliques.Size(); s++)
      {
        if (!CliqueEqual(m_peerCliques[s-1],m_peerCliques[s]))
        {
          if (s!=d) m_peerCliques[d] = m_peerCliques[s];
          d++;
        }
      }
      m_peerCliques.Resize(d);
      m_peerCliques.Compact();
    }
#if 0
    //only testing (create many cliques to make cliques packet cut)
    for (int bebul=0; bebul<100; bebul++)
    {
      int ix = m_peerCliques.Add();
      AutoArray<VoNChannelId> &clq = m_peerCliques[ix];
      const int fakeSize = 32;
      clq.Realloc(fakeSize); clq.Resize(fakeSize);
      for (int ixc=0; ixc<fakeSize; ixc++) clq[ixc]=0;
    }
#endif
    
    // now we need to send the list of cliques to all clients
    // The convention is: 
    //     msg[0] >= 0 ... cliques should rewrite the client's cliques (reset first)
    //     msg[0] <  0 ... cliques should be added to client's cliques (without reset)
    // note: negative msg[0] is used to avoid too large cliques packets
    const int maxCliquePacketItemCount = (NetChannelBasic::par.maxPacketSize - IP_UDP_HEADER - MSG_HEADER_LEN) / sizeof(int);
    // first build a message
    AutoArray<int, MemAllocLocal<int,1024> > msg;
    
  #if defined NET_LOG_VOICE || NET_LOG_VOICE_P2P>0
      NetLog("von: sending clique to everyone");
  #endif
    
    msg.Add(m_peerCliques.Size()); // msg[0] is the number of cliques
    int packetNum=0;
    int cliqueCount=0;
    for (int i=0; i<m_peerCliques.Size(); i++)
    {
      const AutoArray<int> &clique = m_peerCliques[i];
      #if defined NET_LOG_VOICE || NET_LOG_VOICE_P2P>0
        NetLog("  clique %d",i);
      #endif
      if ( (msg.Size()+clique.Size()+1) < maxCliquePacketItemCount )
      { // there is enough space to add new clique into packet
        cliqueCount++; //add clique count only when some cliques are really inserted
        msg.Add(clique.Size());
        for (int j=0; j<clique.Size(); j++)
        {
          // convert matrix indexes to real dpid
          msg.Add(m_matrixIds[clique[j]]);
        }
      }
      else
      { // we should cut the cliques into more packets
        if (!packetNum++)
          msg[0] = cliqueCount;  // positive number will cause the cliques array reset on client side
        else
          msg[0] = -cliqueCount; //next part of the clique message
        sendCtrl(EXT_BOT_CLIENT, VON_CMD_PEER_CLIQUES, msg.Data(), msg.Size()*sizeof(int),true);
        // prepare everything to send the rest of the cliques
        msg.Resize(0);   //clear the message
        msg.Add(m_peerCliques.Size()); // msg[0] is the number of cliques (reserve space)
        cliqueCount = 0;
        i--;  // current clique was not sent yet
      }
      #if NET_LOG_VOICE_P2P>0
        BString<256> text;
        for (int j=0; j<clique.Size(); j++)
        {
          BString<80> idString;
          sprintf(idString," %d",m_matrixIds[clique[j]]);
          strcat(text,idString);
        }
        NetLog("    %s",cc_cast(text));
      #endif
    }
    
    // we need to notify all recipients
    // any channel should do, as everyone should be connected with everyone
    if (!packetNum)
      msg[0] = cliqueCount;
    else
      msg[0] = -cliqueCount;
    sendCtrl(EXT_BOT_CLIENT, VON_CMD_PEER_CLIQUES, msg.Data(), msg.Size()*sizeof(int),true);
    // our local client needs data as well
    // send him a message
  }
 
  leave();
}


/// Global DebugVoNBank for collecting clients VoN features for server
DEBUG_VON_BANK_GLOBAL
DebugVoNBank GDebugVoNBank;

#if DEBUG_VON_SAVE_BUFFERS
DebugVoNSaveBuffers GDebugVoNSaveBuffers;
#endif

/// gets P2P adjacency matrix and cliques
void VoNServer::GetVoNTopologyDiag(QOStrStream &out, Array<ConvPair> &convTable)
{
#ifndef DISABLE_VON_TOPOLOGY_DIAG
  //build conversion table based on convTable (index into row,col of m_matrix should match dpid2cdid indexes)
  int siz=m_matrix.Size();
  AutoArray<RString, MemAllocLocal<RString, 64> > dpid2cdid;
  AutoArray<RString, MemAllocLocal<RString, 64> > dpid2name;
  dpid2cdid.Realloc(siz);dpid2cdid.Resize(siz);
  dpid2name.Realloc(siz);dpid2name.Resize(siz);
  for (int i=0; i<siz; i++)
  {
    int dpid = m_matrixIds[i];
    for (int j=0; j<convTable.Size(); j++)
    {
      if (convTable[j]._dpnid==dpid) { dpid2cdid[i] = convTable[j]._id; dpid2name[i] = convTable[j]._name; break; }
    }
    if (dpid2cdid[i].IsEmpty()) dpid2cdid[i] = Format("#%d", dpid);
  }
  //store P2P matrix 
  bool isAssymetric = false;
  out << "Matrix:\n";
  for (int i=0; i<siz; i++)
  {
    out <<  Format("  %s (%s): ", cc_cast(dpid2cdid[i]), cc_cast(dpid2name[i]));
    bool first = true;
    for (int j=0; j<siz; j++)
    {
      if (m_matrix(i,j)) 
      {
        if (!m_matrix(j,i)) isAssymetric=true;
        if (!first) out << ",";
        else first = false;
        out << dpid2cdid[j];
      }
    }
    out << "\n";
  }
  //store P2P cliques
  out << "Cliques:\n";
  for (int i=0; i<m_peerCliques.Size(); i++)
  {
    out << "  ";
    for (int j=0, siz=m_peerCliques[i].Size(); j<siz; j++)
    {
      out << dpid2cdid[m_peerCliques[i][j]];
      if (j<siz-1) out << ",";
    }
    if (i!=m_peerCliques.Size()-1) out << "\n"; //no newline at the end of report
  }
  if (isAssymetric) out << "\nAssymetric P2P matrix!";
#endif
}

void VoNServer::setChannel ( VoNChannelId id, AutoArray<int, MemAllocSA> &to )
{
  int toSize = to.Size();
#ifdef NET_LOG_VOICE
  char buf[512];
  char *ptr = buf;
  for ( int j = 0; j < toSize; j++ )
    ptr += sprintf(ptr," %d",to[j]);
  ptr[0] = (char)0;
#endif
  enter();
  RefD<VoNChannel> act;

  if ( !m_ch.get(id,act) )                  // brand-new channel (I've never seen it before)
  {
    act = new VoNChannel(id);
    m_ch.put(id,act);
    
    m_peerCliquesDirty = true;
#ifdef NET_LOG_VOICE
    NetLog("VoNServer::setChannel: from=%d, to:%s (was 0)",(int)id,buf);
#endif
  }
#ifdef NET_LOG_VOICE
  if ( act->ids.card() != toSize )          // log only changes..
    NetLog("VoNServer::setChannel: from=%d, to:%s (was %u)",(int)id,buf,act->ids.card());
#endif

    // 1. create new items:
  int i;
  for ( i = 0; i < toSize; i++ )
    if ( act->ids.present(to[i]) )
      act->ids.remove(to[i]);               // won't suspend it
    else
      //sendCreate(id,to[i]);                 // new destination => send VON_CTRL_CREATE message
      sendCreate(to[i],id);                 // new destination => send VON_CTRL_CREATE message

    // 3. build new channel-map:
  act->ids.reset();
  for ( i = 0; i < toSize; i++ )
    act->ids.put(to[i]);

  
  leave();
}

void VoNServer::getChannel ( VoNChannelId id, AutoArray<int, MemAllocSA> &to )
{
  enter();
  RefD<VoNChannel> act;
  to.Clear();
  if ( m_ch.get(id,act) )
  {
    IteratorState it;
    VoNIdentity i;
    act->ids.getFirst(it,i);
    while ( it != ITERATOR_NULL )
    {
      to.Append() = i;
      act->ids.getNext(it,i);
    }
  }
  leave();
}

RefD<VoNClient> VoNServer::getClient()
{
  return m_von->getClient();
}

void VoNServer::connect ( VoNChannelId id )
{
  RefD<NetChannel> netCh = netChannelFromIdentity(id);
  if ( !netCh ) return;
    // network routing-table item:
  RoutingItem ri;
  ri.andFlag        = MSG_VOICE_FLAG;
  ri.eqFlag         = MSG_VOICE_FLAG;
  ri.processRoutine = vonServerReceive;
  ri.data           = this;
  netCh->setSubsetRoutine(ri);
}

/**
@param echo send to the channel owner as well
*/
void VoNServer::sendCtrl (
  VoNChannelId id, unsigned command, const void *data, int size, bool echo
)
{
  if (echo)
  {
    RefD<NetChannel> channel = netChannelFromIdentity(id);
    if (channel)
    {
      Ref<NetMessage> out = NetMessagePool::pool()->newMessage(sizeof(VoNControlPacket)+size,channel);
      if ( out )
      {
#ifdef VON_CTRL_VIM
        out->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG|MSG_VIM_FLAG);
#else
        out->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG);
#endif
        out->setLength(sizeof(VoNControlPacket)+size);
        VoNControlPacket *vonHdr = (VoNControlPacket*)out->getData();
        vonHdr->magic   = MAGIC_VON_CTRL;
        vonHdr->channel = id;
        vonHdr->command = command;
        vonHdr->size    = size;
        
        memcpy(vonHdr->data,data,size);
        
        out->send(true);
#if _ENABLE_REPORT
        OnCtrlSent(out->getLength()+MSG_HEADER_LEN);
#endif
      }
    }
  }
    // send CTRL message to all potential listeners..
  RefD<VoNChannel> act;
  if ( m_ch.get(id,act) )                   // known voice-channel -> send the message
  {
    IteratorState it;
    VoNIdentity dest;
    act->ids.getFirst(it,dest);
    while ( it != ITERATOR_NULL )           // route message to one destination:
    {
      RefD<NetChannel> channel =            // destination net-channel
        netChannelFromIdentity(dest);
      if ( channel )                        // valid net-channel
      {
          // copy the message:
        Ref<NetMessage> out = NetMessagePool::pool()->newMessage(sizeof(VoNControlPacket)+size,channel);
        if ( out )
        {
#ifdef VON_CTRL_VIM
          out->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG|MSG_VIM_FLAG);
#else
          out->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG);
#endif
          out->setLength(sizeof(VoNControlPacket)+size);
          VoNControlPacket *vonHdr = (VoNControlPacket*)out->getData();
          vonHdr->magic   = MAGIC_VON_CTRL;
          vonHdr->channel = id;
          vonHdr->command = command;
          vonHdr->size    = size;
          
          memcpy(vonHdr->data,data,size);
          
          out->send(true);
#if _ENABLE_REPORT
          OnCtrlSent(out->getLength()+MSG_HEADER_LEN);
#endif
        }
      }
      act->ids.getNext(it,dest);
    }
  }
}

void VoNServer::disconnect ( VoNChannelId id )
{
  sendCtrl(id,VON_CMD_REMOVE);
  RefD<NetChannel> netCh = netChannelFromIdentity(id);
  if (netCh)
  {
      // network routing-table item:
    RoutingItem ri;
    ri.andFlag        = MSG_VOICE_FLAG;
    ri.eqFlag         = MSG_VOICE_FLAG;
    ri.processRoutine = vonServerReceive;
    ri.data           = this;
    netCh->removeSubsetRoutine(ri);
  }
  
  enter();
  // remove his channel
  m_ch.removeKey(id);
  // remove his create message as well
  m_createMessages.removeKey(id);
  // remove his peer info
  m_peerInfo.removeKey(id);
  
  // and remove his channel from all other channels
  IteratorState it;
  RefD<VoNChannel> ch;
  // loop through all channels
  m_ch.getFirst(it,ch);
  while ( it != ITERATOR_NULL )
  {
    ch->ids.removeKey(id);
    //to.Append() = i;
    m_ch.getNext(it,ch);
  }

  // force recalc.
  m_peerCliquesDirty = true;
  leave();
  
}

VoNServer::~VoNServer ()
{
#ifdef NET_LOG_VOICE
  NetLog("VoNServer::~VoNServer");
#endif
}

//----------------------------------------------------------------------
//  VoN client (VoNClient class):

NetStatus vonClientReceive ( NetMessage *msg, NetStatus event, void *data )
{
  if ( !msg ) return nsNoMoreCallbacks;     // empty message
  unsigned len = msg->getLength();
  unsigned flags = msg->getFlags();
  if ( !(flags & MSG_VOICE_FLAG) ||
       len < sizeof(VoNMagicPacket) )
    return nsNoMoreCallbacks;               // bad message format

  VoNMagicPacket *vonHdr = (VoNMagicPacket*)msg->getData();
  VoNChannelId channel = vonHdr->channel;
  VoNClient *client = (VoNClient*)data;
  Assert( client );

  switch ( vonHdr->magic )
  {
    case MAGIC_VON_CTRL:                    // voice-channel control message
    {
#if _ENABLE_REPORT
      client->OnCtrlReceived(len+MSG_HEADER_LEN);
#endif
      VoNControlPacket *vonCtrl = (VoNControlPacket*)vonHdr;
      if ( len < VON_CONTROL_OFFSET + vonCtrl->size )
        return nsNoMoreCallbacks;           // message is too short

#if defined NET_LOG_VOICE || NET_LOG_VOICE_P2P>0
      NetLog("vonClientReceive(ctrl): ch=%d, command=%d",(int)channel,(int)vonCtrl->command);
#endif
      switch ( vonCtrl->command )
      {
        case VON_CMD_REMOVE:                // remove the replayer
          client->removeChannel(channel);
          break;
        case VON_CMD_CREATE:
          // create new replayer?
#if _ENABLE_VON
          if (!client->RetranslateOnly()) client->createChannel(channel,vonCtrl);
#endif
          break;
        case VON_CMD_PEER_CLIQUES:
          client->rememberCliques(msg);
          break;
        case VON_CMD_ENCAPSULATE_VOICE:
          struct sockaddr_in distantAddr;
          msg->getDistant(distantAddr);

#ifdef DEBUG_VON_RETRANSLATE
          LogF("VoNClientReceive ENCAPSULATE_VOICE");
#endif

          client->PeerToPeerReceive(vonCtrl->data, vonCtrl->size, &distantAddr,false);
          break;
        case VON_CMD_RESET_CONNECTION:
          {
            PeerToPeerEdge *edge = reinterpret_cast<PeerToPeerEdge *>(vonCtrl->data);
            client->enterTransmitTargets();
            TransmitTargetExt *tgt = client->FindTransmitTarget(edge->dpnidTo);
            if (tgt) 
            {
              tgt->peerToPeer = false;
              client->SetTargetsDirty();
            }
            client->leaveTransmitTargets();
          }
          break;
        default:
          LogF("Old voice control message %d",vonCtrl->command);
          break;
      }
      break;
    }

    default:
      return nsNoMoreCallbacks;             // unknown MAGIC
  }

  return nsNoMoreCallbacks;
}

#if _GAMES_FOR_WINDOWS
VoNClient::VoNClient(VoNSystem *von, IXAudio2 *xaudio2, bool isBotClient, int localPlayer, const AutoArray<int> &localTalkerIndices)
#else
VoNClient::VoNClient ( VoNSystem *von, bool isBotClient )
#endif
{
#ifdef NET_LOG_VOICE
  NetLog("VoNClient::VoNClient");
#endif
  Assert( von );
  m_von = von;
  outChannel = EXT_BOT_CLIENT;              // default value for bot-client
  m_allTargetsPeerToPeerChanged = false;
#if _ENABLE_REPORT
  ResetStats();
#endif
  m_voiceTransmitionEnabled=false;
  m_voiceToggleOn=false;
  _callback = NULL;
  m_chatChannel = 0;
  _botClient = isBotClient;

#if _GAMES_FOR_WINDOWS
  _XHVEngine = NULL;
  _XHVWorkerThread = NULL;
  _localPlayer = localPlayer;
  _localTalkerIndices = localTalkerIndices;

  HRESULT result;

  // Set up parameters for the voice chat engine
  XHV_PROCESSING_MODE localMode = XHV_VOICECHAT_MODE;
  XHV_PROCESSING_MODE remoteMode = XHV_VOICECHAT_MODE;
  XHV_INIT_PARAMS params;
  memset(&params, 0, sizeof(params));
  params.dwMaxRemoteTalkers = XHV_MAX_REMOTE_TALKERS;
  params.dwMaxLocalTalkers = XUSER_MAX_COUNT;
  params.localTalkerEnabledModes = &localMode;
  params.dwNumLocalTalkerEnabledModes = 1;
  params.remoteTalkerEnabledModes = &remoteMode;
  params.dwNumRemoteTalkerEnabledModes = 1;
  params.pXAudio2 = xaudio2;

  // Create the engine
  result = XHV2CreateEngine(&params, &_XHVWorkerThread, &_XHVEngine);
  if (FAILED(result))
  {
# ifdef NET_LOG_VOICE
    NetLog("XHVCreateEngine failed, error 0x%x", result);
# endif
    return;
  }

  for (int i=0; i<_localTalkerIndices.Size(); ++i)
  {
    int userIndex = _localTalkerIndices[i];
    result = _XHVEngine->RegisterLocalTalker(userIndex);
    if (FAILED(result))
    {
# ifdef NET_LOG_VOICE
      NetLog("RegisterLocalTalker failed, error 0x%x", result);
# endif
    }
    else
    {
      result = _XHVEngine->StartLocalProcessingModes(userIndex, &localMode, 1);
      if (FAILED(result))
      {
# ifdef NET_LOG_VOICE
        NetLog("StartLocalProcessingModes failed, error 0x%x", result);
# endif
      }
    }
  }

#endif // _GAMES_FOR_WINDOWS
}

/*!
\patch 5156 Date 5/15/2007 by Bebul
- Fixed: VoN peer to peer connections are kept alive by sending special packets
*/

// static method
void VoNClient::vonKeepAliveProc(void *ctx)
{
  VoNClient *cl = ((VoNClient *)ctx);
  // send stored VoN messages (if any) first
  cl->SendStoredVoice();
  // and process keep alive
  cl->ProcessKeepAlive();
}

void VoNClient::ProcessKeepAlive()
{
#if _GAMES_FOR_WINDOWS
  // TODOXNET: Voice implementation
#else
  UpdateKAStarts();
  //Rest of KeepAlive should not be processed too often (use 20ms delay)
  unsigned64 now = getSystemTime();
  static unsigned64 nextProcessTime = now;
  if (nextProcessTime<=now)
  {
    nextProcessTime = now + 20000; //20ms
    enterTransmitTargets();
    for (int targetIx=0, siz = m_allTargets.Size(); targetIx<siz; targetIx++)
    { // check one TransmitTarget
      // test whether to send new KeepAlive/Probe packet
      // test whether the time to open verify the P2P connection has elapsed
      if ( !m_allTargets[targetIx].keepAlive.TestAlive() && m_allTargets[targetIx].keepAlive.IsCmdResetConnectionPossible() )
      { // connection seems not to be working
        PeerToPeerEdge p2pEdge(m_allTargets[targetIx].dpid, outChannel);
        sendCtrlTo(m_allTargets[targetIx].dpid, VON_CMD_RESET_CONNECTION, &p2pEdge, sizeof(PeerToPeerEdge));
        m_allTargets[targetIx].keepAlive.ForceAlive();
      }
      else if ( m_allTargets[targetIx].keepAlive.Test(now) ) //notice: Test cannot pass for keepAlive which is not active (Start must be called)
      {
#if 1
        // TESTING 
        static bool doNotSendProbes = false;
        static bool doIt = false;
        if (doIt)
        {
          doNotSendProbes = true;
        }
        if (doNotSendProbes) continue;
        //       static int count = 0;
        //       if (count++ > 2) continue; //TESTING ONLY: it will simulate connection broken after 2 probes
#endif
        if (m_allTargets[targetIx].peerToPeer) //send KeepAlive
        { // sent probes no more than every 20 seconds
          // it serves to 1. keep NAT traversed 2. verify the connection is still working
          m_allTargets[targetIx].keepAlive.FinishProbes();
          SendPeerToPeerSpecial(outChannel, OriginKeepAliveValue, m_allTargets[targetIx]);
        }
        else if (m_allTargets[targetIx].keepAlive.NextProbe()) //increase probe number (send at most MaxProbes)
        { // send Probe
          SendPeerToPeerSpecial(outChannel, OriginProbeValue, m_allTargets[targetIx]);
        }
        //reset time
        m_allTargets[targetIx].keepAlive.ResetTime();
      }
    }
    leaveTransmitTargets();
  }
#endif
}

void VoNClient::SendStoredVoice()
{
  EnterVoiceMessageToSend();
  AutoArray<StoredMessages::VoiceMessageToSend,MemAllocSafe> &voiceMessages = _storedMessages.GetVoiceMessagesToStore();
  _storedMessages.SwitchPools(); //other threads could write to the other message array than we are going to process
  // note: _ctrlMessages are not in Pool, as the only place where these could be added is from the following lines (SendPeerToPeerMessage)
  LeaveVoiceMessageToSend();

  // Send stored voice messages and VoN heartbeats 
  // Note: we are safe outside EnterVoiceMessageToSend critical section!
  enterTransmitTargets();

  for (int i=0; i<voiceMessages.Size(); i++)
  {
    const StoredMessages::VoiceMessageToSend &msg = voiceMessages[i];
    if (msg._heartBeat)
    { // HEART BEAT
      int len = msg._size;
      RefD<NetChannel> netCh = getChannel();
      RefD<NetMessage> heart = NetMessagePool::pool()->newMessage(len,netCh);
      if ( heart )
      {
#ifdef VON_CTRL_VIM
        heart->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG|MSG_VIM_FLAG);
#else
        heart->setFlags(MSG_ALL_FLAGS,MSG_VOICE_FLAG);
#endif
        heart->setLength(len);
        void *data = heart->getData();
        memcpy(data, msg._data.Data(), msg._size);
        heart->send(true);
#if _ENABLE_REPORT
        OnCtrlSent(heart->getLength()+MSG_HEADER_LEN);
#endif
      }
    }
    else
    { // ENCODED VOICE MESSAGE
      // set diagnostics: increase number of voice packets sent
      for (int i=0; i<msg._nTargets; i++)
      {
        TransmitTargetExt *tgt = FindTransmitTarget(msg._targets[i].dpid);
        if (tgt) tgt->keepAlive.SetVoicePacketSent();
      }
      SendPeerToPeerMessage(msg._data.Data(),msg._size,msg._encryptedSize, msg._targets,msg._nTargets);
    }
  }
  voiceMessages.Resize(0); //clear

  leaveTransmitTargets();

  EnterVoiceMessageToSend();
  AutoArray<StoredMessages::CtrlMessageToSend,MemAllocSafe> &ctrlMessages = _storedMessages.GetCtrlMessagesToStore();
  // sendCtrl messages (these were added while processing _voiceMessages in the first part of this function)
  for (int i=0; i<ctrlMessages.Size(); i++)
  {
    const StoredMessages::CtrlMessageToSend &msg = ctrlMessages[i];
    { // CTRL message
      // send through VoNClient::sendCtrlTo
      sendCtrlTo(msg._id, msg._command, msg._data.Data(), msg._size, msg._useVim);
    }
  }
  ctrlMessages.Resize(0); //clear
  LeaveVoiceMessageToSend();
}

void VoNClient::StoreVoiceToSend(const void *data, int size, int encryptedSize, const TransmitTarget *targets, int nTargets, bool heartBeat)
{
  EnterVoiceMessageToSend();
  AutoArray<StoredMessages::VoiceMessageToSend,MemAllocSafe> & voiceMessages = _storedMessages.GetVoiceMessagesToStore();
  int ix = voiceMessages.Add();
  StoredMessages::VoiceMessageToSend &msg = voiceMessages.Set(ix);
  msg.Init(data, size, encryptedSize, targets, nTargets, heartBeat);
  LeaveVoiceMessageToSend();
}

void VoNClient::StoredMessages::VoiceMessageToSend::Init(const void *data, int size, int encryptedSize, const TransmitTarget *targets, int nTargets, bool heartBeat)
{
  // Data
  _data.Copy((const char *)data, size);
  _size = size;
  _heartBeat = heartBeat;
  if (!_heartBeat)
  {
    _encryptedSize = encryptedSize;
    // Transmit targets
    memcpy(_targets, targets, nTargets*sizeof(TransmitTarget));
    _nTargets = nTargets;
  }
}

void VoNClient::StoreCtrlToSend(VoNChannelId id, unsigned command, const void *data, int size, bool useVim)
{
  EnterVoiceMessageToSend();
  AutoArray<StoredMessages::CtrlMessageToSend,MemAllocSafe> & ctrlMessages = _storedMessages.GetCtrlMessagesToStore();
  int ix = ctrlMessages.Add();
  StoredMessages::CtrlMessageToSend &msg = ctrlMessages.Set(ix);
  msg.Init(id, command, data, size, useVim);
  LeaveVoiceMessageToSend();
}

void VoNClient::StoredMessages::CtrlMessageToSend::Init(VoNChannelId id, unsigned command, const void *data, int size, bool useVim)
{
  // Data
  _data.Copy((const char *)data, size);
  _size = size;
  _useVim = useVim;
  _id = id;
  _command = command;
}

RString VoNClient::GetTargetDiagnostics(int dpid)
{
  enterTransmitTargets();
  TransmitTargetExt *tgt = FindTransmitTarget(dpid);
  if (!tgt) 
  {
    leaveTransmitTargets();
    return RString("Target not found!");
  }
  // We want to know, whether this target is in the actual TransmitTarget list to send packets
  char activeTargetInfo[32]; 
  *activeTargetInfo = 0; //no info by default
  for (int i=0; i<m_transmitTargets.Size(); i++)
  {
    if (m_transmitTargets[i].dpid==dpid)
    {
      BYTE extInfo = m_transmitTargets[i].extInfo;
      int chan = GetChatChannel();
      int is2D = ((extInfo & TTChan2D) != 0);
      int is3D = ((extInfo & TTChan3D) != 0);
      sprintf(activeTargetInfo, " SAY(%d,%d,%d)", chan, is2D, is3D);
      break;
    }
  }

  int64 now = getSystemTime(); //ms
#pragma warning(push)
#pragma warning(disable: 4244)
  int timeSnd = (now - tgt->keepAlive._lastKeepAliveTime) / 1000000; //sec
  int timeRcv = (now - tgt->keepAlive._lastGotResponseTime) / 1000000; //sec
#pragma warning(pop)
  RString p2pInfo;
  const char *wasP2P = (tgt->peerToPeer ? "P2P, " : "");
  if (!*wasP2P) wasP2P = (tgt->wasPeerToPeer ? "wasP2P, " : "FAIL, ");
  p2pInfo = p2pInfo + wasP2P;

  RString ipaddr = Format("%d.%d.%d.%d:%d",
    (unsigned)IP4(tgt->addr),(unsigned)IP3(tgt->addr),(unsigned)IP2(tgt->addr),(unsigned)IP1(tgt->addr),
    ntohs(tgt->addr.sin_port)
    );

  RefD<VoNReplayer> repl = getReplayer(dpid);
  bool is2D=false, is3D=false;
  int chChan = -5; //-1 is reserved for CCNone (diagnose "impossible" !repl)
  if (repl) 
  {
    chChan = repl->GetChatChannel();
    is2D =  repl->GetPlay2D();
    is3D =  repl->GetPlay3D();
  }
  RString replInfo = Format(" Repl(%d,%d,%d)", chChan, is2D, is3D);
  RString output = Format("%s%d KA, snd: %d sec, rcv: %d sec, (%s) snd=%d rcv=%d(%d)%s%s", 
                         cc_cast(p2pInfo), tgt->keepAlive.GetP2PRecvCount(), timeSnd, timeRcv, cc_cast(ipaddr),
                         tgt->keepAlive._voicePacketsSndCount,
                         tgt->keepAlive._voicePacketsCount, tgt->keepAlive._badVoicePacketsCount,
                         cc_cast(replInfo),
                         activeTargetInfo);
  leaveTransmitTargets();
  return output;
}

void VoNClient::GetVoiceCliquesDiagnostics(QOStrStream &out, Array<ConvPair> &convTable)
{
#ifndef DISABLE_VON_TOPOLOGY_DIAG
  //store P2P cliques
  out << "Cliques:\n";
  for (int i=0; i<m_cliques.Size(); i++)
  {
    out << "  ";
    for (int j=0, siz=m_cliques[i].Size(); j<siz; j++)
    {
      out << cc_cast(DebugVoNBank::Dpid2Cdid(m_cliques[i][j], convTable));
      if (j<siz-1) out << ",";
    }
    out << "\n";
  }
#endif
}

void VoNClient::StartKeepAlive(bool start, int dpid)
{
  enterTransmitTargets();
  TransmitTargetExt *tgt = FindTransmitTarget(dpid);
  if (!tgt) 
  { // we must be careful, the transmit target was probably not created yet (message from server was delayed)
    _waitingKAStarts.Add(KAStartItem(dpid, start));
  }
  else tgt->keepAlive.Start(start);
  leaveTransmitTargets();
}

void VoNClient::UpdateKAStarts()
{
  enterTransmitTargets();
  int now = GetTickCount();
  for (int i=0; i<_waitingKAStarts.Size(); i++)
  {
    TransmitTargetExt *tgt = FindTransmitTarget(_waitingKAStarts[i]._dpid);
    if (tgt)
    { //found, we should start the KeepAlive for it and remove it from the waiting array
      tgt->keepAlive.Start(_waitingKAStarts[i]._start);
      _waitingKAStarts.Delete(i); //TODO: use TListBidir instead of AutoArray
      break; //possible other waitingKAStarts will be server next time
    }
    const int KAStartTimeout = 30000; //30sec
    if (now - _waitingKAStarts[i]._time > KAStartTimeout)
    { //timeout
      _waitingKAStarts.Delete(i); //TODO: use TListBidir instead of AutoArray
      break; //possible other waitingKAStarts will be server next time
    }
  }
  leaveTransmitTargets();
}

TransmitTargetExt *VoNClient::FindTransmitTarget(int dpid)
{
  for (int t=0; t<m_allTargets.Size(); t++)
  {
    // get IA
    if (m_allTargets[t].dpid==dpid) return &m_allTargets[t];
  }
  return NULL;
}

/*!
\patch 5178 Date 11/5/2007 by Bebul
- New: Improved Voice over Net packet retranslation (voice is retranslated when no direct Peer to Peer connection is available or to spare some network bandwidth).
\patch 5218 Date 1/31/2008 by Bebul
- Fixed: VoN - mouth is moving but no voice is heard.
\patch 5218 Date 1/31/2008 by Bebul
- Fixed: VoN - voice crackling reduced.
\patch 5218 Date 1/31/2008 by Bebul
- Fixed: VoN - VOID channel indication.
*/
bool VoNClient::PeerToPeerReceive (const void *data, int size, sockaddr_in *from, bool detectVoicePackets)
{
#if _ENABLE_REPORT
  OnDataReceived(size);
#endif
  if (_callback) 
  {
    if ((*_callback)((char *)data, size, from)) // offer to callback (test to NAT negotiation)
      return true;
  }

  // when detection needed, but there is no prefix, skip the detection
  if (detectVoicePackets && sizeof(VON_PREFIX)-1==0) return false;

  // remove leading prefix - even if we do not require it
  if (size<sizeof(VON_PREFIX)-1) return false;
  const char *dataC = reinterpret_cast<const char *>(data);
  if (strncmp(dataC,VON_PREFIX,sizeof(VON_PREFIX)-1))
  {
    if (detectVoicePackets)
    {
      return false;
    }
  }
  else
  {
    data = dataC+sizeof(VON_PREFIX)-1;
    size -= sizeof(VON_PREFIX)-1;
  }

  if (size<sizeof(VoNDataPacket))
  {
    Fail("Too short peer to peer packet");
    return false;
  }
  VoNDataPacket *vonHdr = (VoNDataPacket*)data;
  VoNChannelId channel = vonHdr->channel;

  // check if it is a real message
  if (vonHdr->samples==0)
  {
    // not a real message - check special value
    int special = (int)vonHdr->origin;
    VoNChannelId channel = vonHdr->channel;
#ifdef DEBUG_VON_KEEP_ALIVE
    const char *specId = "???";
    switch (special)
    {
      // Keep Alive System - connection verification
      case OriginProbeValue: specId = "probe"; break;
      case OriginResponseValue: specId = "response"; break;
      case OriginKeepAliveValue: specId = "keep alive"; break;
    }
    LogF(
      "VoNLog, peerToPeerReceive: got %s from %d (%d.%d.%d.%d:%d)", specId, channel,
      from->sin_addr.S_un.S_un_b.s_b1, from->sin_addr.S_un.S_un_b.s_b2, from->sin_addr.S_un.S_un_b.s_b3, from->sin_addr.S_un.S_un_b.s_b4, ntohs(from->sin_port)
    );
    #endif
    // Keep Alive System - connection verification
    switch (special)
    {
      case OriginProbeValue:
        {
          // we received a probe - always respond
          enterTransmitTargets();
          TransmitTargetExt *tgt = FindTransmitTarget(channel);
          if (tgt)
          { // packet from other side has come, so mark the connection being fresh
            tgt->keepAlive.SetProbeResponseReceived();
            #if _ENABLE_REPORT
            static bool respond = true;
            if (respond)
            #endif
            {
              #if NET_LOG_VOICE_P2P>10
                NetLog("Voice responding to %d",channel);
              #endif
              tgt->keepAlive.SetCmdResetConnectionPossible();
              SendPeerToPeerSpecial(outChannel,OriginResponseValue,*tgt); // send response
            }
          }
          leaveTransmitTargets();
        }
        break;
      case OriginResponseValue:
        {
          // we received a response - mark success
          enterTransmitTargets();
          // probes are used to verify the connection is working
          TransmitTargetExt *tgt = FindTransmitTarget(channel);
          if (tgt)
          {
            tgt->keepAlive.FinishProbes(); //do not send any other probes (only keepAlive)
            tgt->keepAlive.SetProbeResponseReceived(); //mark connection as fresh
            tgt->peerToPeer = tgt->wasPeerToPeer = true; //the only place this can be set (connection verified)
            m_allTargetsPeerToPeerChanged = true;  //recalculate cliques etc.
            #if NET_LOG_VOICE_P2P>10
              NetLog("Voice confirmed from %d",channel);
            #endif
          }
          leaveTransmitTargets();
        }
        break;
      case OriginKeepAliveValue:
        { // we received a keep alive - mark connection being fresh
          enterTransmitTargets();
          // keep alive are used only to Keep NAT traversed and to verify the connection is still working
          TransmitTargetExt *tgt = FindTransmitTarget(channel);
          if (tgt)
          {
            tgt->keepAlive.SetProbeResponseReceived(); //mark connection as fresh
          }
          leaveTransmitTargets();
        }
        break;
      default:
        Fail("Bad special value");
        break;
    }
    // test whether IP:PORT of received packet differs from that set in transmit targets
    enterTransmitTargets();
    TransmitTargetExt *tgt = FindTransmitTarget(channel);
    if (tgt && tgt->peerToPeer)
    {
      if (
        from->sin_addr.s_addr != tgt->addr.sin_addr.s_addr || 
        from->sin_port != tgt->addr.sin_port
        )
      {
        // DebugVoNBank diagnostics cannot be used as usual for MT Safety broken: (Format for RString uses new)
        GDebugVoNBank.SetSpecialIPMess(channel, from->sin_addr.s_addr, ntohs(from->sin_port), tgt->addr.sin_addr.s_addr, ntohs(tgt->addr.sin_port));
        // When packets are coming from different IP:PORT, it seems there was something queer during NAT traversal
        // When only Ports differ, we can assume the one we can receive packets from is NAT traversed and use it.
        if (from->sin_addr.s_addr == tgt->addr.sin_addr.s_addr)
        {
          // Not sufficient. PlayerIdentity::_voiceAddress must be changed too
          tgt->addr.sin_port = from->sin_port;
          m_peerChanged.Add(PeerChanged(tgt->dpid, from->sin_port));
        }
      }
    }
    leaveTransmitTargets();
    // not a real message - check special value
    // no retranslation for probes
    return true;
  }

  // We cannot "Reset time to test whether P2P connection is alive"
  // as packet could be retranslated through another peer, no call to SetProbeResponseReceived here!

  // we may need to retransmit the data to other recipients
  unsigned8 retranslate = ((unsigned8 *)data)[size-1];
  // remove the re-translation count byte
  --size;
  // should we play this message?
  // if there is no re-translation, we should
  // if there is some, then we should play only if we are in the recipients list
  bool play = true;
  BYTE playExtInfo = vonHdr->chatChannel; //it is final for no retranslation
  if (retranslate>0)
  {
    const int MaxVoicePlayers = 256;
    if (retranslate<MaxVoicePlayers)
    {
      IndirectInfo indirects[MaxVoicePlayers];
      size -= retranslate*sizeof(IndirectInfo);
      // avoid crashed possibly caused by modified packages (changes of retranslate count  could cause crash) 
      // note: max retranslate is 255, so test whether size is positive but still too large is not necessary
      if (size<0) return true; //corrupted packet
      unsigned8 *retransInfo = ((unsigned8 *)data)+size;
      memcpy(indirects,retransInfo,sizeof(IndirectInfo)*retranslate);

      #if NET_LOG_VOICE_P2P>10
        BString<256> retrans;
        for (int p=0; p<retranslate; p++)
        {
          BString<80> idString;
          sprintf(idString," %d",indirects[p].dpid);
          strcat(retrans,idString);
        }
        NetLog("Voice from %d -> %d:%s",channel,retranslate,cc_cast(retrans));
      #endif
      
      enterTransmitTargets();
      
      play = false;
      for (int i=0; i<retranslate; i++)
      {
        if ((indirects[i].extInfo & TTChannelMask)==0)
        {
          GDebugVoNBank.SetSpecialEmptyVoice(channel, indirects[i].dpid);
        }
        int dpid = indirects[i].dpid;
        if (dpid==outChannel)
        {
          // message is to us - do not retranslate;
          play = true;
          playExtInfo = indirects[i].extInfo;
          continue;
        }
        // retranslate the message
        // convert dpid to ia
        TransmitTargetExt *tgt = FindTransmitTarget(dpid);
        if (tgt)
        {
          // we know all targets are in the same clique and retranslation should be easy
          // but diagnostic showed there were requests out of the mediator peerToPeer set
          // if this happen, mediator uses retranslation through the server
          #if NET_LOG_VOICE_P2P>20 || defined DEBUG_VON_RETRANSLATE
            LogF("  Voice to %d",dpid);
          #endif
          // diagnose of retranslation to peer which is not P2P connected
          bool clientServerEdge = (dpid==EXT_BOT_CLIENT || _botClient);
          if (!tgt->peerToPeer && !clientServerEdge)
          {
            // diagnose retranslation through bad mediator
            GDebugVoNBank.SetSpecialRetranslate(channel, tgt->dpid);
            // retranslate through the server
            IndirectInfo info;
            info.dpid = indirects[i].dpid;
            info.extInfo = indirects[i].extInfo;
            TransmitTargetExt *host = FindTransmitTarget(EXT_BOT_CLIENT);
            if (host)
            { // server should be always in the list
              VoNDataPacket *vonPacket = (VoNDataPacket*)data;
              vonPacket; //so that compiler does not show warning
              // channel inside vonPacket is always correct. 
              // moreover extInfo contains 2D3D flags along with the VoNChatChannel already
              SendPeerToPeerMessage(EXT_BOT_CLIENT,host->addr,unconst_cast(data),size,sizeof(VoNDataPacket),&info,1);
            }
          }
          else
          {
            VoNDataPacket *vonHeader = static_cast<VoNDataPacket*>(unconst_cast(data));
            // extInfo contains 2D3D flags along with the VoNChatChannel already
            vonHeader->chatChannel = indirects[i].extInfo;
            SendPeerToPeerMessage(dpid,tgt->addr,unconst_cast(data),size,sizeof(VoNDataPacket),NULL,0);
          }
        }
      }
      leaveTransmitTargets();
    }
    else GDebugVoNBank.SetFeature(DebugVoNBank::VoNFeature_MaxRetranslateTargetsOverflow);
  }
  else
  {
    #if NET_LOG_VOICE_P2P
      NetLog("Received voice from %d",channel);
    #endif
  }
  
  if (play)
  {
    // if the message is for us, create a message out of data
    RefD<NetMessage> msg = NetMessagePool::pool()->newMessage(size,NULL);
    vonHdr->chatChannel = playExtInfo;
    
    msg->setData((unsigned8 *)data,size);
    
#ifdef DEBUG_VON_RETRANSLATE
    LogF(" play voice");
#endif
    VoNDataPacket *vonData = (VoNDataPacket*)vonHdr;
    if ( (vonData->chatChannel & TTChannelMask)==0 )
    {
      GDebugVoNBank.SetFeature(DebugVoNBank::VoNFeature_RecvEmptyVoice);
    }

    enterTransmitTargets();
    TransmitTargetExt *tgt = FindTransmitTarget(channel);
    if (tgt)
    { // voice packet from other side has come - update diagnostics)
      tgt->keepAlive.SetVoicePacketReceived();
    }

    if ( (unsigned)size < VON_DATA_OFFSET + vonData->size )
    { // message is too short - skip the packet
      // diagnose number of such bad voice packets
      if (tgt) tgt->keepAlive.SetBadVoicePacketReceived();
      leaveTransmitTargets();
      return true;
    }

    leaveTransmitTargets();

  #if defined NET_LOG_VOICE_VERBOSE || defined DEBUG_VON_RETRANSLATE
    LogF("   vonClientReceive(data): ch=%d, origin=%d, samples=%d",(int)channel,(int)vonData->origin,(int)vonData->samples);
  #endif

#if _GAMES_FOR_WINDOWS
    ReplayData(channel, vonData);
#else
    RefD<VoNReplayer> repl;
    if ( m_repl.get(channel,repl) ) // I know this channel
    {
      repl->newDataMessage(msg);
    }
    else
      return true;
#endif
  }
  return true;
}

void VoNClient::connectPeerToPeer ( RefD<NetPeerToPeerChannel> ch, RawMessageCallback callback )
{
  m_peerToPeer = ch;
  _callback = callback;
  if (m_peerToPeer)
  {
    m_peerToPeer->RegisterKeepAliveCallback(vonKeepAliveProc,this);
  }
}

/**
Calls enterTransmitTargets and leaveTransmitTargets.
Can be called only for main thread - uses memory allocation.
*/
void VoNClient::setTransmitTargets(const TransmitTarget *addr, int nAddr)
{
  enterTransmitTargets();
  if (IsTransmitionEnabled()) 
    m_transmitTargets.Copy(addr,nAddr);
  else 
    m_transmitTargets.Clear();
  leaveTransmitTargets();
}

void VoNClient::setChatChannel(VoNChatChannel chatChan)
{
  enterChatChannel();
  m_chatChannel = chatChan;
  leaveChatChannel();
}

int VoNClient::CodecQuality()
{ 
  return m_von->CodecQuality(); 
}

const VoNChatChannel VoNClient::GetChatChannel()
{
  return m_chatChannel;
}

void VoNReplayer::CheckChatChannel(VoNDataPacket *packet)
{
  m_playerInfo.m_chatChannel = packet->chatChannel & ~TTChannelMask;
  //we need not determine, whether this packet should be replayed 3D, 2D or both,
  //as this work was done by sender and sent in packet (upper two bits of m_chatChannel)
  m_playerInfo.m_play2D = (packet->chatChannel & TTChan2D)!=0;
  m_playerInfo.m_play3D = (packet->chatChannel & TTChan3D)!=0;
}

/*!
\patch 5179 Date 11/7/2007 by Bebul
- Fixed: VoN loudness is sometimes not matching selected voice channel (too quiet on Global channel or too loud on Direct channel).
*/
void VoNClient::setAllTargets(const TransmitTarget *addr, int nAddr)
{
  enterTransmitTargets();
  // for each target find a corresponding target based on dpid
  // source and target arrays are sorted by dpid
  // the former targets should be kept, as TransmitTargetExt contains KeepAliveInfo member
  int t = 0;
  for (int s=0; s<nAddr; s++)
  { //add/update each addr[s] target
    int dpid = addr[s].dpid;
    // check if there exists corresponding entry in target array
    if (t<m_allTargets.Size() && m_allTargets[t].dpid==dpid)
    {
      // entry found
      TransmitTargetExt &tExt = m_allTargets[t];
      tExt.extInfo = addr[s].extInfo; // assign operator must be used (addr[s].extInfo is definitive)
      if (!tExt.peerToPeer && addr[s].addr.sin_port != 0)
      {
#if _GAMES_FOR_WINDOWS
        tExt.xuid = addr[s].xuid;
#endif
        // set the address
        tExt.addr = addr[s].addr;
        // peerToPeer should be set only after OriginResponseValue receive
#ifdef MIMIC_108
        // but on 1.08 ArmA version it was set always true (mimic it for linux server)
        tExt.peerToPeer = true;
        m_allTargetsPeerToPeerChanged = true;
#endif
      }
      t++;
    }
    else if (t>=m_allTargets.Size() || dpid<m_allTargets[t].dpid)
    {
      // s entry should be inserted into t
      TransmitTargetExt &tExt = m_allTargets.Insert(t);
      tExt.dpid = addr[s].dpid;
      tExt.addr = addr[s].addr;
#if _GAMES_FOR_WINDOWS
      tExt.xuid = addr[s].xuid;
#endif
      tExt.extInfo = addr[s].extInfo;  // assign operator must be used (addr[s].extInfo is definitive)
      // peerToPeer should be set only after OriginResponseValue receive
#ifdef MIMIC_108
      // but on 1.08 ArmA version it was set always true (mimic it for linux server)
      if (addr[s].addr.sin_port != 0)
      {
        tExt.peerToPeer = true;
        m_allTargetsPeerToPeerChanged = true;
      }
#endif
      t++;
    }
    else
    {
      // t entry should be deleted
      m_allTargets.Delete(t);
      m_allTargetsPeerToPeerChanged = true;
      s--;  //because source entry should be still added
    }
  }
  // remove not existing (disconnected) transmit targets
  if (t<m_allTargets.Size())
    m_allTargets.Delete(t,m_allTargets.Size()-t); //delete all remaining targets

  if (m_allTargetsPeerToPeerChanged)
  {
    // we need to send the server information about our peer to peer targets
    AutoArray<int, MemAllocLocal<int, 64> > peerToPeerList;
    
    for (int i=0; i<m_allTargets.Size(); i++)
    {
      const TransmitTargetExt &tExt = m_allTargets[i];
      if (!tExt.peerToPeer) continue;
      peerToPeerList.Add(tExt.dpid);
    }
    
    // we have the target list - notify the server
    
    // build up the message as array of int-s
    AutoArray<int, MemAllocLocal<int, 64> > msg;
    
    msg.Add(peerToPeerList.Size());
    msg.Merge(peerToPeerList);
    
    sendCtrl(VON_CMD_PEER_INFO,msg.Data(),msg.Size()*sizeof(int));

#if _GAMES_FOR_WINDOWS
    // moved inside enterTransmitTargets ... leaveTransmitTargets, need to update only when m_allTargets changed
    // LUKE - registration stuff now moved to Login/Logout messages
    /*
    if (_XHVEngine)
    {
      int newTalkersCount = intMin(m_allTargets.Size(), XHV_MAX_REMOTE_TALKERS);
      DWORD oldTalkersCount = XHV_MAX_REMOTE_TALKERS;
      XUID oldTalkers[XHV_MAX_REMOTE_TALKERS];
      _XHVEngine->GetRemoteTalkers(&oldTalkersCount, oldTalkers);
      int freeSlots = XHV_MAX_REMOTE_TALKERS - oldTalkersCount;
      // unregister old talkers
      for (DWORD i=0; i<oldTalkersCount; i++)
      {
        // check if the talker is still present
        bool found = false;
        for (int j=0; j<newTalkersCount; j++)
        {
          if (XOnlineAreUsersIdentical(m_allTargets[j].xuid, oldTalkers[i]))
          {
            found = true;
            break;
          }
        }
        // unregister talker if not
        if (!found)
        {
          _XHVEngine->UnregisterRemoteTalker(oldTalkers[i]);
          freeSlots++;
        }
      }
      // register new talkers
      if (freeSlots > 0)
      {
        XHV_PROCESSING_MODE mode = XHV_VOICECHAT_MODE;
        for (int i=0; i<newTalkersCount; i++)
        {
          if (m_allTargets[i].xuid == INVALID_XUID)
          {
            // dedicated server is not talker
            if (newTalkersCount < m_allTargets.Size()) newTalkersCount++;
            continue;
          }
          // check if the talker was present before
          bool found = false;
          for (DWORD j=0; j<oldTalkersCount; j++)
          {
            if (XOnlineAreUsersIdentical(m_allTargets[i].xuid, oldTalkers[j]))
            {
              found = true;
              break;
            }
          }
          // register talker if not
          if (!found)
          {
            if (_XHVEngine->RegisterRemoteTalker(m_allTargets[i].xuid, NULL, NULL, NULL) == S_OK)
            {
              _XHVEngine->StartRemoteProcessingModes(m_allTargets[i].xuid, &mode, 1);
              freeSlots--;
              if (freeSlots <= 0) break;
            }
          }
        }
      }
    }

    */
#endif

    m_allTargetsPeerToPeerChanged = false;
  }
  leaveTransmitTargets();


  // called periodically - good place to handle cliques
  // 
  enter();
  
  if (m_cliquesMessages.Size())
  {
    int cliqueNum = 0;
    for (int i=0; i<m_cliquesMessages.Size(); i++)
    { // read cliques and store them for each vertex
      const VoNControlPacket *header = (const VoNControlPacket *)m_cliquesMessages[i]->getData();
      const int *msg = (const int *)header->data;
      
      // decode the message - see VoNServer::updateTransmitTargets for encoding
      if (msg[0]>=0)
      { // no need to reset m_cliques, it will be reallocated, rewritten
        cliqueNum = 0;
        m_cliques.Resize(msg[0]);
      }
      else
      {
        cliqueNum = m_cliques.Size();
        m_cliques.Resize(m_cliques.Size()-msg[0]); //only add new items (note, msg[0] was negative!)
      }
      // read number of cliques
      int n = abs(*msg++);
      for (int j=0; j<n; j++)
      {
        int size = *msg++;
        m_cliques[cliqueNum].Realloc(size),m_cliques[cliqueNum].Resize(size);
        for (int k=0; k<size; k++)
        {
          m_cliques[cliqueNum][k] = *msg++;
        }
        cliqueNum++;
      }
      m_cliquesMessages[i]=NULL; //destroy message
    }
    
    m_cliquesMessages.Resize(0);
  }
  
  leave();

#if _GAMES_FOR_WINDOWS
  // Process the local talker
  if (_XHVEngine && _localPlayer >= 0 && _XHVEngine->IsLocalTalking(_localPlayer))
  {
    OnMicrophoneDataReady(_localPlayer);
  }
#endif
}

#if _GAMES_FOR_WINDOWS
bool VoNClient::RegisterRemoteTalker(XUID xuid)
{
  DoAssert(INVALID_XUID != xuid);
  if (INVALID_XUID == xuid)
    return false;

  enterTransmitTargets();

  if (_XHVEngine)
  {
    // get all registered talkers
    DWORD oldTalkersCount = XHV_MAX_REMOTE_TALKERS;
    XUID oldTalkers[XHV_MAX_REMOTE_TALKERS];
    _XHVEngine->GetRemoteTalkers(&oldTalkersCount, oldTalkers);
    for (DWORD i=0; i<oldTalkersCount; i++)
    {
      // check if the talker is already present
      if (XOnlineAreUsersIdentical(oldTalkers[i], xuid))
      {
        leaveTransmitTargets();
        return true;
      }
    }

    // check free slots
    int freeSlots = XHV_MAX_REMOTE_TALKERS - oldTalkersCount;

    if (freeSlots > 0)
    {
      // register new talker
      HRESULT result = _XHVEngine->RegisterRemoteTalker(xuid, NULL, NULL, NULL);
      if (FAILED(result))
      {
# ifdef NET_LOG_VOICE
        NetLog("RegisterRemoteTalker failed, error 0x%x", result);
# endif
      }
      else
      {
        XHV_PROCESSING_MODE mode = XHV_VOICECHAT_MODE;
        result = _XHVEngine->StartRemoteProcessingModes(xuid, &mode, 1);
        if (FAILED(result))
        {
# ifdef NET_LOG_VOICE
          NetLog("StartRemoteProcessingModes failed, error 0x%x", result);
# endif
        }
        else
        {
          leaveTransmitTargets();
          return true;
        }
      }
    }//if (freeSlots > 0)
  }//if (_XHVEngine)

  leaveTransmitTargets();
  return false;
}

bool VoNClient::UnregisterRemoteTalker(XUID xuid)
{
  DoAssert(INVALID_XUID != xuid);
  if (INVALID_XUID == xuid)
    return false;

  enterTransmitTargets();
  if (_XHVEngine)
  {
    HRESULT result = _XHVEngine->UnregisterRemoteTalker(xuid);
    if (FAILED(result))
    {
# ifdef NET_LOG_VOICE
      NetLog("UnregisterRemoteTalker failed, error 0x%x", result);
# endif
    }
    else
    {
      leaveTransmitTargets();
      return true;
    }
  }
  leaveTransmitTargets();
  return false;
}

bool VoNClient::RegisterLocalTalker(int userIndex)
{
  DoAssert(userIndex >= 0 && userIndex < XUSER_MAX_COUNT);
  if (userIndex < 0 || userIndex > XUSER_MAX_COUNT)
    return false;

  // check if player is already registered
  for (int i=0; i<_localTalkerIndices.Size(); ++i)
  {
    if (_localTalkerIndices[i] == userIndex)
      return true;
  }

  enterTransmitTargets();

  if (_XHVEngine)
  {
    // register new talker
    HRESULT result = _XHVEngine->RegisterLocalTalker(userIndex);
    if (FAILED(result))
    {
# ifdef NET_LOG_VOICE
      NetLog("RegisterLocalTalker failed, error 0x%x", result);
# endif
    }
    else
    {
      XHV_PROCESSING_MODE mode = XHV_VOICECHAT_MODE;
      result = _XHVEngine->StartLocalProcessingModes(userIndex, &mode, 1);
      if (FAILED(result))
      {
# ifdef NET_LOG_VOICE
        NetLog("StartLocalProcessingModes failed, error 0x%x", result);
# endif
      }
      else
      {
        _localTalkerIndices.Add(userIndex);
        leaveTransmitTargets();
        return true;
      }
    }

  }//  if (_XHVEngine)

  leaveTransmitTargets();
  return false;
}

bool VoNClient::UnregisterLocalTalker(int userIndex)
{
  DoAssert(userIndex >= 0 && userIndex < XUSER_MAX_COUNT);
  if (userIndex < 0 || userIndex > XUSER_MAX_COUNT)
    return false;

  // check if player is already registered
  int indexInArray = -1;
  for (int i=0; i<_localTalkerIndices.Size(); ++i)
  {
    if (_localTalkerIndices[i] == userIndex)
    {
      indexInArray = i;
      break;
    }
  }

  DoAssert(-1 != indexInArray);
  if (-1 == indexInArray)
  {
    // user is not registered as local talker
    return false;
  }

  enterTransmitTargets();
  if (_XHVEngine)
  {
    HRESULT result = _XHVEngine->UnregisterLocalTalker(userIndex);
    if (FAILED(result))
    {
# ifdef NET_LOG_VOICE
      NetLog("UnregisterLocalTalker failed, error 0x%x", result);
# endif
    }
    else
    {
      _localTalkerIndices.Delete(indexInArray);
      leaveTransmitTargets();
      return true;
    }
  }
  leaveTransmitTargets();
  return false;
}

void VoNClient::OnMicrophoneDataReady(DWORD userIndex)
{
  // the max. size the message can contain
  DWORD msgSize = m_netCh->maxMessageData();
  // raw data can be stored in the message
  DWORD size = msgSize - VON_DATA_OFFSET;
  // the number of voice packets in the message
  DWORD packets = 0;

  // Create the message
  RefD<NetMessage> msg = (NetMessage *)NetMessagePool::pool()->newMessage(msgSize, m_netCh);
  Assert(msg);
  msg->setFlags(MSG_ALL_FLAGS, MSG_VOICE_FLAG);
  VoNDataPacket *vonData = (VoNDataPacket*)msg->getData();
  vonData->channel = outChannel;
  vonData->chatChannel = m_chatChannel;

  // Store the data to the message
  HRESULT result = _XHVEngine->GetLocalChatData(userIndex, (BYTE *)vonData->data, &size, &packets);
  if (FAILED(result))
  {
    return;
  }
  vonData->size = (unsigned16)(size);
  msg->setLength(size + VON_DATA_OFFSET);
  // TODOXNET: Voice - correct parameters  
  vonData->samples = (unsigned16)packets;
  vonData->origin = 0;

  if (m_peerToPeer)
  {
    // get recipients and send the message
    enterTransmitTargets();
    const AutoArray<TransmitTarget> &targets = getTransmitTargets();
    SendPeerToPeerMessage(msg->getData(), msg->getLength(), sizeof(VoNDataPacket), targets.Data(),targets.Size());
    leaveTransmitTargets();
  }
  else
  {
#if _ENABLE_REPORT
    OnDataSent(msg->getLength() + MSG_HEADER_LEN);
#endif
    // encrypt header
    msg->setEncryptedLength(sizeof(VoNDataPacket));
    msg->send(true);
  }
}

void VoNClient::ReplayData(VoNChannelId channel, VoNDataPacket *vonData)
{
  if (!_XHVEngine) return;

  // find XUID in the list of all targets
  enterTransmitTargets();
  for (int i=0; i<m_allTargets.Size(); i++)
  {
    if (m_allTargets[i].dpid == channel)
    {
      const BYTE *data = (const BYTE *)vonData->data;
      DWORD size = vonData->size;
      _XHVEngine->SubmitIncomingChatData(m_allTargets[i].xuid, data, &size);
      break;
    }
  }
  leaveTransmitTargets();
}

#endif // _GAMES_FOR_WINDOWS

void VoNClient::connect ( RefD<NetChannel> ch )
{
#if defined(NET_LOG_VOICE) || DEBUG_VON
  //NetLog("VoNClient::connect: channel=%u",ch.NotNull()?ch->getChannelId():0);
  LogF("VoNClient::connect: channel=%u",ch.NotNull()?ch->getChannelId():0);
#endif
    // network routing-table item:
  RoutingItem ri;
  ri.andFlag        = MSG_VOICE_FLAG;
  ri.eqFlag         = MSG_VOICE_FLAG;
  ri.processRoutine = vonClientReceive;
  ri.data           = this;

  if ( m_netCh )
    m_netCh->removeSubsetRoutine(ri);
  m_netCh = ch;
  if ( m_netCh )
    m_netCh->setSubsetRoutine(ri);
}

#if _ENABLE_VON

void VoNClient::createChannel ( VoNChannelId channel, VoNControlPacket *msg )
{
#if defined(NET_LOG_VOICE) || DEBUG_VON
  //NetLog("VoNClient::createChannel: chid=%d, codecInfo=%u",(int)channel,(unsigned)msg->size);
  LogF("VoNClient::createChannel: chid=%d, codecInfo=%u",(int)channel,(unsigned)msg->size);
#endif

#if _GAMES_FOR_WINDOWS
  // TODOXNET: Voice implementation
#else
  enter();
  RefD<VoNReplayer> replayer;
  if ( m_repl.get(channel,replayer) )       // channel already exists => ignore the message
  {
    replayer->suspend(false);
    m_von->setSuspend(channel,false);       // code identical to "resumeChannel()"
    leave();
    return;
  }

      // try to restore codec from transmitted binary info:
  Assert( m_von );
  VoNCodec *codec = m_von->restoreCodec(msg->size,msg->data);
  if ( !codec )                             // codec restore has failed!
  {
    leave();
    return;
  }
  RefD<VoNReplayer> newRepl = new VoNReplayer(this,channel,codec);
  
  m_repl.put(channel,newRepl);
  m_von->setReplay(channel,true);           // application-level response
  leave();
#endif
}

#endif

void VoNClient::rememberCliques(RefD<NetMessage> msg)
{
  enter();
  // cliques message can be separated into more packets, so we need to remember all messages
  const VoNControlPacket *header = (const VoNControlPacket *)msg->getData();
  const int *msgdata = (const int *)header->data;
  if (msgdata[0]>=0)
  { // reset cliques messages
    for (int i=0; i<m_cliquesMessages.Size(); i++) m_cliquesMessages[i]=NULL;
    m_cliquesMessages.Resize(0);
  }
  m_cliquesMessages.Add(msg);
  leave();
}

void VoNClient::suspendChannel ( VoNChannelId channel )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNClient::suspendChannel: chid=%d",(int)channel);
#endif

#if _GAMES_FOR_WINDOWS
  // TODOXNET: Voice implementation
#else
  enter();
  RefD<VoNReplayer> replayer;
  if ( !m_repl.get(channel,replayer) )      // unknown channel => ignore the command
  {
    leave();
    return;
  }
  Assert( m_von );
  m_von->setSuspend(channel,true);          // application-level response
  replayer->suspend(true);
  leave();
#endif
}

void VoNClient::resumeChannel ( VoNChannelId channel )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNClient::resumeChannel: chid=%d",(int)channel);
#endif

#if _GAMES_FOR_WINDOWS
  // TODOXNET: Voice implementation - let the client know the data was received
#else
  enter();
  RefD<VoNReplayer> replayer;
  if ( !m_repl.get(channel,replayer) )      // unknown channel => ignore the command
  {
    leave();
    return;
  }
  replayer->suspend(false);
  Assert( m_von );
  m_von->setSuspend(channel,false);         // application-level response
  leave();
#endif
}

void VoNClient::removeChannel ( VoNChannelId channel )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNClient::removeChannel: chid=%d",(int)channel);
#endif

#if _GAMES_FOR_WINDOWS
  // TODOXNET: Voice implementation - let the client know the data was received
#else
  enter();
  if ( !m_repl.presentKey(channel) )        // unknown channel => ignore the command
  {
    leave();
    return;
  }
  Assert( m_von );
  m_von->setReplay(channel,false);          // application-level response
  m_repl.removeKey(channel);
  leave();
#endif
}

RefD<VoNServer> VoNClient::getServer()
{
  return m_von->getServer();
}

#if _GAMES_FOR_WINDOWS

#else

RefD<VoNReplayer> VoNClient::getReplayer ( VoNChannelId channel )
{
  RefD<VoNReplayer> replayer;
  m_repl.get(channel,replayer);
  return replayer;
}

void VoNClient::setRecorder ( RefD<VoNRecorder> recorder )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNClient::setRecorder");
#endif
  enter();
  m_rec = recorder;
  leave();
}

#endif

bool VoNClient::getRecording ()
{
#if _GAMES_FOR_WINDOWS
  return _XHVEngine->IsLocalTalking(_localPlayer) ? true : false;
#else
  bool result = false;
  enter();
  if ( m_rec )
    result = m_rec->getRecording();
  leave();
  return result;
#endif
}

float VoNClient::getPlaying ( VoNChannelId channel )
{
#if _GAMES_FOR_WINDOWS
  enterTransmitTargets();
  float result = 0.0f;
  for (int i=0; i<m_allTargets.Size(); i++)
  {
    const TransmitTargetExt &target = m_allTargets[i];
    if (target.dpid == channel)
    {
      if (_XHVEngine->IsRemoteTalking(target.xuid)) result = 1.0f;
      break;
    }
  }
  leaveTransmitTargets();
  return result;
#else
  enter();
  RefD<VoNReplayer> replayer;
  if ( !m_repl.get(channel,replayer) )      // unknown channel
  {
    leave();
    return 0.0f;
  }
  float result = replayer->getPlaying();
  leave();
  return result;
#endif
}

void VoNClient::setCapture ( bool on, int quality )
{
#ifdef NET_LOG_VOICE
  NetLog("VoNClient::setCapture(%d)",(int)on);
#endif

#if _GAMES_FOR_WINDOWS
  // TODOXNET: Voice implementation
#else
  enter();
  Assert( m_von );
  m_von->setCapture(on,quality);                    // application-level response
  if ( m_rec )
    m_rec->setCapture(on);
  leave();
#endif
}

VoNClient::~VoNClient ()
{
#if _GAMES_FOR_WINDOWS
  if (_XHVEngine)
  {
    // Unregister remote players
    DWORD oldTalkersCount = XHV_MAX_REMOTE_TALKERS;
    XUID oldTalkers[XHV_MAX_REMOTE_TALKERS];
    HRESULT result = _XHVEngine->GetRemoteTalkers(&oldTalkersCount, oldTalkers);
    if (SUCCEEDED(result))
    {
      for (DWORD i=0; i<oldTalkersCount; i++)
      {
        _XHVEngine->UnregisterRemoteTalker(oldTalkers[i]);
      }
    }

    /// unregister all local talkers
    for (int i=0; i<_localTalkerIndices.Size(); ++i)
    {
      int userIndex = _localTalkerIndices[i];
      result = _XHVEngine->UnregisterLocalTalker(userIndex);
      if (FAILED(result))
      {
# ifdef NET_LOG_VOICE
        NetLog("UnregisterLocalTalker failed, error 0x%x", result);
# endif
      }
    }

    // Destroy the engine
    _XHVEngine->Release();
    _XHVEngine = NULL;
  }
#endif

#ifdef NET_LOG_VOICE
  NetLog("VoNClient::~VoNClient");
#endif
  if (m_peerToPeer->RefCounter()>1) 
  {
    RString msg = Format("NetPeerToPeerChannel referenced %d times", m_peerToPeer->RefCounter());
    Fail(cc_cast(msg));
  }
  //we need to stop the listener thread, as it uses VoNClient extensively in _keepAliveProc
  m_peerToPeer->StopListener(); // cannot set only m_peerToPeer=NULL; as voiceListenSend can use m_peerToPeer when sending StoredVoice messages
  m_peerToPeer = NULL; 
  connect(NULL);
}

bool VoNClient::RetranslateOnly()
{
  return m_von->RetranslateOnly();
}

#include "Es/Memory/normalNew.hpp"

void* VoNClient::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNClient::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNClient::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  VoN factory (VoNSystem class):

VoNSystem::VoNSystem ()
{
  inBps  = 16;
  outBps = 16;
  defaultVoiceMask.init();
  m_codecQuality = 3; // default value
  m_maxTransmitterRange2 = Square(1000.f);
}

VoNSystem::~VoNSystem ()
{
#ifdef NET_LOG_VOICE
  NetLog("VoNSystem::~VoNSystem");
#endif
  // we need to close channel, otherwise m_cLocal/m_sLocal contain cyclical Ref
  if ( m_cLocal )
    m_cLocal->close();                      // closes both peers
  m_server.Free();
  m_client.Free();
  m_sLocal.Free();
  m_cLocal.Free();
}

/// Sets the actual server (can be NULL).
void VoNSystem::setServer ( VoNServer *server )
{
  Assert( !m_server || !server );
  if ( m_server )
  {
    if ( m_sLocal )
    {
      m_sLocal->close();
      m_sLocal = NULL;
    }
  }
  m_server = server;
}

/// Sets the actual client (can be NULL).
void VoNSystem::setClient ( VoNClient *client )
{
  Assert( !m_client || !client );
  if ( m_client )
  {
    stopClient();
    m_client->connect(NULL);
    
    if ( m_cLocal )
    {
      // we need to close m_sLocal, because it contains reference to m_cLocal
      if (m_sLocal)
      {
        m_sLocal->close();
      }
      
      m_cLocal->close();
      m_cLocal = NULL;
    }
  }
  m_client = client;
  if ( (bool)m_client && (bool)m_cLocal )   // re-connect the new client to existing local channel
    m_client->connect(m_cLocal.GetRef());
}

void VoNSystem::stopClient ()
{
}

void VoNSystem::localConnect ()
{
  if ( !m_server || !m_client ) return;     // nothing to do
  if ( (bool)m_sLocal && (bool)m_cLocal ) return; // both local channels exist
    // create local channels:
  if ( !m_sLocal )
    m_sLocal = new DirectChannel;
  if ( !m_cLocal )
    m_cLocal = new DirectChannel;
    // connect them together:
  Assert( m_cLocal );
  Assert( m_sLocal );
  m_cLocal->connect(m_sLocal.GetRef());
  m_sLocal->connect(m_cLocal.GetRef());
    // I'm able to connect the client's side:
  m_client->connect(m_cLocal.GetRef());
    // .. and server's side (for global-channel)
  m_server->connect(EXT_BOT_CLIENT);
}

VoNResult VoNSystem::setReplay ( VoNChannelId chId, bool create )
  // should be overriden in the application!
{
  return VonError;
}

VoNResult VoNSystem::setSuspend ( VoNChannelId chId, bool suspend )
  // should be overriden in the application!
{
  return VonError;
}

VoNResult VoNSystem::setCapture ( bool on, int quality )
  // should be overriden in the application!
{
  return VonError;
}

VoNResult VoNSystem::startRecorder ()
{
  return VonError;
}

int VoNSystem::stopRecorder ( AutoArray<char,MemAllocSafe> &buf )
{
  return 0;
}

VoNResult VoNSystem::startReplay ( AutoArray<char,MemAllocSafe> &buf, int duration =0 )
{
  return VonError;
}

VoNResult VoNSystem::getReplayStatus ()
{
  return VonError;
}

VoNResult VoNSystem::stopReplay ()
{
  return VonError;
}

void VoNSystem::changeVoiceMask ( VoiceMask &mask )
{
  defaultVoiceMask = mask;
}
