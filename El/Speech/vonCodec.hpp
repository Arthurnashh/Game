#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   vonCodec.hpp
  @brief  Abstract VoN codec interface.

  Communication between VoN system and individual codec modules.
  <p>Copyright &copy; 2002-2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  24.9.2002
  @date   17.7.2003
*/

#ifndef _VONCODEC_H
#define _VONCODEC_H

#include <El/Speech/vonPch.hpp>
#if _ENABLE_VON

#include "vonApp.hpp"

//----------------------------------------------------------------------
//  VoNCodec API:

class VoNCodec : public RefCountSafe
{

public:

  /**
      Retrieves actual codec info-structure.
  */
  virtual void getInfo ( CodecInfo &info ) =0;

  /**
      Sets start of independent group (check-point, intra-coding).
      Must be used in encoding, some decoders can handle it automatically.
  */
  virtual void groupStart ()
  {}

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
                             unsigned8 *outPtr, unsigned &outSize, int DC=0, float boost=1.0f ) =0;

  virtual void encodeBeep(unsigned8 *outPtr, unsigned &outSize, int frequency, int encSamples, int shift) = 0;

  /**
      Estimates maximum frame-code length (of the next frame) in bytes (encoded).
  */
  virtual unsigned estFrameCodeLen () =0;
  
  /// typical frame size (decoded) - used for granularity
  virtual unsigned estFrameLen() = 0;

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
                             unsigned8 *inPtr, unsigned &inSize ) =0;

  /// Returns true if the given info matches the XYCodec specification.
  // static bool match ( const CodecInfo & );

  /// Saves binary info for the codec (codec can be restored later from it).
  virtual unsigned16 saveInfo ( unsigned8 *data =NULL ) =0;

  /// Restores codec from previously saved binary info. Returns NULL if failed.
  // static XYCodec *restore ( VoNSystem *von, unsigned16 size, unsigned8 *data );

  /// Sets voice-mask (voice distortion mechanism is non-mandatory).
  virtual void setVoiceMask ( const VoiceMask &newMask )
  {}

  /// Sets arbitrary property of the codec (used for sharing COM objects).
  virtual void setProperty ( int id, void *prop )
  {}

  /// Sets the quality of codec (for encoding)
  virtual void SetQuality(int quality)
  {}

  /// VAD (Voice Activiti Detection) - if true then VoNRecorder::SilenceAnalyzer always returns false
  virtual bool VADEnabled() const {return false;}

};

#endif //_ENABLE_VON

#endif
