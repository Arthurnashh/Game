#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   vonApp.hpp
  @brief  VoN application interface.

  Communication between application and VoN system.
  Application is responsible for sound recording/playback,
    VoN system does everything else..
  <p>Copyright &copy; 2002-2004 by BIStudio (www.bistudio.com)
  @author PE
  @since  24.9.2002
  @date   8.4.2004
*/

#ifndef _VONAPP_H
#define _VONAPP_H

#include "El/Math/math3d.hpp"
#if _GAMES_FOR_WINDOWS
  #define XAUDIO2_HELPER_FUNCTIONS
  #include <xaudio2.h>
#endif
#include "El/QStream/qStream.hpp"
#include "El/Graph/clique.hpp"
#include "El/Network/netchannel.hpp"
#include "El/Common/randomJames.h"

#define _USE_TIMER_QUEUE 0

//----------------------------------------------------------------------
//  Global types:

/// Channel Id (exclusive channel identification, dpid of other side).
typedef int VoNChannelId;
/// Notice VoNChatChannel has CCDirect on zero position (zero value is swapped with ChatChannel CCDirect)
/// this is in order to keep vonApp.hpp without knowledge of ChatChannels which are engine specific
typedef BYTE VoNChatChannel;

/// Void channel-id.
const VoNChannelId VOID_CHANNEL     = -1;

/// Reserved channel-id (for internal purposes).
const VoNChannelId RESERVED_CHANNEL = -2;

/// Channel type (can be used by the application /reproduction mode/distortion/etc.).
typedef int VoNChannelType;

/// Identity: voice source/target (semantics is application-dependent).
typedef int VoNIdentity;

/// Void VoN identity.
const VoNIdentity VOID_IDENTITY     = -1;

/// Reserved VoN identity (for internal purposes).
const VoNIdentity RESERVED_IDENTITY = -2;

/// Externally reserved identities (0 to EXT_RESERVED_IDS-1).
const VoNIdentity EXT_RESERVED_IDS  = 8;

/// Default value for VoNClient::outChannel.
const VoNIdentity EXT_TO_SERVER     = 1;
const VoNIdentity EXT_BOT_CLIENT    = 2;

/// General result code used in VoN system.
enum VoNResult
{
  VonOK,
  VonError,                                 ///< Generic VoN error.
  VonBreak,                                 ///< Sound-channel was interrupted.
  VonDropout,                               ///< Sound is ready but contains only a synthetic noise..
  VonSilence,                               ///< No sound should be replayed (silence-state).
  VonBadData,                               ///< Bad data size/format/alignment/etc. for codecs.
  VonBusy                                   ///< System is busy (the requested mode is already in use..).
};

/// Voice-mask (portable version, equivalent of XVOICE_MASK).
struct VoiceMask
{
  float specEnergyWeight;
  float pitchScale;
  float whisperValue;
  float roboticValue;
  void init ();
};
TypeIsSimple(VoiceMask);

//----------------------------------------------------------------------
//  Codec:

/// Sound-codec type categories:
enum SoundCodecType
{
  ScUndetermined,                           ///< undetermined codec type.
  ScUncompressed,                           ///< trivial (void) codec.
  ScUniversal,                              ///< universal sound codec.
  ScVoice                                   ///< codec optimized for human voice.
};

/// Length of codec-name.
const int MAX_CODEC_NAME = 64;

/// 'doesn't matter' value.
const int CODEC_ANY = 0;

/// Codec has special type of frame (independent of previous ones).
const int CODEC_INDEPENDENT_FRAMES = 0x0001;

/// Codec uses dependent (predicted) frames.
const int CODEC_PREDICTED_FRAMES   = 0x0002;

/// All frames must have the same size (=nominalFrame) after codec initialization.
const int CODEC_STRICT_FRAME_SIZE  = 0x0004;

/// Decoder has 'time-warp' capability.
const int CODEC_TIME_WARP          = 0x0008;

#ifndef __GNUC__
  #pragma pack(push)
  #pragma pack(1)
#endif

/**
    Codec information record.
*/
struct CodecInfo
{

  /// Codec name in readeable format.
  char name[MAX_CODEC_NAME];

  /// Type of the codec.
  SoundCodecType type;

  /// Various codec capabilities (see CODEC_* constants).
  int32 flags;

  /// Nominal (actual) rate in bits-per-second.
  int32 nominalRate;

  /// Minimal rate in bits-per-second.
  int32 minRate;

  /// Maximal rate in bits-per-second.
  int32 maxRate;

  /// Nominal (actual) input depth in bits-per-sample.
  int32 nominalBps;

  /// Minimal input depth in bits-per-sample.
  int32 minBps;

  /// Maximal input depth in bits-per-sample.
  int32 maxBps;

  /// Nominal (actual) frequency in Hz.
  int32 nominalFrequency;

  /// Minimal frequency in Hz.
  int32 minFrequency;

  /// Maximal frequency in Hz.
  int32 maxFrequency;

  /// Nominal (actual) frame size in samples.
  int32 nominalFrame;

  /// Minimal frame size in samples.
  int32 minFrame;

  /// Maximal frame size in samples.
  int32 maxFrame;

} PACKED;

/**
    Portable sound-format structure.
*/
struct VoNSoundFormat
{

  /// number of bits per one sample (8 or 16)
  int32 bitsPerSample;

  /// frequency (in Hz).
  int32 frequency;

  /// data alignment (in bytes).
  int32 alignment;

  /// preferred data granularity (in samples).
  int32 granularity;

} PACKED;

//----------------------------------------------------------------------
//  Sound-channel and identity information records:

/**
    Application dependent channel information. Variable-length record.
*/
struct VoNChannelInfo
{

  /// structure length in bytes.
  unsigned32 structLen;

  /// Channel type (defined by application).
  VoNChannelType type;

} PACKED;

/**
    Application dependent identity information. Variable-length record.
*/
struct VoNIdentityInfo
{

  /// structure length in bytes.
  unsigned32 structLen;

  /// Identity id.
  VoNIdentity id;

} PACKED;

#ifndef __GNUC__
  #pragma pack(pop)
#endif

//----------------------------------------------------------------------
//  Low-level circular sound-buffer:

/**
    Structure holding pointers to circular sound-data buffer.
*/
struct CircularBufferPointers
{

  /// Allow data split (if it is false, len2 will be 0).
  bool split;

  /// 1st data pointer.
  union
  {
    int8  *bps8;
    int16 *bps16;
  } ptr1;

  /// 1st data length (in samples).
  unsigned len1;

  /// 2nd data pointer.
  union
  {
    int8  *bps8;
    int16 *bps16;
  } ptr2;

  /// 2nd data length (in samples).
  unsigned len2;

  /// Writes one sample to "ptr1.bps8" ("ptr2.bps8" if "len1 == 0").
  void write8 ( int8 sample )
  {
    if ( len1 )
    {
      *ptr1.bps8++ = sample;
      len1--;
    }
    else
    {
      *ptr2.bps8++ = sample;
      len2--;
    }
  }

  /// Returns the current 8-bit sample.
  int8 current8 ()
  {
    return ( len1 ? *ptr1.bps8 : *ptr2.bps8 );
  }

  int8 *ptr8 ()
  {
    return ( len1 ? ptr1.bps8 : ptr2.bps8 );
  }

  /// Reads the next 8-bit sample (advances the buffer pointer).
  int8 read8 ()
  {
    if ( len1 )
    {
      len1--;
      return *ptr1.bps8++;
    }
    len2--;
    return *ptr2.bps8++;
  }

  /// Advances (skips) the given number of 8-bit samples.
  void skip8 ( unsigned skip )
  {
    if ( skip >= len1 + len2 )
    {
      len1 = len2 = 0;
      return;
    }
    if ( skip >= len1 )
    {
      skip -= len1;
      len1 = 0;
    }
    if ( !skip ) return;
    if ( len1 )
    {
      len1 -= skip;
      ptr1.bps8 += skip;
      return;
    }
    len2 -= skip;
    ptr2.bps8 += skip;
  }

  /// Writes one sample to "ptr1.bps16" ("ptr2.bps16" if "len1 == 0").
  void write16 ( int16 sample )
  {
    if ( len1 )
    {
      *ptr1.bps16++ = sample;
      len1--;
    }
    else
    {
      *ptr2.bps16++ = sample;
      len2--;
    }
  }

  /// Returns the current 16-bit sample.
  int16 current16 ()
  {
    return ( len1 ? *ptr1.bps16 : *ptr2.bps16 );
  }

  int16 *ptr16 ()
  {
    return ( len1 ? ptr1.bps16 : ptr2.bps16 );
  }

  /// Reads the next 16-bit sample (advances the buffer pointer).
  int16 read16 ()
  {
    if ( len1 )
    {
      len1--;
      return *ptr1.bps16++;
    }
    len2--;
    return *ptr2.bps16++;
  }

  /// Advances (skips) the given number of 16-bit samples.
  void skip16 ( unsigned skip )
  {
    if ( skip >= len1 + len2 )
    {
      len1 = len2 = 0;
      return;
    }
    if ( skip >= len1 )
    {
      skip -= len1;
      len1 = 0;
    }
    if ( !skip ) return;
    if ( len1 )
    {
      len1 -= skip;
      ptr1.bps16 += skip;
      return;
    }
    len2 -= skip;
    ptr2.bps16 += skip;
  }

};

/// Generates silence sound of the given size.
void generateSilence ( CircularBufferPointers &ptr, int bps, unsigned size );

//----------------------------------------------------------------------
//  Sound-buffer:

/**
    Abstract sound buffer. Used for both sound capture & replay.
*/
class VoNSoundBuffer : public RefCountSafe
{

public:

  /// input (capture) flag.
  bool input;

  /// Suspended flag.
  bool suspended;

  /// used sound-format.
  VoNSoundFormat format;

  /// buffer size in samples.
  unsigned size;

  /**
      Creates (allocates) buffer of the required size.
      'format' structure should be filled before!
      @param  len Buffer size in samples.
  */
  virtual void createBuffer ( unsigned len ) =0;

  /// Suspends (temporarily stops) the replaying/capturing.
  virtual void suspendBuffer () =0;

  /// Resumes (restarts) the replaying/capturing.
  virtual void resumeBuffer () =0;

  /**
      Destroys all buffer memory.
  */
  virtual void destroyBuffer () =0;

  /**
    Prepares (encodes/decodes) new bunch of data. Returns number of samples & changes "pos".
    Data are leaved unlocked.
  */
  virtual unsigned newData ( unsigned &pos, unsigned minSamples, unsigned maxSamples ) =0;

  /**
      Increments the safe-position pointer.
  */
  virtual void advanceSafePosition ( unsigned inc ) =0;

  /**
      Locks the given fragment of the buffer.
      Only one lock can be active at the same time.
      Pointers returned in 'ptr' are valid till next 'unlockData()'
      call only.
  */
  virtual void lockData ( unsigned pos, unsigned len, CircularBufferPointers &ptr ) =0;

  /**
      Unlocks the buffer.
      Invalidates the previously returned 'ptr' structure.
  */
  virtual void unlockData ( unsigned processed, CircularBufferPointers &ptr ) =0;

  /**
      Updates 2D/3D related stuff.
      @param play2D3D combination of TTChan2D, TTChan3D flags
      @param channel chat channel
  */
  virtual void update2D3D(int play2D3D) {};
  virtual void get2D3DPlaying(bool &audible2D, bool &audible3D) { audible2D=audible3D=false; }

  /// get number of currently queued buffers
  virtual int GetQueuedBuffersCount() {return 0;}

  /// returns if audio buffers are ready
  virtual bool AreBuffersReady() {return true;}

};

//----------------------------------------------------------------------
//  Recorder (sound-capture):

#include "vonNet.hpp"
#include "El/Network/directchannel.hpp"

class VoNCodec;
class VoNSystem;
class VoNServer;
class VoNClient;

/// Maximum interval between VoN heart-beats (channel/codec info packet)
const unsigned64 MAX_VON_HEART_BEAT = 300000000; // 300 seconds

#include "Es/Memory/normalNew.hpp"

/**
    Sound recording peer (on the recording i.e. capturing side).
    VoNSystem creates this object, application calls it to encode
    recorded audio samples & send them to attached channel.
*/
class VoNRecorder : public RefCountSafe
{

protected:

  /// Actual VoN client.
  VoNClient *m_client;

  /// Actual VoN channel.
  VoNChannelId m_channel;

  /// Associated voice codec (encoder part is used here).
  RefD<VoNCodec> m_codec;

  /// Result code of the last 'encode()' operation.
  VoNResult m_res;

  /// Actual voice-capturing mode.
  bool m_isCapturing;

  /// is there is silence now
  bool m_isSilence;

  /// Actual sound frequency in Hz (accelerator).
  unsigned m_frequency;

  /// Actual sound depth in bits per sample (8 or 16).
  int m_bps;

  /// Minimum frame size in samples.
  unsigned m_minFrame;

  /// Maximum frame size in samples.
  unsigned m_maxFrame;

  /// Maximum voice-packet size (in samples).
  unsigned m_maxPacketSamples;

  /// Negative in silent time-intervals.
  int m_sound;

  /// Next encoded frame origin (in samples).
  unsigned64 m_toEncode;
  
  /// origin of last non-silence encoded
  unsigned64 m_nonSilence;

  /// Voice-data message in the middle of processing. Its "VoNDataPacket::origin" is valid.
  RefD<NetMessage> m_msg;

  /// Pointer to next unencoded segment (see "vonNet.hpp").
  unsigned16 *m_codePtr;

  /// Room left in "msg" (in bytes).
  int m_msgRoom;

  /// Equal to "vonData->origin" (accelerator).
  unsigned64 m_msgStart;

  /// System time of last control-message arrival (voice-channel "heart-beat").
  unsigned64 m_lastHeartBeat;

  /// Floating-averaged maximal amplitude (for "sounding" frames only).
  float m_floatingMax;

  /// DC estimated so far is DCsum/DCcount
  int DCsum;
  int DCcount;  //number of sections used for DC estimation
  /// Power estimated so far is PWavg
  float PWavg;
  int PWcount;  //number of samples used for Power estimation
  /// Clip factor is used to multiply recorded signal not to exceed the [MINSHORT,MAXSHORT] range
  float ClipFactor;
  int ClipCount; //number of samples used for Clip factor estimation

  /// temporary buffer for storing encoded data
  Temp<unsigned char> m_tmpBuf;

  /// threshold for silence analyzer
  float m_recThreshold;

  /// current frequency the CCItemTransmitter operates on
  float m_transmitterFreq;

public:
  /// If maximum amplitude is less than this, force "silence" state!
  static const float DEFAULT_REC_THRESHOLD;


  /// \param recThreshold threshold for silence analyzer
  VoNRecorder ( VoNClient *client, VoNChannelId ch, VoNCodec *codec, float recThreshold );

  virtual ~VoNRecorder ();

  /// MT-safe new operator.
  static void* operator new ( size_t size );
  /// MT-safe new operator.
  static void* operator new ( size_t size, const char *file, int line );
  /// MT-safe delete operator.
  static void operator delete ( void *mem );

  /**
      Encodes another batch of recorded (captured) sound data.
      @param  sb Sound-buffer containing input data.
      @param  pos Start position for encoding.
      @param  minEnc Minimum data amount being encoded (in number of samples).
      @param  maxEnc Maximum data amount being encoded (in number of samples).
      @return Actually encoded number of samples (0 implies an error or vonBreak state).
  */
  virtual unsigned encode ( VoNSoundBuffer *sb,
                            unsigned pos, unsigned minEnc, unsigned maxEnc );

  /// Returns the associated channel
  VoNChannelId GetChannel() const { return m_channel; }
  /// compute direct current
  int GetDC ( const CircularBufferPointers &cb, unsigned size );
  /// updates direct current using simple averaging
  void UpdateDC (int curDC);
  /// compute Power (volume)
  float GetPower ( const CircularBufferPointers &cb, unsigned size );
  /// updates Power using simple averaging
  void UpdatePower (float curPW, unsigned sampleCount);
  /// updates the Clipper factor using simple averaging (used to force amplitude not to exceed given range)
  void UpdateAmlitudeClipper (float maxAmpl, int sampleCount);
  /// compute the current boost (value used to multiply encoded data to get better gain level)
  float GetBoost();

  int getFrequency() const {return m_frequency;}
  /**
      Retrieves actual encoder state.
      Valid answers: vonOK or vonBreak (vonError?).
  */
  virtual VoNResult result ()
  { return m_res; }

  /// Retrieves actual voice-recording status (non-silence).
  virtual bool getRecording ();

  /// Sets voice-capture status.
  virtual void setCapture ( bool on );

  /// Sets maximum voice-packet size (in seconds).
  virtual void setMaxPacketTime ( double maxPacketTime );

  /// Returns "true" if recorded sound seems to be "silence".
  virtual bool silenceAnalyzer ( const CircularBufferPointers &cb, unsigned size, bool remember =false );

  /// Returns the actual codec.
  virtual VoNCodec *getCodec ()
  { return m_codec; }

  /// set quality of codec for encoding (Speex)
  void SetCodecQuality(int quality);

  float GetRecThreshold() const {return m_recThreshold;}
  void SetRecThreshold(float val) {m_recThreshold = val;}
  void SetTransmitterFreq(float f) {m_transmitterFreq = f;}
  float GetTransmitterFreq() const {return m_transmitterFreq;}

private:
  // sends message
  bool SendMsg();
};

template <>
struct HeapTraits < RefD<NetMessage> >
{
  static bool IsLess ( const RefD<NetMessage> &a, const RefD<NetMessage> &b )
  {
    VoNDataPacket *pa = (VoNDataPacket*)a->getData();
    VoNDataPacket *pb = (VoNDataPacket*)b->getData();
    return pa->origin < pb->origin;
  }
};


//----------------------------------------------------------------------
//  Replayer (sound-player):

/// Maximum number of frames to adapt getPlaying() return value.
const int MAX_PLAYING_DELAY = 6;

/*!
\patch 5213 Date 1/16/2008 by Bebul
- Fixed: MP freeze opportunity fixed.
*/

/**
    Sound replaying peer (on the replaying side).
	VoNClient creates this object, application calls it to fetch
	recieved audio samples.
*/
class VoNReplayer : public RefCountSafe
{

protected:
  /// Critical section to lock object. RefCountSafe lock cannot be used due to possible DeadLock. 
  PoCriticalSection lock;

  /// Enter the object's critical section.
  inline void enter() const
  { lock.enter(); }

  /// Leave the object's critical section.
  inline void leave() const
  { lock.leave(); }

  /// Actual VoN client.
  VoNClient *m_client;

  /// Actual VoN channel.
  VoNChannelId m_channel;

  /// Associated voice codec (decoder part is used here).
  RefD<VoNCodec> m_codec;

  /// Result code of the last 'decode()' operation.
  VoNResult m_res;

  /// Origin of voice-channel (in local system-time units).
  unsigned64 m_timeOrigin;

  /// Next decoded batch origin (in samples). Should be replayed in (timeOrigin+toDecode/freq) time.
  unsigned64 m_toDecode;

  /// Next arrived voice data (in samples).
  unsigned64 m_newest;

  /**
      Pointer to next undecoded segment (see "vonNet.hpp").
      Segment must not be empty (codePtr[0] != 0).
      If "codePtr != NULL", "toDecode" must be equal to "dataMessages.HeapGetFirst().origin".
      If "codePtr == NULL", "dataMessages" must be searched again to find next unprocessed message..
  */
  unsigned16 *m_codePtr;

  /// Actual sound frequency in Hz (accelerator).
  unsigned m_frequency;

  /// Actual sound depth in bits per sample (8 or 16).
  int m_bps;

  /// Minimum frame size in samples.
  unsigned m_minFrame;

  /// Maximum frame size in samples.
  unsigned m_maxFrame;

  /// Incoming data messages (not decoded yet). Key = data origin in samples.
  HeapArray<RefD<NetMessage>,MemAllocSafe> m_dataMessages;

  /// In suspended state?
  bool m_suspended;

  /// Drop-out state: 0 .. linear up, 1 .. exp. down
  int m_dropoutState;

  /// Actual dropout amplitude.
  float m_dropoutAmp;

  /// Counter for getPlaying() delay (>0 .. sounding, <=0 .. silence). abs(m_playingCounter) <= MAX_PLAYING_DELAY.
  int m_playingCounter;

  /// counter - number of decodes, when there was no message (is reseted to 0, when message arrives) 
  unsigned m_noMsgCount;

  /// counter - number of decodes, where we are waiting for audio buffers to be prepared
  unsigned m_waitingForBuffersCount;

  /// flag indicating that this is new voice
  bool m_newVoice;

  struct ReplayerPlayerInfo
  {
    /// Chat channel of the player behind the replayer
    VoNChatChannel m_chatChannel;
    bool m_play2D;
    bool m_play3D;

    ReplayerPlayerInfo() : m_chatChannel(0), m_play2D(false), m_play3D(false) {}
  };

  ReplayerPlayerInfo m_playerInfo;

  // Set to true when VoNSystem is unable to create sound buffer, so messages should be discarded instead of played
  bool m_discardVoNMessages;

public:

  VoNReplayer ( VoNClient *client, VoNChannelId ch, VoNCodec *codec );

  virtual ~VoNReplayer ();

  /// MT-safe new operator.
  static void* operator new ( size_t size );
  /// MT-safe new operator.
  static void* operator new ( size_t size, const char *file, int line );
  /// MT-safe delete operator.
  static void operator delete ( void *mem );

  /**
      Decodes another batch of transferred sound data. res = {vonOK, vonDropout, vonSilence, vonBreak}
      @param  sb Sound-buffer to hold raw output data.
      @param  pos Start position for decoding.
      @param  minDec Minimum data amount being decoded (in number of samples).
      @param  maxDec Maximum data amount being decoded (in number of samples).
      @return Actually decoded number of samples (0 implies an error or vonBreak state).
  */
  virtual unsigned decode ( VoNSoundBuffer *sb,
                            unsigned pos, unsigned minDec, unsigned maxDec );

  VoNChannelId GetChannel() { return m_channel; }

  // VoNSay has no VoNRecorder, but it needs some simple recording management.
  unsigned64 m_toEncode; // used only for VoNSay purpose

  /**
      Retrieves actual decoder state (result of the last 'decode()' call).
  */
  virtual VoNResult result ()
  { return m_res; }

  /// New data message arrival.
  virtual void newDataMessage ( NetMessage *msg );

  /// Enters/leaves the "suspended" state (won't accept any data message..).
  virtual void suspend ( bool on );

  /// Actual replayer-frequency in Hz.
  unsigned getFrequency () const
  { return m_frequency; }

  /// Replaying amplitude (0.0f to 1.0f).
  virtual float getPlaying ();

  /// Returns associated codec instance.
  VoNCodec *getCodec () const
  { return m_codec; }

  /// Returns true, if silence. Fills in the amplitude of signal in circular buffer
  // only for debugging purposes
  bool GetAmplitude ( const CircularBufferPointers &cb, unsigned size, float &ampl );

  /// check whether chat channel of the remote player has changed and initiate related actions
  void CheckChatChannel(VoNDataPacket *packet);

  /// called when VoNSystem is unable to create sound buffer, so messages should be discarded instead of played
  void ForceVoNMessagesDiscarding() { m_discardVoNMessages=true; };

  /// get current VoN chat channel
  VoNChatChannel GetChatChannel() const { return m_playerInfo.m_chatChannel; }

  /// get whether 2D voice should be played
  bool GetPlay2D() const { return m_playerInfo.m_play2D; }

  /// get whether 3D voice should be played
  bool GetPlay3D() const { return m_playerInfo.m_play3D; }

public:
  float m_transmitterFreq;

protected:

  /// Resets dropout state (to "linear-up"). "sounding" is true for sounding frames, false for silence.
  void resetDropout ( bool sounding );

  /// Generates dropout sound of the given size (in samples).
  void generateDropout ( CircularBufferPointers &ptr, unsigned size );

  /// Generates dropout sound of the given size (in samples).
  void generateDropoutSilence ( CircularBufferPointers &ptr, unsigned size );
  
  /// Sets/resets actual sounding status (should be called once for each frame).
  void setSound ( bool sounding );

};

extern RandomJames vonRnd;

//----------------------------------------------------------------------
//  VoNChannel data record (used internally in VoNServer):

//extern unsigned vonIdentityKey ( VoNIdentity &id );

class VoNChannel : public RefCountSafe
{

public:

  /// Voice-channel id.
  VoNChannelId id;

  /// List of identities.
  ImplicitSet<VoNIdentity,intToUnsigned,true,MemAllocSafe> ids;

  VoNChannel ( VoNChannelId i_id );

  /// MT-safe new operator.
  static void* operator new ( size_t size );
  /// MT-safe new operator.
  static void* operator new ( size_t size, const char *file, int line );
  /// MT-safe delete operator.
  static void operator delete ( void *mem );

};

//----------------------------------------------------------------------
//  VoN server (VoNServer class):

extern VoNChannelId netMessageToVoiceChannel (const RefD<NetMessage> &msg);

#ifdef _WIN32
template ImplicitMap<VoNChannelId,RefD<NetMessage>,netMessageToVoiceChannel,true,MemAllocSafe>;
#endif

/**
    This object acts as a VoN-packet-router only. Routing tables (=voice-channels)
    will be set externally.
*/
class VoNServer : public RefCountSafe
{
  friend class VoNClient;
protected:
  /// Critical section to lock object. RefCountSafe lock cannot be used due to possible DeadLock. 
  PoCriticalSection lock;

  /// Enter the object's critical section.
  inline void enter() const
  { lock.enter(); }

  /// Leave the object's critical section.
  inline void leave() const
  { lock.leave(); }

  /// Actual VoNSystem instance.
  VoNSystem *m_von;
  AdjacencyMatrix m_matrix;
  AutoArray< AutoArray<VoNChannelId> > m_peerCliques;
  FindArray<int> m_matrixIds;

  /// Channel storage.
  ExplicitMap<VoNChannelId,RefD<VoNChannel>,true,MemAllocSafe> m_ch;

  /// Backup of VON_CMD_CREATE messages.
  ImplicitMap<VoNChannelId,RefD<NetMessage>,netMessageToVoiceChannel,true,MemAllocSafe> m_createMessages;

  /// Last received VON_CMD_PEER_INFO messages.
  ImplicitMap<VoNChannelId,RefD<NetMessage>,netMessageToVoiceChannel,true,MemAllocSafe> m_peerInfo;

  /// m_peerCliques needs updating - channel info or peer info changed
  bool m_peerCliquesDirty;
  
  /// Protected constructor <- this is abstract class.
  VoNServer ( VoNSystem *von );

  /// Sends CTRL message to all receivers of the given channel.
  void sendCtrl ( VoNChannelId id, unsigned command, const void *data=NULL, int size=0, bool echo=false );
  
  /// Sends VON_CMD_CREATE message to the given receiver.
  void sendCreate ( VoNChannelId id, VoNIdentity dest );

  friend NetStatus vonServerReceive ( NetMessage *msg, NetStatus event, void *data );

#if _ENABLE_REPORT
  int _receivedDataCount;
  int _receivedDataSize;
  int _receivedCtrlCount;
  int _receivedCtrlSize;
  int _sentDataCount;
  int _sentDataSize;
  int _sentCtrlCount;
  int _sentCtrlSize;
#endif

public:

  /**
      External mapping (VoNIdentity -> NetChannel) is needed for packet-routing.
      This mapping must interpret identities < ExtReservedIds as local connection.
  */
  virtual RefD<NetChannel> netChannelFromIdentity ( VoNIdentity id ) =0;

  /// (Re)sets the whole voice-channel mapping. If |to|==0, destroys the channel.
  void setChannel ( VoNChannelId id, AutoArray<int, MemAllocSA> &to );

  /// Gets the whole voice-channel mapping.
  void getChannel ( VoNChannelId id, AutoArray<int, MemAllocSA> &to );

  /// get the client (if exists)
  RefD<VoNClient> getClient();

  /// opportunity to update peer to peer matrix
  void updateTransmitTargets();

  /// gets P2P adjacency matrix and cliques
  void GetVoNTopologyDiag(QOStrStream &out, Array<ConvPair> &convTable);

  /// Connects the given net-channel to VoNServer instance.
  void connect ( VoNChannelId id );

  /// Disconnects the given net-channel from VoNServer.
  void disconnect ( VoNChannelId id );

  virtual ~VoNServer ();

#if _ENABLE_REPORT
  int GetReceivedDataCount() const {return _receivedDataCount;}
  int GetReceivedDataSize() const {return _receivedDataSize;}
  int GetReceivedCtrlCount() const {return _receivedCtrlCount;}
  int GetReceivedCtrlSize() const {return _receivedCtrlSize;}
  int GetSentDataCount() const {return _sentDataCount;}
  int GetSentDataSize() const {return _sentDataSize;}
  int GetSentCtrlCount() const {return _sentCtrlCount;}
  int GetSentCtrlSize() const {return _sentCtrlSize;}
  void ResetStats()
  {
    _receivedDataCount = 0;
    _receivedDataSize = 0;
    _receivedCtrlCount = 0;
    _receivedCtrlSize = 0;
    _sentDataCount = 0;
    _sentDataSize = 0;
    _sentCtrlCount = 0;
    _sentCtrlSize = 0;
  }
  void OnDataReceived(size_t size)
  {
    _receivedDataCount++;
    _receivedDataSize += size + IP_UDP_HEADER;
  }
  void OnCtrlReceived(size_t size)
  {
    _receivedCtrlCount++;
    _receivedCtrlSize += size + IP_UDP_HEADER;
  }
  void OnDataSent(size_t size)
  {
    _sentDataCount++;
    _sentDataSize += size + IP_UDP_HEADER;
  }
  void OnCtrlSent(size_t size)
  {
    _sentCtrlCount++;
    _sentCtrlSize += size + IP_UDP_HEADER;
  }
#endif
};

/// information about peer to peer transmission target
const BYTE TTChan2D = 0x80;
const BYTE TTChan3D = 0x40;
const BYTE TTBufQueued = 0x20;
const BYTE TTDirectChan = 0x08;
const BYTE TTChannelMask = 0xc0;
struct TransmitTarget
{
  sockaddr_in addr;
  int dpid;
#if _GAMES_FOR_WINDOWS
  XUID xuid;
#endif
  BYTE extInfo; 
};

TypeIsSimple(TransmitTarget)

/// extended information about peer to peer transmission target
/**
Contains information about peer to peer connection established
Time to next probe is some "minimal time" * (2^probeNumber)
*/
struct TransmitTargetExt
{
  sockaddr_in addr;
  int dpid;
#if _GAMES_FOR_WINDOWS
  XUID xuid;
#endif
  BYTE extInfo; 
  /// peer to peer successful
  bool peerToPeer;
  // whether peerToPeer was set to true at least once
  bool wasPeerToPeer;


  /// traversed NAT needs to be updated by sending keepAlive packets to get firewall open
  struct KeepAliveInfo
  {
    static const int MaxProbes = 20;
    static const unsigned64 KeepAlivePeriod = 20000000; // microseconds (20 sec)
    static const unsigned64 MinProbeTime =      100000; //0.1 sec
    static const unsigned64 KeepAliveMaxWaitTime = 120000000; //2 minutes
    bool       _active;              //inactive is skipped in ProcessKeepAlive
    int        _probeNumber;         //the number of probes sent
    int        _recvPacketsCount;    //number of packets received
    int        _voicePacketsCount;   //number of voice packet received
    int        _badVoicePacketsCount; //voice packets that were skipped because of being invalid
    int        _voicePacketsSndCount; //number of voice packet send 
    unsigned64 _lastKeepAliveTime;   //last time the OriginProbeValue was sent
    unsigned64 _lastGotResponseTime; //last time the OriginResponseValue has been received
    bool _cmdResetConPossible;       //VON_CMD_RESET_CONNECTION can be send only when Response packet was sent

    void ResetTime() { _lastKeepAliveTime = getSystemTime(); }
    // return true whenever there is a time to send new KeepAlive packet (probe)
    // after connection is verified Test returns true after every KeepAlivePeriod
    bool Test(unsigned64 curTime)
    {
      if (!_active) return false; //no probing when inactive
      //maximal time span is KeepAlivePeriod
      unsigned64 testTime = KeepAlivePeriod+_lastKeepAliveTime;
      if (_probeNumber!=MaxProbes)
      {
        unsigned64 toNextProbe = (1ULL<<_probeNumber)*MinProbeTime;
        if (_lastKeepAliveTime+toNextProbe < testTime) //probes should be sent more frequently until connection is verified
          testTime = _lastKeepAliveTime+toNextProbe;
      }
      return (testTime < curTime);
    }
    // called whenever new probe is sent
    // Returns true when there are probes to be send yet
    bool NextProbe() 
    { 
      bool doProbe = _probeNumber < MaxProbes;
      if (doProbe) _probeNumber++; 
      return doProbe;
    }
    // called when connection is verified
    void FinishProbes() { _probeNumber = MaxProbes; }
    // reset time of last OriginXXXValue received
    void SetProbeResponseReceived()
    {
      _lastGotResponseTime = getSystemTime();
      _recvPacketsCount++;
    }
    // new voice packet for me
    void SetVoicePacketReceived() { _voicePacketsCount++; }
    // bad voice packets count diagnostics
    void SetBadVoicePacketReceived() { _badVoicePacketsCount++; }
    // Force TestAlive to return true (i.e. do not send VON_CMD_RESET_CONNECTION)
    // new voice packet sent to
    void SetVoicePacketSent() { _voicePacketsSndCount++; }
    void ForceAlive()
    {
      _lastGotResponseTime = getSystemTime() + 1800000000ULL; // half an hour
    }
    // return false whenever no OriginXXXValue was received in KeepAliveMaxWaitTime period
    bool TestAlive()
    {
      unsigned64 now = getSystemTime();
      return (_lastGotResponseTime + KeepAliveMaxWaitTime > now);
    }
    // VON_CMD_RESET_CONNECTION can be send only when Response packet was sent
    void SetCmdResetConnectionPossible() { _cmdResetConPossible=true; };
    bool IsCmdResetConnectionPossible() { return _cmdResetConPossible; };

    /// Diagnostics - how many packets (Probe, Response, KeepAlive or Voice) has been received
    int GetP2PRecvCount() { return _recvPacketsCount; }

    /// make keepAlive active or inactive
    void Start(bool start) { _active = start; }

    KeepAliveInfo() : _probeNumber(0), _cmdResetConPossible(false), _active(false)
    {
      SetProbeResponseReceived(); //wait on connection
      ResetTime();
      _recvPacketsCount = 0;
      _voicePacketsCount = 0;
      _voicePacketsSndCount = 0;
      _badVoicePacketsCount = 0;
    }
  };
  KeepAliveInfo keepAlive;
  
  TransmitTargetExt()
  {
    peerToPeer = wasPeerToPeer = false;
  }
};
TypeIsMovable(TransmitTargetExt)

#if _GAMES_FOR_WINDOWS
struct IXHV2Engine;
#endif

// inform server to delete corresponding P2P connection (from tgt to me!)
/// PeerToPeerEdge is being sent in VON_CMD_RESET_CONNECTION messages when connection does not seem to work anymore
struct PeerToPeerEdge 
{
  int dpnidFrom, dpnidTo; //ids of corresponding peers
  PeerToPeerEdge(int from, int to) : dpnidFrom(from), dpnidTo(to) {};
};

//----------------------------------------------------------------------
//  VoN client (VoNClient class):

typedef void (*PeerChangedCallback)(int dpid, int port, void *context);

/**
    This object manages a VoNRecorder and set of VoNReplayers to record/replay
    sounds.
*/
class VoNClient : public RefCountSafe
{
  friend NetStatus vonServerReceive ( NetMessage *msg, NetStatus event, void *data );

protected:
  /// Critical section to lock object. RefCountSafe lock cannot be used due to possible DeadLock. 
  PoCriticalSection lock;

  /// Enter the object's critical section.
  inline void enter() const
  { lock.enter(); }

  /// Leave the object's critical section.
  inline void leave() const
  { lock.leave(); }
#if _USE_DEAD_LOCK_DETECTOR
  inline void enterNoDeadLockDetector() const
  { lock.enterNoDeadLockDetector(); }
  inline void leaveNoDeadLockDetector() const
  { lock.leaveNoDeadLockDetector(); }
#endif 

  /// Actual VoNSystem instance.
  VoNSystem *m_von;

  /// Associated network channel (duplex).
  RefD<NetChannel> m_netCh;

#if _GAMES_FOR_WINDOWS
  // TODOXNET: Voice implementation

  // XHV engine
  IXHV2Engine *_XHVEngine;
  // XHV worker thread
  HANDLE _XHVWorkerThread;
  // Local player index
  int _localPlayer;

  /// indices of all registered local talkers
  AutoArray<int> _localTalkerIndices;

#else
  /// Set of active replayers.
  ExplicitMap<VoNChannelId,RefD<VoNReplayer>,true,MemAllocSafe> m_repl;

  /// Recorder instance.
  RefD<VoNRecorder> m_rec;
#endif

  /// we should know about beeing BOT_CLIENT (for retranslation through game socket)
  bool _botClient;

  /// Peer-to-peer voice socket
  RefD<NetPeerToPeerChannel> m_peerToPeer;

  /// Callback function for peer-to-peer socket
  RawMessageCallback _callback;

  /// list of peer-to-peer recipients  
  AutoArray<TransmitTarget> m_transmitTargets;
  /// list of all players
  AutoArray<TransmitTargetExt> m_allTargets;

  //array of peers with their port changed
  struct PeerChanged
  {
    int _dpid;
    int _port;
    PeerChanged() : _dpid(0) {}
    PeerChanged(int dpid, int port) : _dpid(dpid), _port(port) {}
    ClassIsMovableZeroed(PeerChanged)
  };
  AutoArray<PeerChanged> m_peerChanged;

  /// peer to peer status of some target has changed
  bool m_allTargetsPeerToPeerChanged;
  
  ///  clique information received
  AutoArray<RefD<NetMessage>, MemAllocSafe > m_cliquesMessages;
  
  /// all detected cliques
  AutoArray< FindArray<int> > m_cliques;
  
  /// lock for both m_transmitTargets and m_allTargets
  PoCriticalSection m_transmitTargetsLock;
  
  /// lock for m_chatChannel
  PoCriticalSection m_chatChannelLock;

  /// active chat channel
  VoNChatChannel m_chatChannel;

  /// voice transmition ON/OFF based on UAVoiceOverNet
  bool m_voiceTransmitionEnabled;
  /// voice transmition is toggled on (that means user is not using push to talk button)
  bool m_voiceToggleOn;

  /**
  When VoiceConnect is called, KeepAlive system should start.
  But starting KeepAlive requires TransmitTarget is already created, which is not neccesarily true.
  (Transmit target update message can be delayed.)
  Array of KAStartItems servers to start KeepAlive system in this case.
  */
  struct KAStartItem
  {
    int _dpid;
    bool _start;   //whether to start or stop keepAlive
    DWORD _time;   //time of insertion (when some timeout elapses, the item is removed)
    KAStartItem() : _dpid(0), _start(false), _time(0) {};
    KAStartItem(int dpid, bool start) : _dpid(dpid), _start(start) { _time = GetTickCount(); };
    ClassIsSimpleZeroed(KAStartItem)
  };
  //  TODO: use TListBidir instead of AutoArray
  AutoArray<KAStartItem> _waitingKAStarts;

  /// Find transmit targets for waiting KAStarts and start keepAlive for them
  void UpdateKAStarts();

  friend NetStatus vonClientReceive ( NetMessage *msg, NetStatus event, void *data );

  /// static method is used in order to be passed as simple function pointer callback to PcPeerToPeerChannel
  static void vonKeepAliveProc(void *ctx);
  /// non static is used to access non static members
  void ProcessKeepAlive();
  /// enter and leaveTransmitTargets should be called around this call
  void SetTargetsDirty() { m_allTargetsPeerToPeerChanged=true; };

  /// The limit of speaking players
  static const int maxVoicePlayers = 256;
  
  class StoredMessages
  {
    friend class VoNClient;
  public:
    struct VoiceMessageToSend
    {
      AutoArray<char, MemAllocSafe> _data;
      int   _size;
      int   _encryptedSize;
      TransmitTarget _targets[VoNClient::maxVoicePlayers];
      int   _nTargets;
      bool  _heartBeat;
      VoiceMessageToSend() : _data(NetChannelBasic::par.maxPacketSize), _size(0), _nTargets(0), _heartBeat(false)
      {}
      void Init(const void *data, int size, int encryptedSize, const TransmitTarget *targets, int nTargets, bool heartBeat=false);
      ClassIsMovable(VoiceMessageToSend)
    };
    struct CtrlMessageToSend
    {
      unsigned _command;
      AutoArray<char, MemAllocSafe> _data;
      int      _size;
      VoNChannelId _id;
      bool         _useVim;
      CtrlMessageToSend() : _data(NetChannelBasic::par.maxPacketSize), _size(0)
      {}
      void Init(VoNChannelId id, unsigned command, const void *data, int size, bool useVim);
      ClassIsMovable(CtrlMessageToSend)
    };
  protected:
    /// lock for sending stored Voice messages
    PoCriticalSection m_voiceToSendLock;

    AutoArray<CtrlMessageToSend,MemAllocSafe> _ctrlMessages;
    /// index of which _voiceMessagesPool is used for current messages storing
    int _poolIx;
    /// when sending stored messages, other thread can store into the other message array
    AutoArray<VoiceMessageToSend,MemAllocSafe> _messagesPool[2];

    void SwitchPools() {_poolIx = (_poolIx+1)%2; }
    AutoArray<VoiceMessageToSend,MemAllocSafe> & GetVoiceMessagesToStore() { return _messagesPool[_poolIx]; }
    AutoArray<CtrlMessageToSend,MemAllocSafe>  & GetCtrlMessagesToStore() { return _ctrlMessages; } 
  public:
    StoredMessages() : _poolIx(0), _ctrlMessages(16) 
    {
      _messagesPool[0].Realloc(16);
      _messagesPool[1].Realloc(16); //16 should be sufficient not to cause reallocation
    }
  };

  /// Voice and control VON_CMD_ENCAPSULATE_VOICE messages stored for later send (in order to avoid deadLocks)
  StoredMessages _storedMessages;
  /// captured (and codec encoded) voice is not send directly after encoding, but from the voiceListenSend thread
  void SendStoredVoice();
public:
  bool PeerToPeerReceive(const void *data, int size, sockaddr_in *from, bool detectVoicePackets);
  void StoreVoiceToSend(
    const void *data, int size, int encryptedSize,
    const TransmitTarget *targets, int nTargets, bool heartBeat=false
  );
  void StoreCtrlToSend(VoNChannelId id, unsigned command, const void *data=NULL, int size=0, bool useVim=false);
protected:
  void EnterVoiceMessageToSend() { _storedMessages.m_voiceToSendLock.Lock(); };
  void LeaveVoiceMessageToSend() { _storedMessages.m_voiceToSendLock.Unlock(); };

#if _ENABLE_REPORT
  int _receivedDataCount;
  int _receivedDataSize;
  int _receivedCtrlCount;
  int _receivedCtrlSize;
  int _sentDataCount;
  int _sentDataSize;
  int _sentCtrlCount;
  int _sentCtrlSize;
#endif

public:
#if _GAMES_FOR_WINDOWS
  /// localPlayerIndices...iser indices of all local players we want to register as local talkers
  VoNClient(VoNSystem *von, IXAudio2 *xaudio2, bool isBotClient, int localPlayer, const AutoArray<int> &localTalkerIndices);
#else
  VoNClient(VoNSystem *von, bool isBotClient);
#endif

  /// Default output voice-channel.
  VoNChannelId outChannel;

  /// Connects/disconnects the client to/from network channel.
  void connectPeerToPeer ( RefD<NetPeerToPeerChannel> ch, RawMessageCallback callback );

  /// Connects/disconnects the client to/from network channel.
  void connect ( RefD<NetChannel> ch );

  /// Port where bind to
  int GetPort() const {return m_peerToPeer ? m_peerToPeer->GetPort() : 0;}

  /// Used socket
  SOCKET GetSocket() const {return m_peerToPeer ? m_peerToPeer->GetSocket() : INVALID_SOCKET;}

  /// Creates new voice-channel (replayer).
  void createChannel ( VoNChannelId channel, VoNControlPacket *msg );

  /// Removes the existing voice-channel (replayer).
  void removeChannel ( VoNChannelId channel );

  /// Suspends the existing voice-channel (replayer).
  void suspendChannel ( VoNChannelId channel );

  /// Resumes the existing voice-channel (replayer).
  void resumeChannel ( VoNChannelId channel );

  /// Store peer-to-peer clique information for later processing
  void rememberCliques(RefD<NetMessage> msg);
  
#if _GAMES_FOR_WINDOWS
  /// Registers user with user index as local talker
  bool RegisterLocalTalker(int userIndex);
  /// Unregisters user with user index as local talker
  bool UnregisterLocalTalker(int userIndex);


  /// Registers user with given xuid as remote talker
  bool RegisterRemoteTalker(XUID xuid);
  /// Unregisters user with given xuid as remote talker
  bool UnregisterRemoteTalker(XUID xuid);

  /// Handle data prepared on microphone
  void OnMicrophoneDataReady(DWORD userIndex);

  /// Handle data arrived through network
  void ReplayData(VoNChannelId channel, VoNDataPacket *vonData);

#else
  /// Retrieves the given replayer.
  RefD<VoNReplayer> getReplayer ( VoNChannelId channel );

  /// (Re)sets recorder instance.
  void setRecorder ( RefD<VoNRecorder> recorder );

  /// Returns the actual recorder instance.
  RefD<VoNRecorder> getRecorder () { return m_rec; }
#endif

  /// get access to server (if exists)  
  RefD<VoNServer> getServer();

  /// Returns associated net-channel pointer.
  RefD<NetChannel> &getChannel () { return m_netCh; }

  /// Returns associated peer-to-peer pointer.
  RefD<NetPeerToPeerChannel> &getPeerToPeer () { return m_peerToPeer; }

  /// transmit targets need to be locked while accessing them
  void enterTransmitTargets() {m_transmitTargetsLock.Lock();}
  /// transmit targets need to be locked while accessing them
  void leaveTransmitTargets() {m_transmitTargetsLock.Unlock();}
  
  /// get extended information for any target
  /** call enterTransmitTargets and leaveTransmitTargets around this call */
  TransmitTargetExt *FindTransmitTarget(int dpid);

  /// Transmit Target diagnostics (time elapsed for keepAlive, number of keepAlives received, wasPeerToPeer etc.)
  RString GetTargetDiagnostics(int dpid);
  
  /// Active Cliques diagnostics
  void GetVoiceCliquesDiagnostics(QOStrStream &out, Array<ConvPair> &convTable);

  /// start/stop VoN KeepAlive system (sending probes,response,keepAlive packets)
  void StartKeepAlive(bool start, int dpid);

  /// process specified action for all peerChanged
  void ProcessPeerChanged(PeerChangedCallback proc, void *context)
  {
    enterTransmitTargets();
    for (int i=0; i<m_peerChanged.Size(); i++)
    {
      proc(m_peerChanged[i]._dpid, m_peerChanged[i]._port, context);
    }
    m_peerChanged.Resize(0);
    leaveTransmitTargets();
  }
  
  /// get list of receivers
  /** call enterTransmitTargets and leaveTransmitTargets around using the result */
  const AutoArray<TransmitTarget> &getTransmitTargets() {return m_transmitTargets;}
  
  /// get list of all players
  /** call enterTransmitTargets and leaveTransmitTargets around using the result */
  const AutoArray<TransmitTargetExt> &getAllTargets() {return m_allTargets;}

  /// get active chat channel
  const VoNChatChannel GetChatChannel();

  // helper struct to pass indirect targets to SendPeerToPeerMessage to retranslate
  struct IndirectInfo;

  /// helper for peer to peer transmissions with re-translation (does not check for retranslation through game port to server)
  void SendPeerToPeerMessageDirect(
    int directId, const sockaddr_in &ia, const void *data, int size, int encryptedSize,
    IndirectInfo *indirect, int nIndirect
    );
  /// helper for peer to peer transmissions with re-translation 
  /// it checks for possible necessity or retranslation through game socket to server
  void SendPeerToPeerMessage(
    int directId, const sockaddr_in &ia, const void *data, int size, int encryptedSize,
    IndirectInfo *indirect, int nIndirect
    );
  
  /// peer to peer transmissions with re-translation
  void SendPeerToPeerMessage(
    const void *data, int size, int encryptedSize,
    const TransmitTarget *targets, int nTargets
  );
  
  /// send a control message to the server
  void sendCtrl(unsigned command, const void *data=NULL, int size=0 );
  
  /// send a control message to given channel
  void sendCtrlTo(VoNChannelId id, unsigned command, const void *data, int size, bool useVimFlag=true);
  
  void SendPeerToPeerData(int dpid, const sockaddr_in &addr, const void *data, size_t size, size_t encryptedSize);

  /// send a probe checking peer to peer communication
  void SendPeerToPeerSpecial(int srcChannel, int value, TransmitTargetExt &target);

  /// set list of receivers
  void setTransmitTargets(const TransmitTarget *addr, int nAddr);
  
  /// set list of receivers
  void setAllTargets(const TransmitTarget *addr, int nAddr);

  /// chat channel need to be locked while accessing them
  void enterChatChannel() {m_chatChannelLock.Lock();}
  /// chat channel need to be locked while accessing them
  void leaveChatChannel() {m_chatChannelLock.Unlock();}

  /// set active chat channel
  void setChatChannel(VoNChatChannel chatChan);
  
  /// Retrieves actual voice-recording status.
  bool getRecording ();

  /// Actual replaying amplitude (0.0f to 1.0f).
  float getPlaying ( VoNChannelId channel );

  /// Sets voice-capture status.
  void setCapture ( bool on, int quality );
  
  /// Set voice transmition ON/OFF based on UAVoiceOverNet
  void setVoiceTransmition(bool val) { m_voiceTransmitionEnabled=val; }
  /// true when VoN based on UAVoiceOverNet is enabled
  bool IsTransmitionEnabled() { return m_voiceTransmitionEnabled; }

  /// set if voice is toggled on (that means user is not using push to talk button)
  void SetVoiceToggleOn(bool val) { m_voiceToggleOn=val; }
  bool IsVoiceToggleOn() const {return m_voiceToggleOn;}

  /// sets threshold for silence analyzer (0.0f - 1.0f)
  void SetVoNRecThreshold(float val) 
  {
    if (m_rec) m_rec->SetRecThreshold(val);
  }
  /// set the frequency current CCItemTransmitter operates on
  void SetVoNTransmitterFreq(float f)
  {
    if (m_rec) m_rec->SetTransmitterFreq(f);
  }
  float GetVoNTransmitterFreq() const
  {
    if (m_rec) return m_rec->GetTransmitterFreq();
    return 0;
  }

  /// for dedicated server, replaying and recording is disabled, only retranslate mechanism should work
  bool RetranslateOnly();

  int CodecQuality();

#if _ENABLE_VON
  void VoNSay(int dpnid, int channel, int frequency, int seconds);
  void EncodeBeep(int dpnid, int channel, int frequency, int seconds, VoNReplayer *replayer);
#endif

  ~VoNClient ();

  /// MT-safe new operator.
  static void* operator new ( size_t size );
  /// MT-safe new operator.
  static void* operator new ( size_t size, const char *file, int line );
  /// MT-safe delete operator.
  static void operator delete ( void *mem );

#if _ENABLE_REPORT
  int GetReceivedDataCount() const {return _receivedDataCount;}
  int GetReceivedDataSize() const {return _receivedDataSize;}
  int GetReceivedCtrlCount() const {return _receivedCtrlCount;}
  int GetReceivedCtrlSize() const {return _receivedCtrlSize;}
  int GetSentDataCount() const {return _sentDataCount;}
  int GetSentDataSize() const {return _sentDataSize;}
  int GetSentCtrlCount() const {return _sentCtrlCount;}
  int GetSentCtrlSize() const {return _sentCtrlSize;}
  void ResetStats()
  {
    _receivedDataCount = 0;
    _receivedDataSize = 0;
    _receivedCtrlCount = 0;
    _receivedCtrlSize = 0;
    _sentDataCount = 0;
    _sentDataSize = 0;
    _sentCtrlCount = 0;
    _sentCtrlSize = 0;
  }
  void OnDataReceived(size_t size)
  {
    _receivedDataCount++;
    _receivedDataSize += size + IP_UDP_HEADER;
  }
  void OnCtrlReceived(size_t size)
  {
    _receivedCtrlCount++;
    _receivedCtrlSize += size + IP_UDP_HEADER;
  }
  void OnDataSent(size_t size)
  {
    _sentDataCount++;
    _sentDataSize += size + IP_UDP_HEADER;
  }
  void OnCtrlSent(size_t size)
  {
    _sentCtrlCount++;
    _sentCtrlSize += size + IP_UDP_HEADER;
  }
#endif
};

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  VoN factory (VoNSystem class):

/**
    Global VoN engine/factory.
    Manages voice servers (VoNServer) and clients (VoNClient).
    Lives transparently both on VoN servers and clients.
    VoN server acts as VoN-packet router only ..
*/
class VoNSystem : public RefCountSafe
{

protected:

  /// Actual VoN server (one server and one client can reside in one VoNSystem).
  RefD<VoNServer> m_server;

  /// Actual VoN client (one server and one client can reside in one VoNSystem).
  RefD<VoNClient> m_client;

  /// Local communication peer for the server.
  RefD<DirectChannel> m_sLocal;

  /// Local communication peer for the client.
  RefD<DirectChannel> m_cLocal;

  /// Listener position
  Vector3 m_listenerPos;
  DWORD m_listenerPosSetTime;

  /// codec quality for encoding
  int m_codecQuality;

public:
  /// Square of Max transmitter range distance
  float m_maxTransmitterRange2;

  /// Preferred bits-per-sample for recording.
  int inBps;

  /// Preferred bits-per-sample for replaying.
  int outBps;

	/// Default voice-mask used in encoder-construction.
	VoiceMask defaultVoiceMask;

  /// Creates an empty VoN system.
  VoNSystem ();

  /// Destroys server & client instance(s) implicitly.
  virtual ~VoNSystem ();

  /// Sets the actual server (can be NULL).
  virtual void setServer ( VoNServer *server );

  /// Sets the actual client (can be NULL).
  virtual void setClient ( VoNClient *client );

  /// Stops all client activities (capturing, replaying).
  virtual void stopClient ();

  /// Connects server & client by local network channel (DirectChannel).
  virtual void localConnect ();

  virtual const RefD<VoNServer> &getServer () const
  { return m_server; }

  virtual const RefD<VoNClient> &getClient () const
  { return m_client; }

  virtual const RefD<DirectChannel> &getServerChannel () const
  { return m_sLocal; }

  virtual const RefD<DirectChannel> &getClientChannel () const
  { return m_cLocal; }

#if _ENABLE_VON
  /// Finds a codec which fits best the given request.
  virtual VoNCodec *findCodec ( CodecInfo &info );

  /// Restores codec from the saved binary info.
  virtual VoNCodec *restoreCodec ( unsigned16 size, unsigned8 *data );
#endif

  /// Application-level response to replayer create/destroy. Should be overriden!
  virtual VoNResult setReplay ( VoNChannelId chId, bool create );

  /// Application-level response to replayer suspend/resume. Should be overriden!
  virtual VoNResult setSuspend ( VoNChannelId chId, bool suspend );

  /// Application-level response to capture on/off (microphone switch). Should be overriden!
  virtual VoNResult setCapture ( bool on, int quality );

  /// Switches on the internal memory-based recorder.
  virtual VoNResult startRecorder ();

  /// Stops internal memory-based recorder, returns recorded data, returns back the regular recorder.
  virtual int stopRecorder ( AutoArray<char,MemAllocSafe> &buf );

  /// Starts replaying of memory data.
  virtual VoNResult startReplay ( AutoArray<char,MemAllocSafe> &buf, int duration );

  /// Returns memory-replayer status (VonOK if finished, VonBusy, VonBadData).
  virtual VoNResult getReplayStatus ();

  /// Stops memory-replayer and returns its status.
  virtual VoNResult stopReplay ();

  virtual RefD<VoNSoundBuffer> getSoundBuffer ( VoNChannelId chId ) = 0;

  /// Changes actual voice-mask. Should re-initialize all object instances which depend on it..
  virtual void changeVoiceMask ( VoiceMask &mask );

  /// retranslate only, when on dedicated server
  virtual bool RetranslateOnly() { return false; }

  /// Set position of player (affects source)
  virtual void setPosition(int dpnid, Vector3Par pos, Vector3Par speed) {}

  /// Set position of listener player
  virtual void setPlayerPosition(Vector3Par pos, Vector3Par speed) 
  { 
    m_listenerPos = pos; 
    m_listenerPosSetTime = ::GetTickCount();
  }
  // Get Position of listener player
  Vector3 getPlayerPosition(Vector3Par pos, Vector3Par speed) const { return m_listenerPos; }

  /// Set obstruction of player (affects source)
  virtual void setObstruction(int dpnid, float obstruction,float occlusion,float deltaT) {}

  /// Update volume and ear accomodation of player
  virtual void updateVolumeAndAccomodation(int dpnid, float volume, float accomodation) {};

  /// check what channel can we currently hear the player on
  virtual VoNChatChannel checkChatChannel(int dpnid, bool &audible2D, bool &audible3D)
  {
    audible2D = audible3D = false;
    return 0;
  }

  virtual void get2D3DPlaying(int dpnid, bool &audible2D, bool &audible3D)
  {
    audible2D = audible3D = false;
  }

  /// fill in the array by dpnid of all players within the maxDist distance using position from their VoNSoundBufferOAL
  virtual void WhatUnitsVoNDirect(AutoArray<int> &units, Vector3Par pos, float maxDist) {}
  
  /// Advance all voices (even when muted)
  virtual void advanceAll(float deltaT, bool paused, bool quiet) {};

  /// codec quality for encoding
  int CodecQuality() { return m_codecQuality; };
  void SetCodecQuality(int quality) { m_codecQuality = quality; }
};

#endif
