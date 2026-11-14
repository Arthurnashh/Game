#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   pcm.hpp
  @brief  PCM codec (reducing 64kbit to 48kbit).

  <p>Copyright &copy; 2002-2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  9.12.2002
  @date   17.7.2003
*/

#ifndef _PCM_H
#define _PCM_H

#include <El/Speech/vonPch.hpp>
#if _ENABLE_VON

#include "El/Speech/vonCodec.hpp"

//----------------------------------------------------------------------
//  PCMCodec:

/// Default frame size in samples.
const unsigned PCM_DEFAULT_FRAME = 500;

#include "Es/Memory/normalNew.hpp"

class PCMCodec : public VoNCodec
{

protected:

  /// Actual frequency in Hz.
  int m_freq;

  /// Bits per sample (must be either 8 or 16).
  int m_bps;

  /// Actual frame size in samples.
  unsigned m_frame;

public:

  PCMCodec ( int freq, int bps, unsigned frame );

  /// MT-safe new operator.
  static void* operator new ( size_t size );
  /// MT-safe new operator.
  static void* operator new ( size_t size, const char *file, int line );
  /// MT-safe delete operator.
  static void operator delete ( void *mem );

  /**
      Retrieves actual codec info-structure.
  */
  virtual void getInfo ( CodecInfo &info );

  /**
      Encodes one batch (independent group) of sound data.
      @param  ptr Pointers to circular sound buffer (raw data
                  to be encoded).
      @param  encoded How many samples should be/were encoded.
      @param  outPtr Output (code) pointer.
      @param  outSize Available/compressed code size in bytes.
      @return Result code: vonOK, vonBadData.
  */
  virtual VoNResult encode ( CircularBufferPointers &ptr, unsigned &encoded,
                             unsigned8 *outPtr, unsigned &outSize, int DC=0, float boost=1.0f );

  virtual void encodeBeep(unsigned8 *outPtr, unsigned &outSize, int frequency, int encSamples, int shift) 
  {
    // Not implemented
  };

  /**
      Estimates maximum frame-code length (of the next frame) in bytes.
  */
  virtual unsigned estFrameCodeLen ();

  virtual unsigned estFrameLen();

  /**
      Decodes one batch of sound data.
      @param  ptr Pointers to circular sound buffer (raw data
                  after decoding).
      @param  decoded How many samples should be/were decoded.
      @param  inPtr Input (code) pointer.
      @param  inSize Available/consumed code size in bytes.
      @return Result code: vonOK, vonBadData.
  */
  virtual VoNResult decode ( CircularBufferPointers &ptr, unsigned &decoded,
                             unsigned8 *inPtr, unsigned &inSize );

  /// Returns true if the given info matches the PCMCodec specification.
  static bool match ( const CodecInfo & );

  /// Saves binary info for the codec (codec can be restored later from it).
  virtual unsigned16 saveInfo ( unsigned8 *data =NULL );

  /// Restores codec from previously saved binary info. Returns NULL if failed.
  static PCMCodec *restore ( VoNSystem *von, unsigned16 size, unsigned8 *data );

};

#include "Es/Memory/debugNew.hpp"

#endif //_ENABLE_VON

#endif
