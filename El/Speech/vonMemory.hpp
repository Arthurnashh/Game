#ifdef _MSC_VER
#  pragma once
#endif

/**
 *  @file   vonMemory.hpp
 *  @brief  Standalone voice codec (works w/o network).
 *
 *  <p>Copyright &copy; 2004 by BIStudio (www.bistudio.com)
 *  @author PE
 *  @since  31.3.2004
 *  @date   8.4.2004
 */

#ifndef _VONMEMORY_H
#define _VONMEMORY_H

#include "vonPch.hpp"

//----------------------------------------------------------------------
//  Memory-targeted recorder:

#include "Es/Memory/normalNew.hpp"

/// Initial encoded buffer size in bytes..
const int INIT_BUFFER_SIZE = 1024;

/// Magic first byte of an encoded voice stream (for future non-compatible versions).
const int VOICE_MAGIC = 12;

/**
 *  Sound recording peer (on the recording i.e. capturing side).
 *  VoNSystem creates this object, application calls it to encode
 *  recorded audio samples & store them in memory.
 */
class VoNMemoryRecorder : public VoNRecorder
{

protected:

  /// Store only original binary data from the codec (no headers).
  bool m_raw;

  /**
   *  Buffer holding recorded and compressed data.
   *  Size() and MaxSize() are used instead of external pointers..
   */
  AutoArray<char,MemAllocSafe> m_buf;

  /// Total number of encoded samples.
  unsigned m_samples;

public:

  VoNMemoryRecorder ( VoNCodec *codec, bool raw =true );

  virtual ~VoNMemoryRecorder ();

  /// MT-safe new operator.
  static void* operator new ( size_t size );
  /// MT-safe new operator.
  static void* operator new ( size_t size, const char *file, int line );
  /// MT-safe delete operator.
  static void operator delete ( void *mem );

  /**
   *  Encodes another batch of recorded (captured) sound data.
   *   @param  sb Sound-buffer containing input data.
   *   @param  pos Start position for encoding.
   *   @param  minEnc Minimum data amount being encoded (in number of samples).
   *   @param  maxEnc Maximum data amount being encoded (in number of samples).
   *   @return Actually encoded number of samples (0 implies an error or vonBreak state).
   */
  virtual unsigned encode ( VoNSoundBuffer *sb,
                            unsigned pos, unsigned minEnc, unsigned maxEnc );

  /**
   *  Returns recorded and encoded data in char[] buffer.
   *  Buffer object is shared and must not be modified outside of VoNMemoryRecorder.
   */
  virtual const AutoArray<char,MemAllocSafe> &getData ();

  /// Returns number of encoded samples.
  virtual unsigned getSamples ()
  { return m_samples; }

};

//----------------------------------------------------------------------
//  Memory-targeted replayer:

/**
 *  Sound replaying peer (on the replaying side).
 *  VoNSystem creates this object, application calls it to fetch
 *  recieved audio samples.
 */
class VoNMemoryReplayer : public VoNReplayer
{

protected:

  /// Store only original binary data from the codec (no headers).
  bool m_raw;

  /**
   *  Buffer holding compressed voice data.
   *  Size() and MaxSize() are used instead of external pointers..
   */
  AutoArray<char,MemAllocSafe> m_buf;

  /// Number of voice samples.
  unsigned m_samples;

  /// Sample pointer.
  unsigned m_samplePtr;

  /// Code pointer (points into m_buf array).
  unsigned m_codePtr;

public:

  VoNMemoryReplayer ( VoNCodec *codec, bool raw =true );

  virtual ~VoNMemoryReplayer ();

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

  /**
   *  Sets encoded data. Data must be encoded by VoNMemoryRecorder (or binary compatible).
   */
  virtual void setData ( const AutoArray<char,MemAllocSafe> &buf, int duration =0 );

  /// Returns replayer status (VonBadData, VonBusy or VonOK).
  virtual VoNResult getStatus ();

};

#include "Es/Memory/debugNew.hpp"

#endif
