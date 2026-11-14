// parameter file parser

#include "wpch.hpp"
#include "paramFileExt.hpp"

#include <El/QStream/QBStream.hpp>

//#include "strIncl.hpp"
#include "stringtableExt.hpp"
#include "fileLocator.hpp"
#include "mbcs.hpp"

#ifndef ACCESS_ONLY
#include <El/ParamArchive/paramArchive.hpp>
#endif

FontID GetFontID( RString baseName )
{
	int langID = English;
	const ParamEntry *cls = (Pars >> "CfgFonts").FindEntry(GLanguage);
	if (cls)
	{
		const ParamEntry *entry = cls->FindEntry(baseName);
		if (entry)
		{
			baseName = *entry;
			langID = GetLangID();
		}
	}
	return FontID(GetDefaultName(baseName,"fonts\\",""), langID);
}

PackedColor GetPackedColor(const ParamEntry &entry)
{
	if (entry.GetSize() != 4)
	{
		// ErrorMessage("Config: '%s' not an ARGB color",(const char *)GetName());
		return PackedWhite;
	}
	int r8 = toInt(entry[0].GetFloat() * 255);
	int g8 = toInt(entry[1].GetFloat() * 255);
	int b8 = toInt(entry[2].GetFloat() * 255);
	int a8 = toInt(entry[3].GetFloat() * 255);
	saturate(r8, 0, 255);
	saturate(g8, 0, 255);
	saturate(b8, 0, 255);
	saturate(a8, 0, 255);
	return PackedColor(r8, g8, b8, a8);
}

Color GetColor(const ParamEntry &entry)
{
	if (entry.GetSize() != 4)
	{
		//ErrorMessage("Config: '%s' not an ARGB color",(const char *)GetName());
		return HWhite;
	}
	return Color(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat(), entry[3].GetFloat());
}

bool GetValue(SoundPars &pars, const ParamEntry &entry)
{
	if (entry.GetSize() < 3)
	{
		const static SoundPars nil;
		//ErrorMessage("Config: '%s' not sound parameters.",(const char *)GetName());
		pars = nil;
		return false;
	}
	pars.name = GetSoundName(entry[0]);
	pars.vol = entry[1].GetFloat();
	pars.freq = entry[2].GetFloat();
	pars.freqRnd = 0;
	pars.volRnd = 0.05;
	return true;
}

bool GetValue(SoundPars &pars, const IParamArrayValue &entry)
{
	if (entry.GetItemCount() < 3)
	{
		const static SoundPars nil;
		//ErrorMessage("Config: '%s' not sound parameters.",(const char *)GetName());
		pars = nil;
		return false;
	}
	pars.name = GetSoundName(entry[0]);
	pars.vol = entry[1].GetFloat();
	pars.freq = entry[2].GetFloat();
	pars.freqRnd = 0;
	pars.volRnd = 0.05;
	return true;
}

bool GetValue(PackedColor &val, const ParamEntry &entry)
{
	if (entry.GetSize() != 4)
	{
		val = PackedWhite;
		return false;
	}
	val = PackedColor(Color(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat(), entry[3].GetFloat()));
	return true;
}

bool GetValue(Color &val, const ParamEntry &entry)
{
	if (entry.GetSize() != 4)
	{
		val = HWhite;
		return false;
	}
	val = Color(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat(), entry[3].GetFloat());
	return true;
}


#ifndef ACCESS_ONLY
LSError SoundPars::Serialize(ParamArchive &ar)
{
	CHECK(ar.Serialize("name", name, 1))
	CHECK(ar.Serialize("vol", vol, 1))
	CHECK(ar.Serialize("freq", freq, 1))
	CHECK(ar.Serialize("volRnd", volRnd, 1))
	CHECK(ar.Serialize("freqRnd", freqRnd, 1))
	return LSOK;
}
#endif

/*!
\patch 1.21 Date 08/21/2001 by Jirka
- Added: allow new worlds in AddOns
*/
RString GetWorldName( RString baseName )
{
#if _FORCE_DEMO_ISLAND
	RString demo = Pars >> "CfgWorlds" >> "demoWorld";
	if (stricmp(baseName, demo) != 0)
		WarningMessage(LocalizeString(IDS_MSG_NO_WORLD), (const char *)baseName);
	const ParamEntry *entry = (Pars >> "CfgWorlds").FindEntry(demo);
#else
	const ParamEntry *entry = (Pars >> "CfgWorlds").FindEntry(baseName);
	if (!entry)
	{
		baseName = Pars >> "CfgWorlds" >> "initWorld";
		entry = (Pars >> "CfgWorlds").FindEntry(baseName);
	}
#endif
	RString world = (*entry) >> "worldName";
	return FindFile(world,"worlds\\",".wrp");
}

static RString PathFirstFolder(const char *path)
{
	const char *next = strchr(path,'/');
	if (!next) return "";
	return RString(path,next-path);
}

const ParamEntry *FindConfigParamEntry(const char *path)
{
	if (*path==0)
	{
		return NULL;
	}
	// check path base
	const ParamEntry *entry = NULL;
	RString base = PathFirstFolder(path);
	if (!strcmpi(base,"cfg"))
	{
		entry = &Pars;
	}
	else if (!strcmpi(base,"rsc"))
	{
		entry = &Res;
	}
	else
	{
		LogF("Invalid base in %s",path);
		return NULL;
	}
	path += strlen(base)+1;

	while (strlen(path)>0)
	{
		RString base = PathFirstFolder(path);
		if (base.GetLength()<=0)
		{
			ParamEntry *nEntry = entry->FindEntry(path);
			if (!nEntry) break;
			if (!nEntry->IsClass()) break;
			entry = nEntry;
			break;
		}
		else
		{
			ParamEntry *nEntry = entry->FindEntry(base);
			if (!nEntry) break;
			if (!nEntry->IsClass()) break;
			entry = nEntry;
			path += strlen(base)+1;
		}
	}
	return entry;
}

/*!
\patch 1.12 Date 08/06/2001 by Ondra
- Added: MP, Anticheat: Config integrity verification.
*/
unsigned int CalculateConfigCRC(const char *path)
{
	// check path base
	SRef<SumCalculator> sum = ParamFile::CreateSumCalculator();
	DoAssert(sum);

	const ParamEntry *entry = FindConfigParamEntry(path);
	if (!entry)
	{
		sum->Reset();
		Pars.CalculateCheckValue(*sum);
		Res.CalculateCheckValue(*sum);
		return sum->GetResult();
	}

	sum->Reset();
	entry->CalculateCheckValue(*sum);
	return sum->GetResult();
}
