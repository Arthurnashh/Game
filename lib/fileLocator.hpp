#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FILE_LOCATOR_HPP
#define _FILE_LOCATOR_HPP

#include <Es/Strings/rString.hpp>
#include <Es/Containers/forEach.hpp>

//! add mod to the list
void AddMod(RString add);

//! get readable mod list
RString GetModListShort();

//! mod directory callback
typedef bool ModDirectoryCallback(RString dir, void *context);

//! call given function for all mod directories
bool EnumModDirectories(ModDirectoryCallback callback, void *context);

struct ModBankInfo
{
  RString prefix;
  RString path;
  ModBankInfo() : prefix(), path() {}
  ModBankInfo(RString pf, RString pt) : prefix(pf), path(pt) {}
};
TypeIsMovableZeroed(ModBankInfo)

struct ModInfo
{
  RString fullPath; //full path to mod directory, where the mod was found (or empty)
  RString modDir;  //CA, expansion, ...
  RString name;    //name to be shown (ArmA 2 instead of CA, etc.)
  RString action;  //url (or script?) to be triggered when mod button is clicked
  RString actionName; //what to put on Action Button
  RString picture; //picture to be shown in Mod Launcher
  RString hash;    //hash computed from the mod directory content (all pbos, configs, string tables and other files are processed)
  bool    hide;    //this is NOT MOD at all in fact, remove it from ModLauncher list
  bool    active;  //active mod (activated through command line or stored in profile by mod launcher)
  bool    disabled; //false when mod was Clicked to be Enabled
  bool    parsed;   //config.cpp was parsed to get mod's name, picture, action etc.
  bool    hidePicture; //do not show mod picture icon in the main menu mod list
  bool    hideName;    //do not show mod name in the main menu mod list
  bool    cannotChange; //true when the modDir is the same as for another mod but in different place
  bool    cannotDisable; //true when the mod should be always enabled
  bool    defaultMod;   //default mods cannot be moved or disabled by ModLauncher
  RString loadBefore; //loadBefore value read from registry
  enum ModOrigin
  {
    ModInConfig,  //mod in core mods config
    ModGameDir,
    ModOtherDir,
    ModInRegistry,
    ModNotFound,
    ModDefault
  } origin;  //location where was the mod found
  RString regName; //(used only for Registry mods)
  AutoArray<RString> loadAfter; //list of regNames (used only for Registry mods)
  AutoArray<RString> laDep; //initially the same as loadAfter, but modified during SortByLoadAfterDependencies
  FindArray<RString> require, requiredBy; //list of regNames (used only for Registry mods) - which cannot be disabled when I am enabled
  int modLauncherIx; //help variable to sort mods and update the order in ModLauncher

  // prefixes and paths of loaded banks
  AutoArray<ModBankInfo> loadedBanks;

  RString GetName() const { return (name.IsEmpty() ? modDir : name); }
  ModInfo() : active(false), disabled(false), parsed(false), hidePicture(false), hideName(false), cannotChange(false), origin(ModInConfig), defaultMod(false), modDir(""), hide(false), cannotDisable(false) {}
  ModInfo(RString dir) : modDir(dir), active(false), disabled(false), hidePicture(false), hideName(false), cannotChange(false), origin(ModInConfig), parsed(false), defaultMod(false), hide(false), cannotDisable(false) {}

  bool ModDirDrivenMod() const
  {
    return (origin==ModNotFound) || (origin==ModInConfig) || (origin==ModInRegistry) || (origin==ModDefault);
  }

  // parse semicolon separated values into loadAfter RString array
  void ParseLoadAfter(RString loadAfterValue);
  // true when this is greater, meaning it must be below the smaller mod in the ModLauncher list
  bool operator > (const ModInfo &mod) const;
  // helper function to sort mods using LoadAfter
  void Resolved(const ModInfo &dep);
  // registry value REQUIRED can contain list of mods which must be enabled when I am enabled
  void ParseRequired(RString requiredValue);
public:
  // helper variable for some recursive maintanance
  bool _processed;
};
TypeIsMovableZeroed(ModInfo)

class ModInfos : public AutoArray<ModInfo>, private NoCopy
{
public:
  typedef AutoArray<ModInfo> base;

  // true when there was command line parameter -mod=xxx (affects ModLauncher dialog to be ReadOnly)
  bool modsReadOnly;
  // true when VK_LSHIFT was pressed during game start phase (do not load mods from flashpoint cfg config)
  bool safeModsActivated;
  // hash of all mods hashes (computed in function void GetModsContentHash())
  RString hash;

  ModInfos() : modsReadOnly(false), safeModsActivated(false) {}

  // report the mods information - LogF 
  void Report();

  int Add(const ModInfo &info, bool reverseOrder=false)
  {
    // add only new mod
    for (int i=0; i<Size(); i++)
    {
      if ( ( !operator[](i).fullPath.IsEmpty() && !stricmp(operator[](i).fullPath, info.fullPath) ) //fullPaths present and equal
        || ( operator[](i).ModDirDrivenMod() && info.ModDirDrivenMod() && !stricmp(info.modDir,operator[](i).modDir) ) //config mods with equal modDir
        ) 
      return -1; //not added
    }
    if (reverseOrder)
    {
      base::Insert(0, info);
      return 0;
    }
    else return base::Add(info);
  }
  int AddRegMod(const ModInfo &info, bool reverseOrder=false)
  {
    // mods with the same fullPath are possible (example: "ArmA 2" and "Arma 2 OA Common")
    if (reverseOrder)
    {
      base::Insert(0, info);
      return 0;
    }
    else return base::Add(info);
  }
  // possibly moves the mod on modIx position to the position complying LOADAFTER order
  void ParseLoadBefore(RString modName, RString loadBeforeValue);
  // sort mods by dependencies given by LoadAfter registry keys
  void SortByLoadAfterDependencies();

  void ProcessRequire(int modIx); //called recursively
  void UpdateRequired();

  void Copy(const AutoArray<ModInfo> &src)
  {
    AutoArray<ModInfo>::operator = (src);
  }

  private:
  ModInfos(const ModInfos &src);
  ModInfos &operator =( const ModInfos &src);
};
/// final mod list as used in the game
extern ModInfos GModInfos;

/// mods sources from -beta command line
extern ModInfos GModInfosBeta;
/// mods sources from the .cfg file (ModLauncherList)
extern ModInfos GModInfosProfile;
/// mods sources from the registry
extern ModInfos GModInfosRegistry;

template <class Callback, class Context>
bool ForEachModDirectory(Callback callback, Context &context)
{
  for (int i=0; i<GModInfos.Size(); i++) //mods are stored in reverse order already
  {
    if ( callback( &GModInfos[i], context ) ) return true;
  }
  return callback(NULL, context);
}

//! make full relative path when default path and extension is given
RString GetDefaultName(RString baseName, const char *dExt);

//! make full relative path when default path and extension is given
//! search for an existing file in all mods
RString FindFile(RString baseName, const char *dExt);

RString GetAnimationName( RString baseName );
RString GetShapeName( RString baseName );
RString GetPictureName( RString baseName );
RString GetSoundName( RString baseName );

struct ModPathTraits
{
  typedef RString Container;

  typedef const char *Iterator;
  typedef RString ItemType;

  static Iterator Start(const RString &c) {return c;}
  static void Advance(const Container &c, Iterator &it)
  {
    it = strchr(it,';');
    if (it) it++;
  }
  static bool NotEnd(const Container &c, Iterator it) {return it!=NULL;}

  static ItemType Select(const Container &c, Iterator it)
  {
    const char *ptr = strchr(it,';');
    if (!ptr) return it;
    return RString(it,ptr-it);
  }
};

//! call given function for all mod directories in the given string
template <class Callback, class Context>
bool EnumModDirectories(RString modPath, Callback callback, Context &context)
{
  if (modPath.GetLength() > 0)
  {
    bool found = ForEach<ModPathTraits>(modPath,callback,context);
    if (found) return true;
  }
  return false;
}

extern RString ConvertToHex(const char *data, int size, bool useLowerCase=true);
extern bool ParseModConfig(RString modFilePath, ModInfo &mod);
extern void LoadCfgModInfo(ModInfo &modInfo);
extern void  UpdateModsHashes();
extern void  GetModsContentHash();

#endif

