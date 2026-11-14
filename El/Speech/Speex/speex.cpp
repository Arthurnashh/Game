#ifdef _MSC_VER
#  pragma once
#endif

/**
@file   speex.cpp
@brief  speex voice codec
*/

#include <El/Speech/vonPch.hpp>
#if _ENABLE_VON && !defined _M_PPC
// TODOX360: compile speex for PPC or use XHV

#include "speex.hpp"
#include <El/Credits/credits.hpp>

//----------------------------------------------------------------------
//  SpeexCodec:

/*!
\patch 5160 Date 5/23/2007 by Ondra
- Fixed: Improved Voice codec quality, bandwidth for voice increased from 6 kbps to 8 kbps
*/
SpeexCodec::SpeexCodec ( int freq )
{
  m_VAD = false;
  //m_bps = 16;
  Assert (freq==8000 || freq==16000 || freq==32000);

  /*Initialization of the structure that holds the bits*/
  speex_bits_init(&m_enc_bits);
  speex_bits_init(&m_dec_bits);
  /*Create a new encoder state in narrow band or wide mode*/
  if (freq<=8000)
  {
    m_freq = 8000; //narrow band
    m_enc_state = speex_encoder_init(&speex_nb_mode);
    m_dec_state = speex_decoder_init(&speex_nb_mode);
  }
  else if (freq<=16000)
  {
    m_freq = 16000; //wide band
    m_enc_state = speex_encoder_init(&speex_wb_mode);
    m_dec_state = speex_decoder_init(&speex_wb_mode);
  }
  else
  {
    m_freq = 16000; //wide band
    m_enc_state = speex_encoder_init(&speex_uwb_mode);
    m_dec_state = speex_decoder_init(&speex_uwb_mode);
  }
  /*Set the speex codec quality
  This is only preliminary setting of the quality.
  The quality can be configured by the entry VoNCodecQuality in the dedicated server config.
  Default values are placed inside constructors of VoNSystem and NetworkManager.
  (Value of codec quality is duplicated, because VoNSystem is not present on dedicated servers and
   NetworkManager cannot be accessible from VoNSystem.)
   For more info about possible quality values see SpeexCodec::SetQuality(int quality)
  */
  int tmp=3; //Artifacts/noise sometimes noticeable
  speex_encoder_ctl(m_enc_state, SPEEX_SET_QUALITY, &tmp); //default should be used also in constructors of VoNSystem and NetworkManager
  /*Set low complexity*/
  tmp=2;
  speex_encoder_ctl(m_enc_state, SPEEX_SET_COMPLEXITY, &tmp);
#if SPEEX_ENABLE_VAD_DTX
  /*Enable DTX  discontinuous transmission*/
  tmp = 1;
  speex_encoder_ctl(m_enc_state, SPEEX_SET_DTX, &tmp);
  /*Enable VAD voice activity detection*/
  tmp = 1;
  speex_encoder_ctl(m_enc_state, SPEEX_SET_VAD, &tmp);
  SetVADEnabled(true); //but do not forget to enable DTX too
#endif
  /*Get the frame size*/
  speex_encoder_ctl(m_enc_state,SPEEX_GET_FRAME_SIZE,&m_frame); //number of samples, not bytes
  /*Set the perceptual enhancement on*/
  tmp=1;
  speex_decoder_ctl(m_dec_state, SPEEX_SET_ENH, &tmp);
#if SPEEX_USE_PREPROCESOR
  /*Speex preprocessor*/
  m_pre_state = speex_preprocess_state_init(m_frame, m_freq);
  tmp = 1;
  speex_preprocess_ctl(m_pre_state, SPEEX_PREPROCESS_SET_AGC, &tmp);
  float ftmp = 8000.0f;
  speex_preprocess_ctl(m_pre_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &ftmp);

  speex_preprocess_ctl(m_pre_state, SPEEX_PREPROCESS_SET_DENOISE, &tmp);
//  speex_preprocess_ctl(m_pre_state, SPEEX_PREPROCESS_SET_DEREVERB, &tmp);
  speex_preprocess_ctl(m_pre_state, SPEEX_PREPROCESS_SET_VAD, &tmp);

  tmp = -30;
  //speex_preprocess_ctl(m_pre_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &tmp);

  tmp = 99;
  speex_preprocess_ctl(m_pre_state, SPEEX_PREPROCESS_SET_PROB_START, &tmp);
  tmp = 95;
  speex_preprocess_ctl(m_pre_state, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &tmp);

#endif
}

void SpeexCodec::SetQuality(int quality)
{
/*
  Mode 	Quality 	Bit-rate (bps) 	mflops 	Quality/description
    0 	-        	250 	          0 	    No transmission (DTX)
    1 	0 	      2,150 	        6 	    Vocoder (mostly for comfort noise)
    2 	2 	      5,950 	        9 	    Very noticeable artifacts/noise, good intelligibility
    3 	3-4 	    8,000 	        10 	    Artifacts/noise sometimes noticeable
    4 	5-6 	    11,000 	        14 	    Artifacts usually noticeable only with headphones
    5 	7-8 	    15,000 	        11 	    Need good headphones to tell the difference
    6 	9 	      18,200 	        17.5 	  Hard to tell the difference even with good headphones
    7 	10 	      24,600 	        14.5 	  Completely transparent for voice, good quality music
    8 	1 	      3,950 	        10.5 	  Very noticeable artifacts/noise, good intelligibility
*/
  saturate(quality, 0, 10);
  speex_encoder_ctl(m_enc_state, SPEEX_SET_QUALITY, &quality);
}

SpeexCodec::~SpeexCodec()
{
  /*Destroy the encoder state*/
  speex_encoder_destroy(m_enc_state);
  /*Destroy the decoder state*/
  speex_decoder_destroy(m_dec_state);
  /*Destroy the bit-packing struct*/
  speex_bits_destroy(&m_enc_bits);
  speex_bits_destroy(&m_dec_bits);
#if SPEEX_USE_PREPROCESOR
  speex_preprocess_state_destroy(m_pre_state);
#endif
}

void SpeexCodec::getInfo ( CodecInfo &info )
{
  strcpy(info.name,"Speex codec");
  info.type             = ScVoice;
  info.flags            = CODEC_INDEPENDENT_FRAMES | CODEC_PREDICTED_FRAMES | CODEC_STRICT_FRAME_SIZE;
  info.nominalRate      = speex_encoder_ctl(m_enc_state,SPEEX_GET_BITRATE,&info.nominalRate);
  info.minRate          = 3950;
  info.maxRate          = 24600;
  info.nominalBps       = 
    info.minBps           = 
    info.maxBps           = 16;
  info.nominalFrequency = m_freq;
  info.minFrequency     = 8000;
  info.maxFrequency     = 16000;
  info.nominalFrame     = //not using ultra wide band mode (should always be 160)
    info.minFrame         = 
    info.maxFrame         = m_frame;
}

bool SpeexCodec::match ( const CodecInfo &info )
{
  return( !strncmp(info.name,"Speex",5) &&
          info.type == ScVoice );
}

unsigned16 SpeexCodec::saveInfo ( unsigned8 *data )
{
  if ( data )                               // save the binary info:
  {
    data[0] = 4;
    *((unsigned16*)(data+1)) = m_freq;
  }
  return 3;
}

SpeexCodec *SpeexCodec::restore ( VoNSystem *von, unsigned16 size, unsigned8 *data )
{
  if ( size < 3 || !data || data[0] != 4 ) return NULL;
  return new SpeexCodec(*((unsigned16*)(data+1)));
}

/**
Encodes one batch (independent group) of sound data.
@param  ptr Pointers to circular sound buffer (raw data
to be encoded).
@param  encoded How many samples should be/were encoded.
@param  outPtr Output (code) pointer.
@param  outSize Available/compressed code size in bytes.
@param  DC directCurrent which should be subtracted from encoded signal.
@return Result code: vonOK, vonBadData.
*/
VoNResult SpeexCodec::encode ( CircularBufferPointers &ptr, unsigned &encoded,
                               unsigned8 *outPtr, unsigned &outSize, int DC, float boost)
{
  short wave[512]; //it is far greater than nb ... 160 or wb ... 320
  unsigned ready = ptr.len1 + ptr.len2;
  if ( !ready )
  {
    outSize = 0;
    if ( !encoded ) return VonOK;
    encoded = 0;
    return VonBadData;                      // no data to encode!
  }

  Assert( encoded % m_frame == 0 );
  unsigned frames = (encoded + m_frame - 1) / m_frame;
  unsigned toEncode = frames * m_frame;
  if ( ready < toEncode )
  {
    encoded = outSize = 0;
    return VonBadData;                      // not enough data are ready..
  }
  Assert( outPtr );
  encoded = toEncode;

  int dataSize = 0;
  int sizeLeft = outSize;

#if SPEEX_ENABLE_VAD_DTX
  bool silence = true;
#endif
#if SPEEX_USE_PREPROCESOR
  bool preprocessSilence = true;
#endif
  // encoding:
  for ( unsigned int fr = 0; fr++ < frames; )
  {
    for (unsigned int i=0; i<m_frame; i++)
    {
      int val = toInt((float)(ptr.read16() - DC)*boost);
      saturate(val, SHRT_MIN, SHRT_MAX);
      wave[i] = val;

#if DEBUG_VON_SAVE_BUFFERS
      GDebugVoNSaveBuffers.GetSendBuf().Add(val);
#endif

    }
#if SPEEX_USE_PREPROCESOR
    {
      int prepResult = speex_preprocess_run(m_pre_state, wave);
      if (prepResult != 0)
        preprocessSilence = false;
    }
#endif

    /*Flush all the bits in the struct so we can encode a new frame*/
    speex_bits_reset(&m_enc_bits);

    /*Encode the frame*/
#if SPEEX_ENABLE_VAD_DTX
    int res = 
#endif    
    speex_encode_int(m_enc_state, wave, &m_enc_bits);

#if SPEEX_ENABLE_VAD_DTX
    if (res != 0)
      silence = false;
#endif

    /*Copy the bits to an array of char that can be written*/
    int blockSize = speex_bits_write(&m_enc_bits, (char*)(outPtr+sizeof(char)), sizeLeft-sizeof(char));
    /*Write the size of the frame first. This is what sampledec expects but
    it's likely to be different in your own application*/
    *(char*)outPtr = blockSize; //one byte should be sufficient
    dataSize += blockSize+sizeof(char);
    outPtr += blockSize+sizeof(char);
    sizeLeft -= blockSize+sizeof(char);
  }
  outSize = dataSize;

#if SPEEX_ENABLE_VAD_DTX
  if (silence)
    return VonSilence;
#endif
#if SPEEX_USE_PREPROCESOR
  if (preprocessSilence)
    return VonSilence;
#endif  

  return VonOK;
}

void SpeexCodec::encodeBeep(unsigned8 *outPtr, unsigned &outSize, int frequency, int encoded, int shift)
{
  short wave[512]; //it is far greater than nb ... 160 or wb ... 320
  int codFr = m_freq; saturateMax(codFr,1);
  const float PI = 3.14159265f;
  float freqSin = 2*PI*float(frequency)/codFr;

  unsigned frames = (encoded + m_frame - 1) / m_frame;
  unsigned toEncode = frames * m_frame;
  encoded = toEncode;

  int dataSize = 0;
  int sizeLeft = outSize;
  // encoding:
  for ( unsigned int fr = 0; fr++ < frames; )
  {
    int curShift = fr*m_frame + shift;
    for (unsigned int i=0; i<m_frame; i++)
    {
      wave[i] = short(sin(freqSin*(i+curShift)) * 25000.0);
    }
#if SPEEX_USE_PREPROCESOR
    speex_preprocess(m_pre_state, wave, NULL);
#endif

    /*Flush all the bits in the struct so we can encode a new frame*/
    speex_bits_reset(&m_enc_bits);

    /*Encode the frame*/
    speex_encode_int(m_enc_state, wave, &m_enc_bits);
    /*Copy the bits to an array of char that can be written*/
    int blockSize = speex_bits_write(&m_enc_bits, (char*)(outPtr+3), sizeLeft-3*sizeof(char));
    /*Write the size of the frame first. This is what sampledec expects but
    it's likely to be different in your own application*/
    *(short*)outPtr = blockSize+1; //short - VoNReplayer.decode needs it
    *(char*)(outPtr+2) = blockSize; //one byte should be sufficient
    dataSize += blockSize+3;
    outPtr += blockSize+3;
    sizeLeft -= blockSize+3;
  }
  outSize = dataSize;
}

/**
Estimates maximum frame-code length (of the next frame) in bytes.
*/
unsigned SpeexCodec::estFrameCodeLen ()
{
  return( 200 ); //TODO
}

unsigned SpeexCodec::estFrameLen ()
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
VoNResult SpeexCodec::decode ( CircularBufferPointers &ptr, unsigned &decoded,
                               unsigned8 *inPtr, unsigned &inSize )
{
  float output[512];
  unsigned ready = ptr.len1 + ptr.len2;
  if ( !ready )
  {
    inSize = 0;
    if ( !decoded ) return VonOK;
    decoded = 0;
    return VonBadData;                      // no room for decoded data!
  }

  unsigned toDecode = m_frame;
  if ( ready < toDecode )
  {
    decoded = inSize = 0;
    return VonBadData;                      // not enough room for data..
  }
  Assert( inPtr );
  unsigned decodedCnt = 0;
  unsigned8 *inPtrStart = inPtr;

#define _DEBUG_SPEEX_DECODE 0
#if _DEBUG_SPEEX_DECODE
  //debug what was decoded
  static int16 debugArray[8000];
  static int ix=0;
  static int nextLogIx=8000;
#endif
  while ( decodedCnt < decoded && (ptr.len1 + ptr.len2)>=m_frame )
  {
    /*Read the size encoded by sampleenc, this part will likely be 
    different in your application*/
    int nbBytes = *(char*)inPtr;
    inPtr += sizeof(char);

    /*Copy the data into the bit-stream struct*/
    speex_bits_read_from(&m_dec_bits, (char *)inPtr, nbBytes);

    /*Decode the data*/
    speex_decode(m_dec_state, &m_dec_bits, output);

    /*Copy from float to short (16 bits) for output*/
    for (unsigned i=0;i<m_frame;i++) 
    {
      int val = toInt(output[i]);
      saturate(val,SHRT_MIN, SHRT_MAX);
      ptr.write16(val);
#if _DEBUG_SPEEX_DECODE
      debugArray[(ix++)%8000] = val;
#endif
#if DEBUG_VON_SAVE_BUFFERS
      GDebugVoNSaveBuffers.GetReceiveBuf().Add(val);
#endif
    }

    decodedCnt += m_frame;
    inPtr += nbBytes;
  }
#if _DEBUG_SPEEX_DECODE
  // LogF what was decoded
  if (ix>=nextLogIx)
  { // Debug Log it
    nextLogIx+=8000;
    LogF("((Decoded Voice, ix = %d", ix);
    char buf[1024];
    for (int i=0; i<8000; i+=50)
    {
      *buf = 0;
      for (int j=0; j<50; j++)
      {
        char num[16];
        int val1 = debugArray[i+j];
        int val2 = debugArray[(i+j+1)%8000];
        if (val2==val1) strcat(buf, "=");
        else if (val2<val1) strcat (buf, "-");
        else strcat (buf, "+");
        //sprintf(num, "%d ", val);
        //strcat(buf, num);
      }
      LogF(buf);
    }
    LogF("))Decoded Voice");
  }
#endif

  decoded = decodedCnt;
  inSize = inPtr-inPtrStart;

  return VonOK;
}

#include "Es/Memory/normalNew.hpp"

void* SpeexCodec::operator new ( size_t size )
{
  return safeNew(size);
}

void* SpeexCodec::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void SpeexCodec::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"
REGISTER_CREDITS("Speex", "2002-2006 Jean-Marc Valin",STANDARD_DISCLAIMER)

#endif //_ENABLE_VON
