#ifdef _MSC_VER
#pragma once
#endif

#ifndef _WORLD_HPP
#define _WORLD_HPP

#include "types.hpp"
#include "vehicle.hpp"
#include "scene.hpp"
#include "weapons.hpp"
#include "UI/inGameUI.hpp"
#include "UI/optionsUI.hpp"
#include "UI/uiControls.hpp"
#include "jvmUI.hpp"
#include "UI/chat.hpp"
#include <El/ParamArchive/serializeClass.hpp>
#include <El/Evaluator/expressImpl.hpp>
#if _ENABLE_INDEPENDENT_AGENTS
#include "AI/aiTeams.hpp"
#endif
#include "location.hpp"
#include "fsm.hpp"
#include "jvmFSM.hpp"
#include "scripts.hpp"
#include "progress.hpp"
#if USE_PRECOMPILATION
#include <El/Evaluator/scriptVM.hpp>
#endif
#include <Es/Types/lLinks.hpp>
#include "Es/Containers/array2D.hpp"
#include <El/ParamFile/paramFileCtx.hpp>
#include "camShake.hpp"

#include <Es/Containers/maps.hpp>

//#include "Enforce\include\Physics\enf_worldphysics.h"

#define ENFORCE_PHYSICS_RAGDOLLS_FILE	"physics/ragdolls.h"
#define ENFORCE_PHYSICS_MATERIALS_FILE "physics/materials.h"

#if _HELISIM_ROTORLIB
#include "helicopterRTD.hpp"
#endif

class Landscape;
class ObjectId;
class AIBrain;

class ItemMap;

struct InventoryMenuMessage;

/// interface for ambient life area - AmbLifeArea
class IAmbLifeArea
{
public:
  /// called when initializing after world creation
  virtual void Init(const Camera &camera) = 0;
  /// called once entity decides it is leaving area
  virtual void OnEntityLeft(const Camera &camera, Entity *ent) = 0;
  /// check if given entity is inside of the area
  virtual bool CheckInArea(const Camera &camera, Entity *ent) const = 0;
  /// tell the entity how much it should try to move out
  virtual float CheckMoveOut(const Camera &camera, Entity *ent) const = 0;
  /// ambient layer may have some internal simulation
  virtual void Simulate(const Camera &camera, float deltaT) = 0;
  /// diagnostics
  virtual RString GetDebugText() const {return RString();}
    
  virtual ~IAmbLifeArea() {}
  
  /// return the unique id of area in the manager
  virtual RString GetName() const = 0;
  /// serialization of reference
  static IAmbLifeArea *LoadRef(ParamArchive &ar);
  /// serialization of reference
  virtual LSError SaveRef(ParamArchive &ar) = 0;
  /// serialization of reference
  static LSError SaveInitPtrRef(ParamArchive &ar, InitPtr<IAmbLifeArea> &ptr)
  {
    return ptr->SaveRef(ar);
  }
  /// serialization of reference
  static LSError LoadInitPtrRef(ParamArchive &ar, InitPtr<IAmbLifeArea> &ptr)
  {
    ptr = IAmbLifeArea::LoadRef(ar);
    return (LSError)0;
  }
};

/// interface for managing environmental (ambient) sounds
class IEnvSoundManager
{
public:
  /// get total count of sound slots
  /** call only once per frame */
  virtual int Count(const Camera &camera) = 0;
  /// get slot info + volume
  virtual const SoundPars &GetSound(int i, float &volume) const = 0;
  virtual bool GetRandSound(int i, SoundPars &pars) const = 0;
  virtual ~IEnvSoundManager() {}

#if _ENABLE_CHEATS
  virtual void ShowDiag() const = 0;
#endif
};

/// interface for managing ambient life
class IAmbLifeManager
{
public:
  virtual void Load(ParamEntryPar cfg) = 0;
  virtual void Clear() = 0;
  virtual void Init(const Camera &cam) = 0;
  virtual void Simulate(const Camera &cam, float deltaT) = 0;
  /// search for ambient life area by its name 
  virtual IAmbLifeArea *FindArea(RString name) const = 0;
  virtual ~IAmbLifeManager() {}
};

/// description of ambient life place (source candidate)
struct AmbLifePlace
{
  Vector3 _position;
  float _value;
};
TypeIsMovableZeroed(AmbLifePlace)

/// describe post effect
struct PPEItem
{
  RString _key;
  int _value; // post effect hndl
  Time _time; // last update time
  bool _enabled;
};
TypeIsMovableZeroed(PPEItem);

template<>
struct FindArrayKeyTraits<PPEItem>
{
  typedef const RString &KeyType;

  /// check if two keys are equal
  static bool IsEqual(KeyType a, KeyType b) {return strcmp(cc_cast(a), cc_cast(b)) == 0;}

  /// get a key from an item
  static KeyType GetKey(const PPEItem &a) {return a._key;}
};

struct VideoDataChunk: public RefCount, public AutoArray<char>
{
  enum { ChunkSize = 4096 };
  
  VideoDataChunk()
  {
    Realloc(ChunkSize);
    Resize(ChunkSize);
  }

  ~VideoDataChunk()
  {
    Resize(0);
  }
};

struct VideoChunkDesc
{
  Ref<VideoDataChunk> _data;
  int _usedBytes;
};

const int CBufferSize = 256;

#include <El/Multicore/circularQueue.hpp>

class VideoBufferInfo: public RefCount
{
public:
  VideoBufferInfo(const char *name, OggDecoder *decoder);
  ~VideoBufferInfo();
  void Reset();

  Ref<OggDecoder>   _decoder;
  QIFStreamB        *_inVideoFile;
  CircularArray<VideoChunkDesc, CBufferSize> _cBuffer;
};

class PPEManager
{
  FindArrayKey<PPEItem, FindArrayKeyTraits<PPEItem> > _hndls;

public:
  ~PPEManager() { Clear(); }
  /// update effect parameters
  void Update(const AutoArray<PPEffectType> &ppeTypes);
  /// release post effect assigned to vehicle
  void Clear();
  /// disable all effect - not release them
  void EnableEffects(bool enable);

private:
  int CreateEffect(const RString &name, int priority);
  void DestroyEffect(int hndl);
};

DEFINE_ENUM_BEG(CameraType)
  CamInternal,CamGunner,CamExternal,CamGroup,
  MaxCameraType
DEFINE_ENUM_END(CameraType)

typedef void (Entity::*VehicleSimulation)( float deltaT, SimulationImportance prec );

enum StartVehicle {StartSoldier,StartHelicopter,StartAPC,StartSeaGull};

extern StartVehicle PlayStart;

class HeliPilotWaypoints;
class SeaGullPilotCommander;
class TankPilotFollow;
class CarPilotFollow;
class SoldierPilotFollow;

class BMPPlayer;
class BMPWithAI;

//! Uninitialized camera position
const Vector3 InvalidCamPos(-FLT_MAX, -FLT_MAX, -FLT_MAX);

#define CLIENT_CAMERA_POSITION_MSG_CREATE(MessageName, XX) \
  XX(MessageName, int, dpnid, NDTInteger, int, NCTNone, DEFVALUE(int, 0), DOC_MSG("Client (player) id"), TRANSF)

DECLARE_NET_INDICES_EX(CreateClientCameraPosition, NetworkObject, CLIENT_CAMERA_POSITION_MSG_CREATE)

#define CLIENT_CAMERA_POSITION_MSG_UPDATE(MessageName, XX) \
  XX(MessageName, Vector3, position, NDTVector, Vector3, NCTVectorPositionCamera, DEFVALUE(Vector3, InvalidCamPos), DOC_MSG("Position of camera on client"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_NORMAL) \

DECLARE_NET_INDICES_EX_ERR(UpdateClientCameraPosition, NetworkObject, CLIENT_CAMERA_POSITION_MSG_UPDATE)

///////////
// NetworkObject to keep the camera position of all clients synchronized
class ClientCameraPositionObject :  public NetworkObject
{
public:
  Vector3 _position;
  int     _dpnid;   // Direct play id (necessary for hearing of VoN Direct Speech for dead players)

protected:
  NetworkId _id;
  bool _local;

public:
  OLinkPermO(Person) _person;

  // Constructor
  ClientCameraPositionObject()
  {
    _position = InvalidCamPos;
    _local = true;
  }

  void SetNetworkId(NetworkId &id) {_id = id;}
  NetworkId GetNetworkId() const {return _id;}
  Vector3 GetCurrentPosition() const {return _position;}
  bool IsLocal() const {return _local;}
  void SetLocal(bool local = true) { _local = local; }
  void DestroyObject();

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
    (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
    );
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);
  virtual float CalculateErrorCoef(Vector3Par position, Vector3Par cameraPosition, float updateDistance, bool updating) const override {return 1.0;}
  RString GetDebugName() const;

  // ClientCameraPositonObject is obsolete when owned on different client (change owner after player disconnect)
  bool IsObsolete() const;

  static ClientCameraPositionObject *CreateObject(NetworkMessageContext &ctx);
};

/// list of vehicles - one homogeneous group
/**
Vehicles may be sorted by next simulation time and simulation granularity.
*/

class EntityList: private RefArray<Entity>, public SerializeClass
{
  /// list of items which need to be handled in the MoveOutAndDeleteEntity step
  /** consider: using TListBidir might be faster */
  SortedArray<int, MemAllocLocal<int,32> >  _toMoveOutOrDelete;
  
  /// if we are a member of a distributed list, which one?
  InitPtr<EntitiesDistributed> _parent;

  typedef RefArray<Entity> base;
public:
  EntityList(EntitiesDistributed *parent=NULL) :_parent(parent) {}
  bool IsMoveOutOrDeletePending() const {return _toMoveOutOrDelete.Size() > 0;}
  void AddToMoveOutOrDelete(Entity *entity)
  {
    // if the item is marked, it needs to be present in the list
    int myIndex = Find(entity);
    DoAssert(myIndex >= 0);
    if (myIndex >= 0)
    {
      // the item must not be there yet
      DoAssert(_toMoveOutOrDelete.FindKey(myIndex) < 0);
      _toMoveOutOrDelete.AddUnique(myIndex); // find my own index in the list
    }
  }

  /** no ordering guaranteed, implementation may change */
  Entity *SelectRandom(int random) const {return base::Get(random);}
  void Insert(Entity *object)
  {
    base::Add(object);
    object->SetList(this);
    if (object->ToDelete())
      AddToMoveOutOrDelete(object); //FIX: there were soldiers without Brain without this line (see news:h78q27$t1g$1@new-server.localdomain)
  }
  void Add(Entity *object);

  int VehicleCount() const {return base::Size();}
  
  Entity *Find(const ObjectId &id) const;
  int Find(const Entity *object) const {return base::FindKey(object);}

  EntitiesDistributed *GetParent() const {return _parent;}
  void SetParent(EntitiesDistributed *val) {_parent = val;}
  
  LSError Serialize(ParamArchive &ar);
  
  /// perform given function on some data members
  template <class Functor>
  bool ForEach(const Functor &f, int start, int end) const
  {
    for (int i=start; i<end; i++)
    {
      bool done = f(Get(i));
      if (done)
        return true;
    }
    return false;
  }
  
  /// perform given function on all data members
  template <class Functor>
  __forceinline bool ForEach(const Functor &f) const
  {
    return ForEach(f,0,Size());
  }

  /// perform given function on all data members
  template <class Functor>
  bool ForEach(const Functor &f)
  {
    for (int i=0; i<Size(); i++)
    {
      bool done = f(Get(i));
      if (done)
        return true;
    }
    return false;
  }
  
  /// perform given function on all data members, delete any data as required by the functor
  /** functor indicates the member should be deleted by returning true */
  template <class Functor>
  void ForEachWithDelete(const Functor &f)
  {
    int dst = 0;
    // our current position in _toMoveOutOrDelete - we need to make sure it stays current
    int inMD = 0;
    // all items before inMD are already adjusted (correspond to dst)
    // all items at or after inMD are still old (correspond to i)
    for (int i=dst; i<Size(); i++)
    {
      Entity *entity = Get(i);
      bool deleteIt = f(entity);
      if (!deleteIt)
      {
        if (inMD<_toMoveOutOrDelete.Size() && _toMoveOutOrDelete[inMD]==i)
        {
          // when we have passed the _toMoveOutOrDelete item, we adjust it and go to the other one
          // how many items were already deleted can be always computed as i-dst
          // we are sure we are not changing ordering, therefore we can modify in-place
          _toMoveOutOrDelete.ReplaceAt(inMD, _toMoveOutOrDelete.Get(i) - (i-dst) );
          inMD++;
        }
        Set(dst++) = entity;
      }
      else
      {
        // never delete entity which is listed in _toMoveOutOrDelete
        // also, never delete entity which is before current _toMoveOutOrDelete (we should have passed it already)
        Assert(inMD>=_toMoveOutOrDelete.Size() || _toMoveOutOrDelete[inMD]<=i);
      }
    }
    Resize(dst);
  }
  
  struct EmptyCallBackOnDelete
  {
    void operator()(int i) const {}
  };

  /// perform given function on all data members marked for processing, delete any data as required by the functor
  /** functor indicates the member should be deleted by returning true */
  template <class Functor, class CallBackOnDelete>
  void ForEachToMoveOutOrDeleteWithCallback(const Functor &f, const CallBackOnDelete &callBackOnDelete)
  {
    // optimization: if there is nothing to process, return immediately
    if (_toMoveOutOrDelete.Size()<=0) return;
    // optimization: skip everything before first entry to move out or delete
    int noProcessingNeeded = _toMoveOutOrDelete[0];
    // we need to process only _toMoveOutOrDelete
    // while processing it, we keep track of original / new indices
    int i = noProcessingNeeded; // original indices
    int dst = noProcessingNeeded; // target indices
    
    for (int inMD=0; inMD<_toMoveOutOrDelete.Size(); inMD++)
    {
      int mdIndex = _toMoveOutOrDelete[inMD];
      if(mdIndex >= Size()) break;
      
      // skip everything from i to mdIndex
      for (; i<mdIndex; i++)
        Set(dst++) = Get(i);
      
      // process the one which is marked for processing
      Entity *entity = Get(mdIndex);
      bool deleteIt = f(entity);
      
      // it is possible the item was marked but still not deleted
      // while this is not optimal, we do not care
      if (!deleteIt)
        Set(dst++) = entity;
      else
        // notify anyone interested the item was deleted
        callBackOnDelete(dst);
      i++; // skip the entry pointed to by mdIndex
      Assert(i<=Size()); // this should never bring us out of range
      
    }
    _toMoveOutOrDelete.Clear(); // list processed, clear it
    
    // skip the rest
    for (;i<Size(); i++)
      Set(dst++) = Get(i);
    Resize(dst);
  }
  template <class Functor>
  void ForEachToMoveOutOrDelete(const Functor &f)
  {
    ForEachToMoveOutOrDeleteWithCallback(f, EmptyCallBackOnDelete());
  }
  
  /// interface for a simple for loops
  void Clear()
  {
    base::Clear();
    _toMoveOutOrDelete.Clear();
  }
  bool Delete(Entity *ent){return base::Delete(ent);}
  bool IsPresent(Entity *ent) const {return base::FindKey(ent)>=0;}
};


class Transport;
class SensorList;

//! often used AI entity types
enum PreloadedVType
{
  VTypeStatic,
  VTypeBuilding, // all static but fortreses
  VTypeStrategic, VTypeNonStrategic,
  VTypeObjective,VTypePrimaryObjective,VTypeSecondaryObjective,
  VTypeTarget,
  VTypeAllVehicles,
  VTypeAir,VTypePlane,
  VTypeShip,VTypeBigShip,
  VTypeAPC,VTypeTank,VTypeCar,
  VTypeMan,VTypeHelipad,VTypePaperCar,VTypeFireSectorTarget,
  NPreloadedVTypes,
};

//! often used entity types
enum PreloadedEType
{
  ETypeMark,
  ETypeSmoke,
  ETypeAnimator,
  NPreloadedETypes,
};

enum GameMode
{
  GModeNetware,
  GModeArcade,
  GModeIntro,
};

struct AnimationDescriptor;
#include "objLink.hpp"

class CameraEffect: public RefCountWithLinks
{
protected:
  OLinkObject _object; // effect base
  Matrix4 _transform;
  /// draw in-game UI when effect is used
  bool _hudEnabled;

public:
  CameraEffect(Object *object); // each camera effect is assigned to something
  ~CameraEffect();
  
  Object *GetObject() const {return _object;}
  void SetObject(Object *obj) {_object = obj;}

  bool IsHUDEnabled() const {return _hudEnabled;}
  void EnableHUD(bool enable) {_hudEnabled = enable;}

  virtual void Simulate(float deltaT) {}
  virtual Matrix4 GetTransform() const {return _transform;} // get camera position
  virtual void SetFOV(float fov) {} // set to default
  virtual float GetFOV() const {return -1;} // set to default
  virtual bool IsInside() const {return false;} // set to default
  virtual void Draw() const; // draw anything
  virtual bool IsTerminated() const {return true;}

  virtual int GetType() const = 0;
  virtual LSError Serialize(ParamArchive &ar);
  static CameraEffect *CreateObject(ParamArchive &ar);

#if _HELISIM_RENDER_TO_TEXTURE
  virtual void SetAssignedR2TTgtIndex(int index) { }
  virtual R2TTgtDescription& GetTgtDesc() { static R2TTgtDescription TgtDesc; return TgtDesc; }
#endif
};

DECL_ENUM(CamEffectPosition)

class TitleEffect: public RefCountWithLinks
{
public:
  virtual void Draw() {}
  virtual void Simulate(float deltaT) {}
  /// terminate the main phase (before fade out) of the effect, if fadeOutTime is not negative, set duration of fade out phase to it
  virtual void Terminate(float fadeOutTime=-1.0f) {}
  virtual void Prolong(float time) {}
  // make sure all data are preloaded
  virtual bool Preload() {return true;}

  virtual bool DrawInMap() {return true;}

  virtual bool IsTerminated() const {return true;}
  virtual bool IsTransparent() const {return true;}
};

DECL_ENUM(TitEffectName)
DECL_ENUM(EEndMode)

static int NVehicles(ParamArchive &ar, RString name, const EntitiesDistributed &list);

class EntitiesDistributed : public SerializeClass
{
  friend class World;
  friend int NVehicles(ParamArchive &ar, RString name, const EntitiesDistributed &list);

  EntityList _visibleNear;
  EntityList _visibleFar;
  EntityList _invisibleNear;
  EntityList _invisibleFar;

public:
  void Clear();
  EntitiesDistributed();

  void Add(Entity *vehicle ); // add to list and landscape
  void Insert(Entity *vehicle); // add to list only
  /// add to given list
  void Insert(SimulationImportance prec, Entity *vehicle);

  void Delete(Entity *vehicle); // delete from list and landscape
  void Remove(Entity *vehicle); // delete from list only
  bool IsPresent(Entity *vehicle) const;

  bool IsList(EntityList *list) const {return list==&_visibleNear || list==&_visibleFar || list==&_invisibleNear || list==&_invisibleFar;}
  /// not intended for general use - debugging only
  int VehicleCount() const
  {
    return _visibleNear.VehicleCount() + _visibleFar.VehicleCount() + _invisibleNear.VehicleCount() + _invisibleFar.VehicleCount();
  }
  //int Size() const;

  const EntityList &Select(SimulationImportance sim) const;
  Entity *Find(const ObjectId &id) const;

  template <class Functor>
  bool ForEach(const Functor &func) const
  {
    if (_visibleNear.ForEach(func))
      return true;
    if (_visibleFar.ForEach(func))
      return true;
    if (_invisibleNear.ForEach(func))
      return true;
    if (_invisibleFar.ForEach(func))
      return true;
    return false;
  }

  LSError Serialize(ParamArchive &ar);
};

#if _HELISIM_ROTORLIB && !_DUMMY_ROTORLIB

struct RTDSimulateThread
{
  SignaledObject _thread;
  Event _terminate;
  Event _simulate;
  Event _simDone;
  DWORD _threadId;
  World *_world;
  bool _doSim;
  float _deltaT;

  RTDSimulateThread()
    : _world(NULL),
    _terminate(false),
    _simulate(false),
    _simDone(false),
    _thread(NoInitHandle),
    _doSim(false),
    _deltaT(0)
  {
    ;
  }

  void Init(World *world, DWORD stackSize, int cpu, int priority, const char *name)
  {
    if (world)
    {
      _world = world;
      _thread.Init(CreateThreadOnCPU(stackSize, RTDSimulateThread::ThreadProcedureCallback, this, cpu, priority, name, &_threadId));
    }
  }

  void Done()
  {
    _terminate.Set();
    // wait until termination is done
    _thread.Wait();
    _thread.Done();
  }

  void SimulateOnce(bool doSim, float deltaT);

  void IsSimulationDone();

  static DWORD WINAPI ThreadProcedureCallback(void *context);

  DWORD ThreadProcedure();
};

//world representing rotorlib simulation
class WorldRTD: public RTD::World
{
  typedef RTD::World base;

  float _timeToSimulate;
  float _timeToSimulateRest;
  float _accumulatedSimulationTime;
  smVec3r _windVelocity;

public:
  static float FixedSimulationStep;
  //external forces 
  AutoArray<ForceGeneratorRTD> _externalForceGeneratorsRTD;

public:
  WorldRTD():RTD::World(),_timeToSimulate(0), _timeToSimulateRest(0), _accumulatedSimulationTime(0)
  {
    setFixedDeltaTime(FixedSimulationStep);
    _windVelocity = smVec3r(0,0,0);
  };

  LSError Serialize(ParamArchive &ar);
  //update rtd world
  virtual float update(float timestep, bool useIterativeUpdate = true );
  //reset rtd world
  virtual void reset() ;
  //reset external forces
  void resetForcegenerators() {_externalForceGeneratorsRTD.Clear();}

  float AccumulatedSimulationTime(void) { return _accumulatedSimulationTime; }
  void ClearAccumulatedSimulationTime(void) { _accumulatedSimulationTime = 0; }

  void SetWindSpeed(Vector3 wind)
  {

    if(!Glob.config.WindEnabled()) _windVelocity = smVec3r(0, 0, 0);
    else _windVelocity = smVec3r(wind[0],wind[2],wind[1]);
  }

  smVec3r GetWindSpeed() { return _windVelocity;}

  __forceinline float GetTimeToSimulateRest() const { return _timeToSimulateRest; }
  __forceinline void SetFixedDeltaTime(float deltaT) { setFixedDeltaTime(deltaT); }
  __forceinline float GetFixedDeltaTime() const { return (float) getFixedDeltaTime(); }
};
#endif

struct MissionHeader;
class ControlsContainer;

DEFINE_ENUM_BEG(VehicleListType)
  VLTVehicle,
  VLTAnimal,
  VLTBuilding,
  VLTCloudlet,
  VLTFast,
  VLTOut,
  VLTSlowVehicle,
DEFINE_ENUM_END(VehicleListType)

#if _VBS2 // key bindings
struct KeyBinding
{
  int userKeyCode; //can be keycode or UserAction index, depends on isDIK
  RString command;
  int ID;
  bool isDik;

  KeyBinding(UserAction code, RString cmd, int new_id, bool dik = false)
  {
    userKeyCode = code; command = cmd; ID = new_id; isDik = dik;
  }
};
TypeIsMovableZeroed(KeyBinding)
#endif

struct GridInfo
{
  float zoomMax;
  RString format;
  RString formatX;
  RString formatY;
  float stepX;
  float stepY;
  float invStepX;
  float invStepY;

  GridInfo(float z, RString f, RString fX, RString fY, float sX, float sY)
  {
    zoomMax = z; format = f; formatX = fX; formatY = fY;
    stepX = sX; invStepX = sX == 0 ? 0 : 1.0f / sX; stepY = sY; invStepY = sY == 0 ? 0 : 1.0f / sY;
  }
  GridInfo()
  {
    zoomMax = 0;
    stepX = 0; invStepX = 0; stepY = 0; invStepY = 0;
  }
};
TypeIsMovableZeroed(GridInfo)

DECL_ENUM(AICenterMode)

/// Definition of save slots
#define SAVE_GAME_TYPE_ENUM(type,prefix,XX) \
  XX(type, prefix, Autosave) \
  XX(type, prefix, Usersave) \
  XX(type, prefix, Continue) \
  XX(type, prefix, Usersave2) \
  XX(type, prefix, Usersave3) \
  XX(type, prefix, Usersave4) \
  XX(type, prefix, Usersave5) \
  XX(type, prefix, Usersave6) \

#ifndef DECL_ENUM_SAVE_GAME_TYPE
#define DECL_ENUM_SAVE_GAME_TYPE
DECL_ENUM(SaveGameType)
#endif
DECLARE_ENUM(SaveGameType, SGT, SAVE_GAME_TYPE_ENUM);

struct UserFileErrorInfo
{
  RString _dir;
  RString _file;
  RString _name;
  bool _canDelete; //< if it's true, dialog can delete corrupted file
};
TypeIsMovableZeroed(UserFileErrorInfo)

typedef MemAllocLocal< OLink(Entity), 64> OLinkEntityArrayAlloc;
/// list of entities which can affect wind
class WindEmitterList: public OLinkArrayEx(Entity,OLinkEntityArrayAlloc)
{
  float _windSlowSize;

  public:

  WindEmitterList();
  float GetWindSlowSize() const {return _windSlowSize;}

  void Begin();
};

/// list of object target categories
/**
Used to control visibility testing.
*/
class TargetCategories
{
  DynEnum _categories;

  Array2D<float> _maxRange;

public:
  TargetCategories();

  int GetCategory(const char *name);

  void SetRangeLimit(const char *from, float limit);
  void SetRangeLimit(const char *from, const char *target, float limit);
  float GetRangeLimit(int fromCat, int targetCat) const;
  void Reset();
};


/// list of entities which flatten grass around themselves dynamically
/**
This is not permanent grass flattening, grass flattened this way recover immediately
on flattener goes away.
*/
class GrassFlattenerList: public OLinkArrayEx(Entity,OLinkEntityArrayAlloc)
{
};

struct VehicleInitMessage
{
  OLink(EntityAI) _vehicle;
  RString _init;
};
TypeContainsOLink(VehicleInitMessage)

struct VehicleInitMessages : public AutoArray<VehicleInitMessage, MemAllocSA>
{
};


class BankHashCalculatorAsync;

enum CheckHashReason
{
  CHRMultiplayer,
  CHRDemo
};

struct CheckHashItem
{
  int _bankIndex;
  CheckHashReason _reason;
};
TypeIsSimple(CheckHashItem)

template <>
struct FindArrayKeyTraits<CheckHashItem>
{
  typedef int KeyType;
  static bool IsEqual(int a, int b) {return a == b;}
  static int GetKey(const CheckHashItem &a) {return a._bankIndex;}
};

struct FakeBulletFlySource
{
  // fake bullet sound
  Ref<AbstractWave> _soundSource;
  // 
  Time time;
  /// which projectile is this linked to
  OLinkO(Entity) _source;
};
TypeIsGeneric(FakeBulletFlySource)

struct SupersonicCrackSource
{
  /// if the sound has started, record it here
  Ref<AbstractWave> _soundNear;
  /// we may blend between two different sounds
  Ref<AbstractWave> _soundFar;
  /// which projectile is this linked to
  OLinkO(Entity) _source;
};
TypeIsGeneric(SupersonicCrackSource)

/// item of list of displays
struct DisplayInfo
{
  /// display
  SRef<AbstractOptionsUI> _display;
  /// lower priority means the display is in the front
  float _priority;
};
TypeIsMovableZeroed(DisplayInfo)

const int TT_G = 2;
const int TT_Dot = 2;
const int TT_K = 13;

#if _VBS3_CRATERS_LATEINIT

//! Class to perform late initialization of objects like craters. This mechanism will ensure only limited number of initializations will be performed per frame
class LateInitManager
{
private:
  //! Structure to describe one item to be initialized later
  struct LateInitItem : public TLinkBidirD
  {
    Ref<Object> _object;
    float _priority;
  };
  //! List of objects to be initialized later
  TListBidir<LateInitItem, TLinkBidirD, SimpleCounter<LateInitItem> > _objects;
public:
  //! Destructor
  ~LateInitManager();
  //! Register new item for late initialization
  void Register(Object *object, float priority);
  //! Perform late initialization on reasonable number of objects
  void LateInit();
};

#endif

/// whole world representation
/**
Mostly dynamic entities are stored here. Static is normally in the Landscape.
*/

#if _VBS3 //defined in global.hpp
  #include "global.hpp"
#endif

/// basic info about the 'sqs' script
struct ScriptItem
{
  /// script itself
  Ref<Script> _script;
  /// when the script was updated
  float _age;

  ScriptItem(Script *script) : _script(script), _age(0) {}
  LSError Serialize(ParamArchive &ar);
};
TypeIsMovableZeroed(ScriptItem);

struct ScriptItemTraits
{
  typedef const Script *KeyType;
  /// check if two keys are equal
  static bool IsEqual(KeyType a, KeyType b) {return a == b;}
  /// get a key from an item
  static KeyType GetKey(const ScriptItem &a) {return a._script;}
};

#if USE_PRECOMPILATION
/// basic info about the 'sqf' script
struct ScriptVMItem
{
  /// script itself
  Ref<ScriptVM> _script;
  /// when the script was updated
  float _age;

  ScriptVMItem(ScriptVM *script) : _script(script), _age(0) {}
  LSError Serialize(ParamArchive &ar);
};
TypeIsMovableZeroed(ScriptVMItem);

struct ScriptVMItemTraits
{
  typedef const ScriptVM *KeyType;
  /// check if two keys are equal
  static bool IsEqual(KeyType a, KeyType b) {return a == b;}
  /// get a key from an item
  static KeyType GetKey(const ScriptVMItem &a) {return a._script;}
};
#endif

class IShowDiagDelegate: public RefCount
{
  public:
  virtual void Draw() = 0;
  virtual Vector3 Pos() const = 0;
};
  
class ShowDiagDelegateDefault: public IShowDiagDelegate
{
  public:
  ///  object - contains all parameters in it
  Ref<ObjectColored> _obj;
  /// diagnostic text
  RString _text;
  /// diagnostic text position
  Vector3 _textPos;
  /// diagnostic text priority
  int _textPrior;
  /// diagnostic text size
  float _textSize;
  
  virtual void Draw() override;

  virtual Vector3 Pos() const override;

};
/// information about a 3D diagnostics
struct ShowDiagInfo
{
  Ref<IShowDiagDelegate> _show;
  /// part of the unique handle assigned by the diag manager
  int _handle;
  /// part of the unique handle assigned by the caller
  int _id;
  
  // time until which we want the diags to be shown
  Time _hideTime;
  
  ClassIsMovableZeroed(ShowDiagInfo)
};

/// container of diagnostics arrows to show
class ShowDiags
{
  /// the list of arrows
  AutoArray<ShowDiagInfo> _diags;
  /// the last assigned ShowDiagInfo::_handle value
  int _handle;
  
  int FindHandle(int handle, int id) const
  {
    for (int i=0; i<_diags.Size(); i++)
    {
      if (_diags[i]._handle==handle && _diags[i]._id==id) return i;
    }
    return -1;
  }
  public:
  ShowDiags();
  /// draw colored arrow with text legend, active for given time
  void ShowArrow(int &handle, int id, float timeout, RString text, Vector3Par pos, Vector3Par dir, float size=0.1f, ColorVal color=HWhite, float textSize = 1);
  
  void Show(int &handle, int id, float timeout, IShowDiagDelegate *show);
  
  /// submit all active diags for rendering
  void Render();
  

  bool IsDiagXZ(Vector3Par pos, float maxDist=1.0f) const;
  /// reset all diags
  void Reset();
};

/// list of 3D diagnostics which are currently being shown
extern ShowDiags GDiagsShown;


/// basic info about the mission level (global) FSM
struct FSMItem
{
  /// FSM itself
  SRef<FSM> _fsm;
  /// when the FSM was updated
  float _age;

  FSMItem(FSM *fsm) : _fsm(fsm), _age(0) {}
  LSError Serialize(ParamArchive &ar);
};
TypeIsMovableZeroed(FSMItem);

class ControlObjectWithZoom;

/// post event evaluation interface
class IPostponedEvent: public RefCount
{
public:
  virtual void DoEvent() const = 0;
};

enum UpdateCustomChannelType
{
  UCR_Create,
  UCR_Label,
  UCR_CallSign,
  UCR_Units
};

#if _HELISIM_RENDER_TO_TEXTURE

struct R2TCamera
{
  Ref<CameraEffect> _camEffect;
  Ref<R2TTgtPars> _tgtPars;

  // Render pars
  RenderTargetID _id;  
  float _deltaT;
  float _eyeAccom;
  bool _eyeAccomCut;
  float _customEyeAccom;
  bool _allowCockpitDrawing;
  bool _custom;

  void SetDefault(CameraEffect *effect, int r2tIndex = -1)
  {
    if (effect)
    {
      _camEffect = effect;
      _camEffect->SetAssignedR2TTgtIndex(r2tIndex);
    }

    _custom = false;
    _id = R2TUninitialized;
    _deltaT = 0.0f;
    _eyeAccom = 1.0f;
    _eyeAccomCut = true;
    _customEyeAccom = -1.0f;
    _allowCockpitDrawing = false;
  }

  ~R2TCamera();
  LSError Serialize(ParamArchive &ar);

  __forceinline const bool IsTgtValid() const { return _tgtPars.NotNull() && _tgtPars->IsValid(); }
  __forceinline const R2TCameraDescription& CamDesc() const { return _tgtPars->Desc(); }
  __forceinline const RString& Name() const {  return _tgtPars->Name(); }
  __forceinline bool SetEffect(R2TCameraDescription::R2TCameraPostEffect effect, float *pars, int count) const 
  { 
    if (_custom) _tgtPars->SetEffect(effect, pars, count);
    return _custom; 
  }
  __forceinline bool SetQuality(R2TCameraDescription::R2TRenderQuality quality) const 
  { 
    if (_custom) _tgtPars->SetQuality(quality);
    return _custom;
  }
};

TypeIsMovableZeroed(R2TCamera);

template<>
struct FindArrayKeyTraits<R2TCamera>
{
  typedef const RString& KeyType;

  /// check if two keys are equal
  static bool IsEqual(KeyType a, KeyType b) {return a == b;}

  /// get a key from an item
  static KeyType GetKey(const R2TCamera &a) {return a.Name();}
};

struct R2TTransportTgt
{
  RString _targetName;

  R2TTransportTgt(const RString &targetName): _targetName(targetName) {}
  ~R2TTransportTgt();
};

TypeIsMovableZeroed(R2TTransportTgt);

#endif

namespace enf
{
  class PhysicsManager;
}

class World
{
protected:
  Engine *_engine;

  enf::PhysicsManager* _pPhysicsManager;

  SRef<AbstractUI> _ui;
  /// list of displays
  AutoArray<DisplayInfo> _displays;
  /// user defined (scripted) dialogs
  SRef<AbstractOptionsUI> _userDlg;
  /// main menu or similar UI
  SRef<AbstractOptionsUI> _options;
  /// radio traffic display
  SRef<AbstractOptionsUI> _channel;
  /// radio traffic display
  SRef<AbstractOptionsUI> _chat;
  /// radio traffic display
  SRef<AbstractOptionsUI> _voiceChat;
  SRef<ControlsContainer> _warningMessage;
  /// cutscene cinematic border
  SRef<ControlsContainer> _cinemaBorder;
  /// in-game compass
  OLinkPermO(InventoryItem) _compass;
  /// in-game watch
  OLinkPermO(InventoryItem) _watch;
  /// shown map
  OLinkPermO(ItemMap) _mapItem;
  /// in-game miniMap (GPS)
  SRef<AbstractOptionsUI> _miniMap, _miniMapSmall;
  /// active during non-interactive pause during the mission (preloading, scripting)
  IProgressDisplayRef _loadingScreen;
  /// camera positions of all clients
  RefArray<ClientCameraPositionObject> _clientCameraPositions;

#if _USE_BATTL_EYE_SERVER
  SRef<BattlEyeLicenceMsgBox> _beLicenceMsg;
#endif

  Scene _scene;
  /// navigation map
  SRef<AbstractOptionsUI> _map;
  SRef<SensorList> _sensorList;

  /// global namespace for the mission
  Ref<GameDataNamespace> _missionNamespace;
  /// namespace used to manipulate/store variables in User Profile
  Ref<GameDataNamespace> _profileNamespace;

  //! Background progress screen (don't set this object directly, use SetShowMapProgress instead)
  //! If this object is set, then progress screen is drawn (we can test it for the progress screen appearance)
  IProgressDisplayRef _showMapProgress;
  //! Access method to set the _showMapProgress
  void SetShowMapProgress(IProgressDisplay *progressDisplay);

  struct PreloadAsyncContext;
  /// data for async. preload (used in MP)
  SRef<PreloadAsyncContext> _preloadAsyncCtx;
#if _VBS3_UDEFCHART
  //! Name of the user defined chart. Empty string means there is no chart.
  RString _userDefinedChartName;
  //! Flag to determine objects should be drawn on user defined chart (usually false)
  bool _drawObjectsOnUserDefinedChart;
#endif

  //! Last time in year the temperature table was recalculated
  float _lastTemperatureTableResetTimeInYear;
  //! Table of current temperatures (known as Tc table in the analysis)
  float _temperatureTable[TT_G][TT_Dot][TT_K];

#if _VBS3_CRATERS_LATEINIT
  //! Manager for late initialization objects
  LateInitManager _lateInitManager;
#endif

  /// after a certain count of SwitchLandscape perform extended memory cleanup
  /**
  This is done to prevent fragmentation gradually reducing performance when playing very long.
  */
  int _leakRecoveryCounter;
  /// keep the area around the camera locked
  SRef<RectangleObjectLock> _objAroundCam;

  SRef<IAmbLifeManager> _ambientLife;
  SRef<IEnvSoundManager> _envSound;
  
  Ref<JVMUI> _jUI;

  Ref<CameraEffect> _cameraEffect;
  Ref<TitleEffect> _titleEffect;
  RefArray<TitleEffect> _cutEffects;
  LLink<Script> _cameraScript;

  Matrix4 _camaraBase;
  Matrix4 _camaraTransform;
  /// Old style ('sqs') scripts
  FindArrayKey<ScriptItem, ScriptItemTraits> _scripts;
#if USE_PRECOMPILATION
  /// New style ('sqf') scripts
  FindArrayKey<ScriptVMItem, ScriptVMItemTraits> _scriptVMs;
#endif
  /// Global (mission controlling) FSMs
  AutoArray<FSMItem> _FSMs;
  RefArray<JVMFSM> _jvmFSMs;
  /// the unique (during the mission) ID of the next created FSM
  int _nextFSMId;
  /// this lock will avoid calling SimulateScripts from SimulateScripts itself (opermap false)
  bool _insideSimulateScripts;
#if _VBS2 // allow seagull difficulty option
  bool _lockTitleEffect;
#endif
#if _VBS2 // interact with vehs
  bool _IWVActive;
#endif
#if _VBS2 // death message difficulty option
  bool _friendlyFireMsgSideOnly;
#endif

  Ref<EntityAIType> _preloadedVType[NPreloadedVTypes];
  Ref<EntityType> _preloadedEType[NPreloadedETypes];
  //@{ info needed to keep and unload preloaded type
  struct PreloadedType
  {
    Ref<EntityType> _type;
    int _style;
    
    PreloadedType() {}
    PreloadedType(EntityType *type, int style) :_type(type),_style(style) {}
  
    ClassIsMovable(PreloadedType);
  };
  AutoArray<PreloadedType> _preloadedType;
  //@}

  /// list of active addons - only addons in this list may be used in CreateVehicle
  ParamOwnerList _activeAddons;

  /// principal game-play mode
  GameMode _mode;
  /// game-play mode backup used during serialization
  GameMode _modeBackup;
  
  float _acceleratedTime;
  /// used for timing corrections when using vsync timing
  float _vsyncErrorApplied;
  /// simulation may indicate game was paused and frames dropped are therefore likely
  bool _gameWasNotPaused;

  /// request for synchronous preload (whenever camera position will be known in mission)
  bool _preloadAsked;

  /// use night vision during cutscene
  bool _camUseNVG;

  /// use thermal vision during cutscene
  bool _camUseTi;
  int _camTiIndex;

  UITime _channelChanged;

  /// list of all cloudlets
  RefArray<Entity> _cloudlets;

  //OffTree
  /// shots and other very accurate vehicles
  EntityList _fastVehicles; 
  
  EntitiesDistributed _vehicles;
  EntitiesDistributed _animals;
  EntitiesDistributed _slowVehicles; // slow vehicles (simulated max. once per 500 ms)
  EntityList _verySlowVehicles; // slow vehicles (simulated max. once per 10 sec) - used for streetlamps
  /// index of the next entity in _verySlowVehicles to be simulated
  int _verySlowToSimulate;

  TargetCategories _targetCategories;


   /// vehicles that are not listed anywhere else (usually soldiers in vehicles)
  RefArray<Entity> _outVehicles;

  WindEmitterList _windEmitterScope;

  PPEManager _ppeManager;

  Time _nearImportanceDistributionTime; // when distributions were calculated
  Time _farImportanceDistributionTime; // when distributions were calculated
  void DistributeImportances(EntitiesDistributed &target, EntityList &list, SimulationImportance prec, const Vector3 *viewerPos, int nViewers);

  void DistributeNearImportances(EntitiesDistributed &list); // redistribute vehicles into near...far lists
  void DistributeFarImportances(EntitiesDistributed &list); // redistribute vehicles into near...far lists

  void DistributeNearImportances(); // redistribute vehicles into near...far lists
  void DistributeFarImportances(); // redistribute vehicles into near...far lists

  void GetViewerList(StaticArrayAuto<Vector3> &viewers); // redistribute vehicles into near...far lists

  FindArray< InitPtr<JoinedObject> > _attached;

#if _ENABLE_INDEPENDENT_AGENTS
  // can be deleted later - will be simulated in Person simulation
  FindArrayKey<AITeamMemberRef> _agents;
  FindArrayKey<AITeamMemberRef> _teams;
#endif
  
  //artillery is enabled
  bool _artilleryEnabled;
  //disable items dropping while swimming
  bool _enableItemsDropping;

  //player movement limitation - used for helicopter gauges
  Vector3 _moveMinBordersHUD; //mix speed [m/s], altitude [m], direction [rad]
  Vector3 _moveMaxBordersHUD; //max speed [m/s], altitude [m], direction [rad]
  Vector3 _movePositionHUD;
  Ref<Entity> _targetHUD;

  // dynamic (created during mission) locations
  // Locations _locations;
  // plain structure used to enable attaching to objects
  RefArray<DynamicLocation> _locations;

  OLinkObject _cameraOn;
#if _VBS3
  OLinkObject _cameraTarget;
#endif
  OLinkPermO(Person) _playerOn;
  OLinkPermO(Person) _realPlayer;

  bool _nativeEvent[NNativeEvent];


  /// position of _cameraOn relative to camera
  /**
  This is to avoid numerical inaccuracies during the world space calculations
  */
  Matrix4 _cameraOnRelativeToCamera;
  
  float _actualOvercast; // use for weather changes simulation
  float _wantedOvercast;
  float _speedOvercast; // speed of change

  float _actualFog;
  float _wantedFog;
  float _speedFog; // speed of change

  float _actualWind;
  float _wantedWind;
  float _speedWind; // speed of change

  /// current eye accommodation level - used for smooth change
  float _eyeAccom;
  //! Custom eye accom (if >= 0, then set the _eyeAccom immediately)
  float _customEyeAccom;
  /// camera cut - eye accommodation should chance immediately
  bool _eyeAccomCut;
  /// how much are we blinded by direct lights (connected to flares)
  float _blindIntensity;

  // geography latitude - positive is south
  float _latitude;
  // geography longitude - positive is east
  float _longitude;
  // elevation offset from config file
  float _elevationOffset;
    
#if _VBS3 
  //store World UTM info, this is always the info stored in the World file
  UTMInfo _utmInfo;
  //store water level in the world file
  float _waterLevel;
#endif
  
  float _weatherTime; // actual time
  float _nextWeatherChange; // time when _wantedOvercast should be reached 
  float _nextWeatherChangeOvercast; // time when _wantedOvercast should be reached 

  float _timeToSkip;

  bool _playerManual;
  bool _playerSuspended;

  bool _editor; // world can be create in world editing or game playing mode
  bool _showMap;
  bool _showMiniMap, _showMiniMapToggle; //toggle miniMap is smaller
  bool _noDisplay;
  bool _enableSimulation; // used to pause game with cheat
  bool _singleStepSimulation; // used to single step while debugging
  bool _simulationFocus; // true when application should run and render
  bool _enableRadio;
  bool _enableSentences; // similar to enableRadio, but this one allows KBTell sentences RadioMessageTalk

  bool _cameraExternal;
  
  bool _endDialogEnabled;
  bool _endForced;

  bool _forceMap;
  bool _autoSaveRequest;
  bool _loadAutoSaveRequest;

  /// Request to disable saving (works together with _autoSaveRequest)
  bool _disableSavingRequest;

  /// Saving the game is enabled by the script
  bool _savingEnabled;

  DWORD _showMapUntil;
  bool _showMapforced;

  int _userInputDisabled;

#if _HELISIM_ROTORLIB
  bool _enableUserActions;
  bool _enableStressDamage;
#endif

  /// smooth camera distance to avoid camera in the object
  float _camMaxDist[MaxCameraType];

  /// user selected camera state
  CameraType _camTypeMain;
  /// target camera state based on _camTypeMain
  CameraType _camTypeNew;
  /// current camera state
  CameraType _camTypeOld;
  /// transition between _camTypeNew (1) and _camTypeOld (0) - using camera movement
  float _camTypeNewFactor;
  /// camera transition using fade-in/fade out
  /**
  Value 0-0.5 means _camTypeOld is fading out, 0.5-1 means _camTypeNew is fading in
  While _camTypeTransition <0.5, _camTypeNewFactor == 0.
  */
  float _camTypeTransition;

  Link <RadioChannel> _channelVehicle;
  SRef <RadioChannel> _radio;
  SRef<Speaker> _speaker;
  SRef<RadioChannel> _customRadios[CCCustomN];

  UITime _missionEndExpired;
  RString _endMission;
  bool _endMissionCheated;
  bool _endFailed;

  bool _isInsideSerialization; // in order to know we are inside World serialization (and Glob.time should not be changed in CleanUpInit)

  AutoArray< Ref<IPostponedEvent> > _postponedEvents;

#if _HELISIM_ROTORLIB && !_DUMMY_ROTORLIB
#if _HELISIM_ROTORLIB_SIM_THREAD
  RTDSimulateThread _rtdThread;
#endif
  WorldRTD  _rtdWorld;
#endif

  struct ItemInfo
  {
    float _x, _y, _w, _h, _z;
    void Load(ParamEntryPar cls)
    {
      _x = cls >> "x"; _y = cls >> "y";
      _w = cls >> "w"; _h = cls >> "h";
      _z = cls >> "z";
    }
  };
  ItemInfo _watchInfo;
  ItemInfo _compassInfo;

public:
  /// airport description
  struct AirportInfo
  {
    // @{ landing glide slope description
    /// touch point
    Vector3 _ilsPos;
    /// direction from touch point into the glide slope, used for AI and autopilot
    Vector3 _ilsDir;
    // @}
    //@{ full taxiway descriptions - used for AI
    AutoArray<Vector3> _taxiIn;
    AutoArray<Vector3> _taxiOff;
    //@}
    //@{ approximate runway and taxiway positions used for drawing and searching
    Vector3 _runwayBeg,_runwayEnd;
    Vector3 _taxiwayBeg,_taxiwayEnd;
    float _runwayWidth;
    float _taxiwayWidth;
    /// should taxi-way be drawn on the map?
    bool _drawTaxiway;
    //@}
    /// which side is holding this airport by default
    TargetSide _side;
    
    bool STOL() const;

    ClassIsMovableZeroed(AirportInfo)
  };
  
  __forceinline enf::PhysicsManager* GetPhysicsManager() const
	{
		return _pPhysicsManager;
	}

#if _HELISIM_RENDER_TO_TEXTURE

  FindArrayKey<R2TCamera, FindArrayKeyTraits<R2TCamera> > _r2tSourceCamera;
  AutoArray<R2TTransportTgt> _r2tTransportTgts;
  OLinkO(Transport) _r2tLastTransport;

  bool IsR2TSrcCamUsed(const RString &name) const;
  void RemoveR2TSourceCameras();
  void RemoveR2TCustomCameras();
  void RemoveR2TVehicleCameras();
  
  void AddR2TSourceCamera(CameraEffect *effect, Object *camObject, RString name);
  void RemoveR2TSourceCamera(RString name);

  void DrawSceneR2T(Entity *camInsideVehicle, bool frameRepeated, float deltaT);

  __forceinline R2TCamera* SelectR2TCamera(const RString &name)
  {
    int idx = _r2tSourceCamera.FindKey(name);
    return (idx != -1 ? &_r2tSourceCamera[idx] : NULL);
  }
#endif

#if _ENABLE_CHEATS
  OLinkO(Entity) _soundCtrlsForVeh;
  AutoArray<RString> _soundCtrlsWanted;
  void ShowEnvSoundsDiag() { if (_envSound) _envSound->ShowDiag(); }
#endif

  // nearby ItemTransmitters
  RefArray<InventoryItem> _nearbyItemTransmitters;
  AutoArray<float> _transmittersFreqSent;
  bool _nearbyItemTransmittersChanged;

protected:
  AutoArray<AirportInfo> _airports;

  float _gridOffsetX;
  float _gridOffsetY;
  AutoArray<GridInfo> _gridInfo;
  AutoArray<SupersonicCrackSource> _supersonicSource;
  AutoArray<FakeBulletFlySource> _fakeBulletsSource;

#if _ENABLE_CHEATS
  float _cpuStress;
#endif

  /// terrain grid set by scrip - hard override
  float _terrainGridHard;
  /// view distance set by script - hard override
  float _viewDistanceHard;
  /// view distance set by script - use best value
  float _viewDistanceMin;
 
  AutoArray<UserFileErrorInfo> _fileErrorsReported;
  AutoArray<UserFileErrorInfo> _fileErrorsIgnored;

#if _VBS2 // key bindings
  AutoArray<KeyBinding> _boundkeys;
  int _boundKeyCount;
  bool _pauseSimulation;
#endif

  AutoArray<VehicleInitMessage> _initMessages;

  /// queue of banks waiting for hash check
  FindArrayKey<CheckHashItem> _checkHashQueue;
  /// currently checked hash
  SRef<BankHashCalculatorAsync> _checkingHash;
#if _VBS3 // support for dynamic briefing
  RString _alternateBriefing;
#endif

#if _ENABLE_CHEATS
  //!{ Video capturing parameters
  RString _videoCaptureFolderName;
  RString _videoCaptureFilePrefix;
  float _videoCaptureFrameDuration;
  float _videoCaptureTimeToReadResources;
  int _videoCaptureFrameID;
  //!}
#endif

#if _VBS3
  bool _allowMovementCtrlsInDlg;
#endif

  /// camera shakes manager
  CameraShakeManager _cameraShakeManager;
  /// true if camera is inside vehicle
  bool _isCameraInsideVeh;

  typedef RefArray<VideoBufferInfo> TVideoBufferInfoList;
  TVideoBufferInfoList _videoBufferInfo;

  RefArray<VideoBufferInfo, MemAllocLocal<Ref<VideoBufferInfo>, 8> > _toDeleteVideoBuffInfo;

  Time _nextSchedulerUpdateZombieAI;
  Time _nextSchedulerUpdateReSpawn;

protected:
  void BrowseCamera(int dir);

public:
  World(Engine *engine, bool editor=false, bool skipPhysicsManager=false);
  virtual ~World();

#if _VBS3_CRATERS_LATEINIT
  //! Add item to be lately initialized
  void RegisterLateInitObject(Object *object, float priority) {_lateInitManager.Register(object, priority);}
#endif

  void SetActiveChannels();

  void InitCameraPos() {SelectCameraPosition(NULL, 0.0f);}

  __forceinline bool IsCameraInsideVeh() const { return _isCameraInsideVeh; }

private: // no copy possible
  World(); // no default constructor
  World(const World &src);
  void operator=(const World &src);

public:
  void Init(bool editor);

  //temporary buffer for playing videos
  int OpenVideoBuffer(const char *name, OggDecoder *decoder);
  void CloseVideoBuffer(int ident);
  int  CopyVideoBuffer(char* buffer, int ident );
  bool VideoBufferEof(int ident);
  bool ReadVideoBuffer(VideoBufferInfo* videoBufferInfo);
  void ReleaseVideoBuffers();
  bool ReadVideoBuffer(int ident);
  void ReadAllVideoBuffers();
  int ResetVideoBuffer(int ident);
  void TerminateVideoBuffers();
  void PauseVideoBuffers(bool pause);

  void GetEyeAccomPars(float &eyeAccom, bool &eyeAccomCut, float &customEyeAccom) const;
  void SetEyeAccomPars(float eyeAccom, bool eyeAccomCut, float customEyeAccom);

  Matrix4 GetCamaraBase() {return _camaraBase;};
  Matrix4 GetCamaraTransform() {return _camaraTransform;};

  void UpdateTransmitterItem(InventoryItem *radio);

#if _VBS2 // key bindings
  int AddKeyBinding(RString keycode,RString command);
  int AddKeyBinding(int keycode,RString command);
  void RemoveKeyBinding(int index);
#endif

#if _VBS3
  void SetAllowMovementCtrlsInDlg(bool allow) {_allowMovementCtrlsInDlg = allow;}
  bool GetAllowMovementCtrlsInDlg() {return _allowMovementCtrlsInDlg;}
  // support for dynamic briefing
  void SetBriefing(RString briefing) {_alternateBriefing = briefing;}
  const RString &GetBriefing() {return _alternateBriefing;}
#endif

#if _VBS2 // death message difficulty option
  void SetFriendlyFireMsgSideOnly(bool mode) {_friendlyFireMsgSideOnly = mode;}
  bool GetFriendlyFireMsgSideOnly() {return _friendlyFireMsgSideOnly;}
#endif

#if _ENABLE_CHEATS
  Object *GetObserver() const
  {
    Object *observer = _map ? _map->GetObserver() : NULL;
    return observer ? observer : CameraOn();
  }
#endif

  void InitGeneral(); // at the beginning of InitXX
  void InitGeneral(ParamEntryPar cfg);
  void InitGeneral(const ArcadeIntel &intel);
  void InitClient(); // InitVehicles for multiplayer client
  bool GetGrassEnabled() const;
  // _RENDER_TO_TEXTURE
  void GetCurrentTerrainSettings(float &viewDist, float &gridSize, GameMode mode, bool isR2T = false) const;
  void AdjustSubdivision(GameMode mode);
  /// adjust derived visibility properties, like shadow distance
  void AdjustDerivedVisibility();
  void InitAmbientLife(ParamEntryPar cfg);
  /// search for ambient life area by its name 
  IAmbLifeArea *FindAmbientArea(RString name) const {return _ambientLife ? _ambientLife->FindArea(name) : NULL;}
  
  /// set view distance by script - hard override
  void SetTerrainGridHard(float grid, bool initCache);
  /// set view distance by script - hard override
  void SetViewDistanceHard(float viewDist) {_viewDistanceHard = viewDist;}
  /// set view distance by script - use best value
  void SetViewDistanceMin(float viewDist) {_viewDistanceMin = viewDist;}

  /// return view distance set by script - hard override
  float GetViewDistanceHard() const {return _viewDistanceHard;}

  void OnPreferredViewDistanceChanged(GameMode mode) {AdjustSubdivision(mode);}
  void OnPreferredTerrainGridChanged(GameMode mode) {AdjustSubdivision(mode);}

  /// react to entity quick movement (avoid delay caused by incremental updates, used on get-out)
  void OnEntityMovedFar(EntityAI *obj);
  /// react to entity embarking a vehicle
  void OnEntityGetIn(EntityAI *soldier,Transport *trans);

  void OnKill(EntityAI *killer, EntityAI *obj);

  bool InitVehicles(IMission &t);

  void FinishMissionInit(bool progress);

  void InitLandscape(Landscape *landscape);
  void InitEditor(Landscape *landscape, Entity *cursor); // buldozer init

  void AutoSaveRequest() {_autoSaveRequest = true;}
  void LoadAutoSaveRequest() {_loadAutoSaveRequest = true;}

  /// called when a new mission is starting
  void OnInitMission(bool preload);

  /// called once mission is fully loaded, before first frame is rendered
  void PreloadMission();
  /// called when the mission is finished
  void OnFinishMission();

  /// create fade in effect on mission or cutscene start
  void FadeInMission();

  /// used during mission briefing to preload data - called repeatedly
  bool PreloadAroundCamera(float maxTime);

  /// used during mission briefing to preload data - called once
  void PreloadAroundCameraSync(float maxTime);

  /// used when going ingame in MP
  bool PreloadAroundCameraAsync(PreloadAsyncContext &ctx);
  /// check if synchronous preload of data around camera is needed
  bool IsPreloadAsked() const {return _preloadAsked;}

  /// set that synchronous preload of data around camera is needed
  void AskPreload() {_preloadAsked = true;}

  /// used on UI discontinuities to reset any cumulated vsync errors
  void ResetTiming();
  
  /// preload 1st person and optics views
  bool Preload1stPerson(bool logging=false);
  
  /// preload data which are used very often
  bool PreloadBasicData(bool logging=false);
  
#if _HELISIM_ROTORLIB && !_DUMMY_ROTORLIB
  WorldRTD *GetRTDWorld() {return &_rtdWorld;}
  inline AutoArray<ForceGeneratorRTD> &GetExternalForceGeneratorsRTD() {return _rtdWorld._externalForceGeneratorsRTD;};
#endif

#if _ENABLE_BULDOZER
  //@{ ObjView/Buldozer support

  void SetViewerPhase(float time);
  float GetViewerPhase() const;

  void SetViewerRTMName(RString rtmName);

  void ReloadViewer(void *buf, int len, const char *classDesc, const char *filename, SkeletonSource *ss=NULL);
  void ReloadViewer(const char *filename, const char *classDesc, SkeletonSource *ss=NULL);

  /// set given model.cfg animations 
  void SetViewerController(RString name, float value, float &minVal, float &maxVal);
  /// get list of all model.cfg animations
  void GetViewerControllers(FindArrayRStringCI &controllers) const;
  //@}
#endif
  /// Force using of night vision in cut-scenes
  void SetCamUseNVG(bool set) {_camUseNVG = set;}
  void SetCamUseTi(bool set, int index) { _camUseTi = set; _camTiIndex = index; }
#if _VBS3 // IsCamUseNVG()
  bool IsCamUseNVG() {return _camUseNVG;}
  void PauseSimulation(bool pause){ _pauseSimulation = pause; }
  bool IsPaused(){ return _pauseSimulation; }
  //sends World info to hla and set's new Grid
  void SetUtmInfo(UTMInfo utmInfo);
#endif
  //! Sets the eye accommodation to a custom value (-1 to keep it automatic)
  void SetEyeAccom(float eyeAccom) {_customEyeAccom = eyeAccom;}
  //! Gets the eye accommodation (user settings);
  float GetCustomEyeAccom() const {return _customEyeAccom;}

  //@{ get airport information
  /// total number of airports/airstrips on the island
  int GetAirportCount() const;
  /// get airport position - used for map drawing
  void GetAirportRunwayRect(int id, Vector3 *runway, Vector3 *taxi);

  /// get ILS cone based on the plane position  
  int GetILS(Vector3 &pos, Vector3 &dir, bool &stol, TargetSide side, bool canSTOL, int airportId=-1) const;

  private:
  int GetAirportId(Vector3Par pos, Vector3Par dir, TargetSide side, bool canSTOL, int airportId=-1) const;
  
  public:
  /// get airport info based on the plane position
  const AirportInfo *GetAirport(Vector3Par pos, Vector3Par dir, TargetSide side, bool canSTOL, int airportId=-1) const;

  TargetSide GetAirportSide(int airportId) const;
  void SetAirportSide(int airportId, TargetSide side);

  const AutoArray<Vector3> &GetTaxiInPath(TargetSide side, Vector3Par pos, Vector3Par dir, int *airportId=NULL) const;
  const AutoArray<Vector3> &GetTaxiOffPath(TargetSide side, Vector3Par pos, Vector3Par dir, int *airportId=NULL) const;

  // check which airport are we currently on
  int CheckAirport(Vector3Par pos, Vector3Par dir) const;
  //@}

  float GetGridOffsetX() const {return _gridOffsetX;}
  float GetGridOffsetY() const {return _gridOffsetY;}
  const GridInfo *GetGridInfo(float zoom) const;
  void ParseCfgWorld(ParamEntryPar cls);

  void CleanUpDeinit();
  void CleanUpInit();
  void CleanUp();
  // make world clear
  void Clear() {CleanUp();}
  void Reset();
  
  bool IsUserInputEnabled() const;
  void DisableUserInput(bool disable=true);

  /// returns whether player or his vehicle has GPS enabled
  bool HasPlayerGPS() const;
  /// returns whether player or his vehicle has Map enabled
  bool HasPlayerMap() const;
  /// returns whether the unit or its vehicle has radio station supporting Side and Group channels
  bool HasUnitRadio(AIBrain *unit) const;

  void SetCameraEffect(CameraEffect *effect);
  CameraEffect *GetCameraEffect() const {return _cameraEffect;}

  void SetCameraScript(Script *script);
  Script *GetCameraScript() const {return _cameraScript;}

  void StartCameraScript(Script *script);
  void TerminateCameraScript();
  
  /// calculate auto focus distance for given camera
  float AutoFocus(VisualStateAge age, Object *ignore, Vector3Par pos, Vector3Par dir, float distanceFactor=1, float facingImportance=1, float minDistance=0);
  /// set title effect for displaying
  void SetTitleEffect(TitleEffect *effect);
  void TerminateTitleEffect(float fadeOutTime);
  TitleEffect *GetTitleEffect() const {return _titleEffect;}
#if _VBS2 // allow seagull difficulty option
  void LockTitleEffect( bool lockTitleEffect ) {_lockTitleEffect = lockTitleEffect;}
#endif
#if _VBS2 // interact with vehs
  bool IsIWVActive() {return _IWVActive;}
#endif

  void FreelookChange(bool active);

  bool IsEndDialogEnabled() const {return _endDialogEnabled;}
  void EnableEndDialog(bool enable=true) {_endDialogEnabled = enable;}

  bool IsEndForced() const {return _endForced;}
  void ForceEnd(bool force=true) {_endForced = force;}

  void ClearCutEffects();
  void SetCutEffect(int layer, TitleEffect *effect);
  void TerminateCutEffect(int layer, float fadeOutTime);
  const RefArray<TitleEffect> &GetCutEffects() const {return _cutEffects;}

  void StartLoadingScreen(RString text, RString resource=RString());
  void EndLoadingScreen() {_loadingScreen = NULL;}
  void UpdateLoadingScreen(float progress);

  JVMUI *GetJVMUI() const {return _jUI;}

  void AddScript(Script *script, bool simulate);
  void TerminateScript(Script *script);

#if USE_PRECOMPILATION
  void AddScriptVM(ScriptVM *vm, bool simulate);
#endif

  const AutoArray<FSMItem> &GetFSMs() const {return _FSMs;}

  /// register the new global (valid during the mission) FSM
  void AddFSM(FSM *fsm);
  /// search the global (valid during the mission) FSM by the unique id
  FSM *FindFSM(int id) const;
  /// the unique id for the new global (valid during the mission) FSM
  int NewFSMId() {return _nextFSMId++;}

  void AddJVMFSM(JVMFSM *fsm);

#if _ENABLE_CHEATS
  void SetCPUStress(float time) {_cpuStress = time;}
#endif

  void AddAttachment(JoinedObject *attach);
  void RemoveAttachment(JoinedObject *attach);

  void DeleteAnyVehicle(Entity *vehicle);
  void RemoveAnyVehicle(Entity *vehicle);


  //@{ wind emitter management
  const WindEmitterList &GetWindEmitterList(WindEmitterList &work, Vector3Par pos, float radius);
  Vector3 GetWind(Vector3Par pos, const WindEmitterList &emittors);
  Vector3 GetWindSlow(Vector3Par pos, const WindEmitterList &emittors);
  
  /// any objects rendered withing the scope will be animated accordingly if needed
  void BeginWindEmitterScope(const Camera *cam, float radius);
  void EndWindEmitterScope();
  const WindEmitterList &GetWindEmitterScope() const {return _windEmitterScope;}
  //@}

  void AddVehicle(Entity *vehicle);
  void RemoveVehicle(Entity *vehicle);
  void InsertVehicle(Entity *vehicle);
  void DeleteVehicle(Entity *vehicle);

  //! find object based on object id
  OLinkObject FindObject(const ObjectId &id) const;

  void AddAnimal(Entity *vehicle) {_animals.Add(vehicle);}
  void RemoveAnimal(Entity *vehicle) {_animals.Remove(vehicle);}
  void InsertAnimal(Entity *vehicle) {_animals.Insert(vehicle);}
  void DeleteAnimal(Entity *vehicle) {_animals.Delete(vehicle);}

#if _VBS3 // GVehicleOnCreated
  void AddSlowVehicle( Entity *vehicle ) 
  {
    _slowVehicles.Add(vehicle);
    void ProcessCreateVehicleEH(Entity *veh, bool isSlowVehicle);
    ProcessCreateVehicleEH(vehicle,true);
  }
#else

  void AppendPostponedEvent(IPostponedEvent *func)
  {
    _postponedEvents.Append(func);
  }

  void EvalPostponedEvents();

  VehicleListType GetListType(EntityList *list);

  void AddSlowVehicle(Entity *vehicle)
  {
    if (vehicle->SimulationPrecision() > 10.0f)
      _verySlowVehicles.Add(vehicle);
    else
      _slowVehicles.Add(vehicle);
  }
#endif
  void InsertSlowVehicle(Entity *vehicle)
  {
    if (vehicle->SimulationPrecision() > 10.0f)
      _verySlowVehicles.Insert(vehicle);
    else
      _slowVehicles.Insert(vehicle);
  }

  void AddFastVehicle(Entity *object) {_fastVehicles.Add(object);}
  
  template <class Functor>
  bool ForEachFastVehicle(Functor f) const {return _fastVehicles.ForEach(f);}

  template <class Functor>
  bool ForEachVehicle(Functor f) const {return _vehicles.ForEach(f);}

  template <class Functor>
  bool ForEachSlowVehicle(Functor f) const
  {
    bool finished = _slowVehicles.ForEach(f);
    if (!finished)
      finished = _verySlowVehicles.ForEach(f);
    return finished;
  }

  template <class Functor>
  bool ForEachOutVehicle(Functor f) const {return _outVehicles.ForEachF(f);}
  
  template <class Functor>
  bool ForEachAnimal(Functor f) const {return _animals.ForEach(f);}
  
  template <class Functor>
  bool ForEachCloudlet(Functor f) const {return _cloudlets.ForEachF(f);}
  
  enum QueryFlags
  {
    QF_NONE = 0,
    QF_STATIC = 1,
    QF_DYNAMIC = 2,
    QF_ORIGIN_DISTANCE = 4 //!When we don't care about distance to object bounds
  };
  
  class ObjectQuery
  {
  public:
    ObjectQuery()
    {
    }
    //!Give opportunity to filter out object based on cheap tests, before actual geometric test
    virtual bool Check(Object* obj) const { return true; }
    virtual bool operator () (Object *obj) const=0;
  };

  bool QueryObjects(ObjectQuery& query, Vector3Par origin, float radius, unsigned int queryFlags);
  bool QueryObjects(ObjectQuery& query, Vector3Par origin, float radius, Vector3Par direction, float fov, unsigned int queryFlags);
  bool QueryObjects(ObjectQuery& query, Vector3Par mins, Vector3Par maxs, unsigned int queryFlags);
  bool UseObjects(ObjectQuery& query, Vector3Par mins, Vector3Par maxs, unsigned int queryFlags);

  void AddOutVehicle(Entity *object);
  void RemoveOutVehicle(Entity *object);
  bool IsOutVehicle(Entity *object);

  bool ValidateOutVehicle(Entity *veh) const;
  bool ValidateOutVehicles() const;
  
  /// add given entity as a potential supersonic crack source
  void AddSupersonicSource(Entity *veh);
  /// simulate supersonic cracks
  void SimulateSupersonicCracks();
  /// reset all supersonic crash info based on current world state
  void ResetSupersonicCracks();

  /// add given entry as a potential bullet sound
  void AddFakeBulletsSource(Entity *veh);
  /// simulate flying bullets sounds
  void SimulateFakeBullets();
  /// reset flying sound
  void ResetFakeBullets();

  Entity *GetCloudlet(int i) const {return _cloudlets[i];}
  int NCloudlets() const {return _cloudlets.Size();}
  void AddCloudlet(Entity *object) {_cloudlets.Add(object);}
  void RemoveCloudlet(Entity *object) {_cloudlets.Delete(object);}

  RefArray<DynamicLocation> &GetLocations() {return _locations;}
  const RefArray<DynamicLocation> &GetLocations() const {return _locations;}

  void PerformSound(Entity *inside, float deltaT);

  void UnloadSounds();

  /// interface for callbacks
  class UseWaitingTime
  {
    public:
    virtual void operator()() const {}
  };

  /// Get the path to the file currently used during play (such as model of tree) near the given position. Used by server to ask relevant signature checks.
  RString RandomUsedFile(Vector3Par pos) const;

  // if camera is inside of any vehicle, return it, otherwise return NULL
  Object *GetCamInsideVehicle() const;
  /// prepare Scene object lists for drawing
  void PrepareDraw(Object *camInsideVehicle, bool inPreload, float preloadDist);
  /// draw & generate ambient traffic cars
  void DrawAmbientTraffic();

  void UpdateAttachedPositions();
  /// draw diagnostics
  void DrawDiags(Object *camInsideVehicle);
  /// complete World rendering
  void Draw(Scene::DrawContext &drawContext, const GridRectangle &groundPrimed, Entity *camInsideVehicle,
    bool frameRepeated, bool wantNV, bool wantFlir, float deltaT, const UseWaitingTime &useTime);
  /// simulation including rendering
  void Simulate(float deltaT, bool &enableSceneDraw, bool &enableHUDDraw);
  /// RotorLib world simulation
  void SimulateRTD(bool doSim, float deltaT);
  // simulate turrets post effect used in vehicles
  void SimulatePostEffect(bool drawingEnable);

  void AdjustEyeAccom(bool wantNV
#if _HELISIM_RENDER_TO_TEXTURE
    , bool isR2T, RenderTargetID rtid = R2TUninitialized
#endif
    );
  /// AI simulation
  void PerformAI(float deltaT);

  void CheckMissionEnd();
  /// network transfers
  void ProcessNetwork();
  /// physical simulation
  void SimulateAllVehicles(float deltaT);
  /// weather and other global processes simulation
  void SimulateLandscape(float deltaT);
  //! Update the temperature table
  void UpdateTemperatureTable(float temperature, float temperatureBlack, float temperatureWhite, float deltaT);
  //! Do the simulation step, update the Engine values
  void SimulateTemperature(float deltaT, bool enabled);
  /// user scripts processing
  void SimulateScripts(float deltaT, float timeLimit);

  //FIXME: TEMPORARY, move it elsewhere
  void ZombieAIScheduler(float deltaT);
  void SuspendEntity(Entity *vehicle);
  void DeleteSuspendedEntity(Entity *vehicle);
  void WorldResScheduler(float deltaT);

  /// check if all object grids in given rectangle are ready
  bool CheckObjectsReady(int xMin, int xMax, int zMin, int zMax, bool fast) const;

  /// check intersection with current camera object depending on the camera mode
  float IntersectWithCockpit(Vector3Par from, Vector3Par dir, float minDist=0, float maxDist=1e5) const;
  
protected:
  //@{ helpers for MoveOutAndDeleteEntity
  void DeleteEntity(Entity *vehicle);
  void MoveOutEntity(Entity *vehicle);
  //@}

public:
  /// functor helper for MoveOutAndDelete
  class MoveOutAndDeleteEntity;
  
  void MoveOutAndDelete(EntityList &vehicles, int *keepIndex=NULL);
  void MoveOutAndDelete(EntitiesDistributed &vehicles);
  void SimulateOnly(EntityList &vehicles, float deltaT, VehicleSimulation simul, SimulationImportance prec);
  void SimulateOnly(EntitiesDistributed &vehicles, float deltaT, VehicleSimulation simul, VehicleSimulation simulOver, SimulationImportance maxPrec);
  /// simulate all decisions made by entities
  void SimulateVehiclesAI(float deltaT, Entity *cameraVehicle, float minOptimizeDistance);
  void SimulateVehicles(float deltaT, VehicleSimulation simul, VehicleSimulation simulOver, SimulationImportance minPrec);
  // TODO: remove completely?
  void SimulateBuildings(float deltaT, VehicleSimulation simul) {}
  void SimulateSlowVehicles(float deltaT, VehicleSimulation simul);

  // camera can be only inside normal or slow vehicle
  void SimulateFastVehicles(float deltaT, VehicleSimulation simul);
  void SimulateCloudlets(float deltaT);

  Transport *FindFreeVehicle(Person *driver) const;

  void VehicleSwitched(Object *from, Object *to);

  //! Switching to other camera
  /*!
    \param cameraHasChanged Flag to determine the eye or camera belongs to a different subject and that accomodation should be reset
  */
  void SwitchCameraTo(Object *vehicle, CameraType cam, bool cameraHasChanged);
  void BrowseCamera(Entity *vehicle);
  /// called when camera pos or dir is changed abruptly (camera cut)
  void OnCameraChanged();
  void OnTimeSkipped(float time);
  
  /// object at which camera is looking, used for diagnostics
  Object *GetCameraFocusObject() const;
  
  Object *CameraOn() const {return _cameraOn;}
  Entity *CameraOnVehicle() const;

  /// used to recognize when to enable Scene Drawing in order not to preload the scene multiple times on mission start
  bool IsSceneReady() const { return CameraOn() || GetCameraEffect() || GetTitleEffect() || GetCameraScript(); }
   
  void SimulateRestTrackedByCamera();
  bool IsTrackedByCamera(const Object *obj) const;
  Person *PlayerOn() const;
  Person *GetRealPlayer() const;
  void SetRealPlayer(Person *veh);

  bool CheckNativeEvent( NativeEvent condition ) const;
  void SetNativeEvent( NativeEvent condition );
  void ResetNativeEvents();

  Matrix4Val GetCameraOnRelativeToCamera() const {return _cameraOnRelativeToCamera;}

  bool PlayerManual() const {return _playerManual;}
  void SetPlayerManual(bool manual) {_playerManual = manual;}

  bool GetPlayerSuspended() const {return _playerSuspended;}
  void SetPlayerSuspended(bool susp) {_playerSuspended = susp;}

  AIBrain *FocusFromCamera(Object *camera) const;
  AIBrain *FocusOn() const {return FocusFromCamera(CameraOn());}

  GameMode GetMode() const {return _mode;}
  GameMode GetModeBackup() const {return _modeBackup;}
  RString GetEndMode() const {return _endMission;}
  EEndMode GetEEndMode() const {return GetEnumValue<EEndMode>(cc_cast(_endMission));}
  bool IsEndModeCheated() const {return _endMissionCheated;}
  bool IsEndFailed() const {return _endFailed;}

  /// some functions should be disabled when cutscene is played back
  bool CheckCutscene() const {return _cameraEffect.NotNull() || _mode==GModeIntro;}
  
  void SetMode(GameMode mode) {_mode = mode;}
  void SetEndMode(EEndMode mode) { _endMission = FindEnumName(mode); }
  void SetEndMode(RString mode) {_endMission = mode;}
  void SetEndFailed(bool endFailed) {_endFailed = endFailed;}

  RadioChannel &GetRadio() {return *_radio;}
  const RadioChannel &GetRadio() const {return *_radio;}
  Speaker *GetSpeaker() const {return _speaker;}
  void SetSpeaker(RString speaker, float pitch);

  RadioChannel &GetCustomRadio(int index) 
  {
    if (index>=0 && index<CCCustomN) 
    {
      return *_customRadios[index];
    }
    Fail("Incorrect custom radio!");
    return *_customRadios[0];
  }
  RadioChannel &GetCustomRadio(int index) const 
  {
    if (index>=0 && index<CCCustomN) 
    {
      return *_customRadios[index];
    }
    Fail("Incorrect custom radio!");
    return *_customRadios[0];
  }

  const EntityAIType *Preloaded(PreloadedVType type) const
  {
    Assert(type <= NPreloadedVTypes);
    return _preloadedVType[type];
  }
  const EntityType *Preloaded(PreloadedEType type) const
  {
    Assert(type <= NPreloadedETypes);
    return _preloadedEType[type];
  }
  
  void SwitchPlayerTo(Person *veh);

  float GetAcceleratedTime() const {return _acceleratedTime;}
  void SetAcceleratedTime(float time) {_acceleratedTime = time;}
  
  void SkipTime(float time) {_timeToSkip = time;}
  
  AbstractUI *UI() const {return _ui;}
  AbstractOptionsUI *Options() const {return _options;}
  AbstractOptionsUI *UserDialog() const {return _userDlg;}
  AbstractOptionsUI *Map() const {return _map;}
  AbstractOptionsUI *ChatChannel() const {return _channel;}
  AbstractOptionsUI *Chat() const {return _chat;}
  AbstractOptionsUI *VoiceChat() const {return _voiceChat;}
  ControlsContainer *GetWarningMessage() const {return _warningMessage;}
  
  // set to true when MAIN display should be reinitialized (used when player profile has changed, so some IGUI settings could be reset)
  bool _resetOptions;

  void OnChannelChanged();
  void CheckChatChannel();
  
  /// sometimes (during the game) exit by key (Alt-F4) is not nice
  bool IsExitByKeyEnabled() const;
  bool IsSimulationEnabled() const;
  /// check if data preloading is enabled
  bool IsPreloadEnabled() const;
  /// check if scene rendering is enabled
  bool IsDisplayEnabled() const;
  bool IsUIEnabled() const;
  bool IsCopyFromDVDEnabled() const;
  bool HasOptions() const;
  bool HasMap() const {return _showMap && (_map != NULL);}
  void ShowMap(bool state);
  bool IsShowMapWanted() const {return _showMap && (_showMapUntil == 0);}
  void ShowMapForced(bool state, bool forced);
  bool IsShowMapForced() const {return _showMapforced;}
  void ShowWatch(InventoryItem *item);
  void ShowCompass(InventoryItem *item);
  void ShowMapItem(ItemMap *item);
  const ItemMap *GetMapItem() const;

  void ShowInventoryMenu(const InventoryMenuMessage &msg);

#if _HELISIM_ROTORLIB
  __forceinline bool IsUserActionEnabled() const{return _enableUserActions;}
  __forceinline bool IsStressDamageEnabled()const {return _enableStressDamage;}
  __forceinline void EnableUserActions(bool enable) {_enableUserActions = enable;}
  __forceinline void EnableStressDamage(bool enable) {_enableStressDamage = enable;}
#endif
  bool HasMiniMap() const;
  void CreateMiniMaps();
  void ForceMap(bool force=true) {_forceMap = force;}
  void DestroyOptions(int exitCode);
  void DestroyMap(int exitCode);
  void DestroyChat(int exitCode);
  void DestroyVoiceChat(int exitCode);
  void CreateDSOptions();
  void CreateMainOptions();
  void SetOptions(AbstractOptionsUI *options);
  void CreateEndOptions(int mode);
  void CreateChat();
  void CreateVoiceChat();
  void CreateMainMap();
  // ADDED in Patch 1.01 - reimplementation of WarningMessage
  void CreateWarningMessage(RString text);
  void OnControllerRemoved();
  void OnNetworkDisconnected();
  void OnAskForShutDown();
  void ReportCreateSaveError();
  void ReportUserFileError(const RString &dir, const RString &file, const RString &name, bool once, bool canDelete=false);
  void DoneUserFileError(bool ignored);

#if _USE_BATTL_EYE_SERVER
  void OnBELicenceNotAgreed(RString reconnectGUI, RString reconnectPassword);
#endif

  bool IsInsideSerialization() const {return _isInsideSerialization;}
  void SetIsInsideSerialization(bool val) {_isInsideSerialization = val;}

  bool SetUserDialog(AbstractOptionsUI *dlg)
  {
    if (_userDlg)
      return false;
    else
    {
      _userDlg = dlg;
      return true;
    }
  }
  /// used to enable correctly move the dialog from source SRef
  void SetUserDialog(SRef<AbstractOptionsUI> dlg) {_userDlg = dlg;}
  void DestroyUserDialog() {if (_userDlg) _userDlg.Free();}
  /// used to add user dialog as possible child of another existing user dialog
  void AddUserDialog(Display *dlg);
  /// add a new display
  void AddDisplay(AbstractOptionsUI *display, float priority);
  /// remove a display
  void RemoveDisplay(AbstractOptionsUI *display);
  /// find the display by its IDD
  AbstractOptionsUI *FindDisplay(int idd) const;

  void DrawCinemaBorder(float alpha);

  void UpdatePerfLog();
    
//  void EnableDisplay(bool val=true);
//  void EnableSimulation(bool val=true);
  void EnableRadio(bool val=true) {_enableRadio = val;}
  void EnableSentences(bool val=true) {_enableSentences = val;}
  bool IsRadioEnabled() const {return _enableRadio;}
  bool IsSentencesEnabled() const {return _enableSentences;}
  // format should contain %s for percent rest
  void SetSimulationFocus(bool val) {_simulationFocus=val;}
  bool GetSimulationFocus() const {return _simulationFocus;}

  bool GetRenderingDisabled() const {return _noDisplay;}

  void StartIntro(RString worldName);

  /// execute an action on addon given as an application argument
  void PerformAddonAction(int actionType, RString action, RString *modName=NULL);

  // returns true if event was caught
  bool DoKeyDown(int dikCode);
  // returns true if event was caught
  bool DoKeyUp(int dikCode);
  void DoChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags);
  void DoIMEChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags);
  void DoIMEComposition(unsigned nChar, unsigned nFlags);

  /// get principal camera type state
  CameraType GetCameraType() const {return (_camTypeNewFactor > 0.5f) ? _camTypeNew : _camTypeOld;}
  /// get "more important" of the states
  CameraType GetCameraTypePreferInternal() const;
  void SetCameraType(CameraType cam);
  //@{ query camera type transition state
  CameraType GetCameraTypeOld() const {return _camTypeOld;}
  CameraType GetCameraTypeNew() const {return _camTypeNew;}
  float GetCameraTypeTransition() const {return _camTypeNewFactor;}
  float GetCameraTransition() const {return _camTypeTransition;}
  /// check if given camera is active (may be in transition, in or out)
  bool IsCameraActive(CameraType cam) const {return _camTypeOld==cam || _camTypeNew==cam;}
  bool IsOtherCameraActive(CameraType cam) const {return _camTypeOld!=cam || _camTypeNew!=cam;}
  //@}
  
  // ADDED in Patch 1.04 - needed for sprint with optics
  CameraType GetCameraTypeWanted() const {return _camTypeMain;}
  
  void DisableTacticalView();
  
#if _VBS3 // command view from RTE
  void SetCameraTypeWanted(CameraType camType) {_camTypeMain = camType;}
#endif
  
  /// get current state of observer's eye
  float GetEyeAccom() const {return _eyeAccom;}

  void SetCameraDirection(CameraType type, float heading, float dive);

  Scene *GetScene() {return &_scene;}
  Engine *GetEngine() const {return _engine;}
  void AddSensor(Person *vehicle);
  /// start tracking a target - useful after the target is created
  void AddTarget(EntityAI *vehicle);

  SensorList *GetSensorList() const {return _sensorList;}
  void RemoveSensor(Entity *vehicle);
  void RemoveTarget(Entity *vehicle);

#if _VBS3_UDEFCHART
  //!{ Wrapper around _userDefinedChartName
  RString GetUserDefinedChartName() const {return _userDefinedChartName;}
  void SetUserDefinedChartName(RString userDefinedChartName) {_userDefinedChartName = userDefinedChartName; _userDefinedChartName.Lower();}
  //!}
  //!{ Wrapper around _drawObjectsOnUserDefinedChart
  bool GetDrawObjectsOnUserDefinedChart() const {return _drawObjectsOnUserDefinedChart;}
  void SetDrawObjectsOnUserDefinedChart(bool drawObjectsOnUserDefinedChart) {_drawObjectsOnUserDefinedChart = drawObjectsOnUserDefinedChart;}
  //!}
#endif

  GameState *GetGameState() const {return &GGameState;}
  GameDataNamespace *GetMissionNamespace() {return _missionNamespace;}
  GameDataNamespace *GetProfileNamespace() {return _profileNamespace;}
  //! Save variables from ProfileNamespace into the user profile class ProfileVariables
  void SaveProfileNamespace();
  void LoadProfileNamespace();

  float Visibility(const AIBrain *from, Object *to, float maxAge=FLT_MAX) const;
  Time VisibilityTime(const AIBrain *from, Object *to) const;
  void CheckSoundObstruction(EntityAI *source,  bool inSource, float &obstruction, float &occlusion);

  TargetCategories &GetTargetCategories() { return _targetCategories; }

#if _ENABLE_INDEPENDENT_AGENTS
  int NTeams() {return _teams.Size();}
  AITeamMember *GetTeam(int i) {return _teams[i];} // used in serialization
  int NAgents() {return _agents.Size();}
  AIAgent *GetAgent(int i) {return _agents[i] ? _agents[i]->GetAgent() : NULL;}
  int AddAgent(AITeamMember *teamMember) {return _agents.AddUnique(teamMember);}
  bool RemoveAgent(AITeamMember *teamMember) {return _agents.DeleteKey(teamMember);}
  int AddTeam(AITeamMember *teamMember) {return _teams.AddUnique(teamMember);}
  bool RemoveTeam(AITeamMember *teamMember) {return _teams.DeleteKey(teamMember);}
  int NTeamMembers() const;
#endif

  bool IsEnabledItemsDropping() const {return _enableItemsDropping;}
  void EnableItemsDropping(bool enable) {_enableItemsDropping = enable;}

  bool IsArtilleryEnabled() const {return _artilleryEnabled;}
  void EnableArtillery(bool enable)  {_artilleryEnabled = enable;}

  Vector3 GetMoveMinBordersHUD() {return _moveMinBordersHUD;}; //mix speed, altitude, direction
  Vector3 GetMoveMaxBordersHUD() {return _moveMaxBordersHUD;};  //max speed, altitude, direction
  Vector3 GetMovePositionHUD() {return _movePositionHUD;};  //max speed, altitude, direction
  Entity *GetMoveTargetHUD() {return _targetHUD;};  //max speed, altitude, direction

  void SetMoveMinBordersHUD(Vector3 value) { _moveMinBordersHUD = value;}; //mix speed, altitude, direction
  void SetMoveMaxBordersHUD(Vector3 value) {_moveMaxBordersHUD = value;};  //max speed, altitude, direction
  void SetMovePositionHUD(Vector3 value) {_movePositionHUD = value;};  //max speed, altitude, direction
  void SetMoveTargetHUD(Entity *entity) {_targetHUD = entity;};  //max speed, altitude, direction


  void ScanPlayers(StaticArrayAuto< OLink(Person) > &players); // used in multiplayers

  void GetWeather(float &overcast, float &fog, float &wind)
  {
    overcast = _actualOvercast;
    fog = _actualFog;
    wind = _actualWind;
  }
  void SetWeather(float overcast, float fog, float wind, float time);
  void SetForecast(float overcast, float fog, float wind, float time);
  void GetForecast(float &overcast, float &fog, float &wind);
  void SetNextWeatherChange(float time);
  void GetDate(int &year, int &month, int &day, int &hour, int &minute);
  void SetDate(int year, int month, int day, int hour, int minute);
  float GetLatitude() const {return _latitude;}
  float GetLongitude() const {return _longitude;}
  
#if _VBS3 //store World UTM info, this is always the info stored in the World file
  UTMInfo GetUTMInfo() const {return _utmInfo;}
  float GetWaterLevel() const {return _waterLevel;}
#endif

  float GetOvercast() const {return _actualOvercast;}
  float GetFog() const {return _actualFog;}
  float GetNextWeatherChange() const {return _nextWeatherChange - _weatherTime;}
  float GetOvercastForecast() const {return _wantedOvercast;}
  float GetNextWeatherChangeOvercast() const {return GetNextWeatherChange();/* _nextWeatherChangeOvercast;*/}
  float GetWeatherTime() const {return _weatherTime;}
  float GetFogForecast() const {return _wantedFog;}

  LLink<Script> GetScript(int scriptId) {return _scripts[scriptId]._script;}
  int GetScriptId(Script *script) {return _scripts.FindKey(script);}
#if USE_PRECOMPILATION
  LLink<ScriptVM> GetScriptVM(int scriptId) {return (scriptId >= 0 && scriptId < _scriptVMs.Size()) ? _scriptVMs[scriptId]._script : NULL;}
  int GetScriptVMIdForSerialization(ScriptVM *script)
  {
    // we need to skip the scripts with disableSerialization, as those will be missing after Load and the scriptId would not match
    int ix=0;
    for (int i=0; i<_scriptVMs.Size(); i++)
    {
      if (!_scriptVMs[i]._script || !_scriptVMs[i]._script->IsSerializationEnabled()) continue;
      if (_scriptVMs[i]._script==script) return ix;
      ix++;
    }
    return -1;
  }
#endif

  // add hash check to queue
  // bankIndex == -1 -> add permutation of all banks
  void CheckHash(int bankIndex, CheckHashReason reason);
  void CheckHash(RString filename, CheckHashReason reason);

private:
  // load/save support
  LSError Serialize(ParamArchive &ar);
  LSError SerializeVehicles(ParamArchive &ar);

  LSError Load(const char *name, int message); // top level
  LSError Save(const char *name, int message) const;

  LSError LoadBin(const char *name, int message, bool disableWarning = false); // top level
  bool SaveBin(const char *name, int message) const;

  // implementation
  void ProcessUserFileErrors();

  void CalculateCameraTransform(Matrix4 &base, Matrix4 &transform, float deltaT);
  void CalculateCameraTransform(Matrix4 &base, Matrix4 &transform, CameraType camType, float deltaT);

public:
  //! switch landscape - load it or reset it as necessary
  void SwitchLandscape(const char *name);
  //! activate add default addons and addons listed in given list
  void ActivateAddons(const FindArrayRStringCI &addons, RString worldName);
  //! check if given entry can be access with active addons
  bool CheckAddon(ParamEntryPar entry);

  /// Check if saving the game is enabled by the script
  bool IsSavingEnabled() const {return _savingEnabled;}
  /// Enable / disable saving the game
  void EnableSaving(bool enable, bool save);

  bool IsSaveGame(SaveGameType type);
  LSError LoadGame(SaveGameType type, bool disableWarning = false);
  bool SaveGame(SaveGameType type);
  bool DeleteSaveGame(SaveGameType type);

  Ref<ProgressHandle> BegLoadProgress(int message) const;
  Ref<ProgressHandle> BegSaveProgress(int message) const;
  void EndLoadSaveProgress(Ref<ProgressHandle> &p) const {ProgressFinish(p);}

  Ref<ProgressHandle> BegLoadMissionProgress(RString world, IMission &t, GameMode gameMode);

  void SaveCrash() const;

  // FIXME: no-op, remove completely?
  void ResetIDs() const; // reset object and vehicle id's

  bool CheckVehicleStructure() const;

  //! create and entity and assign it vehicle ID
  EntityAI *NewVehicleWithID(const EntityAIType *type, bool fullCreate=true);
  
  //! create and entity and assign it vehicle ID
  Entity *NewNonAIVehicleWithID(const EntityType *type, bool fullCreate=true);
  
  //! create and entity and assign it vehicle ID
  EntityAI *NewVehicleWithID(RStringVal typeName, RStringVal shapeName=RString(), bool fullCreate=true);

  //! create and entity and assign it vehicle ID
  Entity *NewNonAIVehicleWithID(RStringVal typeName, RStringVal shapeName=RString(), bool fullCreate=true);

  //! create and entity with uninitialized ID
  Entity *NewNonAIVehicle(RStringVal typeName, RStringVal shapeName=RString(), bool fullCreate=true);

  // mission editor 2 support
  void AddInitMessage(EntityAI *veh, RString init);
  void ClearInitMessage(EntityAI *veh);
  void ProcessInitMessages();

#if _ENABLE_CHEATS
  //!{ Video capturing methods
  void StartVideo(RString folderName, RString filePrefix, float frameDuration, float timeToReadResources);
  void StopVideo();
  //!}
#endif

  CameraShakeManager& GetCameraShakeManager() {return _cameraShakeManager;}
  const CameraShakeManager& GetCameraShakeManager() const {return _cameraShakeManager;}

protected:
  // implementation

  float CalculateFrameDuration(int &wantedVSyncDuration, float &maxVSyncEndTime, float vsyncTime);
  
  void SelectCameraType(float deltaT);
  void SimulateCamera(float deltaT);
  float SelectCameraPosition(Engine *engine, float deltaT, float aspectOverride = -1.0f);
  
  void SimulateUI(float deltaT);
  void SimulateDisplays(float deltaT);
  void ProcessCheats(float noAccDeltaT);


  void DrawScene(Scene::DrawContext &drawContext, const GridRectangle &groundPrimed, bool enableSceneDraw, Entity *camInsideVehicle,
    bool frameRepeated, bool wantNV, bool wantFlir, float deltaT, const UseWaitingTime &useTime);
  void DrawUI(bool wantNV, bool wantFlir);
  void DrawFrameDuration(float displayReserve);

  /// Evaluate camera position relative to vehicle.
  void EvalCameraPosition();

  /// preload data - relative time is given
  void PreloadData(float reserveLeft);
  /// preload data - max. time known
  void PreloadDataTimeLimit(float maxPreloadTime);

  /// undo PreloadVehicles - done during shutdown
  void UnpreloadVehicles();
  /// MARTA types for radio target and moveTo reporting
  void LoadMartaTypes();
public:

  /// make sure some vehicle types are loaded permanently
  void PreloadVehicles(bool progressRefresh = true);

  void AddClientCameraPosition(ClientCameraPositionObject *obj);
  void RemoveClientCameraPosition(ClientCameraPositionObject *obj);
  void ClearClientCameraPositions() { _clientCameraPositions.Clear(); }
  void UpdateCameraPosition(Vector3 &pos, Vector3 &speed, int dpnid);
  void UpdateClientCameraPlayer(int dpnid, Person *person);


  //@{ MARTA types
  struct MartaType
  {
    Ref<EntityType> _type;
    RString _className;
    RString _typeName;
    float _importance;
    float _distanceLevels[2];
    bool  _transportSoldiers;

    MartaType(){}
    MartaType(ParamEntryPar entry);

    ClassIsMovable(MartaType);
  };
  AutoArray<MartaType> _martaType;
  // returns index into _martaType array
  int GetMartaType(const EntityType *type) const;
  // interface to get selected MartaType data (use it only when single piece of data is necessary)
  float GetMartaImportance(const EntityType *type) const;
  float GetMartaEffectiveBattleDistance(const EntityType *type) const;
  //@}

  // Checking whether there are duplicate ObjectIds. Return False when detected.
  bool CheckObjectIds() const;

  //@{ Mission Event handlers
  //! all mission event handlers for given event
  AutoArray<RString> _missionEventHandlers[NMissionEvent];

  //! add event handler (return handle
  int AddMissionEventHandler(MissionEvent event, RString expression)
  {
    return _missionEventHandlers[event].Add(expression);
  }
  //! remove given event handler
  void RemoveMissionEventHandler(MissionEvent event, int handle)
  {
    _missionEventHandlers[event].Delete(handle);
  }
  //! get list of event handlers
  const AutoArray<RString> &GetMissionEventHandlers(MissionEvent event) const
  {
    return _missionEventHandlers[event];
  }
  //! remove all event handlers
  void ClearMissionEventHandlers(MissionEvent event)
  {
    _missionEventHandlers[event].Clear();
  }
  //! clear all mission EH (reset state)
  void ClearMissionEventHandlers()
  {
    for (int i=0; i<NMissionEvent; i++)
    {
      _missionEventHandlers[i].Clear();
    }
  }
  bool IsMissionEventHandler(MissionEvent mEvent) const
  {
    const AutoArray<RString> &handlers = GetMissionEventHandlers(mEvent);
    return handlers.Size()>0;
  }

  void OnMissionEvent(MissionEvent mEvent, const GameValue &pars);
  void OnMissionEvent(MissionEvent mEvent, RString par);
  void OnMissionEvent(MissionEvent mEvent, EntityAI *veh, EntityAI *entity, float par1, float par2);
  void OnMissionEvent(MissionEvent mEvent, EntityAI *entity, float par1, float par2);
  void OnMissionEvent(MissionEvent mEvent, EntityAI *entity1, EntityAI *entity2);
  //@} Mission Event handlers

  // JNI scripting support
private:
  int _nextJID;
  ExplicitMap<int, OLinkO(Entity)> _jEntities;
public:
  Entity *GetEntity(int jID);
  int AddEntity(Entity *entity);
};

extern World *GWorld;

#define GLOB_WORLD ( GWorld )


void ReportUserFileError(const RString &dir, const RString &file, const RString &name, bool once = true, bool canDelete = false);

//
template <class Type, class Traits>
LSError NetworkObject::LoadLinkRef(ParamArchive &ar, SoftLink<Type,Traits> &link)
{
  ObjectId objId;
  LSError err = objId.Serialize(ar, "oid", 1);
  if (err!=LSOK) return err;
  if (!objId.IsObject())
  {
    OLink(Object) obj = GWorld->FindObject(objId);
    link = dyn_cast<Type,Object>(obj);
    Assert(link);
    return LSOK;
  }
  link = SoftLink<Type,Traits>(SoftLinkById,objId);
  return LSOK;
}

template <class Type, class Traits>
LSError NetworkObject::SaveLinkRef(ParamArchive &ar,  SoftLink<Type,Traits> &link)
{
  ObjectId objId = link.GetId();
  if (objId.IsNull())
  {
    // non-recoverable link
    link.Lock();
    objId = ObjectId(VehicleId,link->ID());
    link.Unlock();
  }
  Assert(!objId.IsNull());
  return objId.Serialize(ar, "oid", 1);
}

// Returns the index of bank containing given filename (index into the GFileBanks)
int BankIndex(RString filename);

// actionTypes used in DisplayAddonActions
enum AddonActionType
{
  AATInstall,
  AATPlayMission,
  AATTryEntity,
  AATTryWeapon
};


struct CustomChannelInfo
{
public:
  bool     active;   // true when it was "created" by createRadioChannel script command, its index in GCustomChannels is "occupied" then
  RString  label;    // CUSTOM1, CUSTOM2, ... as default
  RString  callSign; // call sign format
  PackedColor color;
  RefArray<NetworkObject> units;

  CustomChannelInfo() : active(false) {}
  // format the callSign (can use some macros, such as %UNIT_NAME", %UNIT_SIDE%, ...)
  void GetCallSign(Person *sender, /*OUT*/char *callSign) const;
  // check whether given unit is inside the channel unit list
  bool CheckUnit(Person *unit) const;
  void Clear(int ix);
  LSError Serialize(ParamArchive &ar);

  ClassIsMovableZeroed(CustomChannelInfo);
};

class CustomChannelInfos : protected AutoArray<CustomChannelInfo> 
{
public:
  typedef AutoArray<CustomChannelInfo> base;
  CustomChannelInfos() {}

  /// Custom channel is active when it was created by createRadioChannel command and player is listed in its user list
  bool ChannelActive(int channel);
  /// true when Custom channel was created (activated) by createRadioChannel command
  bool ChannelCreated(int channel)
  {
    if (channel>=CCCustom1 && channel<=CCCustomLast)
    {
      return operator[](channel-CCCustom1).active;
    }
    return false;
  }

  CustomChannelInfo &GetCustomChannelInfo(int chan)
  {
    Assert(chan >= CCCustom1 && chan <= CCCustomLast)
      saturate(chan, CCCustom1, CCCustomLast);
    return operator[](chan-CCCustom1);
  }
  void ClearCustomChannels()
  {
    base::Realloc(CCCustomN);
    base::Resize(CCCustomN);
    for (int i=0; i<CCCustomN; i++) { operator[](i).Clear(i); }
  }
  LSError Serialize(ParamArchive &ar);
};

extern CustomChannelInfos GCustomChannels;


struct BankHashes
{
  RString prefix;
  Temp<char> ver1Hash;
  Temp<char> ver2lev0Hash;
  Temp<char> ver2lev2Hash;

  BankHashes(RString prfx) : prefix(prfx) {}
  BankHashes() {}
  ClassIsMovableZeroed(BankHashes)
};
extern AutoArray<BankHashes> GBankHashes;

struct CanonicalHash
{
  //! bank prefix
  RString prefix;
  //! hexadecimal representation of the data content hash or hash taken from bank property "hash=xxx" when present
  RString hash;

  CanonicalHash(RString prfx, RString hsh) : prefix(prfx), hash(hsh) {}
  CanonicalHash() {}

  // cmp function for QSort 
  static int CmpByPrefix(const CanonicalHash*h1, const CanonicalHash*h2)
  {
    return strcmp(h1->prefix, h2->prefix);
  }

  ClassIsMovableZeroed(CanonicalHash)
};
extern AutoArray<CanonicalHash> GCanonicalBankHashes;

//! Sort the GCanonicalBankHashes previously collected and hash all their hashes to get final canonical all content hash
void GetCanonicalBanksHash();
//! result of GetCanonicalBanksHash()
extern RString GLoadedContentHash;

#endif

