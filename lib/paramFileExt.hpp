#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PARAM_FILE_EXT_HPP
#define _PARAM_FILE_EXT_HPP

#include <El/ParamFile/paramFile.hpp>
#include <El/Color/colors.hpp>
#include <El/ParamArchive/serializeClass.hpp>

extern ParamFile Pars;
extern ParamFile ExtParsMission;

struct FontID;

FontID GetFontID( RString baseName );

struct SoundPars : public SerializeClass
{
	RString name;
	float vol,freq;
  float distance;
	float volRnd,freqRnd;

  SoundPars() {};
  #ifndef ACCESS_ONLY
	LSError Serialize(ParamArchive &ar);
	#else
	LSError Serialize(ParamArchive &ar){return LSOK;}
	#endif
};
TypeIsMovableZeroed(SoundPars);

PackedColor GetPackedColor(ParamEntryPar entry);
Color GetColor(ParamEntryVal entry);

bool GetValue(PackedColor &val, ParamEntryPar entry);
bool GetValue(Color &val, ParamEntryPar entry);
bool GetValue(Color &val, const IParamArrayValue &entry);
bool GetValue(Vector3 &val, ParamEntryPar entry);
bool GetValue(SoundPars &val, ParamEntryPar entry);
bool GetValue(SoundPars &val, const IParamArrayValue &entry);

void GetHDRColor(Color &rgb, const IParamArrayValue &color);
void GetHDRColor(Color &rgb, ParamEntryPar entry);

RString GetWorldName( RString baseName );

#endif
