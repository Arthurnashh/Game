#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file
  Voice over Net objects used together with DirectSound API.
*/

#ifndef _VOICE_DX8_HPP
#define _VOICE_DX8_HPP

#ifdef _WIN32

#include "netTransport.hpp"

#if VOICE_OVER_NET

#include "../soundDX8.hpp"
#include "El/Speech/vonPch.hpp"
#include "El/Calendar/eventToCallback.hpp"

//----------------------------------------------------------------------
//  Replayer buffer:

#include "Es/Memory/normalNew.hpp"

/// 3d voice playback
class VoNSoundBufferDS : public VoNSoundBuffer, public VoN3DPosition
{

public:

  /// Sets associated EventToCallback & VoNReplayer objects but doesn't init the buffer yet.
  VoNSoundBufferDS ( EventToCallback *etc, VoNReplayer *repl, IMySound *dSound );

  virtual ~VoNSoundBufferDS ();

  /// Register buffer's events into "etc".
  virtual void registerCallbacks ();

  /// Unregister buffer's events.
  virtual void unregisterCallbacks ();

  /// Temporary suspend of buffer's events.
  virtual void suspendCallbacks ();

  /// Re-enables the buffer's events.
  virtual void resumeCallbacks ();

  //@{ VoN3DPosition implementation
  virtual VoN3DPosition *get3DInterface (){return this;}
  virtual void SetPosition ( float x, float y, float z );
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
  HRESULT GetCurrentPosition(LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor) { return m_buffer->GetCurrentPosition(pdwCurrentPlayCursor, pdwCurrentWriteCursor); }

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

  /// Actual DirectSound instance.
  IMySound *m_ds;

  /// Actual synchronization thread object.
  EventToCallback *m_etc;

  /// Associated VoN replayer instance.
  VoNReplayer *m_repl;

  /// Sound-buffer DS object.
  ComRef<IMySoundBuffer> m_buffer;

  /// 3D sound-buffer DS object.
  ComRef<IMySound3DBuffer> m_buffer3D;

  /// Is buffer in 3D mode?
  bool m_mode3D;

  /// Notification event.
  HANDLE m_ev;

  /// Actual D (in samples). Buffer should be 4*d samples long.
  /// waitingRoutine in eventToCallback.cpp has timeout set to 80ms, so this should be more
  unsigned m_d;

  /// Safe read/write ahead-distance. Should be something like 5/4 * D.
  unsigned m_ahead;

  /// Buffer position to be written (in samples).
  unsigned m_nextPtr;

  /// Temporary store for locked buffer pointers.
  void *m_lockPtr1, *m_lockPtr2;

  /// Temporary store for locked buffer lengths (unit = BYTE!).
  DWORD m_lockLen1, m_lockLen2;

  friend void replayerRoutine ( HANDLE object, void *data );

};

/// voice discarder
class VoNSoundBufferNil: public VoNSoundBuffer
{

public:

  /// Sets associated EventToCallback object but doesn't init the buffer yet.
  VoNSoundBufferNil (EventToCallback *etc, VoNReplayer *repl);

  virtual ~VoNSoundBufferNil ();

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

  /// Actual synchronization thread object.
  EventToCallback *m_etc;

  /// Associated VoN replayer instance.
  VoNReplayer *m_repl;
  
  /// Notification event.
  HANDLE m_ev;

  /// we need to provide some memory when lockData is called
  void *m_data;
};

//----------------------------------------------------------------------
//  Capture buffer:

#if !defined _XBOX && !_GAMES_FOR_WINDOWS


class VoNCaptureBufferDS : public VoNSoundBuffer
{

public:

  /// Sets associated EventToCallback & VoNReplayer objects but doesn't init the buffer yet.
  VoNCaptureBufferDS ( EventToCallback *etc, VoNRecorder *rec );

  virtual ~VoNCaptureBufferDS ();

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

  /// Actual synchronization thread object.
  EventToCallback *m_etc;

  /// Associated VoN replayer instance.
  VoNRecorder *m_rec;

  /// Direct-sound capture instance.
  ComRef<IDirectSoundCapture> m_capture;

  /// DS capture-buffer.
  ComRef<IDirectSoundCaptureBuffer> m_buffer;

  /// Notification event.
  HANDLE m_ev;

  /// Actual D (in samples). Buffer should be 4*d samples long.
  /// waitingRoutine in eventToCallback.cpp has timeout set to 80ms, so this should be more
  unsigned m_d;

  /// Safe read/write ahead-distance. Should be something like 5/4 * D.
  unsigned m_ahead;

  /// Buffer position to be read (in samples).
  unsigned m_nextPtr;

  /// Temporary store for locked buffer pointers.
  void *m_lockPtr1, *m_lockPtr2;

  /// Temporary store for locked buffer lengths (unit = BYTE!).
  DWORD m_lockLen1, m_lockLen2;

  friend void captureRoutine ( HANDLE object, void *data );

  };

#endif

#include "Es/Memory/debugNew.hpp"

//----------------------------------------------------------------------
//  VoN-system:

#if !defined _XBOX && !_GAMES_FOR_WINDOWS

class VoNSystemDS : public VoNSystem
{

protected:

  /// Global DirectSound pointer (can be NULL for dedicated-server).
  ComRef<IMySound> m_ds;

  /// Actual ETC object.
  RefD<EventToCallback> m_etc;

  /// Set of replayer-buffers.
  ExplicitMap<VoNChannelId,RefD<VoNSoundBufferDS>,true,MemAllocSafe> m_bufs;

  /// Actual capture-buffer.
  RefD<VoNCaptureBufferDS> m_capture;

public:

  /// dSound can be NULL (for dedicated-server).
  VoNSystemDS ( IMySound *dSound );

  virtual ~VoNSystemDS ();

  /// Stops all client activities (capturing, replaying).
  virtual void stopClient ();

  /// Application-level response to replayer create/destroy. Should be overriden!
  virtual VoNResult setReplay ( VoNChannelId chId, bool create );

  /// Application-level response to replayer suspend/resume. Should be overriden!
  virtual VoNResult setSuspend ( VoNChannelId chId, bool suspend );

  /// Application-level response to capture on/off (microphone switch). Should be overriden!
  virtual VoNResult setCapture ( bool on, int quality );

  /// Retrieves soud-buffer for the given channel.
  virtual RefD<VoNSoundBuffer> getSoundBuffer ( VoNChannelId chId );

  };

#endif  // !defined(_XBOX)

#endif  // VOICE_OVER_NET

#endif  // _WIN32

#endif
