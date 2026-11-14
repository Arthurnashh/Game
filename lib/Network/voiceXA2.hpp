#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file
  Voice over Net objects used together with XAudio2 API and Windows Multimedia SDK functions. XA2 is used for 
  managing audio playing (audio output). For voice capturing are used function from WM SDK, function waveIn*.
*/

#ifndef _VOICE_XA2_HPP
#define _VOICE_XA2_HPP

#ifdef _WIN32

#include "netTransport.hpp"

#if VOICE_OVER_NET

#include "El/Speech/vonPch.hpp"
#include "El/Speech/vonCodec.hpp"
#include "../soundXA2.hpp"
#include "El/Calendar/eventToCallback.hpp"

//----------------------------------------------------------------------
//  Replayer buffer:

#include "Es/Memory/normalNew.hpp"

#if _ENABLE_REPORT
#define CHECK_XA2_VOICE_ERROR(result, name) \
  if (result != S_OK) \
{ \
  LogF("%s: XAudio2 error %x", name, result); \
}
#else
#define CHECK_XA2_VOICE_ERROR(result, name)
#endif

// memberVariableName, differenceToCache
#define XA2_SOURCE_CACHEABLE(XX) \
  XX(gain2D, 0.01f, TTChan2D) \
  XX(gain3D, 0.01f, TTChan3D)

#define DECLARE_XA2_SOURCE_PAR(name, diff, chan) \
  float name; \
  bool name##Cached; \
  const float name##Diff;

#define INIT_XA2_SOURCE_PAR(name, diff, chan) name = 0; name##Cached = false; 
#define INIT_XA2_SOURCE_PAR_CONST(name, diff, chan) , name##Diff(diff)
#define INVALIDATE_CACHE(name, diff, chan) if (chan & voice2D3D) name##Cached = false;

/// Actual EAX parameters info (occlusion, obstruction, speed, velocity)
struct VoiceXA2StateEAX
{
  /// remember time of last buffer position and speed settings
  DWORD parsTime;

  bool set3DPos;
  /// current position and speed of related player
  Vector3 position, speed;
  /// gain and other AL parameters for caching
  XA2_SOURCE_CACHEABLE(DECLARE_XA2_SOURCE_PAR)

  float volume; // volume used for 3D sound
  float accomodation; // current ear accomodation setting
  //@{ current occlusion/obstruction levels
  float occlusion,obstruction;
  int obstructionSet, occlusionSet;
  //@}
  float current3DGain;
  
  VoiceXA2StateEAX() : position(VZero), speed(VZero), parsTime(0),
    volume(1.0f), accomodation(1.0f), occlusion(1.0f), obstruction(1.0f), current3DGain(0.0f)
    XA2_SOURCE_CACHEABLE(INIT_XA2_SOURCE_PAR_CONST)
  {
    XA2_SOURCE_CACHEABLE(INIT_XA2_SOURCE_PAR)
  }

  void InvalidateCache(int voice2D3D)
  {
    XA2_SOURCE_CACHEABLE(INVALIDATE_CACHE)
  }
};

struct XA2DescBuffer : public RefCountSafe, public XAUDIO2_BUFFER
{
};

class PlayerVoice : public RefCountSafe
{
public:
  bool _playing;

  /// when voice is playing, it sends its data from _sourceVoice to the Submix
  virtual IXAudio2SubmixVoice * Submix() = 0;
  virtual bool   Is2D() const = 0;

  bool  Playing() const { return _playing; }

  PlayerVoice() : _playing(false) {}
};

/// XAudio2SourceVoice just sent to XAudio2MasteringVoice (for other than Directional/Positional speech)
class PlayerVoice2D : public PlayerVoice
{
  IXAudio2SubmixVoice *_submix;
public:
  PlayerVoice2D(IXAudio2 *device);

  virtual bool Is2D() const { return true; }
  virtual IXAudio2SubmixVoice * Submix() {return _submix;}

  void UpdateGain(int flag2D3D);
public:
  /// encapsulates gain,position,speed,obstruction,occlusion etc.
  VoiceXA2StateEAX voiceState;
};

class PlayerVoice3D : public PlayerVoice
{
protected:
  Vector3 _position;
  Vector3 _speed;
  IXAudio2SubmixVoice *_submix;
  /// Actual EAX parameters info (occlusion, obstruction, speed, velocity)
public:
  PlayerVoice3D(float x, float y, float z, IXAudio2 *device);

  virtual bool Is2D() const { return false; }
  virtual IXAudio2SubmixVoice * Submix() {return _submix;}

  void Calculate3DAudio();
  virtual void UpdateSoundPars(int flag2D3D);
  void SetGain(float gain, int flag2D3D);
  void UpdateGain(int flag2D3D);
  void UpdateVolume(int flag2D3D);
  float CalcMinDistance() const;
  
  Vector3 Position() const { return _position; };

  //@{ Set functions used from Main Thread (NetworkManager propagated through layers into the VoN)
  void Set3DPosition(Vector3Par pos, Vector3Par speed);
  void SetObstruction(float obstruction, float occlusion, float deltaT);
  void UpdateVolumeAndAccomodation(float volume, float accomodation);
  //@}
public:
  /// encapsulates gain,position,speed,obstruction,occlusion etc.
  VoiceXA2StateEAX voiceState;
};

class TransmitterVoice : public PlayerVoice3D
{
  typedef PlayerVoice3D base;
  DWORD _nextUpdateTime;
public:
  float _quality;
  TransmitterVoice(float x, float y, float z, float range, IXAudio2 *device);
  void UpdateSoundPars(int flag2D3D) override;
};

/// 3d voice playback
class VoNSoundBufferXA2 : public VoNSoundBuffer
{
  friend class VoNSystemXA2;
  
public:

  RefArray<PlayerVoice, MemAllocSafe> debugVoices;

#if _USE_TIMER_QUEUE
  VoNSoundBufferXA2::VoNSoundBufferXA2(HANDLE qtimer, VoNReplayer *repl, IXAudio2 *device);
#else
  /// Sets associated EventToCallback & VoNReplayer objects but doesn't init the buffer yet.
  VoNSoundBufferXA2 (EventToCallback *etc, VoNReplayer *repl, IXAudio2 *device);
#endif

  virtual ~VoNSoundBufferXA2 ();

  void UpdateSoundPars();

  /// Register buffer's events into "etc".
  virtual void registerCallbacks ();

  /// Unregister buffer's events.
  virtual void unregisterCallbacks ();

  /// Temporary suspend of buffer's events.
  virtual void suspendCallbacks ();

  /// Re-enables the buffer's events.
  virtual void resumeCallbacks ();

  //@{ 3D interface
  virtual void Set3DPosition (Vector3Par pos, Vector3Par speed);
  void Calculate3DAudio();
  //@}
  void SetListenerPos(Vector3Par pos) { _listenerPos = pos; }
  const Vector3 &GetListenerPos() const { return _listenerPos; }

  //@{ EAX implementation
  /// occlusion and obstruction level control
  void SetObstruction(float obstruction, float occlusion, float deltaT);
  void UpdateVolumeAndAccomodation(float volume, float accomodation);
  VoNChatChannel CheckChatChannel(bool &audible2D, bool &audible3D) const;

  void get2D3DPlaying(bool &audible2D, bool &audible3D);

  VoNChannelId GetChannel() 
  { 
    return _repl->GetChannel(); 
  }

  VoNChatChannel GetChatChannel() const { return _repl->GetChatChannel(); }
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

  /// get number of currently queued buffers
  int GetQueuedBuffersCount();

  /// returns if audio buffers are ready
  bool AreBuffersReady();

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
    Look into _playVoices and process it if set (stop/start voice control)
  */
  void doCtrlAction();

  const VoiceXA2StateEAX *GetVoiceState()
  {
    if (_playerVoice3D) return &_playerVoice3D->voiceState;
    return NULL;
  }
  float PlayingVolumeCoef();

  /**
  Start/Stop playing - unplug source/buffer.
  */
  void startSource();
  void stopSource();
  /**
  set Start/Continue playing - schedule the plug/unplug source/buffers.
  */
  void setStartVoice(PlayerVoice *voice);
  void clearVoices();
  void switchVoices();

  /// Get relative volume to compare voices in order to sort them out
  float GetLoudness(int voice2D3D);

  /// MT-safe new operator.
  static void* operator new ( size_t size );
  /// MT-safe new operator.
  static void* operator new ( size_t size, const char *file, int line );
  /// MT-safe delete operator.
  static void operator delete ( void *mem );

protected:

#if _USE_TIMER_QUEUE
  /// Actual synchronization thread object.
  HANDLE _timerQueue;
#else
  /// Actual synchronization thread object.
  EventToCallback *_etc;
#endif
  
  /// Associated VoN replayer instance.
  VoNReplayer *_repl;

  /// Sound-buffer, buffer cycle used for streaming.
  AutoArray< Ref<XA2Buffer>, MemAllocSafe> _buffers;
  AutoArray< Ref<XA2DescBuffer>, MemAllocSafe> _buffersDescr;

  /// Information whether to queue given buffer to m_source or m_source3D or both
  /// (Also chat direct channel can be deduced from this flag ... CCDirect <=> flag & TTDirectChan)
  AutoArray<int,MemAllocSafe> _buffers2D3D;
  /// The length of buffers in milliseconds (used to make time running when voice is muted)
  int m_activeBufferStartTime;
  AutoArray<int,MemAllocSafe> _buffersLen;
  
  /// The time of last speech (used to evaluate relative volume to assign/unassign source)
  DWORD _lastPlayingTime;

  // IXAudio2
  IXAudio2 *_device;

  //@{ XAudio2 Voice Manager
  // only single sourceVoice for all (2D,3D voices are mastered using voice submixes) Only this sourceVoice gets data from buffer
  IXAudio2SourceVoice *_sourceVoice;
  bool _sourceVoiceOpen;
  int  _sourceVoiceQueued;
  // each sound buffer could be connected to two submixes: player directSpeech = _playerVoice3D or/and 2D audio (like global channel)
  RefD<PlayerVoice2D> _playerVoice2D;
  RefD<PlayerVoice3D> _playerVoice3D;
  // the list of voices selected to be played (contain _playerVoice2D, _playerVoice3D when these are to be played + possible ItemTransmitter voices)
  RefArray<PlayerVoice, MemAllocSafe> _playVoices[2];
  int _playVoicesIX;  //index used to ADD voices in
  //@}

  /// pow2 log of _buffer data size - used to convert from samples to buffers and back
  int _bufferSize;
  
  /// Is (2D) buffer in 3D mode?
  //bool m_mode3D;
  bool _mode3D;

  /// Notification event.
  HANDLE _ev;

  /// Actual D (in samples). Buffer should be 4*d samples long.
  /// waitingRoutine in eventToCallback.cpp has timeout set to 80ms, so this should be more
  unsigned _d;

  /// Safe read/write ahead-distance. Should be something like 5/4 * D.
  unsigned _ahead;

  /// Index of buffer to be written (in _buffers array).
  int _write;
  
  /// How many buffers are ready for writing
  int _free;

  /// listener position to estimate relative volume to compare and sort voices
  Vector3 _listenerPos;

  /// critical section to control access to soundBuffer stop/startVoice interface
  PoCriticalSection _soundBufferCriticalSection;

  void enterSoundBuffer() { _soundBufferCriticalSection.enter(); }
  void leaveSoundBuffer() { _soundBufferCriticalSection.leave(); }

  /// unqueue given number of buffers and mark them as free
  void UnqueueStream(int processed);

  /// perform offset conversion using _bufferSizeLog
  unsigned BuffersToSamples(int buffers) const {return buffers * _bufferSize;}

  friend void replayerRoutine ( HANDLE object, void *data );
};

//----------------------------------------------------------------------
//  Capture buffer:


#include "../WaveInput.h"

class VoNCaptureBufferXA2 : public VoNSoundBuffer
{
public:
#if _USE_TIMER_QUEUE
  VoNCaptureBufferXA2 ( HANDLE qtimer, VoNRecorder *rec );
#else
  /// Sets associated EventToCallback & VoNReplayer objects but doesn't init the buffer yet.
  VoNCaptureBufferXA2 ( EventToCallback *etc, VoNRecorder *rec );
#endif

  virtual ~VoNCaptureBufferXA2 ();

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

  // capturing
  WaveCapture _wCapture;

  /// Temporary buffer for data which are already captured but not encoded yet
  AutoArray<int16, MemAllocSafe> m_data;

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

//////////////////////////////////////////////////////////////////////////

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  VoN-system:

class VoNSystemXA2 : public VoNSystem
{
protected:
  // XAudio2 device
  IXAudio2 *_device;

#if _USE_TIMER_QUEUE
  /// Actual timerQueue object to manage waiting operations
  HANDLE m_timerQueue;
#else
  /// Actual ETC object.
  RefD<EventToCallback> m_etc;
#endif

  /// Set of replayer-buffers.
  ExplicitMap<VoNChannelId, RefD<VoNSoundBufferXA2>, true, MemAllocSafe> _bufs;

  /// Actual capture-buffer.
  RefD<VoNCaptureBufferXA2> _capture;

public:
  VoNSystemXA2(IXAudio2 *device);

  virtual ~VoNSystemXA2 ();

  /// Stops all client activities (capturing, replaying).
  virtual void stopClient ();

  /// Returns active DS instance.
  //virtual ALCdevice *getDevice () { return m_device; }

  virtual IXAudio2 *getDevice() { return _device; }

  /// Application-level response to replayer create/destroy. Should be overriden!
  virtual VoNResult setReplay ( VoNChannelId chId, bool create );

  /// Application-level response to replayer suspend/resume. Should be overriden!
  virtual VoNResult setSuspend ( VoNChannelId chId, bool suspend );

  /// Application-level response to capture on/off (microphone switch). Should be overriden!
  virtual VoNResult setCapture ( bool on, int quality );

  /// Retrieves sonud-buffer for the given channel.
  virtual RefD<VoNSoundBuffer> getSoundBuffer ( VoNChannelId chId );

  /// Set position of player (affects source)
  virtual void setPosition(int dpnid, Vector3Par pos, Vector3Par speed);

  virtual void WhatUnitsVoNDirect(AutoArray<int> &units, Vector3Par pos, float maxDist);

  /// Set obstruction of player (it affects source)
  virtual void setObstruction(int dpnid, float obstruction,float occlusion,float deltaT);

  /// Set volume and ear accomodation of player
  virtual void updateVolumeAndAccomodation(int dpnid, float volume, float accomodation);

  /// check what channel can we currently hear the player on
  virtual VoNChatChannel checkChatChannel(int dpnid, bool &audible2D, bool &audible3D);
  
  /// Advance all voices (even when muted)
  virtual void advanceAll(float deltaT, bool paused, bool quiet);

  virtual void get2D3DPlaying(int dpnid, bool &audible2D, bool &audible3D);
};


#endif  // VOICE_OVER_NET

#endif  // _WIN32

#endif
