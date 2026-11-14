#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   pcm.cpp
  @brief  PCM codec (reducing 64kbit to 48kbit).

  <p>Copyright &copy; 2002-2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  9.12.2002
  @date   17.7.2003
*/

#include <El/Speech/vonPch.hpp>
#if _ENABLE_VON

#include "pcm.hpp"

//----------------------------------------------------------------------
//  PCMCodec statistics (for DPCM):

#ifdef NET_LOG_PCM

#ifdef NET_LOG_PCM_OLD

const int DPCM_BOUND = 128;

unsigned HLinear[DPCM_BOUND+DPCM_BOUND+1];
unsigned HQuadratic[DPCM_BOUND+DPCM_BOUND+1];

unsigned HCounter;

void initHist ()
{
  Zero(HLinear);
  Zero(HQuadratic);
  HCounter = 0;
}

#else   // NET_LOG_PCM_OLD

const int DPCM_BOUND = 32;
const int DPCM_SHIFT = 2;
const int DPCM_MASK  = -1 << DPCM_SHIFT;

unsigned Hist[ DPCM_BOUND + DPCM_BOUND + 1];
unsigned HCounter;

void initHist ()
{
  Zero(Hist);
  HCounter = 0;
}

#endif

#endif

//----------------------------------------------------------------------
//  PCMCodec:

PCMCodec::PCMCodec ( int freq, int bps, unsigned frame )
{
  Assert( bps == 8 || bps == 16 );
  m_bps = bps;
  Assert( freq >= 4000 && freq <= 16000 );
  m_freq = freq;
  frame = (frame + 3) & -4;
  Assert( frame >= 100 && frame <= 2000 );
  m_frame = frame;
#ifdef NET_LOG_PCM
  initHist();
#endif
}

void PCMCodec::getInfo ( CodecInfo &info )
{
  strcpy(info.name,"PCM codec (48kbit)");
  info.type             = ScUncompressed;
  info.flags            = CODEC_INDEPENDENT_FRAMES;
  info.nominalRate      =
  info.minRate          =
  info.maxRate          = m_freq * 6;
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

bool PCMCodec::match ( const CodecInfo &info )
{
  return( !strncmp(info.name,"PCM",3) ||
          info.type == ScUncompressed );
}

unsigned16 PCMCodec::saveInfo ( unsigned8 *data )
{
  if ( data )                               // save the binary info:
  {
    data[0] = 1;
    *((unsigned16*)(data+1)) = m_freq;
    *((unsigned16*)(data+3)) = m_frame;
  }
  return 5;
}

PCMCodec *PCMCodec::restore ( VoNSystem *von, unsigned16 size, unsigned8 *data )
{
  if ( size < 5 || !data || data[0] != 1 ) return NULL;
  Assert( von );
  return new PCMCodec(*((unsigned16*)(data+1)),von->outBps,*((unsigned16*)(data+3)));
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
VoNResult PCMCodec::encode ( CircularBufferPointers &ptr, unsigned &encoded,
                             unsigned8 *outPtr, unsigned &outSize, int DC, float boost)
{
  unsigned ready = ptr.len1 + ptr.len2;
  if ( !ready )
  {
    outSize = 0;
    if ( !encoded ) return VonOK;
    encoded = 0;
    return VonBadData;                      // no data to encode!
  }
  unsigned toEncode = (encoded + 3) & -4;   // multiple of 4
  if ( ready < toEncode )
  {
    encoded = outSize = 0;
    return VonBadData;                      // not enough data are ready..
  }
  Assert( outPtr );
  encoded = toEncode;
  outSize = (toEncode * 3) >> 2;            // 6 bits per sample
#ifdef NET_LOG_VOICE_VERBOSE2
  CircularBufferPointers bak = ptr;
#endif
#ifdef NET_LOG_PCM
  CircularBufferPointers bak2 = ptr;
#endif

  if ( m_bps == 8 )
  {
    do                                      // encode one 4-sample bunch
    {
      //also subtract the DC from the input signal
      int8 pcmVal = toInt((ptr.read8() - DC)*boost);  // in: 1
      *outPtr = (unsigned8)(pcmVal & 0xfc);
      pcmVal = toInt((ptr.read8() - DC)*boost);       // in: 2
      *outPtr++ |= (unsigned8)((pcmVal & 0xc0) >> 6); // out: 1
      *outPtr = (unsigned8)((pcmVal & 0x3c) << 2);
      pcmVal = toInt((ptr.read8() - DC)*boost);       // in: 3
      *outPtr++ |= (unsigned8)((pcmVal & 0xf0) >> 4); // out: 2
      *outPtr = (unsigned8)((pcmVal & 0x0c) << 4);
      pcmVal = toInt((ptr.read8() - DC)*boost);       // in: 4
      *outPtr++ |= (unsigned8)((pcmVal & 0xfc) >> 2); // out: 3
    } while ( (toEncode -= 4) );
  }
  else
  {
    do                                      // encode one 4-sample bunch
    {
      //also subtract the DC from the input signal
      int16 pcmVal = toInt((ptr.read16() - DC)*boost);   // in: 1
      *outPtr = (unsigned8)((pcmVal & 0xfc00) >> 8);
      pcmVal = toInt((ptr.read16() - DC)*boost);         // in: 2
      *outPtr++ |= (unsigned8)((pcmVal & 0xc000) >> 14); // out: 1
      *outPtr = (unsigned8)((pcmVal & 0x3c00) >> 6);     
      pcmVal = toInt((ptr.read16() - DC)*boost);         // in: 3
      *outPtr++ |= (unsigned8)((pcmVal & 0xf000) >> 12); // out: 2
      *outPtr = (unsigned8)((pcmVal & 0x0c00) >> 4);
      pcmVal = toInt((ptr.read16() - DC)*boost);         // in: 4
      *outPtr++ |= (unsigned8)((pcmVal & 0xfc00) >> 10); // out: 3
    } while ( (toEncode -= 4) );
  }

#ifdef NET_LOG_VOICE_VERBOSE2
  char buf[500];
  char *bPtr = buf;
  toEncode = encoded;
  if ( toEncode > 50 ) toEncode = 50;
  if ( m_bps == 8 )
    while ( toEncode-- )
      bPtr += sprintf(bPtr," %d",(int)bak.read8());
  else
    while ( toEncode-- )
      bPtr += sprintf(bPtr," %d",((int)bak.read16()>>8));
  bPtr[0] = (char)0;
  NetLog("PCMCodec::encode: total=%u:%s",encoded,buf);
#endif
#ifdef NET_LOG_PCM
#  ifdef NET_LOG_PCM_OLD
  toEncode = encoded;
  int X = 0;
  int A = 0;
  int B = 0;
  int C, prediction;
  if ( m_bps == 8 )
    while ( toEncode-- )
    {
      C = B;
      B = A;
      A = X;
      X = (int)bak2.read8();
        // linear prediction:
      prediction = X - (A + A - B);
      if ( (prediction += DPCM_BOUND) < 0 )
        prediction = 0;
      else
      if ( prediction > DPCM_BOUND + DPCM_BOUND )
        prediction = DPCM_BOUND + DPCM_BOUND;
      HLinear[prediction]++;
        // quadratic prediction:
      prediction = X - (A + A + A - B - B - B + C);
      if ( (prediction += DPCM_BOUND) < 0 )
        prediction = 0;
      else
      if ( prediction > DPCM_BOUND + DPCM_BOUND )
        prediction = DPCM_BOUND + DPCM_BOUND;
      HQuadratic[prediction]++;
    }
  else
    while ( toEncode-- )
    {
      C = B;
      B = A;
      A = X;
      X = (int)bak2.read16();
        // linear prediction:
      prediction = X - (A + A - B);
      prediction >>= 8;
      if ( (prediction += DPCM_BOUND) < 0 )
        prediction = 0;
      else
      if ( prediction > DPCM_BOUND + DPCM_BOUND )
        prediction = DPCM_BOUND + DPCM_BOUND;
      HLinear[prediction]++;
        // quadratic prediction:
      prediction = X - (A + A + A - B - B - B + C);
      prediction >>= 8;
      if ( (prediction += DPCM_BOUND) < 0 )
        prediction = 0;
      else
      if ( prediction > DPCM_BOUND + DPCM_BOUND )
        prediction = DPCM_BOUND + DPCM_BOUND;
      HQuadratic[prediction]++;
    }
  if ( !(++HCounter & 0x7f) )
  {
    char buf[2048];
    char *bPtr = buf;
    for ( toEncode = 0; toEncode < DPCM_BOUND; toEncode++ )
      bPtr += sprintf(bPtr," %u",HLinear[toEncode]);
    NetLog("DPCM(lin1):%s",buf);
    bPtr = buf;
    for ( ; toEncode <= DPCM_BOUND+DPCM_BOUND; toEncode++ )
      bPtr += sprintf(bPtr," %u",HLinear[toEncode]);
    NetLog("DPCM(lin2):%s",buf);
    bPtr = buf;
    for ( toEncode = 0; toEncode < DPCM_BOUND; toEncode++ )
      bPtr += sprintf(bPtr," %u",HQuadratic[toEncode]);
    NetLog("DPCM(qua1):%s",buf);
    bPtr = buf;
    for ( ; toEncode <= DPCM_BOUND+DPCM_BOUND; toEncode++ )
      bPtr += sprintf(bPtr," %u",HQuadratic[toEncode]);
    NetLog("DPCM(qua2):%s",buf);
    if ( !(HCounter & 0x1ff) )
      initHist();
  }
#  else // NET_LOG_PCM_OLD
  toEncode = encoded - 2;
  int X = 0;
  int A = 0;
  int B, prediction;
  if ( m_bps == 8 )
  {
    A = ((int)bak2.read8()) & DPCM_MASK;
    X = ((int)bak2.read8()) & DPCM_MASK;
    while ( toEncode-- )
    {
      B = A;
      A = X;
      X = (int)bak2.read8() & DPCM_MASK;
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
    A = ((int)bak2.read16()) & (DPCM_MASK << 8);
    X = ((int)bak2.read16()) & (DPCM_MASK << 8);
    while ( toEncode-- )
    {
      B = A;
      A = X;
      X = ((int)bak2.read16()) & (DPCM_MASK << 8);
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
#  endif
#endif

  return VonOK;
}

/**
  Estimates maximum frame-code length (of the next frame) in bytes.
*/
unsigned PCMCodec::estFrameCodeLen ()
{
  return( (m_frame * 6) >> 3 );
}

unsigned PCMCodec::estFrameLen()
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
VoNResult PCMCodec::decode ( CircularBufferPointers &ptr, unsigned &decoded,
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
  unsigned toDecode = (decoded + 3) & -4;   // multiple of 4
  if ( ready < toDecode )
  {
    decoded = inSize = 0;
    return VonBadData;                      // not enough room for data..
  }
  Assert( inPtr );
  decoded = toDecode;
  inSize = (toDecode * 3) >> 2;             // 6 bits per sample
#ifdef NET_LOG_VOICE_VERBOSE2
  CircularBufferPointers bak = ptr;
#endif

  if ( m_bps == 8 )
  {
    int8 tmp;
    do                                      // decode one 4-sample bunch
    {
      ptr.write8( (int8)(*inPtr & 0xfc) );              // out: 1
      tmp = (int8)((*inPtr++ & 0x03) << 6);             // in: 1
      ptr.write8( tmp | (int8)((*inPtr & 0xf0) >> 2) ); // out: 2
      tmp = (int8)((*inPtr++ & 0x0f) << 4);             // in: 2
      ptr.write8( tmp | (int8)((*inPtr & 0xc0) >> 4) ); // out: 3
      ptr.write8( (int8)((*inPtr++ & 0x3f) << 2) );     // in: 3, out: 4
    } while ( (toDecode -= 4) );
  }

  else
  {
    int16 tmp;
    do                                      // decode one 4-sample bunch
    {
      ptr.write16( (((int16)(*inPtr & 0xfc)) << 8) );       // out: 1
      tmp = (((int16)(*inPtr++ & 0x03)) << 14);             // in: 1
      ptr.write16( tmp | (((int16)(*inPtr & 0xf0)) << 6) ); // out: 2
      tmp = (((int16)(*inPtr++ & 0x0f)) << 12);             // in: 2
      ptr.write16( tmp | (((int16)(*inPtr & 0xc0)) << 4) ); // out: 3
      ptr.write16( (((int16)(*inPtr++ & 0x3f)) << 10) );    // in: 3, out: 4
    } while ( (toDecode -= 4) );
  }

#ifdef NET_LOG_VOICE_VERBOSE2
  char buf[500];
  char *bPtr = buf;
  toDecode = decoded;
  if ( toDecode > 50 ) toDecode = 50;
  if ( m_bps == 8 )
    while ( toDecode-- )
      bPtr += sprintf(bPtr," %d",(int)bak.read8());
  else
    while ( toDecode-- )
      bPtr += sprintf(bPtr," %d",((int)bak.read16()>>8));
  bPtr[0] = (char)0;
  NetLog("PCMCodec::decode: total=%u:%s",decoded,buf);
#endif
  return VonOK;
}

#include "Es/Memory/normalNew.hpp"

void* PCMCodec::operator new ( size_t size )
{
  return safeNew(size);
}

void* PCMCodec::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void PCMCodec::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

#endif
