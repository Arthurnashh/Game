#include "wpch.hpp"

#include "vehicle.hpp"
#include <El/ParamFile/paramFile.hpp>
#include <Es/Framework/appFrame.hpp>
#include <Es/Files/filenames.hpp>
#include <El/FileServer/fileServer.hpp>
#include "allVehicles.hpp"
#include "world.hpp"
#include "stringtableExt.hpp"
#include <El/Common/perfProf.hpp>
#include "progress.hpp"
#include <lib/Network/voiceXA2.hpp>
#include <lib/Inventory/transmitter.hpp>
#include "Network/network.hpp"
#include <lib/Inventory/itemoptics.hpp>
#include <lib/Inventory/clothing.hpp>
#include <lib/Inventory/radio.hpp>
#include <lib/Inventory/map.hpp>

VehicleTypeBank::VehicleTypeBank()
{
  RegisterFreeOnDemandMemory(this);
}

VehicleTypeBank::~VehicleTypeBank()
{
}

void VehicleTypeBankTraits::ReuseCachedItem(EntityType *item)
{
  #if 0 // _ENABLE_CHEATS
  LogF("## Reused type %s",cc_cast(item->GetName()));
  #endif
}

EntityType *VehicleTypeBank::New(const RString &name, const RString &shapeName)
{
  if( !name || !*name ) return NULL; // no name - return NULL
  EntityTypeName typeId;
  typeId._className = name;
  typeId._shapeName = shapeName;

  //Log("VehicleTypeBank::New %s,%s",name,shapeName);
  
  return base::New(typeId);
}

RString LandscapeObjectName(const char *shape)
{
  char nameS[512];
  // extract shape name (no folder, no extension)
  GetFilename(nameS,shape);
  char *sp;
  while ((sp=strchr(nameS,' '))!=NULL)
  {
    *sp='_';
  }
  // typical name looks like:
  return "Land_"+RString(nameS);
}

static void ReportNoDirect(const char *shape, const char *act)
{
  RString name = LandscapeObjectName(shape);
  LogF("Found %s, expected %s",act,(const char *)name);
}

DEF_RSB(treehard);
DEF_RSB(treesoft);
DEF_RSB(bushhard);
DEF_RSB(bushsoft);

/// check if type (config) is needed based on class property

static bool CheckClassTyped(const RStringB &className)
{
  if (className.IsEmpty())
    return false;
  else if (className==RSB(treehard))
    return false;
  else if (className==RSB(treesoft))
    return false;
  else if (className==RSB(bushhard))
    return false;
  else if (className==RSB(bushsoft))
    return false;
  return true;
}

bool VehicleTypeBank::CheckPlainShapeLoaded( const char *shape, ShapeParameters pars )
{
  BString<256> lowName = shape;
  lowName.StrLwr();

  // when pars is wrong, we might have missed a shape which is actually loaded.
  // This causes some performance hit, but as such situation should be very rare, we do not care
  LODShape *shapeLoaded = Shapes.GetLoaded(lowName,pars);
  #if 0
  if (!shapeLoaded)
  {
    shapeLoaded = Shapes.GetLoaded(lowName,ShapeShadow);
    if (!shapeLoaded) shapeLoaded = Shapes.GetLoaded(lowName,ShapeShadow|ShapeNotAnimated);
    if (!shapeLoaded) shapeLoaded = Shapes.GetLoaded(lowName,ShapeShadow|ShapeReversed);
    if (!shapeLoaded) shapeLoaded = Shapes.GetLoaded(lowName,ShapeShadow|ShapeReversed|ShapeNotAnimated);
    if (shapeLoaded)
    {
      LogF("Bad guess for %s",shape);
    }
  }
  #endif
  if (shapeLoaded)
  {
    if (!CheckClassTyped(shapeLoaded->GetPropertyClass()))
    {
      // no class property implies no config
      // verify the results are consistent
      return true;
    }
  }
  return false;
}

//! find class based on the shape name
/**
@param pars advisory only. Used to optimize class searching.
*/
EntityType *VehicleTypeBank::FindShapeDirect( const char *shape, ShapeParameters pars )
{
  PROFILE_SCOPE(vtFSD);
  if (PROFILE_SCOPE_NAME(vtFSD).IsActive())
  {
    PROFILE_SCOPE_NAME(vtFSD).AddMoreInfo(shape);
  }
  // extract shape name (no folder, no extension)
  RString name = LandscapeObjectName(shape);
  // Find is not enough - we need New
  // we may need to load a new type (if it exists)
  return New(name,RString());  
}

bool VehicleTypeBank::Request(const char *name)
{
  PROFILE_SCOPE(vtReq);
  if (PROFILE_SCOPE_NAME(vtReq).IsActive())
  {
    PROFILE_SCOPE_NAME(vtReq).AddMoreInfo(name);
  }
  // make sure corresponding config class is ready
  return RequestEntry(name);
}

bool VehicleTypeBank::RequestShapeDirect(const char *shape)
{
  return Request(LandscapeObjectName(shape));
}

/// return value context
struct CheckTypeShapeRet
{
  EntityType *ret;
  
  CheckTypeShapeRet(){ret = NULL;}
  operator EntityType *() const {return ret;}
};

/// return value adaptor
template <class Condition>
class CheckTypeShapeFunc
{
  Condition _cond;
  CheckTypeShapeRet &_ret;
  
  public:
  explicit CheckTypeShapeFunc(Condition cond, CheckTypeShapeRet &ret):_cond(cond),_ret(ret)
  {}
  /// perform a functor on the item
  bool operator () (EntityType *type, VehicleTypeBankTraits::ContainerType *bank) const
  {
    if (_cond(type))
    {
      _ret.ret = type;
      return true;
    }
    return false;
  }
  bool operator () (EntityType *type) const
  {
    if (_cond(type))
    {
      _ret.ret = type;
      return true;
    }
    return false;
  }
};

/// creating function
template <class Condition>
CheckTypeShapeFunc<Condition> CheckTypeShape(Condition cond, CheckTypeShapeRet &ret)
{
  return CheckTypeShapeFunc<Condition>(cond,ret);
}

/// adaptor fixing one argument of the functor
template <class Functor,class Type1>
class FixArgument1F
{
  Functor _func;
  Type1 _arg1;

  public:
  FixArgument1F(Functor func, Type1 arg1):_func(func),_arg1(arg1) {}
  
  template <class ArgX>
  bool operator() (ArgX argX) const {return _func(argX,_arg1);}
};

/// creating function for FixArgument1F
template <class Functor,class Type>
FixArgument1F<Functor,Type> FixArgument(Functor func, Type type)
{
  return FixArgument1F<Functor,Type>(func,type);
}

/// adaptor fixing two arguments of the functor
template <class Functor,class Type1,class Type2>
class FixArgument2F
{
  Functor _func;
  Type1 _arg1;
  Type2 _arg2;

  public:
  FixArgument2F(Functor func, Type1 arg1, Type2 arg2):_func(func),_arg1(arg1),_arg2(arg2) {}
  
  template <class ArgX>
  bool operator() (ArgX argX) const {return _func(argX,_arg1,_arg2);}
};

/// creating function for FixArgument2F
template <class Functor, class Type1,class Type2>
FixArgument2F<Functor,Type1,Type2> FixArgument(Functor func, Type1 arg1, Type2 arg2)
{
  return FixArgument2F<Functor,Type1,Type2>(func,arg1,arg2);
}

static inline bool TypeShapeMatch(EntityType *type, const char *shape)
{
  if (type->IsAbstract()) return false;
  if (strcmpi(type->GetShapeName(),shape)) return false;
  ReportNoDirect(shape,type->GetName());
  return true;
}

/// check if name and simulation matches
static inline bool TypeShapeAndSimulationMatch(EntityType *type, const char *shape, const char *sim)
{
  if (type->IsAbstract()) return false;
  if (strcmpi(type->GetShapeName(),shape)) return false;
  if (strcmpi(type->_simName,sim)) return false;
  ReportNoDirect(shape,type->GetName());
  return true;
}


EntityType *VehicleTypeBank::FindShape( const char *shape )
{
  PROFILE_SCOPE(vtFS);
  EntityType *direct = FindShapeDirect(shape);
  if (direct) return direct;
  CheckTypeShapeRet ret;
  ForEachF(CheckTypeShape(FixArgument(TypeShapeMatch,shape),ret));
  return ret;
}

EntityType *VehicleTypeBank::FindShapeAndSimulation
(
  const char *shape, const char *sim
)
{
  PROFILE_SCOPE(vtFSS);
  EntityType *direct = FindShapeDirect(shape);
  if (direct) return direct;

  CheckTypeShapeRet ret;
  ForEachF(CheckTypeShape(FixArgument(TypeShapeAndSimulationMatch,shape,sim),ret));
  return ret; // no vehicle - return
}

float VehicleTypeBank::Priority() const
{
  return 0.02;
}

RString VehicleTypeBank::GetDebugName() const
{
  return "VehTypes";
}

void GlobalAlive();

DEF_RSB(scope);

void VehicleTypeBank::Init()
{
  CacheCommonEntries();
}

const int MaxPreloadScope = 0; // do not preload anything but abstract types
//const int MaxPreloadScope = 1; // preload abstract and protected types

/// when not preloading protected types we risk types may be created when loading a vehicle

void VehicleTypeBank::Preload()
{
  CacheCommonEntries();
  #if _ENABLE_REPORT
  int begMem = MemoryUsed();
  LogF("VehicleTypeBank::Preload beg - mem %d",begMem);
  #endif
  static const char *names[]=
  {
    "CfgVehicles", "CfgAmmo", "CfgNonAIVehicles", "CfgWeapons", "CfgMagazines"
  };
  for( int n=0; n<sizeof(names)/sizeof(*names); n++ )
  {
    const char *name=names[n];
    // scan for all vehicle types used as base (i.e. abstract ones)
    ParamEntryVal vehicles=Pars>>name;
    for( int i=0; i<vehicles.GetEntryCount(); i++ )
    {
      I_AM_ALIVE(); // this loop can take quite long
      if (GProgress) GProgress->Refresh();
      ParamEntryVal entry=vehicles.GetEntry(i);
      const ParamClass *eClass = entry.GetClassInterface();
      if (!eClass) continue;
      if( entry.FindEntry("vehicleClass") )
      {
        // do not preload sounds - sound does not meet EntityAIType requirements
        RString vehClass = entry >> "vehicleClass";
        if (stricmp(vehClass, "Sounds") == 0) continue;
        if (stricmp(vehClass, "Mines") == 0) continue;
      }
      // preload only abstract types
      int scope = 0;
      ConstParamEntryPtr scopeEntry = entry.FindEntry(RSB(scope));
      if (scopeEntry)
      {
        scope = scopeEntry->GetInt();
      }
      if (scope<=MaxPreloadScope)
      {
        Ref<EntityType> type = New(entry.GetName(),RString());
        // note: type will often be destroyed immediately
        // we are warming-up the cache only
      }
    }
  }
  // release all locks on any files (sound files of weapons)
  if (GSoundsys) GSoundsys->FlushBank(NULL);
  if (GFileServer) GFileServer->FlushBank(NULL);
  #if _ENABLE_REPORT
  int endMem = MemoryUsed();
  LogF("VehicleTypeBank::Preload end - mem %d",endMem);
  LogF("VehicleTypeBank::Preload mem %d",endMem-begMem);
  #endif
}

static void VerifyNoRefs(LLink<EntityType> &type, VehicleTypeBankTraits::ContainerType *array, void *ctx)
{
  if (type.NotNull())
  {
    LogF("Type %s not released yet, refcount %d",cc_cast(type->GetName()),type->RefCounter());
  }
}

/** base Clear does not work because of parent dependencies
*/
void VehicleTypeBank::Clear()
{
  //ForEach(UnbindParent);
  // we need to unbind parents for cached entries
  // note: unbinding may add another cached entry
  // destruction will certainly unbind
  ClearCache();
  
  // no Refs should exist now
  #if !_RELEASE
  ForEach(VerifyNoRefs);
  #endif
  
  base::Clear();

  ClearCache();
  
  _cfgVehicles.Free();
  _cfgAmmo.Free();
  _cfgNonAIVehicles.Free();
  _cfgWeapons.Free();
  _cfgMagazines.Free();
}

/*
/// function used to cleanup type bank organized as RefArray
static bool TypeCleanUp(EntityType *type, VehicleTypeBankTraits::ContainerType *container)
{
  if (type->RefCounter()==1)
  {
    // only one reference - type is never used
    // do not discard types with no config entry (internal types)
    const ParamClass *cfg = type->GetParamEntry();
    if (cfg)
    {
      // TODO: use only direct search
      ConstParamEntryPtr scopeEntry = cfg->FindEntry(RSB(scope));
      int scope = 0;
      if (scopeEntry)
      {
        scope = scopeEntry->GetInt();
      }
      if (scope>MaxPreloadScope)
      {
        Log("Discard %s",(const char *)type->GetName());
        const EntityTypeName &name = type->GetEntityTypeName();
        container->Remove(name);
      }
    }
  }
  return false; // always continue - process all items
}
*/

void VehicleTypeBank::CleanUp()
{
  // reduce the hash table if needed
  Optimize();
//  ForEachF(TypeCleanUp);
//  Compact();
}

void VehicleTypeBank::SimpleClear()
{
  base::Clear();
  GWorld->PreloadVehicles(false);
}


class EntityAITypePlain: public EntityAIType
{
  typedef EntityAIType base;

  public:
  EntityAITypePlain( ParamEntryPar param );
  ~EntityAITypePlain();

  bool AbstractOnly() const {return false;}
};

EntityAITypePlain::EntityAITypePlain(ParamEntryPar param)
:base(param)
{
}

EntityAITypePlain::~EntityAITypePlain()
{
}

static Ref<LODShape> TempShape(RString shapeName)
{
  if( shapeName.GetLength()>0 )
  {
    return Shapes.New(shapeName,false,true);
  }
  return NULL;
}

static Ref<LODShape> TempShape(const EntityType *type)
{
  return TempShape(type->GetShapeName());
}

VehicleTypeBankTraitsExt::VehicleTypeBankTraitsExt()
{
}

void VehicleTypeBankTraitsExt::CacheCommonEntries()
{
  if (!_cfgVehicles)
  {
    _cfgVehicles = (Pars>>"CfgVehicles").GetClassInterface();
    _cfgAmmo = (Pars>>"CfgAmmo").GetClassInterface();
    _cfgNonAIVehicles = (Pars>>"CfgNonAIVehicles").GetClassInterface();
    _cfgWeapons = (Pars>>"CfgWeapons").GetClassInterface();
    _cfgMagazines = (Pars>>"CfgMagazines").GetClassInterface();
    
    if (_cfgVehicles)
    {
      if (!_cfgVehicles->IsFindOptimized())
      {
        RptF("Warning: CfgVehicles missing in PreloadConfig - may slow down vehicle creation");
      }
      _cfgVehicles->OptimizeFind();
    }
    if (_cfgAmmo)
    {
      if (!_cfgAmmo->IsFindOptimized())
      {
        RptF("Warning: CfgAmmo missing in PreloadConfig - may slow down vehicle creation");
      }
      _cfgAmmo->OptimizeFind();
    }
    if (_cfgNonAIVehicles)
    {
      if (!_cfgNonAIVehicles->IsFindOptimized())
      {
        RptF("Warning: CfgNonAIVehicles missing in PreloadConfig - may slow down vehicle creation");
      }
      _cfgNonAIVehicles->OptimizeFind();
    }
    if (_cfgWeapons)
    {
      if (!_cfgWeapons->IsFindOptimized())
      {
        RptF("Warning: CfgWeapons missing in PreloadConfig - may slow down vehicle creation");
      }
      _cfgWeapons->OptimizeFind();
    }
    if (_cfgMagazines)
    {
      if (!_cfgMagazines->IsFindOptimized())
      {
        RptF("Warning: CfgMagazines missing in PreloadConfig - may slow down vehicle creation");
      }
      _cfgMagazines->OptimizeFind();
    }
  }
}

VehicleTypeBankTraitsExt::~VehicleTypeBankTraitsExt()
{
  _cfgVehicles.Free();
  _cfgAmmo.Free();
  _cfgNonAIVehicles.Free();
  _cfgWeapons.Free();
  _cfgMagazines.Free();
}


bool VehicleTypeBankTraitsExt::RequestEntry(const char *className)
{
  PROFILE_SCOPE(vtRqE);

  if (className[0]=='#')
  {
    // internal class names - simulation name given directly, no config
    return true;
  }
  if (_cfgVehicles->CheckIfEntryExists(className))
  {
    return _cfgVehicles->Request(className);
  }
  if (_cfgAmmo->CheckIfEntryExists(className))
  {
    return _cfgAmmo->Request(className);
  }
  if (_cfgNonAIVehicles->CheckIfEntryExists(className))
  {
    return _cfgNonAIVehicles->Request(className);
  }
  if (_cfgWeapons->CheckIfEntryExists(className))
  {
    return _cfgWeapons->Request(className);
  }
  if (_cfgMagazines->CheckIfEntryExists(className))
  {
    return _cfgMagazines->Request(className);
  }
  // let error handling to other bodies
  return true;
}

EntityType *VehicleTypeBankTraitsExt::Create(const EntityTypeName &name)
{
  PROFILE_SCOPE(vtCre);
  if (PROFILE_SCOPE_NAME(vtCre).IsActive())
  {
    PROFILE_SCOPE_NAME(vtCre).AddMoreInfo(name._shapeName);
  }
  EntityType *type=NULL;

  if (name._className[0]=='#')
  {
    // internal class names - simulation name given directly, no config
    type=new EntityInternalType();
    type->LoadInternal(name._className,name._shapeName);
    Assert( !type->IsAbstract() );
    return type;
  }
  
  RString testName = name._className; testName.Lower();
  
  ConstParamEntryPtr entry = _cfgVehicles->FindEntry(name._className);
  if (entry)
  {
    ParamEntryVal cfg=*entry;
    if( (cfg>>"scope").GetInt()>0 )
    {
      //LogF("Preload %s",(const char *)name);
      RString simulation=cfg>>"simulation";
      simulation.Lower();

      // WIP: DOOR: remove hack
      if (!strcmp(simulation,"car") && !strcmpi(name._className,"BIS_alice_emptydoor"))
      {
        simulation = "door";
      }
      
      bool unsupportedType = false;
      // first check types which can be a part of the landscape
      if( !strcmp(simulation,"house") ) type=new BuildingType(cfg);           
      else if( !strcmp(simulation,"breakablehouseanimated") ) type=new BuildingType(cfg);
      else if( !strcmp(simulation,"door") ) type=new DoorType(cfg);
      else if( !strcmp(simulation,"church") ) type=new ChurchType(cfg);
      else if( !strcmp(simulation,"fountain") ) type=new FountainType(cfg);
      else if( !strcmp(simulation,"flagcarrier") ) type=new FlagCarrierType(cfg);
#if _ENABLE_TANKS
      else if( !strcmp(simulation,"tank") ) type=new TankType(cfg);
      else if( !strcmp(simulation,"zsu") ) type=new TankType(cfg);  // TODO: relict?
#else
      else if( !strcmp(simulation,"tank") ) unsupportedType = true;
      else if( !strcmp(simulation,"zsu") )  unsupportedType = true;
#endif        
#if _ENABLE_CARS
      else if( !strcmp(simulation,"car") ) type=new CarType(cfg);
#else
      else if( !strcmp(simulation,"car") ) unsupportedType = true;
#endif
#if _ENABLE_MOTORCYCLES
      else if( !strcmp(simulation,"motorcycle") ) type=new MotorcycleType(cfg);
#else
      else if( !strcmp(simulation,"motorcycle") ) unsupportedType = true;
#endif
#if _ENABLE_SHIPS
      else if( !strcmp(simulation,"ship") ) type=new ShipType(cfg);
#else
      else if( !strcmp(simulation,"ship") ) unsupportedType = true; 
#endif
      else if( !strcmp(simulation,"soldierold") ) type=new ManType(cfg);
      else if( !strcmp(simulation,"soldier") ) type=new ManType(cfg);
      else if( !strcmp(simulation,"zombie") ) type=new ZombieType(cfg);
      else if( !strcmp(simulation,"animal") ) type=new AnimalType(cfg);
//      else if( !strcmp(simulation,"zombie") ) type=new ManType(cfg);
#if _ENABLE_HELICOPTERS
      else if( !strcmp(simulation,"helicopter") ) type=new HelicopterType(cfg);
#else
      else if( !strcmp(simulation,"helicopter") ) unsupportedType = true;
#endif
#if _ENABLE_PARACHUTES
      else if( !strcmp(simulation,"parachute") ) type=new ParachuteType(cfg);
#else 
      else if( !strcmp(simulation,"parachute") ) unsupportedType = true;
#endif
#if _ENABLE_AIRPLANES
      else if( !strcmp(simulation,"airplane") ) type=new AirplaneType(cfg);
#else
      else if( !strcmp(simulation,"airplane") ) unsupportedType = true;
#endif
      else if( !strcmp(simulation,"thing") ) type=new ThingType(cfg);
      else if( !strcmp(simulation,"breakablehousepart") ) type=new ThingType(cfg);
      else if( !strcmp(simulation,"breakablehouseanimatedpart") ) type=new ThingType(cfg);
      else if( !strcmp(simulation,"thingeffect") ) type=new ThingType(cfg);
      else if( !strcmp(simulation,"fire") ) type=new BuildingType(cfg);
      else if( !strcmp(simulation,"vasi") ) type=new VASILightsType(cfg);
      else if( !strcmp(simulation,"seagull") ) type=new SeaGullType(cfg);
      else if( !strcmp(simulation,"camera") ) type=new EntityPlainType(cfg);
      else if( !strcmp(simulation,"camconstruct") ) type=new EntityPlainType(cfg);
      else if( !strcmp(simulation,"invisible") ) type=new InvisibleVehicleType(cfg);
      else if( !strcmp(simulation,"inventoryitem") ) type = new InventoryItemType(cfg);
      else if( !strcmp(simulation,"clothing") ) type = new ClothingType(cfg);
      else if( !strcmp(simulation,"itemwatch") ) type = new ItemWatchType(cfg);
      else if( !strcmp(simulation,"itemradio") ) type = new ItemRadioType(cfg);
      else if( !strcmp(simulation,"itemtransmitter") ) type = new ItemTransmitterType(cfg);
      else if( !strcmp(simulation,"itemcompass") ) type = new ItemCompassType(cfg);
      else if( !strcmp(simulation,"itemgps") ) type = new ItemGPSType(cfg);
      else if( !strcmp(simulation,"itemmap") ) type = new ItemMapType(cfg);
      else if( !strcmp(simulation,"itembook") ) type = new ItemBookType(cfg);
      else if( !strcmp(simulation,"itemoptics") ) type = new ItemOpticsType(cfg);
      Assert( type || unsupportedType);
      if( type )
      {
        //LogF("## New type %s",cc_cast(name._className));
        type->Load(cfg,name._shapeName);
        Assert( !type->IsAbstract() );
        //Log("New public type %s (%s)",name,(const char *)type->GetDisplayName()); 
        return type;
      }
      if (!unsupportedType)
      {
        RptF("Unrecognized CfgVehicles simulation %s in %s",cc_cast(simulation),cc_cast(cfg.GetContext()));
      }
    }
    type=new EntityAIType(cfg);
    type->Load(cfg,name._shapeName);
    Assert( type->IsAbstract() );
    Assert( dynamic_cast<EntityAIType *>(type) );
    //LogF("## New type %s",cc_cast(name._className));
    //Log("New private type %s (%s)",name,(const char *)type->GetDisplayName());  
    return type;
  }
  entry = _cfgAmmo->FindEntry(name._className);
  if (entry)
  {
    ParamEntryVal cfg = *entry;
    AmmoType *aType=new AmmoType(cfg);
    type=aType;
    //LogF("## New type %s",cc_cast(name._className));
    type->Load(cfg,name._shapeName);
    return type;
  }
  entry = _cfgNonAIVehicles->FindEntry(name._className);
  if(entry)
  {
    ParamEntryVal cfg = *entry;
    RString simulation=cfg>>"simulation";

    simulation.Lower();

#if _ENABLE_NEWHEAD
    if (!strcmp(simulation,"proxysubpart")) type=new ProxySubpartType(cfg);
    else
#endif
    if( !strcmp(simulation,"proxyinventory") ) type=new ProxyInventoryType(cfg);
    else if( !strcmp(simulation,"streetlamp") ) type=new StreetLampType(cfg);
    else if( !strcmp(simulation,"proxycrew") ) type=new ProxyCrewType(cfg);
    else if( !strcmp(simulation,"flag") ) type=new FlagType(cfg);
    else if( !strcmp(simulation,"windsock") ) type=new WindSockType(cfg);
    else if( !strcmp(simulation,"detector") ) type=new FlagType(cfg);
    else if( !strcmp(simulation,"detectorflag") ) type=new FlagType(cfg);
    else if( !strcmp(simulation,"mark") ) type=new EntityPlainType(cfg);  // TODO: only #mark is used (already handled by now, preload only).
    else if( !strcmp(simulation,"objview") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"camera") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"camconstruct") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"seagull") ) type=new SeaGullType(cfg);
    else if( !strcmp(simulation,"alwaysshow") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"alwayshide") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"maverickweapon") ) type=new EntityPlainTypeNoProxy(cfg);
    else if( !strcmp(simulation,"scud") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"temp") ) type=new EntityPlainType(cfg);  // relict?
    else if( !strcmp(simulation,"smoke") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"track") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"smokesource") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"destructioneffects") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"objectdestructed") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"explosion") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"crater") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"crateronvehicle") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"slop") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"dynamicsound") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"soundonvehicle") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"lightpoint") ) type=new EntityPlainType(cfg);
    else if( !strcmp(simulation,"thunderbolt") ) type=new EntityPlainType(cfg);  // TODO: only #thunderbolt is used (already handled by now).
    else if( !strcmp(simulation,"editcursor") ) type=new EntityPlainType(cfg);
    else if (!strcmp(simulation,"road"))
    {
      // this path is used only in persistence detection (Landscape::CheckPersistance)
      return NULL;
    }
    else
    {
      RptF("Unrecognized CfgNonAIVehicles simulation %s in %s",(const char *)simulation,cc_cast(cfg.GetContext()));
      type=new EntityType(cfg);
    }

    type->Load(cfg,name._shapeName);
    // new types
    //LogF("## New type %s",cc_cast(name._className));
    return type;
  }

  entry = _cfgWeapons->FindEntry(name._className);
  if (entry)
  {
    ParamEntryVal cfg = *entry;
    WeaponType *wType = new WeaponType(cfg);
    type = wType;
    //LogF("## New type %s",cc_cast(name._className));
    type->Load(cfg, name._shapeName);
    return type;
  }

  entry = _cfgMagazines->FindEntry(name._className);
  if (entry)
  {
    ParamEntryVal cfg = *entry;
    MagazineType *mType = new MagazineType(cfg);
    type = mType;
    //LogF("## New type %s",cc_cast(name._className));
    type->Load(cfg, name._shapeName);
    return type;
  }

  //Fail("Type");
  //RptF("Unknown type name %s",name);
  return NULL;
}


/*!
\patch 1.63 Date 6/2/2002 by Ondra
- New: When mission attempts to use addon that is not listed
in mission addOns[] header, error message is shown.
*/

bool CheckAccess(ParamEntryPar entry)
{
  if (!GWorld->CheckAddon(entry))
  {
    RString message = LocalizeString(IDS_MSG_ADDON_MISSING);
    /* do not write addons list on Xbox */
    bool first = true;
    if (first) message = message + RString("\n"), first = false;
    else message = message + RString(", ");
    message = message + entry.GetOwnerName();
#if !_VBS2  // disable MP missing addon warning messages (temp)
    WarningMessage(message);
#endif
    return false;
  }
  return true;
}

//! return false if function should really fail
bool CheckAccessCreate(ParamEntryPar entry)
{
  if (CheckAccess(entry)) return true;
  // type cannot be accessed
  // what now?
  // if we are network client, we will continue
  // we will always continue
  // in any mode there should be some reaction to error message that was displayed
  /*
  if (GWorld->GetMode()==GModeNetware)
  {
    // if unaccessible entry is detected on server
    // we want server admin to fix issue
    // we will therefore not allow creating given unit
    if (GetNetworkManager().IsServer())
    {
      return false;
    }
  }
  else
  {
    // singleplayer
    // we would like to detect if we are in mission editor
    // if not, we may want to continue
    // TODO: detect if running from editor (preview) or not
    // we continue for now, but we may want to be more strict in future
  }
  */
  return true;
}


EntityAI *NewVehicle(const EntityAIType *type, bool fullCreate)
{
  // check if type can be accessed
  if (type->GetParamEntry() && !CheckAccessCreate(*type->GetParamEntry()))
  {
    return NULL;
  }
  
  RString simName = type->_simName;
  
  if (type->IsAbstract())
  {
    RptF("Cannot create entity with abstract type %s (scope = private?)",cc_cast(type->GetName()));
    return NULL;
  }
  type->VehicleAddRef();

  // create the object corresponding to the type
  EntityAI* v = (EntityAI*)type->CreateObject(fullCreate);

  type->VehicleRelease();
  /*
  LogF("Vehicle created %x:%s,%s",v,(const char *)v->GetDebugName(),(const char *)v->GetShape()->Name());
  */
  return v;
}


/*!
\patch 1.01 Date 7/5/2001 by Ondra.
- Improved: Better handling of invalid vehicle type.
*/

EntityAI *NewVehicle(RStringVal typeName, RStringVal shapeName, bool fullCreate)
{
  // create AI vehicle
  //RString simName = Pars >> "CfgVehicles" >> typeName >> "simulation";
  Ref<EntityType> vType=VehicleTypes.New(typeName,shapeName);
  EntityAIType *aiType=dynamic_cast<EntityAIType *>(vType.GetRef());
  ProgressRefresh();
 
  // FIX: better handling of invalid crew specification
  if (!aiType)
  {
    ErrorMessage(EMError,"Bad vehicle type %s",(const char *)typeName);
    return NULL;
  }
  EntityAI *v=NewVehicle(aiType,fullCreate);

  if( v )
  {
    if( v->Object::GetType()==Primary )
    {
      v->SetType(TypeVehicle);
    }
  }
  return v;
}




Object *NewNonAIVehicleQuiet( const EntityType *type, bool fullCreate)
{
  // some types must be caught before trying to create EntityAI
  RString simName = type->_simName;
  simName.Lower();

  Object *v = NULL;
  if( !stricmp(simName,"thingeffect") )
  {
    const ThingType *tType =dynamic_cast<const ThingType *>(type);
    if (tType)
    {
      type->VehicleAddRef();
      v=new ThingEffectLight(tType);
      type->VehicleRelease();
      return v;
    }
  }

  const EntityAIType *vType = dynamic_cast<const EntityAIType *>(type);
  if( vType )
  {
    return NewVehicle(vType,fullCreate);
  }

  const AmmoType *aType=dynamic_cast<const AmmoType *>(type);
  if( aType )
  {
    return NewShot(NULL,aType,NULL,NULL);
  }

  type->VehicleAddRef();
  v = type->CreateObject(fullCreate);
  type->VehicleRelease();
  return v;
}


Object *NewNonAIVehicleQuiet( RString typeName, RString shapeName, bool fullCreate)
{
  // create AI vehicle
  //RString simName = Pars >> "CfgVehicles" >> typeName >> "simulation";
  Ref<EntityType> type=VehicleTypes.New(typeName,shapeName);
  ProgressRefresh();

  if( !type )
  {
    //LogF("No non-AI type %s",(const char *)typeName);
    //RptF("Type: %s",(const char *)typeName);
    return NULL;
  }
  Assert(!strcmpi(type->GetEntityTypeName()._shapeName,shapeName));
  return NewNonAIVehicleQuiet(type,fullCreate);
}

Entity *NewNonAIVehicle(const EntityType *type, bool fullCreate)
{
  Object *obj = NewNonAIVehicleQuiet(type,fullCreate);
  Entity *v = dyn_cast<Entity>(obj);
  if (!v)
  {
    RptF
    (
      "Cannot create non-ai vehicle %s,%s",
      (const char *)type->GetName(),(const char *)type->GetShapeName()
    );
    Ref<Object> temp = obj; // guarantee destroying Ref (it may be the only one)
  }
  else
  {
    if( v->Object::GetType()==Primary )
    {
      v->SetType(TypeVehicle);
    }
  }
  return v;
}

Entity *NewNonAIVehicle(RStringVal typeName, RStringVal shapeName, bool fullCreate)
{
  Object *obj = NewNonAIVehicleQuiet(typeName,shapeName,fullCreate);
  Entity *v = dyn_cast<Entity>(obj);
  if (!v)
  {
    RptF
    (
      "Cannot create non-ai vehicle %s,%s",
      (const char *)typeName,(const char *)shapeName
    );
    Ref<Object> temp = obj; // guarantee destroying Ref (it may be the only one)
  }
  else
  {
    if( v->Object::GetType()==Primary )
    {
      v->SetType(TypeVehicle);
    }
  }
  return v;
}

Object *NewObject( RString typeName, RString shapeName )
{
  // object created this way are considered temporary
  // this is mainly used to create proxy objects
  Object *v = NewNonAIVehicleQuiet(typeName,"",true);
  if (v)
  {
    v->SetType(Temporary);
    return v;
  }

  /* Not more needed. This function is called only from ProxiesHolder::InitLevel...
  Ref<LODShape> shape = Shapes.New(shapeName,ShapeShadow);
  Object *obj = new ObjectPlain(shape,VISITOR_NO_ID);
  obj->SetType(Temporary);
  return obj;*/
  return v;
}

