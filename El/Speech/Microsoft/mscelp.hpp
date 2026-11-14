#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   mscelp.hpp
  @brief  Microsoft's CELP (?) codec (3.6 kbit).

  <p>Copyright &copy; 2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  4.6.2003
  @date   17.7.2003
*/

#ifndef _MSCELP_H
#define _MSCELP_H

#include "El/Speech/vonCodec.hpp"

#if defined _XBOX && !(_XBOX_VER>=2)

#include <xvoice.h>

//----------------------------------------------------------------------
//  MSCELPCodec:

/// Default frame size in samples.
const unsigned MSCELP_DEFAULT_FRAME = 320;    // 640

/// Default frame-code size in bytes.
const unsigned MSCELP_DEFAULT_FRAME_CODE = 18; // 34

#include "Es/Memory/normalNew.hpp"

class MSCELPCodec : public VoNCodec
{

protected:

  /// Actual frame length in bytes.
  unsigned m_frameLen;

  /// Constant code-length of one frame.
  unsigned m_frameCodeLen;

  /// Encoder instance.
  ComRef<IXVoiceEncoder> m_encoder;

  /// Decoder instance.
  ComRef<IXVoiceDecoder> m_decoder;

  /// Asserts (creates if necessary) the COM encoder instance.
  void assertEncoder ();

  /// Asserts (creates if necessary) the COM decoder object.
  void assertDecoder ();

public:

  MSCELPCodec ( unsigned frameLen );

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
                             unsigned8 *outPtr, unsigned &outSize, int DC=0 );

  virtual void encodeBeep(unsigned8 *outPtr, unsigned &outSize, int frequency, int encSamples, int shift) 
  {
    // Not implemented
  };

  /**
      Estimates maximum frame-code length (of the next frame) in bytes.
  */
  virtual unsigned estFrameCodeLen ();

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
  static MSCELPCodec *restore ( VoNSystem *von, unsigned16 size, unsigned8 *data );

  /// Sets voice-mask (voice distortion mechanism is non-mandatory).
  virtual void setVoiceMask ( const VoiceMask &newMask );

  /**
   * Sets arbitrary property of the codec (used for sharing COM objects: IXVoiceEncoder, IXVoiceDecoder).
   * @param  id   0 for IXVoiceEncoder, 1 for IXVoiceDecoder
   * @param  prop IXVoiceEncoder* or IXVoiceDecoder*
   */
  virtual void setProperty ( int id, void *prop );

};

#include "Es/Memory/debugNew.hpp"

#endif  // _XBOX

#endif
