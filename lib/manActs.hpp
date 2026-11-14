#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MAN_ACT_HPP
#define _MAN_ACT_HPP

DEFINE_ENUM_BEG(ManAction)
	#define ACTION(x) ManAct##x,
	#include "../cfg/manActions.hpp"

	ManActNormN, // count of normal values
	// ManActNormN is not separate value
	// avoid increasing enum value
	ManActNoIncrement = ManActNormN-1, // next value will be ManActNormN

	ManActN // count of all values

	#undef ACTION
DEFINE_ENUM_END(ManAction)


#endif
