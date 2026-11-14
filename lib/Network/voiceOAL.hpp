#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file
  Voice over Net objects used together with OpenAL API.
*/

#ifndef _VOICE_OAL_HPP
#define _VOICE_OAL_HPP

#ifdef _WIN32

#include "netTransport.hpp"

#if VOICE_OVER_NET

#include "../soundOAL.hpp"
#include "El/Speech/vonPch.hpp"
#include "El/Speech/vonCodec.hpp"
#include "El/Calendar/eventToCallback.hpp"

//----------------------------------------------------------------------
//  Replayer buffer:

#include "Es/Memory/normalNew.hpp"

/**
  Updates float OAL parameter of SOURCE
  @param SOURCE given OAL source
  @param AL_VAL_ID id of parameter, such as AL_GAIN
  @param VALUE the value the parameter should be set to
  @param MEMBER the member of structVoiceOALStateEAX to set cached value. Should contain also the parent, example: m_voiceState.gain3D)
*/
#define UPDATE_OAL_SOURCE_F(SOURCE, AL_VAL_ID, VALUE, MEMBER) \
  if ( !MEMBER##Cached ) {MEMBER = VALUE; MEMBER##Cached = true; alSourcef(SOURCE, AL_VAL_ID, VALUE); } \
  else if (fabs(MEMBER - VALUE)>MEMBER##Diff * VALUE) { MEMBER = VALUE; alSourcef(SOURCE, AL_VAL_ID, VALUE); }

// memberVariableName, differenceToCache
#define OAL_SOURCE_CACHEABLE(XX) \
  XX(gain2D, 0.01f, TTChan2D) \
  XX(gain3D, 0.01f, TTChan3D) \
  XX(referenceDistance, 0.01f, TTChan3D)

#define DECLARE_OAL_SOURCE_PAR(name, diff, chan) \
  float name; \
  bool name##Cached; \
  const float name##Diff;

#define INIT_OAL_SOURCE_PAR(name, diff, chan) name = 0; name##Cached = false; 
#define INIT_OAL_SOURCE_PAR_CONST(name, diff, chan) , name##Diff(diff)
#define INVALIDATE_CACHE(name, diff, chan) if (chan & voice2D3D) name##Cached = false;

/// Actual OAL and EAX parameters info (occlusion, obstruction, speed, velocity)
struct VoiceOALStateEAX
{
  /// remember time of last buffer position and speed settings
  DWORD parsTime;
  /// remember whether 3D position should be updated
  bool set3DPos;
  /// current position and speed of related player
  Vector3 position, speed;
  /// gain and other AL parameters for caching
  OAL_SOURCE_CACHEABLE(DECLARE_OAL_SOURCE_PAR)

  float volume; // volume used for 3D sound
  float accomodation; // current ear accomodation setting
  //@{ current occlusion/obstruction levels
  float occlusion,obstruction;
  int obstructionSet, occlusionSet;
  //@}
  float current3DGain;
  
  VoiceOALStateEAX() : position(VZero), speed(VZero), parsTime(0), set3DPos(false),
    volume(1.0f), accomodation(1.0f), occlusion(1.0f), obstruction(1.0f), current3DGain(0.0f)
    OAL_SOURCE_CACHEABLE(INIT_OAL_SOURCE_PAR_CONST)
  {
    OAL_SOURCE_CACHEABLE(INIT_OAL_SOURCE_PAR)
  }

  void InvalidateCache(int voice2D3D)
  {
    OAL_SOURCE_CACHEABLE(INVALIDATE_CACHE)
  }
};

/// 3d voice playback
class VoNSoundBufferOAL : public VoNSoundBuffer
{
  friend class VoNSystemOAL;
  
public:

#if _USE_TIMER_QUEUE
  /// Sets associated QueueTimer & VoNReplayer objects but doesn't init the buffer yet.
  VoNSoundBufferOAL ( HANDLE qtimer, VoNReplayer *repl, ALCdevice *device );
#else
  /// Sets associated EventToCallback & VoNReplayer objects but doesn't init the buffer yet.
  VoNSoundBufferOAL ( EventToCallback *etc, VoNReplayer *repl, ALCdevice *device );
#endif

  virtual ~VoNSoundBufferOAL ();

  /// Register buffer's events into "etc".
  virtual void registerCallbacks ();

  /// Unregister buffer's events.
  virtual void unregisterCallbacks ();

  /// Temporary suspend of buffer's events.
  virtual void suspendCallbacks ();

  /// Re-enables the buffer's events.
  virtual void resumeCallbacks ();

  //@{ VoN3DPosition implementation
  virtual void SetPosition ( float x, float y, float z );
  virtual void Set3DPosition ( Vector3Par pos, Vector3Par speed );
  //@}
  void SetListenerPos(Vector3Par pos) { m_listenerPos = pos; }
  const Vector3 &GetListenerPos() const { return m_listenerPos; }

  //@{ EAX implementation
  /// occlusion and obstruction level control
  void SetObstruction(float obstruction, float occlusion, float deltaT);
  void UpdateVolumeAndAccomodation(float volume, float accomodation);
  VoNChatChannel CheckChatChannel(bool &audible2D, bool &audible3D) const;
  void get2D3DPlaying(bool &audible2D, bool &audible3D)
  { 
    audible2D = m_sourceOpen;
    audible3D = m_source3DOpen;
  }
  VoNChannelId GetChannel() 
  { 
    return m_repl->GetChannel(); 
  }

  void UpdateVolume();
  const VoiceOALStateEAX &GetVoiceState() const { return m_voiceState; }
  VoNChatChannel GetChatChannel() const { return m_repl->GetChatChannel(); }
  float CalcMinDistance() const;
  void SetGain(float gain);
  void SetI3DL2Db(int obstruction, int occlusion);
  void DoSetI3DL2Db();
  //@}

  /**
    Creates (allocates) buffer of the required size.
    'format' structure should be filled before!
    @param  len Buffer size in samples.
  */
  virtual void createBuffer ( unsigned len );

  /// Suspends (temporarily stops) the replaying/capturing.
  virtual void suspendBuffer ();

  /// Resumes (restarts) the replaying/capturing.
  virtual void resumeBuffer ();

  /// Get the current buffer position
  /**
  @param free how many buffers are free (starting with write)
  @param write which buffer should we write to
  */
  void GetCurrentBuffers(int &free, int &write);

  /**
    Destroys all buffer memory.
  */
  virtual void destroyBuffer ();

  /**
    Prepares (encodes/decodes) new bunch of data. Returns number of samples & changes "pos".
    Data are leaved unlocked.
  */
  virtual unsigned newData ( unsigned &pos, unsigned minSamples, unsigned maxSamples );

  /**
    Increments the safe-position pointer.
  */
  virtual void advanceSafePosition ( unsigned inc );

  /**
    Locks the given fragment of the buffer.
    Only one lock can be active at the same time.
    Pointers returned in 'ptr' are valid till next 'unlockData()'
    call only.
  */
  virtual void lockData ( unsigned pos, unsigned len, CircularBufferPointers &ptr );

  /**
    Unlocks the buffer.
    Invalidates the previously returned 'ptr' structure.
  */
  virtual void unlockData ( unsigned processed, CircularBufferPointers &ptr );

  /**
    Updates 2D/3D related stuff.
    @param play2D3D combination of TTChan2D, TTChan3D flags
  */
  virtual void update2D3D(int play2D3D);

  /**
    Look into m_ctrlActions and process it if set (stop/start voice control)
  */
  void doCtrlAction();

  /**
  Start/Stop playing - unplug source/buffers.
  @param voice2D3D combination of TTChan2D, TTChan3D flags
  */
  void startVoice(int voice2D3D);
  void stopVoice(int voice2D3D);
  /**
  set Start/Stop playing - schedule the plug/unplug source/buffers.
  @param voice2D3D combination of TTChan2D, TTChan3D flags
  */
  virtual void setStartVoice(int voice2D3D);
  virtual void setStopVoice(int voice2D3D);

  /// Get relative volume to compare voices in order to sort them out
  float GetLoudness(int voice2D3D);


  /// MT-safe new operator.
  static void* operator new ( size_t size );
  /// MT-safe new operator.
  static void* operator new ( size_t size, const char *file, int line );
  /// MT-safe delete operator.
  static void operator delete ( void *mem );

protected:

  /// Actual DirectSound instance.
  ALCdevice *m_device;

#if _USE_TIMER_QUEUE
  /// Actual synchronization thread object.
  HANDLE m_timerQueue;
#else
  /// Actual synchronization thread object.
  EventToCallback *m_etc;
#endif

  /// Associated VoN replayer instance.
  VoNReplayer *m_repl;

  /// Sound-buffer OpenAL object - buffer cycle used for streaming.
  AutoArray<ALuint,MemAllocSafe> m_buffers;
  
  /// Information whether to queue given buffer to m_source or m_source3D or both
  /// (Also chat direct channel can be deduced from this flag ... CCDirect <=> flag & TTDirectChan)
  AutoArray<int,MemAllocSafe> m_buffers2D3D;
  /// The length of buffers in milliseconds (used to make time running when voice is muted)
  int m_activeBufferStartTime;
  AutoArray<int,MemAllocSafe> m_buffersLen;
  
  /// The time of last speech (used to evaluate relative volume to assign/unassign OAL source)
  DWORD m_lastPlayingTime;

  /// Actual OAL and EAX parameters info (occlusion, obstruction, speed, velocity)
  VoiceOALStateEAX m_voiceState;

  /// OpenAL 2D sound source
  ALuint m_source;
  bool m_sourceOpen;
  /// OpenAL 3D sound source
  ALuint m_source3D;
  bool m_source3DOpen;
 
  /// pow2 log of _buffer data size - used to convert from samples to buffers and back
  int m_bufferSize;
  
  /// Is (2D) buffer in 3D mode?
  bool m_mode3D;

  /// Notification event.
  HANDLE m_ev;

  /// Actual D (in samples). Buffer should be 4*d samples long.
  /// waitingRoutine in eventToCallback.cpp has timeout set to 80ms, so this should be more
  unsigned m_d;

  /// Safe read/write ahead-distance. Should be something like 5/4 * D.
  unsigned m_ahead;

  /// Index of buffer to be written (in _buffers array).
  int m_write;
  
  /// How many buffers are ready for writing
  int m_free;

  /// control voice action, stop/start voice
  enum VoNActionCode {VON_BUF_NONE, VON_BUF_START, VON_BUF_STOP};
  struct VoNCtrlAction
  {
    VoNActionCode action;
    int  voice2D3D;
    VoNCtrlAction (VoNActionCode act=VON_BUF_NONE, int von2D3D=0) : action(act), voice2D3D(von2D3D) {}
    ClassIsMovable(VoNCtrlAction);
  };
  AutoArray<VoNCtrlAction,MemAllocSafe> m_ctrlActions;

  /// listener position to estimate relative volume to compare and sort voices
  Vector3 m_listenerPos;

  /// critical section to control access to soundBuffer stop/startVoice interface
  PoCriticalSection m_soundBufferCriticalSection;

  void enterSoundBuffer() { m_soundBufferCriticalSection.enter(); }
  void leaveSoundBuffer() { m_soundBufferCriticalSection.leave(); }

  /// unqueue given number of buffers and mark them as free
  void UnqueueStream(int processed);

  /// Updates Gain of 2D+3D sources using m_buffers2D3D info and current replaying position
  void UpdateGain();

  /// Extended 3D sound initialization (serves to obstruction, occlusion, accomodation, ...)
  void InitEAX();
  
  /// perform offset conversion using _bufferSizeLog
  unsigned BuffersToSamples(int buffers) const {return buffers*m_bufferSize;}

  friend void replayerRoutine ( HANDLE object, void *data );

  // update changes required from main thread
  void UpdateSoundPars();
};

//----------------------------------------------------------------------
//  Capture buffer:

#ifndef _XBOX


class VoNCaptureBufferOAL : public VoNSoundBuffer
{

public:

#if _USE_TIMER_QUEUE
  /// Sets associated EventToCallback & VoNReplayer objects but doesn't init the buffer yet.
  VoNCaptureBufferOAL ( HANDLE qtimer, VoNRecorder *rec );
#else
  /// Sets associated EventToCallback & VoNReplayer objects but doesn't init the buffer yet.
  VoNCaptureBufferOAL ( EventToCallback *etc, VoNRecorder *rec );
#endif

  virtual ~VoNCaptureBufferOAL ();

  /// Register buffer's events into "etc".
  virtual void registerCallbacks ();

  /// Unregister buffer's events.
  virtual void unregisterCallbacks ();

  /// Temporary suspend of buffer's events.
  virtual void suspendCallbacks ();

  /// Re-enables the buffer's events.
  virtual void resumeCallbacks ();

  /**
    Creates (allocates) buffer of the required size.
    'format' structure should be filled before!
    @param  len Buffer size in samples.
  */
  virtual void createBuffer ( unsigned len );

  /// Suspends (temporarily stops) the replaying/capturing.
  virtual void suspendBuffer ();

  /// Resumes (restarts) the replaying/capturing.
  virtual void resumeBuffer ();

  /**
    Destroys all buffer memory.
  */
  virtual void destroyBuffer ();

  /**
    Prepares (encodes/decodes) new bunch of data. Returns number of samples & changes "pos".
    Data are leaved unlocked.
  */
  virtual unsigned newData ( unsigned &pos, unsigned minSamples, unsigned maxSamples );

  /**
    Increments the safe-position pointer.
  */
  virtual void advanceSafePosition ( unsigned inc );

  /**
    Locks the given fragment of the buffer.
    Only one lock can be active at the same time.
    Pointers returned in 'ptr' are valid till next 'unlockData()'
    call only.
  */
  virtual void lockData ( unsigned pos, unsigned len, CircularBufferPointers &ptr );

  /**
    Unlocks the buffer.
    Invalidates the previously returned 'ptr' structure.
  */
  virtual void unlockData ( unsigned processed, CircularBufferPointers &ptr );

  /// MT-safe new operator.
  static void* operator new ( size_t size );
  /// MT-safe new operator.
  static void* operator new ( size_t size, const char *file, int line );
  /// MT-safe delete operator.
  static void operator delete ( void *mem );

protected:

#if _USE_TIMER_QUEUE
  /// Actual synchronization thread object.
  HANDLE m_timerQueue;
#else
  /// Actual synchronization thread object.
  EventToCallback *m_etc;
#endif

  /// Associated VoN replayer instance.
  VoNRecorder *m_rec;

  /// Direct-sound capture instance.
  ALCdevice * m_device;

  /// Temporary buffer for data which are already captured but not encoded yet
  AutoArray<int16,MemAllocSafe> m_data;

  /// how many samples are currently present in m_data
  int m_dataUsed;

  /// Notification event.
  HANDLE m_ev;

  /// Actual D (in samples). Buffer should be 4*d samples long.
  /// waitingRoutine in eventToCallback.cpp has timeout set to 80ms, so this should be more
  unsigned m_d;

  /// Safe read/write ahead-distance. Should be something like 5/4 * D.
  unsigned m_ahead;

  /// Buffer position to be read (in samples).
  unsigned m_nextPtr;

  friend void captureRoutine ( HANDLE object, void *data );

private:
  /// start capturing device, with detecting errors, logging
  void startCapture();
  /// stop capturing device, with detecting errors, logging
  void stopCapture();
};

#endif

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  VoN-system:

#ifndef _XBOX

class VoNSystemOAL : public VoNSystem
{

protected:

  /// Global DirectSound pointer (can be NULL for dedicated-server).
  ALCdevice *m_device;

#if _USE_TIMER_QUEUE
  /// Actual timerQueue object to manage waiting operations
  HANDLE m_timerQueue;
#else
  /// Actual ETC object.
  RefD<EventToCallback> m_etc;
#endif

  /// Set of replayer-buffers.
  ExplicitMap<VoNChannelId,RefD<VoNSoundBufferOAL>,true,MemAllocSafe> m_bufs;

  /// Actual capture-buffer.
  RefD<VoNCaptureBufferOAL> m_capture;

public:

  /// device can be NULL (for dedicated-server).
  VoNSystemOAL (ALCdevice *device);

  virtual ~VoNSystemOAL ();

  /// Stops all client activities (capturing, replaying).
  virtual void stopClient ();

  /// Returns active DS instance.
  virtual ALCdevice *getDevice () { return m_device; }

  /// Application-level response to replayer create/destroy. Should be overriden!
  virtual VoNResult setReplay ( VoNChannelId chId, bool create );

  /// Application-level response to replayer suspend/resume. Should be overriden!
  virtual VoNResult setSuspend ( VoNChannelId chId, bool suspend );

  /// Application-level response to capture on/off (microphone switch). Should be overriden!
  virtual VoNResult setCapture ( bool on, int quality );

  /// Retrieves soud-buffer for the given channel.
  virtual RefD<VoNSoundBuffer> getSoundBuffer ( VoNChannelId chId );

  /// Set position of player (in OAL it affects source)
  virtual void setPosition(int dpnid, Vector3Par pos, Vector3Par speed);

  /// Set obstruction of player (in OAL it affects source)
  virtual void setObstruction(int dpnid, float obstruction,float occlusion,float deltaT);

  /// Set volume and ear accomodation of player
  virtual void updateVolumeAndAccomodation(int dpnid, float volume, float accomodation);

  /// check what channel can we currently hear the player on
  virtual VoNChatChannel checkChatChannel(int dpnid, bool &audible2D, bool &audible3D);
  void get2D3DPlaying(int dpnid, bool &audible2D, bool &audible3D);
  /// fill in the array by dpnid of all players within the maxDist distance using position from their VoNSoundBufferOAL
  void WhatUnitsVoNDirect(AutoArray<int> &units, Vector3Par pos, float maxDist);

  /// Advance all voices (even when muted)
  virtual void advanceAll(float deltaT, bool paused, bool quiet);
};

#endif  // !defined(_XBOX)

#endif  // VOICE_OVER_NET

#endif  // _WIN32

#endif
