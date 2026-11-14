#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TIT_EFFECTS_HPP
#define _TIT_EFFECTS_HPP

// predefined static camera positions
#include <Es/Strings/rString.hpp>

DEFINE_ENUM_BEG(TitEffectName)
	TitPlain,TitPlainDown,
	TitBlack,TitBlackFaded,
	TitBlackOut,TitBlackIn,
	TitWhiteOut,TitWhiteIn,
  TitPlainNoFade,

	NTitEffects
DEFINE_ENUM_END(TitEffectName)

class TitleEffect;

TitleEffect *CreateTitleEffect( TitEffectName name, RString text, float speed=-1, RString font = RString(), float size = 0, bool structured = false, bool showInMap = true );
TitleEffect *CreateTitleEffectObj
(
	TitEffectName name, ParamEntryVal entry, float speed=-1, bool showInMap = true
);
TitleEffect *CreateTitleEffectRsc
(
	TitEffectName name, ParamEntryVal entry, float speed=-1, bool showInMap = true
);

#endif
