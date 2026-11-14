// parameter file parser

#include "wpch.hpp"
#include "paramFileExt.hpp"

#include <El/QStream/qbStream.hpp>
#include <Es/ErrorProp/errorProp.hpp>

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
  ConstParamEntryPtr cls = (Pars >> "CfgFonts").FindEntry(GLanguage);
  if (cls)
  {
    ConstParamEntryPtr entry = cls->FindEntry(baseName);
    if (entry)
    {
      baseName = *entry;
      langID = GetLangID();
    }
  }
//  return FontID(GetDefaultName(baseName, ""), langID);
  return FontID(baseName, langID);
}

PackedColor GetPackedColor(ParamEntryPar entry)
{
  if (entry.GetSize() != 4)
  {
    if (entry.GetSize() == 3)
    {
      int r8 = toInt(entry[0].GetFloat() * 255);
      int g8 = toInt(entry[1].GetFloat() * 255);
      int b8 = toInt(entry[2].GetFloat() * 255);
      saturate(r8, 0, 255);
      saturate(g8, 0, 255);
      saturate(b8, 0, 255);
      return PackedColor(r8, g8, b8, 255);
    }

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

Color GetColor(ParamEntryVal entry)
{
  if (entry.GetSize() != 4)
  {
    if (entry.GetSize() == 3)
    {
      return Color(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat(), 1);
    }

    //ErrorMessage("Config: '%s' not an ARGB color",(const char *)GetName());
    return HWhite;
  }
  return Color(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat(), entry[3].GetFloat());
}

static const float ReferenceDistance = 1000;
static const float InvReferenceDistance = 0.001;

bool GetValue(SoundPars &pars, ParamEntryPar entry)
{
  if (entry.GetSize() < 3)
  {
    const static SoundPars nil;
    //ErrorMessage("Config: '%s' not sound parameters.",(const char *)GetName());
    pars = nil;
    return false;
  }
  ERROR_TRY()
    pars.name = GetSoundName(entry[0]);
    pars.vol = entry[1].GetFloat();
    pars.freq = entry[2].GetFloat();
    pars.freqRnd = 0;
    pars.volRnd = 0.05;
    if (entry.GetSize() >= 4)
    {
      pars.distance = entry[3].GetFloat();
      pars.vol = pars.distance * InvReferenceDistance;
    }
    else
    {
      pars.distance = pars.vol * ReferenceDistance;
    }
  ERROR_CATCH_ANY(exc)
    LogF("Error loading sound %s",cc_cast(entry.GetContext()));
  ERROR_END()
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
  if (entry.GetItemCount() >= 4)
  {
    pars.distance = entry[3].GetFloat();
    pars.vol = pars.distance * InvReferenceDistance;
  }
  else
  {
    pars.distance = pars.vol * ReferenceDistance;
  }
  pars.freqRnd = 0;
  pars.volRnd = 0.05;
  return true;
}

bool GetValue(PackedColor &val, ParamEntryPar entry)
{
  if (entry.GetSize() != 4)
  {
    if (entry.GetSize() == 3)
    {
      val = PackedColor(Color(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat(), 1));
      return true;
    }
    val = PackedWhite;
    return false;
  }
  val = PackedColor(Color(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat(), entry[3].GetFloat()));
  return true;
}

/*!
\patch_internal 2.01 Date 1/21/2003 by Ondra
- New: Colors in config files can be 3 entries only (alpha=1 assumed)
*/

bool GetValue(Color &val, ParamEntryPar entry)
{
  if (entry.GetSize() != 4)
  {
    if (entry.GetSize() == 3)
    {
      val = Color(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat(), 1);
      return true;
    }
    val = HWhite;
    return false;
  }
  val = Color(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat(), entry[3].GetFloat());
  return true;
}

bool GetValue(Color &val, const IParamArrayValue &entry)
{
  if (entry.GetItemCount() != 4)
  {
    if (entry.GetItemCount() == 3)
    {
      val = Color(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat(), 1);
      return true;
    }
    val = HWhite;
    return false;
  }
  val = Color(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat(), entry[3].GetFloat());
  return true;
}

bool GetValue(Vector3 &val, ParamEntryPar entry)
{
  if (entry.GetSize() != 3)
  {
    val = VZero;
    return false;
  }
  val = Vector3(entry[0].GetFloat(), entry[1].GetFloat(), entry[2].GetFloat());
  return true;
}

void GetHDRColor(Color &rgb, const IParamArrayValue &color)
{
  // if it is direct color, read it directly
  if (color.GetItemCount()==2 && color[0].IsArrayValue())
  {
    /// first part is RGB color
    GetValue(rgb,color[0]);
    // normalize - intensity is given separately
    // consider: no normalization, intensity is multiplied
    float brightness = rgb.Brightness();

    float lv = color[1];
    // convert light value into the brightness 
    const float ln2 = 0.69314718056f;
    float b = exp(ln2*lv);

    rgb = rgb *(b/brightness);
  }
  else
  {
    GetValue(rgb,color);
  }
}

void GetHDRColor(Color &rgb, ParamEntryPar entry)
{
  if (entry.IsArray() && entry.GetSize() == 2 && entry[0].IsArrayValue())
  {
    /// first part is RGB color
    GetValue(rgb,entry[0]);
    // normalize - intensity is given separately
    // consider: no normalization, intensity is multiplied
    float brightness = rgb.Brightness();

    if (fabsf(brightness) < 0.001)
      rgb = Color(0,0,0,1);
    else
    {
      float lv = entry[1];
      // convert light value into the brightness 
      const float ln2 = 0.69314718056f;
      float b = exp(ln2*lv);

      rgb = rgb *(b/brightness);
    }
  }
  else
  {
    GetValue(rgb, entry);
  }
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
  ParamEntryVal cfgWorlds = Pars >> "CfgWorlds";
#if _FORCE_DEMO_ISLAND
  RString demo = cfgWorlds >> "demoWorld";
  if (stricmp(baseName, demo) != 0)
    WarningMessage(LocalizeString(IDS_MSG_NO_WORLD), (const char *)baseName);
  ConstParamEntryPtr entry = cfgWorlds.FindEntry(demo);
#else
  ConstParamEntryPtr entry = cfgWorlds.FindEntry(baseName);
  if (!entry)
  {
    baseName = cfgWorlds >> "initWorld";
    entry = cfgWorlds.FindEntry(baseName);
    if (!entry)
    {
      Fail("No world found");
      return "";
    }
  }
#endif
  RString world = (*entry) >> "worldName";
  return FindFile(world, ".wrp");
}

static RString PathFirstFolder(const char *path)
{
  const char *next = strchr(path,'/');
  if (!next) return "";
  return RString(path,next-path);
}

ConstParamEntryPtr FindConfigParamEntry(const char *path)
{
  if (*path==0)
  {
    return ConstParamEntryPtr();
  }
  // check path base
  const ParamFile *entry = NULL;
  RString base = PathFirstFolder(path);
  if (!strcmpi(base,"cfg"))
  {
    entry = &Pars;
  }
/*
  else if (!strcmpi(base,"rsc"))
  {
    entry = &Pars;
  }
*/
  else
  {
    LogF("Invalid base in %s",path);
    return ConstParamEntryPtr();
  }
  path += strlen(base)+1;

  return entry->FindEntryPath(path);
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

  ConstParamEntryPtr entry = FindConfigParamEntry(path);
  if (!entry)
  {
    sum->Reset();
    Pars.CalculateCheckValue(*sum);
    //Res.CalculateCheckValue(*sum);
    return sum->GetResult();
  }

  sum->Reset();
  entry->CalculateCheckValue(*sum);
  return sum->GetResult();
}
