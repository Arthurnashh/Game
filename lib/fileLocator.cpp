#include "wpch.hpp"
#include "fileLocator.hpp"
#include <Es/Containers/forEach.hpp>
#include <El/ParamFile/paramFile.hpp>
#include <El/QStream/qbStream.hpp>

ModInfos GModInfos;
ModInfos GModInfosBeta;
ModInfos GModInfosProfile;
ModInfos GModInfosRegistry;

RString GetDefaultName( RString baseName, const char *dExt )
{
	if( !baseName || !baseName[0] ) return "";
	// check for "procedural" or other symbolic names
	if (baseName[0]=='#') return baseName;

  BString<256> buf;
	if (baseName[0] != '\\')
		strcpy(buf, baseName);
	else
		strcpy(buf, (const char *)baseName + 1);

  // fix: directory can contain '.' as well, look only for '.' in the file name
  const char *filename = strrchr(buf, '\\');
  if (!filename) filename = buf;

  const char *ext = strchr(filename, '.');
  if (!ext) strcat(buf, dExt);
	
  strlwr(buf);
	return cc_cast(buf);
}

/*!
\patch_internal 1.50 Date 4/12/2002 by Jirka
- Added: support for mods
\patch 1.51 Date 4/16/2002 by Jirka
- Added: Additional program parameter: -mod
*/

#ifndef _MOD_PATH_DEFAULT
#define _MOD_PATH_DEFAULT ""
#endif

// used only for EnumModDirectories purposes, contains only default and -mod=xxx mods
RString ModPath = _MOD_PATH_DEFAULT;

template <class Callback, class Context>
bool EnumModDirectories(Callback callback, Context &context)
{
  if (EnumModDirectories(ModPath,callback,context)) return true;
  return callback("",context);
}

void AddMod(RString add)
{
  //the same mod can be added twice (they are processed in the reverse order, later added first)
  //this is handy, when we need to process some _MOD_PATH_DEFAULT mods before other
  if (ModPath.GetLength() == 0) ModPath = add;
	else ModPath = ModPath + RString(";") + add;
}

static bool ModList2ArrayCallback(RString dir, AutoArray<RString> &modList)
{
  const char *lastDir = strrchr(dir, '\\');
  if (lastDir!=NULL) dir = lastDir+1;
  lastDir = strrchr(dir, '/');
  if (lastDir!=NULL) dir = lastDir+1;
  if (dir.GetLength())
  {
    dir.Trim();
    modList.Add(dir);
  }
  return false; //continue enumeration
}

static bool GetModListShortCallback(ModInfo *mod, AutoArray<RString> &modList)
{
  if (mod)
  {
    modList.Add(mod->modDir);
  }
  return false; //continue enumeration
}

//! get mod list, using only last directory name, not full path
RString GetModListShort()
{
  AutoArray<RString> modList;
  ForEachModDirectory(GetModListShortCallback, modList);
  RString modListRetVal;
  for (int i=modList.Size()-1; i>=0; i--)
  {
    if (i)
      modListRetVal = modListRetVal + RString(";") + modList[i];
    else
      modListRetVal = modList[i];
  }
  return modListRetVal;
}

void GetModListShortArray(AutoArray<RString> &modListAr)
{
  ForEachModDirectory(GetModListShortCallback, modListAr);
}

void ModList2Array(RString mods, AutoArray<RString> &modListAr)
{
  ForEach<ModPathTraits>(mods,ModList2ArrayCallback,modListAr);
}

// helper function to get name, picture and other data from possible mod.cpp (or other) file
bool ParseModConfig(RString modFilePath, ModInfo &mod)
{
  if (!modFilePath.IsEmpty() && QIFileFunctions::FileExists(modFilePath))
  {
    mod.parsed = true;
    ParamFile config;
    config.Parse(modFilePath);
    ParamEntryPtr entry = config.FindEntry("name");
    if (entry) mod.name = entry->GetValue();
    entry = config.FindEntry("picture");
    if (entry) mod.picture = entry->GetValue();
    entry = config.FindEntry("action");
    if (entry) mod.action = entry->GetValue();
    entry = config.FindEntry("actionName");
    if (entry) mod.actionName = entry->GetValue();
    entry = config.FindEntry("hidePicture");
    if (entry) mod.hidePicture = *entry;
    entry = config.FindEntry("hideName");
    if (entry) mod.hideName = *entry;
    entry = config.FindEntry("hash");
    if (entry) mod.hash = *entry;
    entry = config.FindEntry("hide");
    if (entry) mod.hide = *entry;
    return true;
  }
  return false;
}

extern ParamFile Pars;
void LoadCfgModInfo(ModInfo &modInfo)
{
  ConstParamEntryPtr entry = Pars.FindEntry("CfgMods");
  if (entry) 
  {
    ParamEntryVal mods = Pars>>"CfgMods";
    if (mods.IsClass())
    {
      ConstParamEntryPtr modClass = mods.FindEntry(modInfo.modDir);
      if (modClass && modClass->IsClass())
      {
        if (modClass->FindEntry("name")) 
        { //when no name is present, the mod is not installed, not even LITE 
          modInfo.name = (*modClass) >> "name";
          modInfo.picture = (*modClass) >> "picture";
          modInfo.action = (*modClass) >> "action";
          entry = modClass->FindEntry("hidePicture");
          if (entry) modInfo.hidePicture = *entry;
          entry = modClass->FindEntry("hideName");
          if (entry) modInfo.hideName = *entry;
          entry = modClass->FindEntry("actionName");
          if (entry) modInfo.actionName = *entry;
          entry = modClass->FindEntry("hash");
          if (entry) modInfo.hash = *entry;
        }
        entry = modClass->FindEntry("default");
        if (entry) 
        {
          modInfo.defaultMod = *entry;
          if (modInfo.defaultMod) 
          {
            modInfo.active = true;
            modInfo.cannotDisable = true;
          }
        }
        if ( !modInfo.fullPath.IsEmpty() )
        { //informations inside mod.cpp are to be considered more important (fresh, updated)
          RString modConfig = modInfo.fullPath + RString("\\mod.cpp");
          ParseModConfig(modConfig, modInfo);
        }
      }
    }
  }
}

//@{ MOD HASH COMPUTING
#include <Es/Files/fileContainer.hpp>
#include <Es/Files/filenames.hpp>
#include <El/DataSignatures/dataSignatures.hpp>

// useLowerCase is default due to backward compatibility
RString ConvertToHex(const char *data, int size, bool useLowerCase)
{
  const char *sfPattern = useLowerCase ? "%02x" : "%02X";
  RString retval; retval.CreateBuffer(2*size+1);
  char *out = retval.MutableData();
  for (int i=0; i<size; i++)
  {
    sprintf(out, sfPattern, (unsigned char)data[i]);
    out++; out++;
  }
  *out=0; //zero terminate
  return retval;
}

#if 0
  #define HshLogF RptF
#else
  #define HshLogF NoLog
#endif


static bool GetFileHash(const FileItem &file, AutoArray<RString> &hashes)
{
  BString<1024> fullPath(file.path + file.filename);
  fullPath.UnixPath();
  HshLogF( "   GetFileHash ... %s", cc_cast(fullPath));
  if ( !file.directory ) // only files content
  { // hash filename and its content
    if ( !stricmp(GetFileExt(file.filename), ".pbo") )
    { // get the hash from .pbo file
#define FILE_HASH_USE_LOADED_BANKS 1
#if FILE_HASH_USE_LOADED_BANKS
    BankList::ReadAccess banks(GFileBanks);
    // Assume: the bank should be already loaded. Find it and get its hash.
    Temp<char> hash;
    for (int i=0; i<banks.Size(); i++)
    {
      HshLogF("           ... %s",cc_cast(banks[i].GetOpenName()));
      BString<1024> bankOpenName(banks[i].GetOpenName());
      bankOpenName.UnixPath();
      if (stricmp(fullPath,bankOpenName)==0) //case insensitive comparison must be used
      {
        HshLogF("    Getting hash of %s", cc_cast(fullPath));
        banks[i].GetHash(hash);
        break;
      }
    }
#elif 0
      // Use LoadBank to get bank hash
      QFBank bank; 
      bank.openFromFile(fullPath);
      bank.Load(); //<- Load() call is too slow (moreover the bank is to be loaded on different place!)
      Temp<char> hash;
      bank.GetHash(hash);
#else
      // Read the bank hash from the end of pbo file
      QFileSize size = QIFileFunctions::GetFileSize(fullPath);
      QIFStream file;
      const int ModHashSize = 20; // the length of mod
      file.open(fullPath, size-ModHashSize);
      Temp<char> hash; hash.Realloc(ModHashSize);
      if (!file.fail())
      {
        file.read(hash, ModHashSize);
        file.close();
      }
#endif
      RString hexHash = ConvertToHex(hash, hash.Size());
      HshLogF("     computed hex hash: %s", cc_cast(hexHash));
      hashes.Add( hexHash );
    }
    #define GET_OTHER_FILE_HASHES 0
    #if GET_OTHER_FILE_HASHES
    else if ( !stricmp(file.filename, "mod.cpp")!=0 )
    {} // skip - do not add mod.cpp
    else
    {
      DSHash hash;
      DataSignatures::GetHash(hash, fullPath);
      hashes.Add( fullPath + RString(" = ") + ConvertToHex(hash._content, hash._content.Size()) );
    }
    #endif
  }
  return false; // continue with enumeration
}

static char *modSubdirs[] = {"dta", "addons", "bin"};
static bool GetDirectoryHash(const FileItem &file, AutoArray<RString> &hashes)
{
  RString fullPath = file.path + file.filename;
  HshLogF("GetDirectoryHash %s", cc_cast(fullPath));
  if (file.directory)
  {
    for (int i=0; i<sizeof(modSubdirs)/sizeof(char *); i++)
    {
      if ( !stricmp(file.filename, modSubdirs[i]) )
      { // compute hashes of files inside
        ForEachFile(fullPath + RString("\\"), GetFileHash, hashes);
      }
    }
  }
  return false; // continue with enumeration
}

static int CompareRStringsCI(const RString *item1, const RString *item2)
{
  return stricmp(cc_cast(*item1), cc_cast(*item2));
}

static bool ComputeModContentHash(ModInfo *mod, int &dummy)
{
  if ( mod && mod->origin != ModInfo::ModNotFound && mod->hash.IsEmpty() )
  {
    AutoArray<RString> hashes;
    ForEachFile(mod->fullPath + RString("\\"), GetDirectoryHash, hashes);
    // directory traverse can possibly differ on different computers? Sort hashes first, before their combination to final MOD HASH
    QSort(hashes, CompareRStringsCI);
    // combine all computed hashes into final hash of hashes
    HashCalculator calculator;
    for (int i=0; i<hashes.Size(); i++)
    {
      calculator.Add(cc_cast(hashes[i]), hashes[i].GetLength());
    }
    AutoArray<char> hashkey;
    calculator.GetResult(hashkey);
    mod->hash = ConvertToHex(hashkey.Data(), hashkey.Size());
    HshLogF("MOD CONTENT HASH: %s", cc_cast(mod->hash));
  }
  return false; //continue enumeration
}

// update the GModInfos hash of all active mods using current hashes in GModInfos
void UpdateModsHashes()
{
  // the hashes are stored inside GModInfos[i].hash
  RString hashes;
  for (int i=0; i<GModInfos.Size(); i++)
  {
    if (i) hashes = hashes + RString(";") + GModInfos[i].hash;
    else hashes = GModInfos[i].hash;
  }
  GModInfos.hash = hashes;
}

// computes the hash of all content of each active mod with its directory found
void GetModsContentHash()
{
  int dummyctx;
  ForEachModDirectory(ComputeModContentHash, dummyctx);
  UpdateModsHashes();
}
//@} MOD HASH COMPUTATION

//! different directory used in Poseidon and PoseidonEx branches
bool EnumModDirectories(ModDirectoryCallback callback, void *context)
{
#if 0
  return callback("",context);
#else
	if (ModPath.GetLength() > 0)
	{
		Temp<char> buffer((const char *)ModPath, ModPath.GetLength() + 1);

		char *ptr;
		while (ptr = strrchr(buffer, ';'))
		{
			*ptr = 0;
			if (callback(ptr + 1, context)) return true;
		}
		if (callback((const char *)buffer, context)) return true;
	}
	return callback("", context);
#endif
}

/// initialize the list of default mods (when given in the product info)
void InitModList(RString list)
{
  ModPath = list;
}

void ModInfos::Report()
{
  LogF("Extensions list: (hash:'%s')", cc_cast(hash));
  for (int i=0; i<base::Size(); i++)
  {
    const ModInfo &info = base::Get(i);
    LogF("   %s: %s (%d,%d,[%s],%d,[%s])", cc_cast(info.modDir), cc_cast(info.GetName()), 
                                        info.defaultMod, info.loadAfter.Size(), cc_cast(info.loadBefore), info.laDep.Size(), cc_cast(info.regName));
  }
}

void ModInfo::ParseLoadAfter(RString loadAfterValue)
{
  // loadAfterValue contains semicolon separated list of regNames
  ModList2Array(loadAfterValue, loadAfter);
}

void ModInfos::ParseLoadBefore(RString modName, RString loadBeforeValue)
{
  AutoArray<RString> mods;
  ModList2Array(loadBeforeValue, mods);
  for (int i=0; i<mods.Size(); i++)
  {
    RString& mod = mods[i];
    for (int j=0; j<this->Size(); j++)
    {
      ModInfo &info = operator [](j);
      if ( stricmp(info.regName, mod)==0 )
      {
        info.loadAfter.Add(modName);
        break;
      }
    }
  }
}

void ModInfo::ParseRequired(RString requiredValue)
{
  // loadAfterValue contains semicolon separated list of regNames
  ModList2Array(requiredValue, require);
}

// called recursively
void ModInfos::ProcessRequire(int modIx)
{
  ModInfo &mod = operator[](modIx);
  if ( mod._processed ) 
    return; //stop recursion

  mod._processed = true;

  FindArray<RString> otherReq; //all req mods collected recursively
  for ( int i=0; i<mod.require.Size(); i++ )
  {
    RString &regName = mod.require[i];
    for ( int j=0; j<this->Size(); j++ )
    {
      if ( stricmp(operator[](j).regName, regName)==0 )
      { // the mod require another mod, resolve it recursively
        ProcessRequire(j);
        ModInfo &rMod = operator[](j);
        for (int k=0; k<rMod.require.Size(); k++)
        {
          otherReq.AddUnique(rMod.require[k]);
        }
        break;
      }
    }
  }
  // make mod.require to contain each mod only once
  for ( int i=0; i<otherReq.Size(); i++ )
  { // merge the lists
    mod.require.AddUnique(otherReq[i]);
  }
}

void ModInfos::UpdateRequired()
{
  // There is no need to remove all require entries which are not present in the mod list, as these MUST be present
  // We check the require mods are present and disable mods which are orphaned
  for (int i=0; i<this->Size(); i++)
  {
    ModInfo &mod = operator[](i);
    mod._processed = false;
    for (int j=0; j<mod.require.Size(); j++)
    {
      RString &regName = mod.require[j];
      bool found = false;
      for (int k=0; k<this->Size(); k++)
      {
        if (stricmp(operator[](k).regName,regName)==0) {found=true; break;}
      }
      if (!found) 
      {
        mod.disabled = mod.cannotChange = mod.cannotDisable = true; 
        mod.require.Delete(j--); //next processing would have problems otherwise
      }
    }
  }

  // resolve recursive require mods
/* Example:
  Registry input:
    CA  
    OA  ... require CA
    BAF ... require OA
    PMC ... require BAF
    Hinds ... require OA
  Means:
    CA
      OA
        BAF
           PMC
        Hinds
  We want to set: 
    CA ... requiredBy OA, BAF, PMC, Hinds
    OA ... requiredBy BAF, PMC, Hinds
           require CA
    PMC ... require BAF, OA, CA
    etc.
*/
  for (int i=0; i<this->Size(); i++)
  {
    ProcessRequire(i); //recursive
  }
  for (int i=0; i<this->Size(); i++)
  {
    ModInfo &mod = operator[](i);
    for (int j=0; j<mod.require.Size(); j++)
    { // add mod into dependant requiredBy lists
      RString &regNameJ = mod.require[j];
      for (int k=0; k<this->Size(); k++)
      {
        if ( stricmp(operator[](k).regName,regNameJ)==0 )
        {
          operator[](k).requiredBy.AddUnique(mod.regName);
          break;
        }
      }
    }
  }
}

bool ModInfo::operator > (const ModInfo &mod) const
{
  if ( mod.regName.IsEmpty() || regName.IsEmpty() ) return false; //not comparable
  for (int j=0; j<mod.loadAfter.Size(); j++)
  {
    if (stricmp(regName,mod.loadAfter[j])==0)
    { //this is inside mod.loadAfter list => this must be listed lower inside the ModLaucher list
      return true;
    }
  }
  return false;
}

void ModInfo::Resolved(const ModInfo &dep)
{
  for (int i=0; i<laDep.Size(); i++)
  {
    if (stricmp(laDep[i],dep.regName)!=0) continue;
    laDep.Delete(i--);
  }
}

void ModInfos::SortByLoadAfterDependencies()
{
  // Remove All loadAfter entries which are not present in the mod list
  for (int i=0; i<this->Size(); i++)
  {
    ModInfo &mod = operator[](i);
    for (int j=0; j<mod.loadAfter.Size(); j++)
    {
      RString &regName = mod.loadAfter[j];
      bool found = false;
      for (int k=0; k<this->Size(); k++)
      {
        if (stricmp(operator[](k).regName,regName)==0) {found=true; break;}
      }
      if (!found) 
      {
        mod.loadAfter.Delete(j--);
      }
    }
  }
  // preserve LoadAfter value and work with its copy in laDep array
  for (int i=0; i<this->Size(); i++)
  {
    operator[](i).laDep = operator[](i).loadAfter;
  }

  //Report();

  AutoArray<ModInfo> resolved;
  while(this->Size()>0)
  {
    // find any mods with resolved dependencies
    bool someResolved = false;
    // traverse array in reverse order to preserve current order for mods which are already sorted
    for (int i=this->Size()-1; i>=0; i--)
    {
      const ModInfo &dep = operator[](i);
      if (dep.laDep.Size()>0) continue;
      resolved.Add(dep);
      // removed this addon from all dependencies lists
      for (int j=0; j<this->Size(); j++)
      {
        operator[](j).Resolved(dep);
      }
      this->Delete(i);
      someResolved = true;
      break;
    }
    if (!someResolved)
    {
      // print some mod name
      RString failStr = Format("Circular registry mods dependency '%s'", cc_cast(operator[](0).name));
      Fail(cc_cast(failStr));
      break;
    }
  }
  // parse all resolved addon configs 
  // note: when there was circular dependency, the cycled mods are still 
  //       inside dependencies array, so all mods will be listed
  for (int i=resolved.Size()-1; i>=0; i--)
  {
    this->Add(resolved[i]);
  }
}

/*
bool Print(int a, void *ctx)
{
	LogF("%d",a);
	return false;
}

struct LinkedInt: public CLRefLink, public RefCount
{
	int val;
	LinkedInt(int v){val=v;}

};


bool PrintLinkedInt(const LinkedInt &a, void *ctx)
{
	LogF("%d",a.val);
	return false;
}

void Test()
{
	AutoArray<int> array;
	array.Add(1);
	array.Add(3);
	array.Add(2);
	array.Add(4);
	void *ctx = NULL;
	ForEachA(array,Print,ctx);

	CLRefList<LinkedInt> list;
	list.Insert(new LinkedInt(1));
	list.Insert(new LinkedInt(3));
	list.Insert(new LinkedInt(2));
	list.Insert(new LinkedInt(4));
	ForEachC(list,PrintLinkedInt,ctx);
}
*/

//! find file in mods callback context
struct FindFileContext
{
	RString relName; //!< [in] relative name of searched file
	RString modName; //!< [out] relative name of found file
};

//! find file in mods callback function
static bool FindFileCallback(RString dir, FindFileContext &ctx)
{
	// find bin directory
	// stringtable filename
	RString file = dir + RString("\\") + ctx.relName;
	if (QFBankQueryFunctions::FileExists(file))
	{
		ctx.modName = file;
		return true;
	}

	return false;
}

RString FindFile(RString baseName, const char *dExt)
{
	//Test();

	FindFileContext ctx;
	ctx.relName = GetDefaultName(baseName, dExt);
	
	if (baseName[0]=='\\')
	{
	  // if absolute path is given as baseName, do not search mods
	  return ctx.relName;
	}
	
	bool found = EnumModDirectories(FindFileCallback, ctx);
	if (found) return ctx.modName;
	LogF("File %s not found in any mod",(const char *)ctx.relName);
	return ctx.relName;
}

RString GetShapeName( RString baseName )
{
	return GetDefaultName(baseName, ".p3d");
}
RString GetAnimationName( RString baseName )
{
	return GetDefaultName(baseName, ".rtm");
}
RString GetPictureName( RString baseName )
{
	return GetDefaultName(baseName, ".paa");
}
RString GetSoundName( RString baseName )
{
	return GetDefaultName(baseName, ".wss");
}
