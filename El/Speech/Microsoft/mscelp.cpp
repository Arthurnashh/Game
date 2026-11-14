/**
  @file   mscelp.cpp
  @brief  Microsoft's CELP (?) codec (3.6 kbit).

  <p>Copyright &copy; 2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  4.6.2003
  @date   17.7.2003
*/

#include "mscelp.hpp"

#if defined _XBOX && !(_XBOX_VER>=2)

//----------------------------------------------------------------------
//  MSCELPCodec:

MSCELPCodec::MSCELPCodec ( unsigned frameLen )
{
  m_frameLen = frameLen;
  m_frameCodeLen = MSCELP_DEFAULT_FRAME_CODE;
}

void MSCELPCodec::getInfo ( CodecInfo &info )
{
  strcpy(info.name,"Microsoft CELP codec (3.6kbit)");
  info.type             = ScVoice;
  info.flags            = CODEC_INDEPENDENT_FRAMES | CODEC_PREDICTED_FRAMES | CODEC_STRICT_FRAME_SIZE;
  info.nominalRate      =
  info.minRate          =
  info.maxRate          = 3600;
  info.nominalBps       =
  info.minBps           =
  info.maxBps           = 16;
  info.nominalFrequency =
  info.minFrequency     =
  info.maxFrequency     = 8000;
  info.nominalFrame     =
  info.minFrame         =
  info.maxFrame         = m_frameLen;
}

bool MSCELPCodec::match ( const CodecInfo &info )
{
  return( strstr(info.name,"Microsoft") &&
          info.type == ScVoice );
}

unsigned16 MSCELPCodec::saveInfo ( unsigned8 *data )
{
  if ( data )                               // save the binary info:
  {
    data[0] = 3;
    *((unsigned16*)(data+1)) = m_frameLen;
  }
  return 3;
}

MSCELPCodec *MSCELPCodec::restore ( VoNSystem *von, unsigned16 size, unsigned8 *data )
{
  if ( size < 3 || !data || data[0] != 3 ) return NULL;
  Assert( von );
  return new MSCELPCodec(*((unsigned16*)(data+1)));
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
VoNResult MSCELPCodec::encode ( CircularBufferPointers &ptr, unsigned &encoded,
                                unsigned8 *outPtr, unsigned &outSize, int DC )
{
  unsigned ready = ptr.len1 + ptr.len2;
  if ( !ready )
  {
    outSize = 0;
    if ( !encoded ) return VonOK;
    encoded = 0;
    return VonBadData;                      // no data to encode!
  }
  unsigned frames = (encoded + m_frameLen - 1) / m_frameLen;
  unsigned toEncode = frames * m_frameLen;  // multiple of frameLen
  if ( ready < toEncode )
  {
    encoded = outSize = 0;
    return VonBadData;                      // not enough data are ready..
  }
  assertEncoder();
  if ( !m_encoder )
  {
    outSize = encoded = 0;
    return VonError;
  }
  if ( frames * m_frameCodeLen > outSize )  // not enough room for code..
  {
    outSize = encoded = 0;
    return VonBadData;
  }
  Assert( outPtr );
  encoded = toEncode;
  outSize = frames * m_frameCodeLen;
  // !!! TODO: buffer may be misaligned ???

  while ( frames-- )                        // encode one frame
  {
    XMEDIAPACKET xmpIn;                     // input samples
    Zero(xmpIn);
    xmpIn.dwMaxSize         = m_frameLen * 2;
    xmpIn.pvBuffer          = ptr.ptr16();

    DWORD compressed;
    XMEDIAPACKET xmpOut;                    // output code
    Zero(xmpOut);
    xmpOut.dwMaxSize        = m_frameCodeLen;
    xmpOut.pvBuffer         = outPtr;
    xmpOut.pdwCompletedSize = &compressed;

    m_encoder->ProcessMultiple(1,&xmpIn,1,&xmpOut);
    if ( compressed )
    {
      Assert( compressed == m_frameCodeLen );
    }
    else
      memset(outPtr,0,m_frameCodeLen);      // !!! TODO: use better SILENCE-MAGIC identifier ???

    outPtr += m_frameCodeLen;
    ptr.skip16(m_frameLen);
  }

#ifdef NET_LOG_VOICE_VERBOSE
  NetLog("MSCELPCodec::encode: samples=%u, code=%u",encoded,outSize);
#endif
  return VonOK;
}

void MSCELPCodec::setProperty ( int id, void *prop )
{
  switch ( id )
  {
    case 0:
      m_encoder = (IXVoiceEncoder*)prop;
      if ( m_encoder )
      {
        m_encoder.GetRef()->AddRef();
        DWORD codeLen = MSCELP_DEFAULT_FRAME_CODE;
        m_encoder->GetCodecBufferSize(m_frameLen*2,&codeLen);
        m_frameCodeLen = codeLen;
#ifdef NET_LOG_VOICE
        NetLog("MSCELPCodec::setProperty(0): %u samples -> %u bytes",m_frameLen,m_frameCodeLen);
#endif
      }
      break;
    case 1:
      m_decoder = (IXVoiceDecoder*)prop;
      if ( m_decoder )
        m_decoder.GetRef()->AddRef();
      break;
  }
}

/// Asserts (creates if necessary) the COM encoder instance.
void MSCELPCodec::assertEncoder ()
{
  if ( !m_encoder )
  {
    Verify( XVoiceCreateOneToOneEncoder(m_encoder.Init()) == S_OK );
    if ( m_encoder )
    {
      DWORD codeLen = MSCELP_DEFAULT_FRAME_CODE;
      m_encoder->GetCodecBufferSize(m_frameLen*2,&codeLen);
      m_frameCodeLen = codeLen;
#ifdef NET_LOG_VOICE
      NetLog("MSCELPCodec::assertEncoder: %u samples -> %u bytes",m_frameLen,m_frameCodeLen);
#endif
    }
  }
}

/// Asserts (creates if necessary) the COM decoder instance.
void MSCELPCodec::assertDecoder ()
{
  if ( !m_decoder )
  {
    Verify( XVoiceCreateOneToOneDecoder(m_decoder.Init()) == S_OK );
  }
}

/**
  Estimates maximum frame-code length (of the next frame) in bytes.
*/
unsigned MSCELPCodec::estFrameCodeLen ()
{
  assertEncoder();
  return m_frameCodeLen;
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
VoNResult MSCELPCodec::decode ( CircularBufferPointers &ptr, unsigned &decoded,
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
  unsigned frames = (decoded + m_frameLen - 1) / m_frameLen;
  unsigned toDecode = frames * m_frameLen;  // multiple of frameLen
  decoded = inSize = 0;
  if ( ready < toDecode )
    return VonBadData;                      // not enough room for data..
  assertDecoder();
  if ( !m_decoder ) return VonError;
  Assert( inPtr );
  VoNResult result = VonSilence;

  while ( frames-- )                        // decode one frame
  {
    unsigned i;
    for ( i = 0; i < m_frameCodeLen; i++ )  // !!! TODO: use better SILENCE-MAGIC identifier ???
      if ( inPtr[0] ) break;
    if ( i == m_frameCodeLen )              // silence
      generateSilence(ptr,16,m_frameLen);
    else
    {
      XMEDIAPACKET xmpIn;                   // input code
      Zero(xmpIn);
      xmpIn.dwMaxSize         = m_frameCodeLen;
      xmpIn.pvBuffer          = inPtr;

      int16 decBuf[640];
      Assert( m_frameLen <= 640 );
      XMEDIAPACKET xmpOut;                  // output samples
      Zero(xmpOut);
      xmpOut.dwMaxSize        = m_frameLen * 2;
      xmpOut.pvBuffer         = decBuf;

      m_decoder->ProcessMultiple(1,&xmpIn,1,&xmpOut);
      for ( i = 0; i < m_frameLen; i++ )
        ptr.write16(decBuf[i]);

      result = VonOK;                       // at least one sounding frame => sounding result
    }
    inPtr += m_frameCodeLen;
    inSize += m_frameCodeLen;
    decoded += m_frameLen;
  }

#ifdef NET_LOG_VOICE_VERBOSE
  NetLog("MSCELPCodec::decode: samples=%u, code=%u",decoded,inSize);
#endif
  return result;
}

void MSCELPCodec::setVoiceMask ( const VoiceMask &newMask )
{
  assertEncoder();
  if ( m_encoder )
  {
    XVOICE_MASK vm;
    vm.fSpecEnergyWeight = newMask.specEnergyWeight;
    vm.fPitchScale       = newMask.pitchScale;
    vm.fWhisperValue     = newMask.whisperValue;
    vm.fRoboticValue     = newMask.roboticValue;
    m_encoder->SetVoiceMask(0,&vm);
  }
}

#include "Es/Memory/normalNew.hpp"

void* MSCELPCodec::operator new ( size_t size )
{
  return safeNew(size);
}

void* MSCELPCodec::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void MSCELPCodec::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

#endif
