#ifdef _MSC_VER
#  pragma once
#endif

/**
  @file   vonCodec.cpp
  @brief  Global VoN codec routines.

  Communication between VoN system and individual codec modules.
  <p>Copyright &copy; 2003 by BIStudio (www.bistudio.com)
  @author PE
  @since  19.1.2003
  @date   15.6.2003
*/

#include "vonPch.hpp"
#if _ENABLE_VON

# include "vonCodec.hpp"
# include "PCM/pcm.hpp"
# define USE_MSCELP 0
# include "DPCM/dpcm.hpp"
# include "LPC/openlpc.hpp"
# include "Speex/speex.hpp"

//----------------------------------------------------------------------
//  VoNCodec API:

/// Finds a codec which fits best the given request.
VoNCodec *VoNSystem::findCodec ( CodecInfo &info )
{
#ifndef _M_PPC
  // TODOX360: compile speex for PPC or use XHV
  //if ( DPCMCodec::match(info) )
  //  return new DPCMCodec(inFreq,inBps,DPCM_DEFAULT_FRAME);
  if ( SpeexCodec::match(info) )
    return new SpeexCodec(info.nominalFrequency?info.nominalFrequency:SPEEX_FS);
  if ( OpenLPCCodec::match(info) )
    return new OpenLPCCodec(inBps,info.nominalFrame?info.nominalFrame:OPENLPC_DEFAULT_FRAME,
                            OPENLPC_DEFAULT_LPC_BITS);
  return new PCMCodec(8000,inBps,PCM_DEFAULT_FRAME);
#else
  return NULL;
#endif
}

/// Restores codec from the saved binary info.
VoNCodec *VoNSystem::restoreCodec ( unsigned16 size, unsigned8 *data )
{
#ifndef _M_PPC
  // TODOX360: compile speex for PPC or use XHV
  VoNCodec *codec;
  if ( (codec = PCMCodec::restore(this,size,data)) ) return codec;
#if USE_MSCELP
  if ( (codec = MSCELPCodec::restore(this,size,data)) ) return codec;
#else
  //if ( (codec = DPCMCodec::restore(this,size,data)) ) return codec;
  if ( (codec = SpeexCodec::restore(this,size,data)) ) return codec;
    if ( (codec = OpenLPCCodec::restore(this,size,data)) ) return codec;
#endif
#endif
  return NULL;
}

#endif //_ENABLE_VON
