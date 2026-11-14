#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   voiceXbox.hpp
  @brief  Voice over Net objects using for Xbox SDK.

  Copyright &copy; 2003-2004 by BIStudio (www.bistudio.com)
  @author PE
  @since  4.6.2003
  @date   8.4.2004
*/

#ifndef _VOICE_XBOX_HPP
#define _VOICE_XBOX_HPP


#if VOICE_OVER_NET

#include "El/Speech/vonApp.hpp"

#if _GAMES_FOR_WINDOWS

class VoNSystemXbox : public VoNSystem
{
protected:


public:

  VoNSystemXbox();

  virtual ~VoNSystemXbox ();

  /// Stops all client activities (capturing, replaying).
  virtual void stopClient ();

  /// Changes actual communicator port (defined separately for capture & replay).
  virtual void changePort ( DWORD inPort, DWORD outPort, bool disablePlayback );

  /// Application-level response to replayer create/destroy. Should be overridden!
  virtual VoNResult setReplay ( VoNChannelId chId, bool create );

  /// Application-level response to replayer suspend/resume. Should be overridden!
  virtual VoNResult setSuspend ( VoNChannelId chId, bool suspend );

  /// Application-level response to capture on/off (microphone switch). Should be overridden!
  virtual VoNResult setCapture ( bool on );

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

  /// Retrieves soud-buffer for the given channel.
  virtual RefD<VoNSoundBuffer> getSoundBuffer ( VoNChannelId chId );

  /// Changes actual voice-mask. Should re-initialize all object instances which depend on it..
  virtual void changeVoiceMask ( VoiceMask &mask );

  /// Mixes the given amount of new data from all slave-replayers.
  virtual unsigned mixData ( VoNSoundBuffer *sb, unsigned pos, unsigned minSamples, unsigned maxSamples );
};

#elif defined _XBOX

#include "voiceDX8.hpp"
#include "El/Speech/vonMemory.hpp"

/// Must be defined to share Xbox COM decoder/encoder instances.
#define COM_SHARED

#ifdef COM_SHARED
#  include <xvoice.h>
#endif

//----------------------------------------------------------------------
//  Replayer buffer for Xbox Communicator Headphone:

/// Packet-size in samples (40ms).
const unsigned PACKET_SIZE        = 320;    // 640

/// Number of packets for microphone device (total 160ms).
const unsigned PACKET_NUMBER_MIC  = 4;

/// Number of packets for headphone device (total 320ms).
const unsigned PACKET_NUMBER_HEAD = 8;

/// Headphone timer interval in milliseconds.
const long HEADPHONE_TIMER        = 100L;

#include "Es/Memory/normalNew.hpp"

#if _DEBUG
const unsigned HEADPHONE_MAGIC = 0x8fa61033;
#endif

class VoNSystemXbox;

class VoNSoundBufferHead : public VoNSoundBuffer
{

public:

  /// Sets associated EventToCallback object but doesn't init the buffer yet.
  VoNSoundBufferHead ( EventToCallback *etc, DWORD port, VoNSystemXbox *von );

  virtual ~VoNSoundBufferHead ();

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

#if _DEBUG
  /// Magic number to check DPC routines.
  unsigned m_magic;
#endif

  /// Actual synchronization thread object.
  EventToCallback *m_etc;

  /// Notification semaphore.
  HANDLE m_ev;

  /// Associated VoNSystem (for sound-mixing purpose).
  VoNSystemXbox *m_von;

  /// Headphone object.
  ComRef<XMediaObject> m_headPhone;

  /// Communicator's port.
  DWORD m_port;

  /// Output buffer.
  int16 m_buffer[ PACKET_SIZE * PACKET_NUMBER_HEAD ];

  /// Next packet to be processed (replayed).
  unsigned m_headphonePacket;

  /// Headphone-packet processing status.
  DWORD m_status[PACKET_NUMBER_HEAD];

  /// Internal error (during headPhone->Process). Replaying must be stopped immediately..
  bool m_error;

  friend void CALLBACK dpcHeadphone ( LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus );
  friend void headReplayerRoutine ( HANDLE object, void *data );

};

//----------------------------------------------------------------------
//  Slave replayer buffer:

class VoNSoundBufferSlave : public VoNSoundBuffer
{

public:

  /// Sets associated VoNReplayer object but doesn't init the buffer yet.
  VoNSoundBufferSlave ( VoNReplayer *repl );

  virtual ~VoNSoundBufferSlave ();

  /// Application could set 3D replayer's position using this method.
  virtual void SetPosition ( float x, float y, float z );

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

  /// Associated VoN replayer instance.
  VoNReplayer *m_repl;

  /// Output buffer.
  int16 m_buffer[ PACKET_SIZE * PACKET_NUMBER_HEAD ];

  /// Next packet to be processed (replayed).
  unsigned m_nextPacket;

  /// First free packet (after waiting packets).
  unsigned m_lastPacket;

  friend void headReplayerRoutine ( HANDLE object, void *data );

};

//----------------------------------------------------------------------
//  Capture buffer:

#if _DEBUG
const unsigned MICROPHONE_MAGIC = 0xb088a615;
#endif

class VoNCaptureBufferMic : public VoNSoundBuffer
{

public:

  /// Sets associated EventToCallback & VoNReplayer objects but doesn't init the buffer yet.
  VoNCaptureBufferMic ( EventToCallback *etc, VoNRecorder *rec, DWORD port );

  virtual ~VoNCaptureBufferMic ();

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

  /// Changes actual recorder instance (must be called in suspend mode).
  virtual void setRecorder ( VoNRecorder *rec );

  /// MT-safe new operator.
  static void* operator new ( size_t size );
  /// MT-safe new operator.
  static void* operator new ( size_t size, const char *file, int line );
  /// MT-safe delete operator.
  static void operator delete ( void *mem );

protected:

#if _DEBUG
  /// Magic number to check DPC routines.
  unsigned m_magic;
#endif

  /// Associated VoN recorder instance.
  VoNRecorder *m_rec;

  /// Actual synchronization thread object.
  EventToCallback *m_etc;

  /// Notification semaphore.
  HANDLE m_ev;

  /// Microphone object.
  ComRef<XMediaObject> m_microPhone;

  /// Communicator's port.
  DWORD m_port;

  /// Input buffer.
  int16 m_buffer[ PACKET_SIZE * PACKET_NUMBER_MIC ];

  /// Next packet to be processed (captured).
  unsigned m_micPacket;

  /// Microphone-packet processing status.
  DWORD m_status[PACKET_NUMBER_MIC];

  /// Internal error (during microPhone->Process). Capturing must be stopped immediately..
  bool m_error;

  friend void CALLBACK dpcMicrophone ( LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus );
  friend void micCaptureRoutine ( HANDLE object, void *data );

};

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  VoN-system for Xbox:

/// Number of valid communicator ports.
const DWORD COMMUNICATOR_PORTS = 4;

class VoNSystemXbox : public VoNSystem
{

protected:

  /// Global DirectSound pointer.
  ComRef<IMySound> m_ds;

#ifdef COM_SHARED

  /// Shared voice encoder (has to be allocated in main thread).
  ComRef<IXVoiceEncoder> m_encoder;

  /// Shared voice decoder (has to be allocated in main thread).
  ComRef<IXVoiceDecoder> m_decoder;

#endif

  /// Communicator's port (microphone part).
  DWORD m_inPort;

  /// Communicator's port (headphone part).
  DWORD m_outPort;

  /// Forced replayer's port (used by voice mail replayer).
  bool m_forcedPort;

  /// Actual ETC object.
  RefD<EventToCallback> m_etc;

  /// Set of replayer-buffers.
  ExplicitMap<VoNChannelId,RefD<VoNSoundBuffer>,true,MemAllocSafe> m_bufs;

  /// Master replayer-buffer for the communicator's headphone.
  RefD<VoNSoundBufferHead> m_master;

  /// Actual capture-buffer.
  RefD<VoNCaptureBufferMic> m_capture;

  /// Network-based voice recorder (set in first setCapture() call).
  RefD<VoNRecorder> m_recorder;

  /// Memory-based voice recorder (used only in startRecorder() - stopRecorder() period).
  RefD<VoNMemoryRecorder> m_memRecorder;

  /// Was microphone suspended before the startRecorder() call?
  bool m_micSuspended;

  /// Sound buffer for memory-based replayer.
  RefD<VoNSoundBuffer> m_memBuffer;

  /// Memory-based replayer.
  RefD<VoNMemoryReplayer> m_memReplayer;

public:

  /// Create replayers using Communicator headphone.
  bool _useHeadphone;
  /// playback should be muted
  bool _disablePlayback;

  VoNSystemXbox ( IMySound *dSound, DWORD inPort =0, DWORD outPort =0 );

  virtual ~VoNSystemXbox ();

  /// Stops all client activities (capturing, replaying).
  virtual void stopClient ();

  /// Changes actual communicator port (defined separately for capture & replay).
  virtual void changePort ( DWORD inPort, DWORD outPort, bool disablePlayback );

  /// Application-level response to replayer create/destroy. Should be overridden!
  virtual VoNResult setReplay ( VoNChannelId chId, bool create );

  /// Application-level response to replayer suspend/resume. Should be overridden!
  virtual VoNResult setSuspend ( VoNChannelId chId, bool suspend );

  /// Application-level response to capture on/off (microphone switch). Should be overridden!
  virtual VoNResult setCapture ( bool on );

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

  /// Changes actual voice-mask. Should re-initialize all object instances which depend on it..
  virtual void changeVoiceMask ( VoiceMask &mask );

  /// Mixes the given amount of new data from all slave-replayers.
  virtual unsigned mixData ( VoNSoundBuffer *sb, unsigned pos, unsigned minSamples, unsigned maxSamples );

protected:

  /**
    [Re-]create new master headphone replayer buffer.
    The old master-buffer is destroyed automatically..
  */
  void createMaster ();

  /// Create DS-replayer buffer, prepare it for operation.
  void createDSBuffer ( VoNChannelId chId, bool suspended );
  
  /// Create dummy buffer, which eats data but does not play them
  void createNilBuffer ( VoNChannelId chId, bool suspended );

  /// Create headphone slave-replayer buffer, prepare it for operation.
  void createHeadBuffer ( VoNChannelId chId, bool suspended );

};

#endif // defined(_XBOX)

#endif // VOICE_OVER_NET

#endif // ! defined _VOICE_XBOX_HPP
