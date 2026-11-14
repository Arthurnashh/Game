#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PARAM_FILE_EXT_HPP
#define _PARAM_FILE_EXT_HPP

#include <El/ParamFile/paramFile.hpp>
#include <El/Color/colors.hpp>

extern ParamFile Pars;
extern ParamFile ExtParsCampaign;
extern ParamFile ExtParsMission;
extern ParamFile Res;

struct FontID;

FontID GetFontID( RString baseName );

struct SoundPars : public SerializeClass
{
	RString name;
	float vol,freq;
	float volRnd,freqRnd;

	#ifndef ACCESS_ONLY
	LSError Serialize(ParamArchive &ar);
	#else
	LSError Serialize(ParamArchive &ar){return LSOK;}
	#endif
};
TypeIsMovableZeroed(SoundPars);

PackedColor GetPackedColor(const ParamEntry &entry);
Color GetColor(const ParamEntry &entry);

bool GetValue(PackedColor &val, const ParamEntry &entry);
bool GetValue(Color &val, const ParamEntry &entry);
bool GetValue(SoundPars &val, const ParamEntry &entry);
bool GetValue(SoundPars &val, const IParamArrayValue &entry);

RString GetWorldName( RString baseName );

#endif
