#include "precomp.h"

#ifdef _MSC_VER
// C4512 - "assignment operator could not be generated". Boost classes cause this warning.
// Also C4996 - "std::equal called with parameters that may be unsafe"
#pragma warning(push)
#pragma warning(disable:4512)
#pragma warning(disable:4996)
#endif
#include "Binding.h"
#include "Sky.h"
#ifndef SN_TARGET_PS3
#include <stdlib.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>
#else
#include <stdio.h>
#endif

SIMUL_SKY_EXPORT const char *ConvertToString(const simul::sky::float4 *V)
{
	char txt[20];
	static std::string result;
	result="";
	sprintf(txt,"%4.4g",V->x);
	result=txt;
	sprintf(txt,",%4.4g",V->y);
	result+=txt;
	sprintf(txt,",%4.4g",V->z);
	result+=txt;
	sprintf(txt,",%4.4g",V->w);
	result+=txt;
	return result.c_str();
}
SIMUL_SKY_EXPORT void ConvertFromString(simul::sky::float4 *V,const char *str)
{
	typedef std::vector<std::string> string_vector;
	string_vector SplitVec; // #2: Search for tokens
#ifndef SN_TARGET_PS3
	boost::algorithm::split(SplitVec,str,boost::algorithm::is_any_of("(,)") );
	if(SplitVec.size()>0)
		V->x	=(float)atof((SplitVec[0]).c_str());
	if(SplitVec.size()>1)
		V->y	=(float)atof((SplitVec[1]).c_str());
	if(SplitVec.size()>2)
		V->z	=(float)atof((SplitVec[2]).c_str());
	if(SplitVec.size()>3)
		V->w	=(float)atof((SplitVec[3]).c_str());
#endif
}


SIMUL_SKY_EXPORT const simul::sky::float4 &ConstFromString(const char *str)
{
	static simul::sky::float4 val;
	ConvertFromString(&val,str);
	return val;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif
