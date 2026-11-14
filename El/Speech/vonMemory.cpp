/**
 *  @file   vonMemory.cpp
 *  @brief  Standalone voice codec (works w/o network).
 *
 *  <p>Copyright &copy; 2004 by BIStudio (www.bistudio.com)
 *  @author PE
 *  @since  31.3.2004
 *  @date   9.4.2004
 */

#include "vonPch.hpp"

#if _ENABLE_VON

#include "vonMemory.hpp"
#include "Microsoft/mscelp.hpp"

//----------------------------------------------------------------------
//  Memory-targeted recorder:

VoNMemoryRecorder::VoNMemoryRecorder ( VoNCodec *codec, bool raw )
 : VoNRecorder(NULL,VOID_CHANNEL,codec,VoNRecorder::DEFAULT_REC_THRESHOLD), m_buf(INIT_BUFFER_SIZE)
{
  m_raw = raw;
  m_samples = 0;
  if ( m_raw )
    m_buf.Resize(0);
  else
  {
    m_buf.Resize(7);
    m_buf[0] = (char)VOICE_MAGIC;           // magic number
    m_buf[1] = (char)(8000 & 0xff);         // frequency
    m_buf[2] = (char)(8000 >> 8);
    for ( int i = 3; i < 7; )               // number of samples
      m_buf[i++] = (char)0;
  }
}

VoNMemoryRecorder::~VoNMemoryRecorder ()
{
}

#include "Es/Memory/normalNew.hpp"

void* VoNMemoryRecorder::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNMemoryRecorder::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNMemoryRecorder::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

unsigned VoNMemoryRecorder::encode ( VoNSoundBuffer *sb,
                                     unsigned pos, unsigned minEnc, unsigned maxEnc )
{
  Assert( m_codec );
  m_res = VonOK;

  if ( maxEnc < m_minFrame ) return 0;      // not enough sound data to encode
    // something will be encoded:
  Assert( sb );
  CircularBufferPointers cb;
  sb->lockData(pos,maxEnc,cb);              // prepare "CircularBufferPointers"
    // encode the sound:
  unsigned encoded = 0;
  unsigned enc;                             // bunch-size in samples
  bool finish = false;                      // finish processing
  int sizePtr;

  while ( !finish &&
          encoded + m_minFrame <= maxEnc )  // got something to encode..
  {
    unsigned ready = maxEnc - encoded;
    Assert( ready <= cb.len1 + cb.len2 );

    if ( m_raw )                            // raw mode -> run codec routine only
    {
      unsigned codeSize = m_buf.MaxSize() - m_buf.Size() - 2;
      enc = m_minFrame;
      sizePtr = m_buf.Size();
      m_buf.Resize(sizePtr+m_codec->estFrameCodeLen());
      m_codec->encode(cb,enc,(unsigned8*)&(m_buf.Set(sizePtr)),codeSize);
      if ( enc )
      {                                     // regular voice data
        Assert( codeSize );
        m_buf.Resize(sizePtr+codeSize);
      }
      else                                  // failure -> skip the data
      {
        m_buf.Resize(sizePtr);
        m_samples -= (enc = m_minFrame);
      }
    }
    else                                    // cooked mode -> analyze silence, add my own headers..
    {
        // 2. look for "silence":
      if ( silenceAnalyzer(cb,enc=m_minFrame,true) )
      {
        if ( m_maxFrame > m_minFrame && ready > m_minFrame )
        {
          enc = m_maxFrame;
          if ( enc > ready ) enc = ready;
          if ( !silenceAnalyzer(cb,enc,false) )
            enc = m_minFrame;
        }
          // encode "enc" silent samples:
        sizePtr = m_buf.Size();
        m_buf.Resize(sizePtr+4);
        m_buf[sizePtr  ] = (char)2;
        m_buf[sizePtr+1] = (char)0;
        m_buf[sizePtr+2] = (char)(enc & 0xff);
        m_buf[sizePtr+3] = (char)(enc >> 8);
        if ( m_bps > 8 )
          cb.skip16(enc);
        else
          cb.skip8(enc);
      }

        // 3. else encode voice data (using m_codec):
      else
      {
        unsigned codeSize = m_buf.MaxSize() - m_buf.Size() - 2;
        enc = m_minFrame;
        sizePtr = m_buf.Size();
        m_buf.Resize(sizePtr+2+m_codec->estFrameCodeLen());
        m_codec->encode(cb,enc,(unsigned8*)&(m_buf.Set(sizePtr+2)),codeSize);
        if ( enc )
        {                                   // regular voice data
          Assert( codeSize );
          m_buf[sizePtr]   = (char)(codeSize & 0xff);
          m_buf[sizePtr+1] = (char)(codeSize >> 8);
          m_buf.Resize(sizePtr+2+codeSize);
        }
        else                                // enc == 0 => data-bunch is too small to encode
        {
          if ( encoded < minEnc )           // I have to encode at least "minEnc" samples.. :C
          {
            enc = minEnc - encoded;
              // encode "enc" silent samples:
            m_buf[sizePtr  ] = (char)2;
            m_buf[sizePtr+1] = (char)0;
            m_buf[sizePtr+2] = (char)(enc & 0xff);
            m_buf[sizePtr+3] = (char)(enc >> 8);
            m_buf.Resize(sizePtr+4);
            if ( m_bps > 8 )
              cb.skip16(enc);
            else
              cb.skip8(enc);
          }
          else
          {                                 // revert to old buffer size
            m_buf.Resize(sizePtr);
          }
          finish = true;                    // finish processing of this data-batch
        }
      }
    }

      // 4. finalizing (little work for memory-based encoder):
    encoded  += enc;
  }

  sb->unlockData(encoded,cb);
  m_samples += encoded;
  return encoded;
}

const AutoArray<char,MemAllocSafe> &VoNMemoryRecorder::getData ()
{
  if ( !m_raw )                             // fill in number of samples in the stream header
  {
    unsigned samples = m_samples;
    for ( int i = 3; i < 7; )               // encode number of samples
    {
      m_buf[i++] = (char)(samples & 0xff);
      samples >>= 8;
    }
  }
  return m_buf;
}

//----------------------------------------------------------------------
//  Memory-targeted replayer:

VoNMemoryReplayer::VoNMemoryReplayer ( VoNCodec *codec, bool raw )
 : VoNReplayer(NULL,VOID_CHANNEL,codec)
{
  m_raw = raw;
  m_samples = m_samplePtr = m_codePtr = 0;
}

VoNMemoryReplayer::~VoNMemoryReplayer ()
{
}

#include "Es/Memory/normalNew.hpp"

void* VoNMemoryReplayer::operator new ( size_t size )
{
  return safeNew(size);
}

void* VoNMemoryReplayer::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void VoNMemoryReplayer::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

/**
    Decodes another batch of transferred sound data. res = {vonOK, vonDropout, vonSilence, vonBreak}
    @param  sb Sound-buffer to hold raw output data.
    @param  pos Start position for decoding.
    @param  minDec Minimum data amount being decoded (in number of samples).
    @param  maxDec Maximum data amount being decoded (in number of samples).
    @return Actually decoded number of samples (0 implies an error or vonBreak state).
*/
unsigned VoNMemoryReplayer::decode ( VoNSoundBuffer *sb,
                                     unsigned pos, unsigned minDec, unsigned maxDec )
{
  Assert( m_codec );
  Assert( sb );
  if ( m_samplePtr >= m_samples &&
       minDec == 0 ) return 0;

    // something will be written into *sb:
  CircularBufferPointers cb;
  sb->lockData(pos,maxDec,cb);              // prepare "CircularBufferPointers"
  unsigned decLen;                          // decoded code length in bytes
  unsigned decoded = 0;                     // total count of decoded samples
  int toDecode = minDec;                    // number of samples to decode

  while ( toDecode > 0 &&
          m_samplePtr < m_samples )         // anything to decode
  {
    Assert( (int)m_codePtr < m_buf.Size() );
    unsigned packetSize;
    if ( m_raw )                            // raw mode -> w/o headers
    {
      packetSize = 18;
      decLen = m_minFrame;
      m_codec->decode(cb,decLen,(unsigned8*)&m_buf.Get(m_codePtr),packetSize);
    }
    else                                    // cooked mode -> my own headers, silence
    {
      packetSize = (BYTE)m_buf[m_codePtr] + (((BYTE)m_buf[m_codePtr+1]) << 8);
      Assert( packetSize >= 2 );
      m_codePtr += 2;
      if ( packetSize == 2 )                // generate silence
      {
        decLen = (BYTE)m_buf[m_codePtr] + (((BYTE)m_buf[m_codePtr+1]) << 8);
                                            // number of silent samples
        generateSilence(cb,m_bps,decLen);
      }
      else                                  // decode regular voice data
      {
        unsigned inSize = packetSize;
        decLen = m_minFrame;
        m_codec->decode(cb,decLen,(unsigned8*)&m_buf.Get(m_codePtr),inSize);
        Assert( inSize == packetSize );
      }
    }
    m_codePtr += packetSize;
    decoded += decLen;
    toDecode -= decLen;
    m_samplePtr += decLen;
  }

  if ( toDecode > 0 &&
       m_samplePtr >= m_samples )           // generate silence past the end of data
  {
    generateSilence(cb,m_bps,toDecode);
    decoded += toDecode;
  }

  sb->unlockData(decoded,cb);
  return decoded;
}

/**
  *  Sets encoded data. Data must be encoded by VoNMemoryRecorder (or binary compatible).
  */
void VoNMemoryReplayer::setData ( const AutoArray<char,MemAllocSafe> &buf, int duration )
{
  if ( m_raw )
  {
    m_buf = buf;
    m_samplePtr = m_codePtr = 0;
    m_samples = duration * 8;               // assuming 8kHz sampling rate
  }
  else
  {
    if ( buf[0] != (char)VOICE_MAGIC ||     // version identifier
         buf[1] != (char)(8000 & 0xff) ||   // lo(frequency)
         buf[2] != (char)(8000 >> 8) ||     // hi(frequency)
         buf.Size() < 7 )
      return;
    m_buf = buf;
    m_samplePtr = 0;
    m_codePtr = 7;
    m_samples = (BYTE)m_buf[6];
    for ( int i = 5; i >= 3; )
      m_samples = (m_samples << 8) + (BYTE)m_buf[i--];
    // I'm ignoring "duration"
  }
}

VoNResult VoNMemoryReplayer::getStatus ()
{
  if ( m_samples == 0 ) return VonBadData;
  if ( m_samplePtr < m_samples ) return VonBusy;
  return VonOK;
}

#endif
