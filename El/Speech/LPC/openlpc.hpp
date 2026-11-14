#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   openlpc.hpp
  @brief  Low bitrate LPC codec (based on public domain implementation of Ron Frederick).

  <p>Copyright &copy; 2002-2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  9.12.2002
  @date   17.7.2003
*/

#ifndef _OPENLPC_H
#define _OPENLPC_H

#include <El/Speech/vonPch.hpp>
#if _ENABLE_VON

#include "El/Speech/vonCodec.hpp"

//----------------------------------------------------------------------
//  Support:

/// Default frame size in samples.
const int OPENLPC_DEFAULT_FRAME = 250;

/// Default frame-code length in bits (excluding 2 bytes).
const int OPENLPC_DEFAULT_LPC_BITS = 40;

/// Linear prediction filter order.
const int OPENLPC_FILT_ORDER = 10;

/// Sampling rate in Hz.
const int OPENLPC_FS = 8000;

/// Max analysis window length.
const int OPENLPC_MAX_WINDOW = 1000;

struct OpenLPCEState
{
  float   s[OPENLPC_MAX_WINDOW], y[OPENLPC_MAX_WINDOW], h[OPENLPC_MAX_WINDOW];
  int     buflen;
  float   xv1[3], yv1[3], 
          xv2[2], yv2[2], 
          xv3[1], yv3[3], 
          xv4[2], yv4[2];
  float   w[OPENLPC_MAX_WINDOW], r[OPENLPC_FILT_ORDER + 1];
};

struct OpenLPCDState
{
  float Oldper, OldG, Oldk[OPENLPC_FILT_ORDER + 1];
  float bp[OPENLPC_FILT_ORDER + 1];
  float exc;
  int pitchctr, buflen;
};

//----------------------------------------------------------------------
//  OpenLPCCodec:

#include "Es/Memory/normalNew.hpp"

class OpenLPCCodec : public VoNCodec
{

protected:

  /// Bits per sample (must be either 8 or 16).
  int m_bps;

  /// Frame length in samples.
  int m_framelen;

  /// LPC code bits (32 to 80 bits).
  int m_lpcbits;

  /// Actual encoding state.
  OpenLPCEState m_est;

  /// Actual decoding state.
  OpenLPCDState m_dst;

  /// A new group (check-point) will be started.
  bool m_newGroup;

  int m_parambits[OPENLPC_FILT_ORDER];
  int m_sizeofparm;
  float m_logmaxminper;

public:

  OpenLPCCodec ( int bps, int framelen, int lpcbits );

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
      Sets start of independent group (check-point, intra-coding).
  */
  virtual void groupStart ();

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

  /// Returns true if the given info matches the OpenLPCCodec specification.
  static bool match ( const CodecInfo & );

  /// Saves binary info for the codec (codec can be restored later from it).
  virtual unsigned16 saveInfo ( unsigned8 *data =NULL );

  /// Restores codec from previously saved binary info. Returns NULL if failed.
  static OpenLPCCodec *restore ( VoNSystem *von, unsigned16 size, unsigned8 *data );

};

#include "Es/Memory/debugNew.hpp"

#endif //_ENABLE_VON

#endif
