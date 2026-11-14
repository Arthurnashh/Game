#ifndef SIMUL_BASE_STRINGFUNCTIONS_H
#define SIMUL_BASE_STRINGFUNCTIONS_H
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:

#endif
#include <string>
#include "Simul/Base/Export.h"
namespace simul
{
	namespace base
	{
		extern SIMUL_BASE_EXPORT bool GetDelimited(const std::string &str,std::string &First,std::string &Remainder,char delim);
		extern SIMUL_BASE_EXPORT std::string ExtractDelimited(std::string &str,char delim);
		extern SIMUL_BASE_EXPORT std::string ExtractDelimited(std::string &str,const std::string &delim);
		extern SIMUL_BASE_EXPORT std::string ExtractDelimited(std::string &str,char delim,bool match_brackets);
		extern SIMUL_BASE_EXPORT std::string ExtractDelimited(std::string &str,const std::string &delim,bool match_brackets);
		extern SIMUL_BASE_EXPORT bool GetDelimited(const std::string &str,std::string &First,std::string &Remainder,const std::string &delim);
		   
		extern SIMUL_BASE_EXPORT bool NextSettingFromTag(std::string &Line,std::string &Name,std::string &Value);
		extern SIMUL_BASE_EXPORT void StartXMLWrite(std::string &out);
		extern SIMUL_BASE_EXPORT std::string NextName(const std::string &Line);
		extern SIMUL_BASE_EXPORT void StripDirectoryFromFilename(std::string &str);
		extern SIMUL_BASE_EXPORT std::string NextName(const std::string &Line,size_t &idx);
		extern SIMUL_BASE_EXPORT std::string ExtractNextName(std::string &Line);
		extern SIMUL_BASE_EXPORT void ClipWhitespace(std::string &Line);
		extern SIMUL_BASE_EXPORT int NextInt(const std::string &Line,size_t &idx);
		extern SIMUL_BASE_EXPORT int NextInt(const std::string &Line);
		extern SIMUL_BASE_EXPORT bool NextBool(const std::string &Line);
		extern SIMUL_BASE_EXPORT const float *NextVector3(const std::string &Line);
		extern SIMUL_BASE_EXPORT const float *NextVector2(const std::string &Line);
		extern SIMUL_BASE_EXPORT float NextFloat(const std::string &Line,size_t &idx);
		extern SIMUL_BASE_EXPORT float NextFloat(const std::string &Line);
		extern SIMUL_BASE_EXPORT float NextSpeed(const std::string &Line,size_t &idx);
		extern SIMUL_BASE_EXPORT float NextTorque(const std::string &Line);
		extern SIMUL_BASE_EXPORT float AsPhysicalValue(const std::string &Line);
		extern SIMUL_BASE_EXPORT float NextPower(const std::string &Line);
		extern SIMUL_BASE_EXPORT float NextTorque(const std::string &Line,size_t &idx);
		extern SIMUL_BASE_EXPORT float NextPower(const std::string &Line,size_t &idx);
		extern SIMUL_BASE_EXPORT bool FindXMLToken(const std::string &in,size_t &pos0);
		extern SIMUL_BASE_EXPORT bool FindXMLToken(const std::string &in,size_t &pos0,std::string &name,std::string &setting,std::string &contents);      
		extern SIMUL_BASE_EXPORT void WriteFloat(std::string &out,float setting);
		extern SIMUL_BASE_EXPORT void WriteXMLFloat(std::string &out,const std::string &name,float setting);
		extern SIMUL_BASE_EXPORT void WriteXMLFloatInUnits(std::string &out,const std::string &name,float setting,const std::string &Units);
		extern SIMUL_BASE_EXPORT void WriteXMLOpen(std::string &out,const std::string &name);
		extern SIMUL_BASE_EXPORT void WriteXMLOpen(std::string &out,const std::string &name,const std::string &setting);
		extern SIMUL_BASE_EXPORT void WriteXMLClose(std::string &out,const std::string &name);
		extern SIMUL_BASE_EXPORT void WriteXMLTokenWithSetting(std::string &out,const std::string &name,const std::string &setting);
		extern SIMUL_BASE_EXPORT void WriteXMLToken(std::string &out,const std::string &name);
		extern SIMUL_BASE_EXPORT void WriteXMLTokenWithFloatSetting(std::string &out,const std::string &name,float setting);
		extern SIMUL_BASE_EXPORT void WriteXMLInt(std::string &out,const std::string &name,int setting);
		extern SIMUL_BASE_EXPORT void WriteXMLBoolean(std::string &out,const std::string &name,bool setting);
		extern SIMUL_BASE_EXPORT void WriteXMLVector(std::string &out,const std::string &name,const float *v);
		extern SIMUL_BASE_EXPORT void WriteXMLTokenIfNonZero(std::string &out,const std::string &name,float setting);
		extern SIMUL_BASE_EXPORT void WriteXMLTorque(std::string &out,const std::string &name,float setting);   
		extern SIMUL_BASE_EXPORT void WriteXMLPower(std::string &out,const std::string &name,float setting);
		extern SIMUL_BASE_EXPORT void WriteXMLVector(std::string &out,const std::string &name,float s1,float s2,float s3);
		extern SIMUL_BASE_EXPORT void GetXMLSetting(std::string &setting,std::string &Value,const std::string &Name);
		extern SIMUL_BASE_EXPORT size_t GoToLine(const std::string &Str,size_t line);
		extern SIMUL_BASE_EXPORT std::string StringOf(int i);
		extern SIMUL_BASE_EXPORT std::string StringOf(float i);
		extern SIMUL_BASE_EXPORT std::string stringof(int i);
		extern SIMUL_BASE_EXPORT std::string stringof(float f);
		// for streaming
		extern SIMUL_BASE_EXPORT std::string MakeXMLOpen(const std::string &name);
		extern SIMUL_BASE_EXPORT std::string MakeXMLClose(const std::string &name);
		extern SIMUL_BASE_EXPORT std::string MakeXMLToken(const std::string &name,const std::string &value);
		extern SIMUL_BASE_EXPORT std::string MakeXMLToken(const std::string &name,int value);
		extern SIMUL_BASE_EXPORT std::string MakeXMLToken(const std::string &name,float value);
	}
}
#endif

