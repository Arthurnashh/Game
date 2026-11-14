#ifdef _MSC_VER
#  pragma once
#endif

/**
@file   speex.hpp
@brief  voice codec based on CELP
*/
#ifndef _SPEEX_HPP
#define _SPEEX_HPP

#include <El/Speech/vonPch.hpp>
#if _ENABLE_VON

#include "El/Speech/vonCodec.hpp"

//----------------------------------------------------------------------
//  SpeexCodec:

//enable VAD/DTX - if true, VoNRecorder::SilenceAnalyzer always return false
#define SPEEX_ENABLE_VAD_DTX 0

//enable preprocessor
#define SPEEX_USE_PREPROCESOR 0

/// Default frame size in samples.
const unsigned SPEEX_DEFAULT_FRAME = 160;

/// Default Sampling rate in Hz.
const int SPEEX_FS = 8000;

/*!
\patch 5161 Date 5/29/2007 by Bebul
- New: Speex1.2 beta 2 used as VoN codec
*/

#include "Es/Memory/normalNew.hpp"
//#include "Speex-1.0.5/include/speex/speex.h"
#include "speex-1.2/include/speex/speex.h"
#if SPEEX_USE_PREPROCESOR
#include "speex-1.2/include/speex/speex_preprocess.h"
#endif

class SpeexCodec : public VoNCodec
{

protected:

  /// Actual frequency in Hz.
  int m_freq;

  /// Actual frame size in samples.
  unsigned m_frame;

  /// speex codec specific members
  /*Holds bits so they can be read and written to by the Speex routines*/
  SpeexBits m_enc_bits, m_dec_bits;  //unsure whether two sets of bits needed, this way is safe
  /*Holds the state of the encoder*/
  void *m_enc_state;
  void *m_dec_state;
#if SPEEX_USE_PREPROCESOR
  SpeexPreprocessState *m_pre_state;
#endif
  /// VAD if true then VoNRecorder::SilenceAnalyzer always returns false
  bool m_VAD;

public:

  SpeexCodec ( int freq );
  ~SpeexCodec();

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
    unsigned8 *outPtr, unsigned &outSize, int DC=0, float boost=1.0f);

  virtual void encodeBeep(unsigned8 *outPtr, unsigned &outSize, int frequency, int encSamples, int shift);

  /**
  Estimates maximum frame-code length (of the next frame) in bytes (encoded?).
  */
  virtual unsigned estFrameCodeLen ();

  /// typical frame size (decoded) - used for granularity
  virtual unsigned estFrameLen ();


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

  /// Returns true if the given info matches the SpeexCodec specification.
  static bool match ( const CodecInfo & );

  /// Saves binary info for the codec (codec can be restored later from it).
  virtual unsigned16 saveInfo ( unsigned8 *data =NULL );

  /// Restores codec from previously saved binary info. Returns NULL if failed.
  static SpeexCodec *restore ( VoNSystem *von, unsigned16 size, unsigned8 *data );

  /// Sets the quality of codec (for encoding)
  void SetQuality(int quality);

  /// VAD (Voice Activiti Detection) - if true then VoNRecorder::SilenceAnalyzer always returns false
  virtual bool VADEnabled() const {return m_VAD;}
  void SetVADEnabled(bool val) {m_VAD = val;}
};

#include "Es/Memory/debugNew.hpp"

#endif //_ENABLE_VON

#endif
