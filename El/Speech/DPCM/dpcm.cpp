#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   dpcm.cpp
  @brief  DPCM codec.

  <p>Copyright &copy; 2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  14.6.2003
  @date   17.7.2003
*/

#include <El/Speech/vonPch.hpp>
#if _ENABLE_VON

#include "dpcm.hpp"

//----------------------------------------------------------------------
//  DPCMCodec:

/// DPCM quantization (in bits).
const int DPCM_SHIFT = 2;

/// Minimum value after quantization. -32 for DPCM_SHIFT == 2.
const int DPCM_MIN   = -128 >> DPCM_SHIFT;

/// Maximum value after quantization. 31 for DPCM_SHIFT == 2.
const int DPCM_MAX   =  127 >> DPCM_SHIFT;

/// Minimum DPCM error. -126 for DPCM_SHIFT == 2.
const int DPCM_MIN_ERROR = 2 * (DPCM_MIN - DPCM_MAX);

/// Maximum DPCM error. 126 for DPCM_SHIFT == 2.
const int DPCM_MAX_ERROR = - DPCM_MIN_ERROR;


#ifdef NET_LOG_DPCM

const int DPCM_BOUND = 32;

unsigned Hist[ DPCM_BOUND + DPCM_BOUND + 1];
unsigned HCounter;

void initHist ()
{
  Zero(Hist);
  HCounter = 0;
}

#endif

DPCMCodec::DPCMCodec ( int freq, int bps, unsigned frame )
{
  Assert( bps == 8 || bps == 16 );
  m_bps = bps;
  Assert( freq >= 4000 && freq <= 16000 );
  m_freq = freq;
  Assert( frame >= 100 && frame <= 2000 );
  m_frame = frame;
#ifdef NET_LOG_DPCM
  initHist();
#endif
}

void DPCMCodec::getInfo ( CodecInfo &info )
{
  strcpy(info.name,"DPCM codec (linear prediction)");
  info.type             = ScUniversal;
  info.flags            = CODEC_INDEPENDENT_FRAMES;
  info.nominalRate      =
  info.minRate          =
  info.maxRate          = (unsigned)(m_freq * 3.8);
  info.nominalBps       = m_bps;
  info.minBps           = 8;
  info.maxBps           = 16;
  info.nominalFrequency = m_freq;
  info.minFrequency     =   4000;
  info.maxFrequency     =  16000;
  info.nominalFrame     = m_frame;
  info.minFrame         =     100;
  info.maxFrame         =    2000;
}

bool DPCMCodec::match ( const CodecInfo &info )
{
  return( !strncmp(info.name,"DPCM",4) ||
          info.type == ScUniversal );
}

unsigned16 DPCMCodec::saveInfo ( unsigned8 *data )
{
  if ( data )                               // save the binary info:
  {
    data[0] = 4;
    *((unsigned16*)(data+1)) = m_freq;
    *((unsigned16*)(data+3)) = m_frame;
  }
  return 5;
}

DPCMCodec *DPCMCodec::restore ( VoNSystem *von, unsigned16 size, unsigned8 *data )
{
  if ( size < 5 || !data || data[0] != 4 ) return NULL;
  Assert( von );
  return new DPCMCodec(*((unsigned16*)(data+1)),von->outBps,*((unsigned16*)(data+3)));
}

/**
  Encodes one batch (independent group) of sound data.
  @param  ptr Pointers to circular sound buffer (raw data
              to be encoded).
  @param  encoded How many samples should be/were encoded.
  @param  outPtr Output (code) pointer.
  @param  outSize Available/compressed code size in bytes.
  @return Result code: vonOK, vonBadData.
*/
VoNResult DPCMCodec::encode ( CircularBufferPointers &ptr, unsigned &encoded,
                              unsigned8 *outPtr, unsigned &outSize, int DC, float boost )
{
  unsigned ready = ptr.len1 + ptr.len2;
  if ( !ready )
  {
    outSize = 0;
    if ( !encoded ) return VonOK;
    encoded = 0;
    return VonBadData;                      // no data to encode!
  }
  unsigned toEncode = encoded;
  if ( ready < toEncode )
  {
    encoded = outSize = 0;
    return VonBadData;                      // not enough data are ready..
  }
  Assert( outPtr );
  outSize = 0;                              // don't know yet
#ifdef NET_LOG_DPCM
  CircularBufferPointers bak = ptr;
#endif

  *outPtr = 1;
  outSize = 1;
  if ( m_bps == 8 )
    ptr.skip8(toEncode);
  else
    ptr.skip16(toEncode);

#ifdef NET_LOG_DPCM
  toEncode = encoded - 2;
  int X = 0;
  int A = 0;
  int B, prediction;
  if ( m_bps == 8 )
  {
    int8 pcmVal = bak.read8() - DC;
    A = ((int)pcmVal) & DPCM_MASK;
    pcmVal = bak.read8() - DC;
    X = ((int)pcmVal) & DPCM_MASK;
    while ( toEncode-- )
    {
      B = A;
      A = X;
      pcmVal = bak.read8() - DC;
      X = (int)pcmVal & DPCM_MASK;
        // linear prediction:
      prediction = X - (A + A - B);
      prediction >>= DPCM_SHIFT;
      if ( (prediction += DPCM_BOUND) < 0 )
        prediction = 0;
      else
      if ( prediction > DPCM_BOUND + DPCM_BOUND )
        prediction = DPCM_BOUND + DPCM_BOUND;
      Hist[prediction]++;
    }
  }
  else
  {
    int16 pcmVal = bak.read16() - DC;
    A = ((int)pcmVal) & (DPCM_MASK << 8);
    pcmVal = bak.read16() - DC;
    X = ((int)pcmVal) & (DPCM_MASK << 8);
    while ( toEncode-- )
    {
      B = A;
      A = X;
      pcmVal = bak.read16() - DC;
      X = ((int)pcmVal) & (DPCM_MASK << 8);
        // linear prediction:
      prediction = X - (A + A - B);
      prediction >>= (8 + DPCM_SHIFT);
      if ( (prediction += DPCM_BOUND) < 0 )
        prediction = 0;
      else
      if ( prediction > DPCM_BOUND + DPCM_BOUND )
        prediction = DPCM_BOUND + DPCM_BOUND;
      Hist[prediction]++;
    }
  }
  if ( !(++HCounter & 0x7f) )
  {
    char buf[1024];
    char *bPtr = buf;
    for ( toEncode = 0; toEncode <= DPCM_BOUND+DPCM_BOUND; toEncode++ )
      bPtr += sprintf(bPtr," %u",Hist[toEncode]);
    NetLog("DPCM:%s",buf);
    if ( !(HCounter & 0x1ff) )
      initHist();
  }
#endif

  return VonOK;
}

/**
  Estimates maximum frame-code length (of the next frame) in bytes.
*/
unsigned DPCMCodec::estFrameCodeLen ()
{
  return( (unsigned)( (m_frame * 3.8) / 8.0 ) );
}

unsigned DPCMCodec::estFrameLen()
{
  return m_frame;
}

/**
  Decodes one batch of sound data.
  @param  ptr Pointers to circular sound buffer (raw data
              after decoding).
  @param  decoded How many samples should be/were decoded.
  @param  inPtr Input (code) pointer.
  @param  inSize Available/consumed code size in bytes.
  @return Result code: vonOK, vonBadData.
*/
VoNResult DPCMCodec::decode ( CircularBufferPointers &ptr, unsigned &decoded,
                             unsigned8 *inPtr, unsigned &inSize )
{
  unsigned ready = ptr.len1 + ptr.len2;
  if ( !ready )
  {
    inSize = 0;
    if ( !decoded ) return VonOK;
    decoded = 0;
    return VonBadData;                      // no room for decoded data!
  }
  unsigned toDecode = decoded;
  if ( ready < toDecode )
  {
    decoded = inSize = 0;
    return VonBadData;                      // not enough room for data..
  }
  Assert( inPtr );
  inSize = 0;                               // don't know yet
  unsigned i;

  inSize = 1;
  if ( m_bps == 8 )
    for ( i = 0; i++ < toDecode; )
      ptr.write8( ((int8)(i&4) - 2) << 4 );
  else
    for ( i = 0; i++ < toDecode; )
      ptr.write16( ((int16)(i&4) - 2) << 12 );

  return VonOK;
}

#include "Es/Memory/normalNew.hpp"

void* DPCMCodec::operator new ( size_t size )
{
  return safeNew(size);
}

void* DPCMCodec::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void DPCMCodec::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

#endif
