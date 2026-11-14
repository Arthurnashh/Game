#ifdef _MSC_VER
#pragma once
#endif

#ifndef _OBJECT_HPP
#define _OBJECT_HPP

/*!
\file
Interface file for Object class
*/
// (C) 1997, SUMA
#include <El/elementpch.hpp>
#include "Shape/shape.hpp"
#include <El/Time/time.hpp>
#include "Network/networkObject.hpp"
#include "visual.hpp" 
#include "objectHierarchy.hpp"
#include "global.hpp"
#include "engine.hpp"

#include <Es/Containers/maps.hpp>

#pragma warning(push)
#pragma warning(disable:4530)
#include <functional>
#pragma warning(pop)

class UIActions;
class UIAction;
class Action;
struct HitInfo;
struct CollisionInfo;

class INode;
typedef AutoArray<RefR<INode>, MemAllocLocal<RefR<INode>, 16> > UIActionParams;

class AIBrain;

// using std::pair from utility caused compile errors (confliction operator new defintions)
template <class T1, class T2>
struct Pair
{
  T1 first;
  T2 second;
  Pair(){}
  Pair(const T1 &t1, const T2 &t2):first(t1),second(t2){}
};

template <class T1, class T2>
Pair<T1,T2> MakePair(const T1 &t1, const T2 &t2)
{
  return Pair<T1,T2>(t1,t2);
}

#ifdef _MSC_VER
  #pragma warning( default: 4355 )
#endif

#ifndef SERIAL_BRANCH
const int SerializeBranch = 0xffff0000;
#define SERIAL_BRANCH(ver) ((ver) & SerializeBranch)
#define SERIAL_VERSION(ver) ((ver) & ~SerializeBranch)
#endif

// load / save unit status
#ifndef IS_UNIT_STATUS_BRANCH
const int UnitStatusBase = 0x00010000;
#define IS_UNIT_STATUS_BRANCH(ver) (SERIAL_BRANCH(ver) == UnitStatusBase)
#endif

class DamageRegions;

//! enum ObjectType - basic function of the object
#ifndef DECL_ENUM_OBJECT_TYPE
#define DECL_ENUM_OBJECT_TYPE
DECL_ENUM(ObjectType)
#endif
DEFINE_ENUM_BEG(ObjectType)
  Primary=1, //!< Normal object placed in Visitor, part of landscape
  Network=2, //!< Road placed in Visitor, part of landscape
  Temporary=4, //!< Temporary object (like tracks)
  TypeVehicle=8, //!< Some entity added by game
  TypeTempVehicle=16, //!< Temporary entity
#if _ARMA3_TERRAIN_DECALS
  LandDecal=32, // << Land decal
#endif
  Any=~0 //!< Used when seeking object of any type
DEFINE_ENUM_END(ObjectType)

//! enum DestructType - destruction type
#ifndef DECL_ENUM_DESTRUCT_TYPE
#define DECL_ENUM_DESTRUCT_TYPE
DECL_ENUM(DestructType)
#endif
DEFINE_ENUM_BEG(DestructType)
  DestructNo, //!<No Destruction
  DestructBuilding, //!<Destruct as building (shape animation)
  DestructEngine, //!<Destruct as vehicle (explosion, shape animation)
  DestructTree, //!<Destruct as tree (matrix animation)
  DestructTent, //!<Destruct as tent (shape animation)
  DestructMan, //!<Destruct as man (no shape animation)
  DestructDefault, //!<Autodetect as one of above during object loading
  DestructWreck, //!<Similar to DestructEngine, but with wreck simulation and drawing
DEFINE_ENUM_END(DestructType)

#ifndef DECL_ENUM_TARGET_SIDE
#define DECL_ENUM_TARGET_SIDE
DECL_ENUM(TargetSide)
#endif
DEFINE_ENUM_BEG(TargetSide)
  TEast,
  TWest,
  TGuerrila,
  TCivilian,
  TSideUnknown, // !!! TSideUnknown must be the last item
  TEnemy, // we are sure it is enemy (do not know which)
  TFriendly, // we are sure it is friendly (do not know which)
  TLogic,
  TEmpty, // used for serialize
  TAmbientLife, // helper side used to mark ambient life in the mission editor
  NTargetSide,
DEFINE_ENUM_END(TargetSide)

class CollisionBuffer;

class Shot;
class ObjectMerger;

DECL_ENUM(CameraType)



class SortObject;
class IPaths;
class ObjShadows;
class RememberSplit;
class FrameBase;
class IVisibilityQuery;
class EntityType;

#include <Es/Memory/normalNew.hpp>
struct ShadowIndex
{ 
  InitPtr<RememberSplit> _lods[MAX_LOD_LEVELS];
  int _nShadows;

  USE_FAST_ALLOCATOR
};
#include <Es/Memory/debugNew.hpp>

#define CREATE_OBJECT_MSG(MessageName, XX)

DECLARE_NET_INDICES_EX(CreateObject, NetworkObject, CREATE_OBJECT_MSG)

#define UPDATE_OBJECT_MSG(MessageName, XX) \

DECLARE_NET_INDICES_EX_ERR(UpdateObject, NetworkObject, UPDATE_OBJECT_MSG)

#define UPDATE_DAMMAGE_OBJECT_MSG(MessageName, XX) \
  XX(MessageName, bool, isDestroyed, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Object is already destructed"), TRANSF, ET_NOT_EQUAL, ERR_COEF_STRUCTURE) \
  XX(MessageName, float, dammage, NDTFloat, float, NCTFloatMostly0To1, DEFVALUE(float, 0.0f), DOC_MSG("Total damage of object"), TRANSF, ET_ABS_DIF, ERR_COEF_MODE)

DECLARE_NET_INDICES_EX_ERR(UpdateDamageObject, NetworkObject, UPDATE_DAMMAGE_OBJECT_MSG)

#define SUPPORT_RANDOM_SHAPES 0

#ifndef DECL_ENUM_USER_ACTION
#define DECL_ENUM_USER_ACTION
DECL_ENUM(UserAction)
#endif

struct SoundPars;

#include "objectId.hpp"

//! Id assigned to the object when it is created
struct CreateObjectId
{
  VisitorObjId _visitorId;
  ObjectId _objId;

  CreateObjectId()
  :_visitorId(-1)
  {
  }
  //! default constructor fills both Id from visitor Id
  CreateObjectId(VisitorObjId id)
  :_visitorId(id),_objId(VehicleId,id)
  {
  }
  //! constructor with both ids explicitelly provided
  CreateObjectId(VisitorObjId id, const ObjectId &oid)
  :_visitorId(id),_objId(oid)
  {
  }
};

/// context data for UnlockLinkPrepare/UnlockLinkFinish
struct UnlockLinkContext
{
  bool locked;
  int x,z;
};

class ObjectDestroyAnim;
class Skeleton;
class MagazineType;

struct WeaponModePtr;

typedef StaticArrayAuto<Matrix4> Matrix4Array;
class IExplosion;
class GameVarSpace;

//! Structure to hold a reference to the animation sequence located in one texture
/*!
  For instance, the whole texture is described by [_ntieth=1, _index=0, _frameCount=1]
*/
struct FrameSource
{
  //! How many frames is the texture in V direction divided to (only virtually - just for the position in the texture calculation)
  int _ntieth;
  //! Index of the frame in the V direction (zero based)
  int _index;
  //! Number of frames of the animation sequence
  int _frameCount;
  //! Flag to determine the animation should loop
  bool _loop;
};

//! Object class
/*!
  Object is interface and default implementation of any objects
  that are present in Landscape. 
*/

/*!
\patch 5137 Date 3/7/2007 by Bebul
- Fixed: Man state was not updated frequently enough on some conditions (for instance two players in the same vehicle)
*/
struct TurretContext;
struct TurretContextV;
struct ViewPars;
class InputInterface;

#if _VBS3
struct DisplayText  
{
  PackedColor color;
  RString     text;
  bool        render3DText;
  Vector3     comPosOffset;
  float       textSize;

  DisplayText():color(ColorP(0,0,0,0)){};
};

#endif

enum PersistenceReason
{
  PersistentSimulation=1, //! some simulation functions required in the grid square
  PersistentSound=2, //! some sound functions required in the grid square
  PersistentRoad=4, //! some road contained in the grid square
  PersistentForest=8, //! some road contained in the grid square
  PersistentHouse=16, //! some road contained in the grid square
};


#if _VBS3 
  //Highlight FLAGS
  #define HIGHLIGHT_FLAG 1
  #define HIGHLIGHT_SNAP_TO_GROUND 2

  //Objects can be made invisible
  enum
  {
    VBS_IFVis = 1,
    VBS_IFFire = 2, 
    VBS_IFView = 4,
    VBS_IFGeo = 8
  };

#endif

#if _VBS2
# define _ENABLE_ATTACHED_OBJECTS 1
#else
# define _ENABLE_ATTACHED_OBJECTS 1
#endif

typedef NetworkObject ObjectBase;

class LogicalPoly
{
  int _n;
  Vector3 _v[MaxPoly];

  public:
  LogicalPoly(){_n=0;}
  void Copy( const LogicalPoly &src );

  LogicalPoly( const LogicalPoly &src ){Copy(src);}
  void operator =( const LogicalPoly &src ){Copy(src);}

  int N() const {return _n;}
  const Vector3* Data() const { return _v; }

  Vector3Val operator [] ( int index ) const {return _v[index];}
  Vector3Val Get( int index ) const {return _v[index];}
  Vector3 &Set( int index ) {return _v[index];}

  void Add( Vector3Par v )
  {
    DoAssert(_n<MaxPoly);
    if (_n<MaxPoly)
    {
      _v[_n++]=v;
    }
    }
  void Clear() {_n=0;}

  // clipping with single plane
  bool Clip( LogicalPoly &res, Vector3Par normal, Coord d ) const;

  // clipping with all planes
  void Clip( float cNear, float cFar, ClipFlags clip );

  void Transform(const Matrix4 &trans)
  {
    // transform all vertices
    for (int i=0; i<_n; i++)
    {
      _v[i] = trans.FastTransform(_v[i]);
    }
  }

  void SumCrossProducts(Vector3 &sum) const;
};

/// position of an object for rendering
struct PositionRender
{
  /// coordinates inside of a coordinate space
  Matrix4 position;
  /// indicate which coordinate space do we use
  #if 1
  bool camSpace;

  PositionRender(Matrix4Val position, bool camSpace=false) // WIP: CAMSPACE remove default argument
  :position(position),camSpace(camSpace)
  {}

  operator const Matrix4 &() const {Assert(!camSpace);return position;} // WIP: CAMSPACE remove this conversion

  #else // dummy implementation, for easier transfer
  static const bool camSpace = false;

  PositionRender(Matrix4Val position, bool camSpace)
  :position(position)
  {
    Assert(!camSpace);
  }
  PositionRender(Matrix4Val position):position(position){}
  operator const Matrix4 &() const {return position;}
  #endif
  
  PositionRender(){}

  /// adjust a coordinate space (used during hierarchy traversal)
  PositionRender operator * (Matrix4Val trans) const {return PositionRender(position*trans,camSpace);}
  
  /// get world space position
  Vector3 PositionWorld(const Matrix4 &cam) const
  {
    return !camSpace ? position.Position() : cam.FastTransform(position.Position());
  }
  Matrix4 TransformWorld(const Matrix4 &cam) const
  {
    return !camSpace ? position : cam*position;
  }

  /// get camera space position
  Vector3 PositionCamera(const Matrix4 &toCam) const
  {
    return !camSpace ? toCam.FastTransform(position.Position()) : position.Position();
  }
};

/// return value for Object::IsAnimated
enum AnimationStyle
{
  /// no animation
  NotAnimated,
  /// only textures or materials are animated (no geometry or vertex position changes)
  AnimatedTextures,
  /// textures or materials may be animated, geometry or vertex position changes as well
  AnimatedGeometry
};

struct ObjectActionsMapAction
{
  RString _name;
  RString _displayName;
  RString _condition;
  RString _statement;
  float _priority;
  bool _showWindow;
  bool _hideOnUse;

  AutoArray<RString> _memoryPoints;

  void Load(ParamEntryPar cls);
};
TypeIsMovable(ObjectActionsMapAction)

typedef SRef< AutoArray<ObjectActionsMapAction> > ObjectActionsMapItem;

template <> 
struct ExplicitMapTraits<RString, ObjectActionsMapItem> 
{
  static ObjectActionsMapItem null;                   ///< Empty target value (can be passed as return value).
  static RString keyNull;                   ///< Empty key value (internal usage only).
  static RString zombie;                    ///< Removed value (internal usage only).

  /// Hash function KeyType -> unsigned.
  static unsigned getHash(RString &key)
  { return CalculateStringHashValue(key); }
};


class ObjectActionsMap : public ExplicitMap<RString, ObjectActionsMapItem>
{
public:
  void Load(ParamEntryPar cls);
  void Clear() {reset();}
};

extern ObjectActionsMap ObjectActions;
extern ObjectActionsMap SurfaceActions;

/// Any variable that determines how an object is drawn is stored in a VisualState.
/* Moving objects (Entity and kids) have a visualStateHistory and interpolate between states during drawing.
 * This allows us to simulate less often without losing animation smoothness.
 */
class ObjectVisualState: public FrameBase, public RefCount
{
  friend class Object;

public:
  // implicit ctor and copy ctor are enough
  ObjectVisualState();

  /// Interpolate between "this" state and "t1state". Put the result in "interpolatedResult".
  /** \param t t=0 for this, t=1 for t1state, t=[0..1] for something in between. */
  virtual void Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const;
  virtual void Copy(ObjectVisualState const& src) {*this=src;}
  virtual ObjectVisualState *Clone() const {return new ObjectVisualState(*this);}
  /// virtual destructor needed, derived types may contain various data
  virtual ~ObjectVisualState() {}

  static Matrix3 Nlerp(Matrix3 const& t0, Matrix3 const& t1, float t);
  static Matrix4 Nlerp(Matrix4 const& t0, Matrix4 const& t1, float t);


  /// helper to make casting easier
  template <class Type>
  const typename Type::VisualState &cast() const
  {
    return static_cast_checked<const typename Type::VisualState &>(*this);
  }
};


extern int JumpySim;

enum CollisionFlags
{
  //!In many cases only collided=true/false is enough
  CF_FIRST_CONTACT = 1,
  //!We want only one, the nearest contact
  CF_NEAREST_CONTACT = 2,
  //!Only static objects
  CF_ONLY_STATIC = 4,
  //!Only dynamic objects
  CF_ONLY_DYNAMIC = 8,
  //!Only water components (legacy support for "walk on geometry")
  CF_ONLY_WATER = 16,
  //!Valid when CF_FIRST_CONTACT, we get first contact for each object
  CF_ALL_OBJECTS = 32,
};


namespace enf
{
  class PhysicsController;
  class PhysicsWorld;
}

/*!
Various Entity flags
*/
enum EntityFlags
{
/*!
Entity is visible. Is rendered if there is some attached object
and event EV_VISIBLE is invoked, when event mask is set.
*/
	TFL_VISIBLE				= BITF(0),


	TFL_SOLID				= BITF(1),	///<Is collidable by various trace methods
	TFL_TRIGGER				= BITF(2),	///<Is not collidable, but invokes touch events.

/*!
Used by tracing methods. When tracing with TRACE_PASSTRANSLUCENT,
then this entity is ignored. Used for glass for example.
*/
	TFL_TRANSLUCENT		= BITF(7),

//!Used by tracing methods. Traceable only with flag TRACE_WATER
	TFL_WATER				= BITF(8),

/*!
Tells that this entity has to be actively updated by engine, her
EV_FRAME has to be called etc.
*/
	TFL_ACTIVE				= BITF(9),

/*!
Tells that this entity will represent mostly static object, so
we can use more precise but slower methods for scene-tree linking.
Also it tells to scene tree that he can count with this entity as
tree-split hint.
*/
	TFL_STATIC				= BITF(10),

/*!
Used by visibility algorithms. This entity is used as occluder
when overlapping area portals. Used by doors for example.
*/
	TFL_AREASPLITTER		= BITF(11),

/*!
Used by visibility algorithms together with TFL_AREASPLITTER.
When set, overlapped areaportal is occluded. Used by doors for
example.
*/
	TFL_AREACLOSED			= BITF(12),

//!Flags for custom usage and filterings.
	TFL_USER1				= BITF(13),
	TFL_USER2				= BITF(14),
	TFL_USER3				= BITF(15),
	TFL_USER4				= BITF(16),
	TFL_USER5				= BITF(17),
	TFL_USER6				= BITF(19),

	TFL_TRANSPARENT_OCCLUDER = BITF(20),
//! When set, Entity calls GetUserBoundBox() virtual method, instead of the standard way
	TFL_USER_BOUNDBOX		= BITF(21),

//!Private flag. Set when Entity have no ortho-normalized matrix
	TFL_UNNORMALIZED		= BITF(26),

//!Private flag. Forces to update AABB, nevertheless it fit's into old one
	TFL_UPDATE_BOUNDS		= BITF(27),

//!Private flag. Set when entity has update attached object.
	TFL_UPDATE_MDL			= BITF(28),

/*!
Private flag. Set when entity has chaged position, orientation or just
have to relink its position in scene tree.
*/
	TFL_UPDATE_LINK		= BITF(29),

//!Private flag. Set when entity has changed something and should be updated.
	TFL_UPDATE				= BITF(30),

//! Private flag. Set when entity is going to be deleted upon UpdateEntities.
	TFL_REMOVE				= BITF(31),

//!Private flag. Mask of all private flags.
	TFL_PRIVATE_FLAGS = TFL_UPDATE_MDL|TFL_UPDATE_LINK|TFL_UPDATE|TFL_REMOVE
};

/*!
Various Entity events
*/
enum EntityEvents
{
	EV_TOUCH			= BITF(0),

	EV_INIT			= BITF(3),
	EV_EXTRA			= BITF(4),
	EV_NOTVISIBLE	= BITF(5),
	EV_FRAME			= BITF(6),
	EV_VISIBLE		= BITF(7),
	EV_POSTFRAME	= BITF(8),

	EV_WORLDPROCESS= BITF(9), ///<only world entity

	EV_FLASHLIGHT	= BITF(10),

	EV_ANIMEND		= BITF(14),	///<animation reached last frame
	EV_ANIMBLEND	= BITF(16),	///<blend just ended

	EV_SOUNDEND		= BITF(19),
	EV_USER			= BITF(20),	///<21,22,23

	EV_TICK			= BITF(26),
	EV_SIMULATE		= BITF(27),
	EV_POSTSIMULATE= BITF(28),

	EV_JOINTBREAK	= BITF(29),
	EV_PHYSICSMOVE	= BITF(30),
	EV_CONTACT		= BITF(31),

	EV_ALL			= 0xffffffff
};

/// basic way how anything can exist in the game world - parent of all "physical" objects
class Object : public ObjectBase, public CountInstances<Object>
{
  typedef ObjectBase base;

#if _VBS3_LARGE_OBJECTS && !_BINARIZE_REDUCEOBJECT
private:
  Vector3P _prevBCPos; // previous bounding center position
public:
  //! add large object to R-tree (dynamic or static, both to different trees)
  void AddToRTree();
  //! remove large dynamic object from R-tree at previous position or actual
  void RemoveFromRTree(bool prevPos = false);
  //! update previous bounding center position and update dynamic object R-tree
  void UpdatePrevBCPosRTree()
  {
    Vector3P bcPos = BCenterPosition();
    if (_prevBCPos.X() != bcPos.X() && _prevBCPos.Z() != bcPos.Z())
    {
      if (_prevBCPos.SquareSize()>0.01f)
        RemoveFromRTree(true); // if there is valid previous bounding center position, remove object at this position
      else
        RemoveFromRTree(false);

      AddToRTree();
      _prevBCPos = bcPos;
    }
  }
#endif
#if _VBS3_LARGE_OBJECTS
public:
  //! calculate bounding rectangle of object on previous position or actual
  void CalcBRect(float& minX, float& minZ, float& maxX, float& maxZ, bool prevPos = false);
#endif

public:
  const Vector3 BCenterPosition() const
  {
    if (_shape /*&& _shape->IsAutoCenterDisabled()*/) //Possible TODO??: missing tech._VBS3_AUTOCENTERING_REMOVED
    {
      return FutureVisualState().PositionModelToWorld(_shape->BoundingCenter());
    }
    else
    {
      return FutureVisualState().Position();
    }
  }

public:
  // @{ Visual state
  ObjectVisualState typedef VisualState;
  friend class ObjectVisualState;
  // @}

  enf::PhysicsController*	_pPhysics;    ///<Physics controller
	
	unsigned int _EventMask;

  enf::PhysicsWorld* GetWorldPhysics() const;
  enf::PhysicsController* GetPhysics() const;

#if _ENABLE_PHYSICS
//  static unsigned int ObjectCnt();
  virtual void RemovePhysics();
  //void CreateTreePhysics(EPhysicsLayers layer, bool bStatic = true);
	virtual void CreateDynamicPhysics(EPhysicsLayers layer);
  virtual void CreatePhysics(EPhysicsLayers layer);
  virtual unsigned int IsRagDollActive() const {return false;}
	void ItemDropPhysicaly(Vector3Par dir, float force);
#endif
	Landscape* GetWorld();
  virtual void EOnContact(Object* other, const CollisionInfo* contact) {}
  virtual void SimulatePhysics(Object* other, float timeslice) {}
  
 
  unsigned int SetEventMask(unsigned int mask);

  Matrix4P IGetMatrix();//{return FutureVisualState().Transform();}  
  unsigned int GetFlags(){/*RlsAssert2(0, "not implemented"); */return 0;}
  unsigned int GetEventMask(){return _EventMask;}
  unsigned int SetFlags(unsigned int flags, bool recursively = false){/*RlsAssert2(0, "not implemented");*/ return 0;};

  void SetPhysicsController(enf::PhysicsController* pc);

protected:
  
  /// always points to _visualStateHistory.Last().
  Ref<ObjectVisualState> _futureVisualState;

  //@}

public:  
	bool IsFutureVisualStateValid(){return _futureVisualState.NotNull();}
  /// @{ Model-->world transformation for the newest visual state.
  /// Setters are overridden in Entity (inverse matrix caching).

  /// Get the basic matrix of the Object.
  FrameBase & GetFrameBase() { return FutureVisualState(); }
  FrameBase const& GetFrameBase() const { return FutureVisualState(); }
  FrameBase const& GetRenderFrameBase() { return RenderVisualState(); }

  virtual void VisualCut() {}

  /// used for GetTypeType
  const EntityType *Type() const {return nullptr;}

  virtual float UpdateDistance() const override;
  ///   @{ Functionality of FrameBase.
  
  // Setters always operate on the newest visual state.
  void SetScale(float scale) { return GetFrameBase().SetScale(scale); }
  virtual void SetPosition(Vector3Par pos) { return GetFrameBase().SetPosition(pos); }
  virtual void SetTransform( Matrix4 const& transform) { return GetFrameBase().SetTransform(transform); }
  virtual void SetTransformSkewed( Matrix4 const& transform) { return GetFrameBase().SetTransformSkewed(transform); }
  virtual void SetTransformPossiblySkewed( Matrix4 const& transform ) { return GetFrameBase().SetTransformPossiblySkewed(transform); }
#ifdef _DEBUG
  virtual void SetOrient(Matrix3 const& dir) { return GetFrameBase().SetOrient(dir, GetShape()->Name()); }
#else
  virtual void SetOrient(Matrix3 const& dir) { return GetFrameBase().SetOrient(dir); }
#endif
  virtual void SetOrientSkewed(Matrix3 const& dir) { return GetFrameBase().SetOrientSkewed(dir); }
  virtual void SetOrient(Vector3Par dir, Vector3Par up) { return GetFrameBase().SetOrient(dir, up); }
  virtual void SetOrientScaleOnly(float scale) { return GetFrameBase().SetOrientScaleOnly(scale); }
  void SetSkewed() { return GetFrameBase().SetSkewed(); }
  ///   @}

  ///   @{ Functionality of Frame.
#ifdef _DEBUG
  Matrix4 CalcInvTransform() const { return GetFrameBase().CalcInvTransform(GetShape()->Name()); }
#else
  Matrix4 CalcInvTransform() const { return GetFrameBase().CalcInvTransform(); }
#endif
  ///   @}

  ///   @{ Functionality of Matrix4.
  // Setters always operate on the newest visual state.
  virtual void SetDirectionAndUp( Vector3PPar dir, Vector3PPar up ) { return GetFrameBase().SetDirectionAndUp(dir, up); }
  virtual void SetOrientation( Matrix3P const& m ) { return GetFrameBase().SetOrientation(m); }
  Vector3P FastTransform( Vector3PPar op ) const { return GetFrameBase().FastTransform(op); }
  virtual void SetUpAndDirection( Vector3PPar up, Vector3PPar dir ) { return GetFrameBase().SetUpAndDirection(up, dir); }
  ///   @}
  /// @}

protected:
  
  Ref<LODShape> _shape; //!< object shape
  VisitorObjId _id; //!< object id (assigned by Visitor)
  ObjectId _objId; //!< object id (assigned in world binarize, encoding object placement)

#if !_BINARIZE_REDUCEOBJECT
#if !_RELEASE
  mutable char _animatedCount; //!< check if Animate/Deanimate is paired
#endif
  unsigned char _destroyPhase; // 0..255 destruction animation
#endif
  SizedEnum<ObjectType,char> _type; //!< basic object type
  SizedEnum<DestructType,char> _destrType; //!< destruction type

#if _VBS2
  Vector3 _destructAxis; //defines the rotation axis for the destruction effect (tent)
  DisplayText _displayText;
#endif

  bool _canSmoke:1; //!< object may smoke when destructed
  bool _isDestroyed:1; //!< object is already destructed
  bool _static:1; //!< object is static (never changing position)
  
#if !_BINARIZE_REDUCEOBJECT
  SRef<DamageRegions> _damage; //!< damage information

  //! optimize enable inserting/deleting
  //! from list of objects drawn during Scene rendering.
#if _HELISIM_RENDER_TO_TEXTURE
public:
  const static int NumDrawObjects = R2TRenderTargetCount + 1;
protected:
  InitPtr<SortObject> _inList[NumDrawObjects];
#else
  InitPtr<SortObject> _inList;
#endif
  //! quick link to object shadow,
  //! used to make searching in ShadowCache faster
  SRef<ShadowIndex> _shadow;
#endif

public:
  /// Checks whether PrepareShapeDrawMatrices are needed and not cached, if so, create a slot in the cache
  bool StoreDrawMatricesNeeded(bool setEngineStuff, int level, SortObject *so, SortObject *soRoot);
  /// Call PrepareShapeDrawMatrices, store results to the (already created slot of) cache
  void StoreDrawMatrices(LODShape *shape, int level, Object *parentObject, const SortObject *soRoot, int rootIndex);
  /// Create position renderer for rendering object
  virtual PositionRender GetPositionRender() { return PositionRender(GetRenderFrameBase(), false); }

  VisualState *CloneFutureVisualState() const {return FutureVisualState().Clone();}
  // @{ Visual state
  // Create the current interpolated visual state.
  // Starts to be useful in Entity.
  virtual void CreateCurrentVisualState() { /* empty */ }

  /// once we use an object with a state, any use with a different state is suspicious
  /// To check this, use the Debug target and put a breakpoint in 'ProtectVisualState::Check()' on the line '_once=false'.
  struct ProtectVisualState
  {
    enum State {None,Render,Newest};
    #if _DEBUG
    /// state of the topmost (protected) level
    State _state;
    /// how many levels are open
    int _levels;
    /// warn only once
    bool _once;
    #endif
    
    ProtectVisualState()
    #if _DEBUG
    :_state(None),_levels(0),_once(true)
    #endif
    {
    }
    
    void Check(State state)
    {
      #if _DEBUG
      if (_levels>0 && _state!=state && _once)
      {
        static bool enabled = true;
        if (enabled)
        {
        LogF("Clashing visual states!");
        }
        _once=false;  // Breakpoint here!
      }
      #endif
    }
        
    void Begin(State state, bool ignoreParent)
    {
      #if _DEBUG
      if (!ignoreParent) Check(state);
      if (_levels++==0 || ignoreParent)
      {
        _state = state;
        _once = true; // topmost scope, reinit
      }
      #endif
    }
    void End()
    {
      #if _DEBUG
      --_levels;
      #endif
    }
  } mutable _protect;

  /// while there is RenderVisualState obtained for any object, warn us when anyone tries to get Newest, and vice versa
  /// To check this, use the Debug target and put a breakpoint in 'ProtectVisualState::Check()' on the line '_once=false'.
  template <class VisualStateT>
  class ProtectedVisualState: private NoCopy
  {
    public:
    ProtectVisualState &_protect;
    VisualStateT &_vs;
    #if _DEBUG
    ProtectVisualState::State _oldState;
    mutable bool _incLevel;
    #endif
    
    ProtectedVisualState(VisualStateT &vs, ProtectVisualState &protect, ProtectVisualState::State state, bool ignoreParent=false)
    :_protect(protect),_vs(vs)
    {
      #if _DEBUG
        _oldState = protect._state;
        _incLevel = true;
        _protect.Begin(state,ignoreParent);
      #endif
    }
    ~ProtectedVisualState()
    {
      #if _DEBUG
        if (_incLevel)
        {
          _protect.End();
          _protect._state = _oldState;
        }
      #endif
    }
    VisualStateT &operator * () const {return _vs;}
    VisualStateT *operator -> () const {return &_vs;}
    operator const VisualStateT &() const {return _vs;}
    
    ProtectedVisualState(const ProtectedVisualState &src)
    :_protect(src._protect),_vs(src._vs)
    {
      #if _DEBUG
      _oldState = src._oldState;
      _incLevel = src._incLevel;
      src._incLevel = false;
      #endif
    }
    template <class OtherVisualState>    
    ProtectedVisualState(const ProtectedVisualState<OtherVisualState> &src)
    :_protect(src._protect),_vs(static_cast<VisualStateT &>(src._vs))
    {
      #if _DEBUG
      _incLevel = src._incLevel;
      _oldState = src._oldState; // does not matter, not used
      src._incLevel = false;
      #endif
    }
  };


  virtual const VisualState& RenderVisualStateRaw() const;
  VisualState& FutureVisualStateRaw() { return *_futureVisualState; }
  VisualState const& FutureVisualStateRaw() const { return *_futureVisualState; }

  const VisualState& RenderVisualState() const {_protect.Check(ProtectVisualState::Render);return RenderVisualStateRaw();}
  VisualState& FutureVisualState() { _protect.Check(ProtectVisualState::Newest);return FutureVisualStateRaw(); }
  VisualState const& FutureVisualState() const { _protect.Check(ProtectVisualState::Newest);return FutureVisualStateRaw(); }

  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(RenderVisualStateRaw(),_protect,ProtectVisualState::Render,ignoreParent);}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(FutureVisualStateRaw(),_protect,ProtectVisualState::Newest,ignoreParent); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(FutureVisualStateRaw(),_protect,ProtectVisualState::Newest,ignoreParent); }


  //@{ manage visual states based on age  
  virtual VisualStateAge GetRenderVisualStateAge() const {return 0;}
  virtual VisualStateAge GetFutureVisualStateAge() const {return 0;}
  virtual const ObjectVisualState &GetVisualStateByAge(VisualStateAge age) const {return FutureVisualState();}
  virtual VisualStateAge GetVisualStateAge(const ObjectVisualState &vs) const {return 0;}
  //@}
  
  /// @}

protected:
  /// Set skinning type and matrices (from the cache) to the engine
  void UseDrawMatrices(int cb, bool setEngineStuff, LODShape *shape, int level, SortObject *so
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawId
#endif
    ) const;

  //! Preparing of drawing of the shape itself (not including proxies)
  // void PrepareShapeDraw(int cb, LODShape *shape, int level, bool setEngineStuff, Object *parentObject);
#if _ENABLE_SKINNEDINSTANCING
  void PrepareShapeDrawInstanced(int cb, LODShape *shape, int level, bool setEngineStuff, const ObjectInstanceInfo *instances, int instanceCount);
#endif

private:
  Object(const Object &src);
  void operator =(const Object &src);
public:

  //! create an object with given shape and id
  Object(LODShape *shape, const CreateObjectId &id, bool allocateVisualState = true);

  //! create an object with given shape and id
  //! allow deferred shape loading
  //Object(RStringB shapeName, const CreateObjectId &id);
  
  //! destructor
  ~Object();

#if _VBS3_CRATERS_LATEINIT
  //! Some objects (like craters) can claim they have roadway even though their shape is not ready yet
  virtual bool HasRoadway() const {return GetShape() && GetShape()->FindRoadwayLevel()>=0;}
  //! Late object initialization
  virtual void LateInit() {}
#endif

  const VisitorObjId &ID() const {return _id;} //!< Get visitor object ID
  //! set visitor ID and object ID, assuming object is vehicle
  void SetID(const VisitorObjId &id);
  //! set visitor ID and object ID based on Create id
  void SetID(const CreateObjectId &id);

  /// Check if the simulation is switched to wreck
  virtual bool IsWreck() const {return false;}

#if _ENABLE_ATTACHED_OBJECTS
  /**
  @return true when attachment was changed (and a message needs to be sent)
  */
  virtual bool AttachTo(Object *obj, Vector3Par modelPos = VZero, int memIndex = -1, int flags = 0);
  virtual void Detach(){}
  virtual bool IsAttached() const {return false;}
  virtual void SetAttachMatrix(Matrix3Par orient) {}
  virtual Matrix3Val GetAttachMatrix() const {return M3Identity;}
  virtual Entity *GetAttachedTo() const {return NULL;}
  virtual Entity *GetDisableCollisionWith() const{return NULL;}
  virtual bool AreAttached(const Object *obj) const { return false; }
#endif

  //! change visitor ID only
  void ChangeVisitorId(const VisitorObjId &id);

  const ObjectId &GetObjectId() const {return _objId;} //!< Get object ID
  void SetObjectId(const ObjectId &id);

  //! Return alive factor of the object (retyped f.i. for transports)
  virtual float GetAliveFactor() const {return 0.0f;}
  //! Return movement factor of the object (retyped f.i. for transports)
  virtual float GetMovementFactor() const {return 0.0f;}
  //! Return metabolism factor of the object (retyped f.i. for characters)
  virtual float GetMetabolismFactor() const {return 0.0f;}
  //! Function to retrieve the own heat source direction and 
  virtual bool GetOwnHeatSource(Vector3 &ownHeatSource) const {return false;}
  //!{ Access to TI properties
  virtual float HTMin() const {return _shape ? _shape->HTMin() : 0;}
  virtual float HTMax() const {return _shape ? _shape->HTMax() : 0;}
  virtual float AFMax() const {return _shape ? _shape->AFMax() : 0;}
  virtual float MFMax() const {return _shape ? _shape->MFMax() : 0;}
  virtual float MFact() const {return _shape ? _shape->MFact() : 0;}
  virtual float TBody() const {return _shape ? _shape->TBody() : 0;}
  virtual DefTIMode GetDefTIMode() const {return TIDefault;}
  
  virtual bool LoadedFromEntityCfg() const {return false;}
  //!}

  //! Method to specify the object's crater color
  virtual Color CraterColor() const {return Color(0.006, 0.005, 0.0025, 1);}
  //! Method to add crater to the object
  void AddCrater(Vector3Par pos, float radius);

  /// set building index - used for house numbers
  virtual void SetBuildingIndex(int index) {}

  //@{
  //! Access to shadow index
#if _BINARIZE_REDUCEOBJECT
  void SetShadowIndex( ShadowIndex *shadow ){Fail("Waring: Object functionality reduced");}
  ShadowIndex *GetShadowIndex() const {Fail("Waring: Object functionality reduced"); return NULL;}
#else
  void SetShadowIndex(ShadowIndex *shadow) {_shadow=shadow;}
  ShadowIndex *GetShadowIndex() const {return _shadow;}
#endif

  //! Select shadow volume level
  virtual int GetPass3ShadowVolumeLevel(int pass, CameraType camType) const {return -1;}

  void RemoveAllShadows();
  void RemoveShadow(int level);
  void SetShadow(int level, RememberSplit *shadow);
  RememberSplit *GetShadow(int level) const;
  /// try to find a shadow which is ready, nearest to given level
  RememberSplit *GetShadowReady(int level) const;
  //@}

  //! Check if object is static.
  bool Static() const {return _static;}

#if _VBS2 //ability to change user created objects
  void SetStatic(bool val = true) {_static = val;}
  // used when object is selected in RTE
  PackedColor _highlightedColor;
  //flags to indicate how the object is highlighted, see HIGHLIGHT_FLAG defines above
  int _highlighted; 
  //ability to make various lods invisible
  int _inVisibilityFlags;
#endif

#if _VBS3
  DisplayText & GetDisplayText() { return _displayText; }
#endif

  /// check object rigidity
  virtual float Rigid() const {return 0.9f;}
  /// by default let deflection be controlled only by ammo
  virtual float MaxDeflectionAngleCos() const {return 1.0f;}
  
  //! if object is drawn as cloudlet, it may require more distance clipping
  virtual float CloudletClippingCoef() const;

#if _BINARIZE_REDUCEOBJECT
#if _HELISIM_RENDER_TO_TEXTURE
    SortObject *GetInList(int idx) const {Fail("Waring: Object functionality reduced"); return NULL;}
    void SetInList(SortObject *inList, int idx) {Fail("Waring: Object functionality reduced");}
  #else
    SortObject *GetInList() const {Fail("Waring: Object functionality reduced"); return NULL;}
    void SetInList( SortObject *inList ) {Fail("Waring: Object functionality reduced");}
  #endif
#else
  #if _HELISIM_RENDER_TO_TEXTURE
    //@{
    //! Access to _inList member
    SortObject *GetInList(int idx) const {return _inList[idx];}
    void SetInList(SortObject *inList, int idx) { _inList[idx] = inList;}
    //@}
  #else
    //@{
    //! Access to _inList member
    SortObject *GetInList() const {return _inList;}
    void SetInList( SortObject *inList ) {_inList=inList;}
    //@}
  #endif
#endif

#if _HELISIM_RENDER_TO_TEXTURE
    virtual bool HasR2TTarget() const { return false; }
    virtual Matrix4Val GetCurrentR2TTargetMatrix(int index) { return M4Identity; }
#endif

  /// access to internal variable space (where available)
  virtual GameVarSpace *GetVars() {return NULL;}
  virtual const GameVarSpace * GetVars() const {return NULL;}

  virtual void SetVariable(RString name, const GameValue &value) {}

  virtual bool PrepareShapeDrawMatrices(Matrix4Array &matrices, const ObjectVisualState &vs, LODShape *shape, int level);

  //! Perform any animation of object shape
  /*!
  * May transform any vertices of the object or change face attributes (texture, flags).
  * \note
  * When changing face attributes, sections attributes need also be changed,
  * otherwise HW T&L implementation will fail.
  * See AnimationSection for more information.
  * \note When Animate is implemented,
  * Deanimate should be also implemented to restore shape state.
  */
  virtual void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  //! Restore shape after Object::Animate.
  virtual void Deanimate( int level, bool setEngineStuff );

  /// prepare any state needed for inside rendering
  /** called for the GWorld->CameraOn() before rendering*/
  virtual void PrepareInsideRendering() {}
  
  //! Function that recognizes object as a sprite
  virtual bool IsSprite() {return false;}

#if _VBS3_CRATERS_DEFORM_TERRAIN
  //! Function to determine the object is aligned with ground (like crater)
  virtual bool IsGroundAligned() const {return false;}
  //! Method to return scale used for ground aligned object
  virtual float DesiredScale() const {return 1.0f;}
  //! Function to align the object with ground
  virtual void AlignWithGround() {}
#endif

#if _VBS3_CRATERS_BIAS
  //! Return model controlled bias value
  virtual int Bias() const {return 0;}
#endif

  //! Function to retrieve frame source (position of the frame in the baste texture)
  /*!
    Function returns true if the structure was filled out.
  */
  virtual bool GetFrameSource(FrameSource &frameSource) const {return false;}
  //! Function returns the current phase of the animation (current frame index)
  virtual float GetAnimationPhase() const {return 0.0f;}
  //! Function returns rotation angle of the object (useful for 2D sprites)
  virtual float GetAngle() const {return 0.0f;}
  
  //! Get bounding sphere estimate of object after animate.
  /** @param bCenter world space result */
  virtual void AnimatedBSphere(
    int level, Vector3 &bCenter, float &bRadius, Matrix4Par pos, float posMaxScale
  ) const;

  enum ClippingType {ClipVisual, ClipShadow, ClipGeometry, ClipGeneral};
  /// Get clipping info (sphere and minmax) - needed to determine if object can be clipped
  /** @param minMax model space bounding box */
  virtual float ClippingInfo(const ObjectVisualState &vs, Vector3 *minMax, ClippingType clip) const;
  /// bounding information for UI purposes
  virtual float BoundingInfoUI(Vector3 &bCenter, Vector3 *minMax) const;
  /// check if object needs to be animate because of destruction status
  bool IsDestroyAnimated() const;
  /// check destruction skeletal animation
  virtual ObjectDestroyAnim *GetDestroyAnimation() const;
  //!Check if object is animated
  virtual AnimationStyle IsAnimated(int level) const; // appearance changed with Animate
  //!Check if object animatiion is identity (and can be ignored)
  virtual bool IsAnimationIdentity(int level) const;

  /// check if this object should cast shadow on other objects
  bool CheckShadowCaster() const;
  //! Check if craters of this object should be drawn. Returns number of maximum possible craters to draw regarding the shading quality
  int CheckCraterDrawing(float distance2, const DrawParameters &dp) const;
  //!Check if object can be instanced
  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const; // appearence changed with Animate
  //!Check if object shadow can be instanced
  virtual bool CanBeInstancedShadow(int level) const;
  /// check if asynchronous CB recording is possible
  virtual bool CanObjDrawAsTask(int level) const;
  /// check if asynchronous CB recording of shadow is possible
  virtual bool CanObjDrawShadowAsTask(int level) const;

  /// persistence check as done in Binarize
  int BinarizeCheckPersistance() const;
  //!Change object position
  //!\note: This function should be used when object is already present in landscape.
  virtual void Move(Matrix4Par transform);
  virtual void Move(Vector3Par position);

  //!Change object position (network aware)
  /*!
    For local object this calls Move(transform) directly.
    For remote object request to move object is send over network.
  */
  void MoveNetAware(Matrix4Par transform);
  void MoveNetAware(Vector3Par pos);
  
  /// sometimes we need to reset the remote state because there is an abrupt position change
  virtual void ResetRemoteState() {}

  //! Get vehicle plate number
  /** Intended as interface for GameStateExt */
  virtual const char *GetPlateNumber() const {return "";}
  //! Set vehicle plate number
  /** Intended as interface for GameStateExt */
  virtual void SetPlateNumber(RString plate);

  //! Total amount of contained fuel (for explosion calculation)
  virtual float GetFuelTotal() const {return 0;}

  //! predict object velocity as perceived by someone watching the unit
  virtual Vector3 PredictVelocity() const;

  //! Get object speed
  virtual Vector3Val ObjectSpeed() const {return VZero;}
  //! Check if object is airborne
  virtual bool Airborne() const {return false;}
  //! Check if object may be locked by given weapon
  virtual bool LockPossible( const AmmoType *ammo ) const {return false;}

  //! Check if object is a "feature"
  /** features require a different handling, e.g. not disappearing with a distance */
  virtual float GetFeatureSize() const;

  enum Visible
  {
    ObjInvisible,
    ObjVisibleFast,
    ObjVisibleSlow
  };
  /** combines Invisible and slow rendering detection to reduce impact of the virtual call in the World::PrepareDraw loop */
  virtual Visible VisibleStyle() const;

  //! Check if object is invisible
  bool Invisible() const {return VisibleStyle()==ObjInvisible;}
  //! Check if object should be tested for bullet intersection
  virtual bool OcclusionFire() const;
  //! Check if object should be tested for line of sight intersection
  virtual bool OcclusionView() const;
  /// objects can avoid changing height during subdivision
  virtual bool FixedHeight() const {return false;}

  virtual bool CanLimitClearance() const {return true;}
  //! Get visual density of the object
  /*!
    This is used to implement volume attenuation of visibility.
  */
  virtual float ViewDensity() const;

  //! Object created - may adjust any internal data to reflect world position.
  virtual void Init( Matrix4Par pos, bool init );

  //! If necessary, animate given component level (see Object::Animate)
  void AnimateComponentLevel(AnimationContext &animContext, int level);
  //! If necessary, deanimate given component level (see Object::Deanimate)
  void DeanimateComponentLevel(int level);

  //@{
  //! Animate/Deanimate given level
  void AnimateGeometry(AnimationContext &animContext);
  void DeanimateGeometry();
  void AnimateViewGeometry(AnimationContext &animContext);
  void DeanimateViewGeometry();
  void AnimateFireGeometry(AnimationContext &animContext);
  void DeanimateFireGeometry();

  void AnimateLandContact(AnimationContext &animContext);
  void DeanimateLandContact();
  //@}

  //! Get user-friendly name of the object
  virtual RString GetDisplayName() const;
  //! Get short variant of user-friendly name of the object
  virtual RString GetShortName() const;
  //! Get voice element name representing the name of the object
  virtual RString GetNameSound() const;
  //! Check if object should be used as reference for Move command
  virtual bool IsMoveTarget() const;
  //! Check which rendering pass should be this object drawn in
  virtual int PassNum( int lod );
  /// ordering inside of given pass
  virtual int PassOrder( int lod ) const;

  /// helper for PassNum - check based on alpha level only
  virtual int GetAlphaLevel(int lod) const;
  
  //! Get special properties of the object shape
  virtual int GetSpecial() const {return _shape ? _shape->Special() : 0;}
  //! Get special properties of the object
  virtual int GetObjSpecial(int shapeSpecial) const {return shapeSpecial;}

  //@{
  //! Virtual access to list of all proxy objects
  virtual int GetProxyCount(int level) const;
  virtual Object *GetProxy(
    VisualStateAge age,
    LODShape *&shape,
    Object *&parentObject,
    int level, 
    Matrix4 &transform,  //!< (out) result transform of proxy in world coord
    const Matrix4 &parentPos, //!< (in) transformation in world coord of proxy parent
    int i
  ) const;
  virtual Matrix4 GetProxyTransform(int level, int i, Matrix4Par proxyPos, Matrix4Val parent) const;
  //@}


  /// some objects may need to pretend their area is lower/higher. Units are in square meters
  virtual float EstimateArea() const;

  /// some objects (e.g. lights) may want to guarantee long-distance visibility
  virtual float MinPixelArea(float minPixelAuto) const {return minPixelAuto;}

  /// change polygon density based on visual properties of the object
  virtual float DensityRatio() const {return 1.0f;}
  /// adjust covered area estimation - used for flat objects
  virtual float DrawingAreaRatio() const {return 1.0f;}
  /// check if PrepareProxiesForDrawing needs to be called
  virtual bool NeedsPrepareProxiesForDrawing() const;
  /// draw all proxy objects using linear scene manager
  virtual void PrepareProxiesForDrawing(
    Object *rootObj, int rootLevel, const AnimationContext &animContext,
    const PositionRender &transform, int level, ClipFlags clipFlags, SortObject *so
  );
  /// check if DrawProxies can be non-empty
  virtual bool DrawProxiesNeeded(int level) const;
  /// make sure all proxy shapes and textures are loaded and ready for rendering
  virtual bool PreloadProxies(int level, float dist2, bool preloadTextures = false) const;
  /// make sure everything visible in the inside view is ready for rendering
  virtual bool PreloadInsideView() const;

  void PrepareTextures(AnimationContext *animContext, LODShape *shape, float dist2, int level, bool prepareBaseTextureOnly);

  //! Determine whether volume shadows should be casted in pass3
  virtual bool CastPass3VolShadow(int level, bool zSpace, float &castShadowDiff, float &castShadowAmb) const;

  /// instancing support
  virtual void DrawSimpleInstanced(
    int cb, int forceLOD, ClipFlags clipFlags, const DrawParameters &dp,
    ObjectInstanceInfo *instances, int instanceCount, float dist2, const LightList &lights,
    SortObject *oi, float coveredArea
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );
  //! Draw object
  virtual void Draw( int cb, int forceLOD, ClipFlags clipFlags, const DrawParameters &dp, const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );
  
  //! Calculate (or estimate) complexity of given proxy
  virtual int GetProxyComplexity(int level, const Matrix4 &pos, float dist2) const;
  //! Calculate (or estimate) complexity of given object level
  virtual int GetComplexity(int level, const Matrix4 &pos) const;
  
  /// we want to ignore some objects in scene management
  virtual bool IgnoreComplexity() const;

  #if _ENABLE_CHEATS
    virtual bool CheckDrawToggle() const;
    virtual float CheckDrawScale() const;
  #endif
  //! Draw diagnostic information
  virtual void DrawDiags();
  //! Draw diagnostic information for the object with camera focus
  virtual void DrawFocusedDiags();

  //! Draw object as billboard (particle)
  void DrawDecal( int cb, int level, ClipFlags clipFlags, const Matrix4 &pos );
  //! Draw object as 2D layer
  //!\note Only simple rectangles are allowed in 2D layers
  void Draw2D(AnimationContext *animContext, int level);
  //! Draw given shape as 2D layer - usually used to draw optics
#if _VBS3 //enable to uses different zoom
  void Draw2D(AnimationContext *animContext, LODShape *lShape, int lod, ColorVal cColor, float zoom = 1.0f);
#else
  void Draw2D(AnimationContext *animContext, LODShape *lShape, int lod, ColorVal cColor);
#endif
  //! Draw given shape as 3D lines
  void DrawLines(int cb, int level, ClipFlags clipFlags, const Matrix4 &pos);
  void DrawShadowVolumeInstanced(
    int cb, LODShape *shape, int level, const ObjectInstanceInfo *instances, int instanceCount, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  );
  virtual void DrawShadowVolume(int cb, LODShape *shape, int level, const PositionRender &frame, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );

  /// exact shadow casting
  void DrawExShadow(
    int cb, LODShape *shape, const PositionRender &pos, int level, bool isVolume, bool usePilotSV, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  ); 

  //! Recalculate (project) shadow or split against landscape rectagle
  Ref<Shape> RecalcShadow(int level, const FrameBase &frame);
  
  //! Get armor value
  virtual float GetArmor() const {return _shape->Armor();} // armor in mm
  //! Get inverse of armor value
  virtual float GetInvArmor() const {return _shape->InvArmor();} // armor in mm
  //! Get logarithm of armor value
  virtual float GetLogArmor() const {return _shape->LogArmor();} // armor in mm

  virtual float GetIndirectHitFactor() const {return 1.0f;}
  

  //! Get mass
  virtual float GetMass() const {return _shape ? _shape->Mass() : 0.0f;}
  //! Get inverse mass
  virtual float GetInvMass() const {return _shape ? _shape->InvMass() : 1.0f;}

  //!\deprecated For compatibility with older code only. See Object::GetMass
  float Mass() const {return GetMass();}
  //!\deprecated For compatibility with older code only. See Object::GetInvMass
  float InvMass() const {return GetInvMass();}

  //! Get  inertia matrix (tensor)
  Matrix3 Inertia() const {return _shape->Inertia();}
  //! Get inverse of inertia matrix (tensor)
  Matrix3 InvInertia() const {return _shape->InvInertia();}

  //! Get center of mass position (model coordinates)
  Vector3 GetCenterOfMass() const {return _shape ? _shape->CenterOfMass() : VZero;}

  //! Get center of mass position (world coordinates)
  virtual Vector3 COMPosition(ObjectVisualState const& vs) const {return vs.PositionModelToWorld(_shape->CenterOfMass());}

  //@{ Get/Set object type (see ObjectType)
  ObjectType GetType() const {return _type.GetEnumValue();}
  void SetType( ObjectType type ) {_type=type;}
  //@}

  //! Get IPaths interface. May be NULL when object does not implement it.
  virtual const IPaths *GetIPaths() const {return NULL;}

  //! Get entity type. May be NULL when object is not entity
  virtual const EntityType *GetEntityType() const {return NULL;}
  //! Get military target side
  virtual TargetSide GetVehicleTargetSide() const {return TSideUnknown;}

  virtual bool AnimationMovesPosition() const {return false;}
  
  //! Get debugging name (debugging only)
  virtual RString GetDebugName() const;
  //! Get variable name (for GameStateExt purposes)
  virtual RString GetVarName() const {return RString();}

#if _VBS3 // SetVarName now works for triggers
  //! Set variable name (for GameStateExt purposes)
  virtual void SetVarName(RString name) {}
#endif

  //@{ Get/Set destruction type (see DestructType)
  DestructType GetDestructType() const {return _destrType.GetEnumValue();}
  void SetDestructType( DestructType type ) {_destrType=type;}
  //@}

  // camera effect parameters
  virtual void DrawCameraCockpit() {}
  virtual void DrawPeripheralVision(Person *person, CameraType camType){}
  virtual bool CameraAutoTerminate() {return false;}

  virtual void SetConstantColor( ColorVal color );
  virtual ColorVal GetConstantColor() const;
  
  //!{ Width is used by the line object
  virtual void SetConstantWidth(float width) {}
  virtual float GetConstantWidth() const {return 0.0f;}
  //!}

  //@{
  //! Get/Set object shape
  __forceinline LODShape *GetShape() const {return _shape;}
  void SetShape( LODShape *shape ) {_shape=shape;}
  //@}

  
  /// when accessing multiple objects, we want to avoid memory latency
  /** one prefetch should handle both _position and _shape */
  void PrefetchShapeAndPos()
  {
    #if USING_SSE
      _mm_prefetch((const char *)&FutureVisualState().Position(),_MM_HINT_T0);
    #endif
  }

  //! Get object size for purpose of how large object is when viewed
  virtual float VisibleSize(ObjectVisualState const& vs) const;
  //! Get object size for purpose of how large part of object must be visible
  virtual float VisibleSizeRequired(ObjectVisualState const& vs) const;
  //! Get object position for purpose of visibility/spotability calculations
  virtual Vector3 VisiblePosition(ObjectVisualState const& vs) const;
  //! Get point in the object where weapons should be aiming to.
  virtual Vector3 AimingPosition(ObjectVisualState const& vs, const AmmoType *ammo, Vector3Par aimFromDir) const;
  //! Get point in the object where weapons should be aiming to - preferred or alternative point
  virtual Vector3 AimingPositionHeadShot(ObjectVisualState const& vs, const AmmoType *ammo, Vector3Par aimFromDir) const;
  //@{ optional arguments overloads - when ammo is given, direction needs to be given as well
  Vector3 AimingPosition(ObjectVisualState const& vs) const {return AimingPosition(vs,NULL,VZero);}
  Vector3 AimingPositionHeadShot(ObjectVisualState const& vs) const {return AimingPositionHeadShot(vs,NULL,VZero);}
  //! Get point in the object where camera should be aiming to
  virtual Vector3 CameraPosition() const;
  //! Get point in the object which the object uses to look around from
  virtual Vector3 EyePosition(ObjectVisualState const& vs) const;

  //! Check if obstacle can be ignored for purposes of line-of-sight test
  //! when targeting to this object
  virtual bool IgnoreObstacle(Object *obstacle, ObjIntersect type=ObjIntersectFire) const;
  
  //! Get size used for predicting collisions.
  virtual float CollisionSize() const;

  /// estimate collision size based on shape
  static float CollisionSizeEstimate(const LODShape *shape);
  //! Attach wave to object
  /*! Used when speaking to perform lip-sync.*/
  virtual void AttachWave(AbstractWave *wave, float freq);
  /// check if all files for AttachWave are ready, if not, request them
  virtual bool AttachWaveReady(const char *wave);
  //! check how aloud is given unit speaking now - this is used to get attention
  virtual float GetSpeaking() const;
  /// check camera interest
  virtual float GetCameraInterest() const;

  //@{ Actions
  //! Get parameters of action as structured texts
  virtual bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const;
  //! Perform action
  virtual void PerformAction(const Action *action, AIBrain *unit);
  //! Get list of available actions
  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit);
  //@}

  /*!
  \name Flag support
  Interface to various functions of FlagCarrier
  */
  //@{
  virtual void LowerFlag(float low) {}
  virtual float GetFlagLowered() const {return 0;}
  //@}

#if _VBS2
#if _VBS3 && _VBS_TRACKING
protected:
    Vector3 _trackerPos;
    Vector3 _trackerOrien;
    bool _isTracking;
public:
    void DisableTracking()
    {
      _isTracking = false;
      _trackerPos   = VZero;
      _trackerOrien = VZero;
    }
    void TrackerUpdate(Vector3 &pos,Vector3 &orien)
    {
      _isTracking = true;
      _trackerPos   = pos;
      _trackerOrien = orien;
    };
    bool IsTrackerConencted() { return _isTracking;}
#endif

#if _VBS_TRACKING
   const Vector3 &GetTrackingPos(){ return _trackerPos; }
#endif
#endif //_VBS2


  //! Get shape bounding sphere average radius. Not all points must be inside such sphere. There could be points outside just for nonorthonormal matrices.
  float GetRadius() const {return _shape ? _shape->BoundingSphere() * FutureVisualState().Scale() : 0.0f;}

  //! Get shape bounding sphere maximal radius. Use this function if you wanna be sure that all points are inside the sphere.
  //float GetMaxRadius() const {return (_shape ? _shape->BoundingSphere()*FutureVisualState().MaxScale() : 0);} // It is not more used replaced by GetCollisionRadius

  /// get radius for collision testing purposes
  //virtual float GetCollisionRadius() const {Vector3 minmax[2];return ClippingInfo(minmax);}
	//we made specialized version without minmax overhead
	virtual float GetCollisionRadius(const ObjectVisualState &vs) const;
  
  //! Get total damage (non-clipped, i.e. returned value may be greater than 1)
  float GetRawTotalDamage() const; 
  //! Get total damage (clipped to range 0..1)
  float GetTotalDamage() const {return floatMin(GetRawTotalDamage(), 1.0f);}
  /// get damage for purposes of fleeing / return to base
  virtual float GetFleeDamage() const {return GetTotalDamage();}
  //! Set total damage, handle dying meanwhile
  virtual void SetTotalDamageHandleDead(float value, bool setHitpoints);
  //! Set total damage
  void SetTotalDamage(float value, bool showDamage); // change value and additional info (smoke)

  //! Check how much explosives is in. Used when object should explode.
  virtual float GetExplosives() const {return 0.0f;}

  //! Repair (or damage) object
  virtual void Repair(float ammount=1.0f);

  //! Set object damage (high level)
  void SetDamage(float damage, bool setHitpoints);
  //! Set object damage (high level, network aware)
  void SetDamageNetAware(float damage);
  //! Set object damage (high level)
  virtual void SetDamageAndHitpoints(float damage);

  virtual void SetMaxHitZoneDamage(float damage) {};

  //@{ Built-in shape destruction function
  /// check if destruction by changing the transformation matrix is currently running
  bool IsDestroyingByPos() const;
  void SetDestroyPhase(int phase);
  void SetDestroyed( float anim );

#if _BINARIZE_REDUCEOBJECT
  float GetDestroyed() const {Fail("Waring: Object functionality reduced"); return 0.0f;}
#else
  float GetDestroyed() const {return _destroyPhase * (1.0f / 255.0f);}
#endif  

  virtual const EntityType *CanDestroyUsingEffects() const;
  bool DestroyUsingEffects(EntityAI *owner, bool immediate=false, bool doDamage=true);
  //@}

  //@{
  //! Attached smoke function.
  IExplosion *GetSmoke() const;
  void SetSmoke(Entity *smoke);
  void DeleteSmoke();

  bool CanSmoke() const;

  /// used after smoke is serialized / network transferred
  virtual void ExplodeSmoke() {}
  //@}

  //! Check if object geometry should be used for collision testing (f.i. tents after destruction returns false)
  virtual bool HasGeometry() const;
  //! check if object can be hit by a shot
  virtual bool CanBeHit() const {return true;}
  //! Check if object is destroyed.
  bool IsDestroyed() const {return _isDestroyed;}
  //! Check if object is passable by men.
  virtual bool IsPassable() const;
  
  //! Check if object is present in top-most level of scene hierarchy (Landscape)
  virtual bool IsInLandscape() const {return true;}

  //! Disable destroying objects.
  //!\note This is always used when object is already destroyed.
  void NeverDestroy() {_isDestroyed = true;}

  //! React to being hit by some EntityEI
  virtual void HitBy(EntityAI *owner, float howMuch, RString ammo, bool wasDestroyed) {}
  //! get sound that should be used when destroyed
  virtual void GetDestroySound(SoundPars &pars) const;
  //! React to being destroyed by some EntityEI 
  virtual void Destroy(EntityAI *owner, float overkill, float minExp, float maxExp);
  //! Check if object is destroyed (checks also fatal local damage)
  virtual bool IsDamageDestroyed() const {return GetRawTotalDamage() >= 1.0f;}

  /// DoDamage result accumulator
  struct DoDamageResult
  {
    /// total (aka. structural) damage of the whole object
    float damage;

    // per hit-point damage
    AutoArray< float,MemAllocLocal<float,32,float> > hits;
    
    DoDamageResult() {damage = 0.0f;}
    
    void operator += (const DoDamageResult &add);
    
    
    /// make sure hit can contain element index
    void Access(int index) {GrowHit(index+1);}
    private:
    /// make sure hit can contain size elements
    void GrowHit(int size);
  };

  /// accumulator to avoid sending multiple damages to one vehicle
  class DoDamageAccumulator
  {
    DoDamageResult _result;
    Object *_obj;
    EntityAI *_owner;
    RString _ammo;
    SkinnedPosition _pos;

    // flush what has been accumulated so far
    void Flush()
    {
      if (_obj)
      {
        _obj->ApplyDoDamage(_result, _owner, _ammo, _pos);
        // reset could be done here, but values are not used anyway
      }
    }
      
    public:
    DoDamageAccumulator():_obj(NULL),_owner(NULL),_pos(nullptr){}
    ~DoDamageAccumulator(){Flush();}

    /// add another force/torque pair  
    void Add(Object *obj, EntityAI *owner, RString ammo, SkinnedPosition pos, const DoDamageResult &result)
    {
      // strictly speaking we should check ammo here as well
      // but it is very unlikely one owner will cause two damages
      // with a different ammo at the same time
      if (obj!=_obj || owner!=_owner || pos!=_pos)
      {
        // when object is changed, flush
        Flush();
        // start a new accumulation
        _obj = obj;
        _owner = owner;
        _ammo = ammo;
        _result = result;
        _pos = pos;
      }
      else
      {
        // accumulate
        _result += result;
      }
    }
  };
  
  //! Handle direct local hit (used for glass damage)
  virtual float DirectLocalHit(DoDamageResult &result, int component, float val) {return 1.0f;}
  //! Handle local hit (called so that object may update its local damage
  /*!\return Returned value is used as multiplier to total (structural) damage*/
  virtual float LocalHit(DoDamageResult &result, Vector3Par pos, float val, float valRange) {return 1.0f;}

  struct ShotInfo
  {
    const AmmoType *ammo;
    EntityAI *owner;
    Vector3 modelPos;
    bool local;
    bool ignoreSmall;
    Vector3 originDir;

    ShotInfo(const AmmoType *ammo, EntityAI *owner, Vector3Par modelPos, bool local, bool ignoreSmall, Vector3Par originDir)
      :ammo(ammo),owner(owner),local(local),ignoreSmall(ignoreSmall),originDir(originDir),modelPos(modelPos)
    {
    }

    ShotInfo(const Shot *shot, EntityAI *owner, Object *hit, Vector3Par modelPos);
  };

  //! Perform damage on object (when direct hit is detected)
  /*!Convert hit data to LocalDamage*/
  void DirectDamage(DoDamageResult &result, const ShotInfo &shot, float val, float energyFactor);
  //! Perform damage on object (when indirect hit is detected)
  /*!Convert hit data to LocalDamage*/
  void IndirectDamage(DoDamageResult &result, const ShotInfo &shot, float val, float valRange);
  //! Perform damage at given position
  void LocalDamage(DoDamageResult &result, const ShotInfo &shot, float val, float valRange);

  void LocalDamageAccum(DoDamageAccumulator &acc, const ShotInfo &shot, float val, float valRange, RString ammo, const SkinnedPosition &pos)
  {
    DoDamageResult result;
    LocalDamage(result,shot,val,valRange);
    // TODO: could ammo be replaced with shot.ammo->GetName()?
    acc.Add(this,shot.owner,ammo,pos,result);
  }

  /** Perform damage on object according to impulse - does nothing for remote objects
    * @param owner - Owner of the damage
    * @param impulse - Impulse
    * @param modelPos - Point where impulse is applied in model coord.
    Ignored for remote objects - they receive impulse instead
    */
  virtual void DamageOnImpulse(EntityAI *owner, float impulse, Vector3Par modelPos);

  /// Simulate damage to the object.
  virtual void DoDamage(DoDamageResult &result, Vector3Par pos, float val, float valRange, RString ammo, Vector3Par originDir, bool ignoreSmall);

  //! Allow simulation of the object
  virtual void OnMoved() {}

  virtual bool IsMoving() {return true;}

  virtual void SimulateImpulseDamage(DoDamageResult &result, EntityAI *owner, float impulse, Vector3Par modelPos);
  virtual void SimulateForceDamage(DoDamageResult &result, EntityAI *owner, float impulse, Vector3Par modelPos);
  /// apply results of DoDamage
  virtual void ApplyDoDamage(const DoDamageResult &result, EntityAI *owner, RString ammo, const SkinnedPosition &pos);
  //! apply direct hit on animated objects (support for ragdolls). TODO: combine it with many similar methods!!
  virtual void DirectHit(const HitInfo& hitInfo, const SkinnedPosition& skinning) {}

  /// apply results of DoDamage - local object only
  virtual void ApplyDoDamageLocal(const DoDamageResult &result, EntityAI *owner, RString ammo, const SkinnedPosition &pos);
  
  virtual void SimulationNeeded(float age){}

  void ApplyDoDamageLocalHandleHitBy(const DoDamageResult &result, EntityAI *owner, RString ammo, const SkinnedPosition &pos);
  void ApplyDoDamageLocalHandleHitByDo(const DoDamageResult &result, EntityAI *owner, RString ammo, const SkinnedPosition &pos);
  
  //! Debugging function: Verify internal structure of the object.
  virtual bool VerifyStructure() const;

  //! Check if point is inside the object
  bool IsInside( Vector3Par pos, ObjIntersect type=ObjIntersectGeom) const;

  /// check if object is overlapping an x-z world space rectangle
  bool DetectRoadwayOverlap(float xMin, float xMax, float zMin, float zMax) const;

  //! Check if given two objects placed on arbitrary positions intersect.
  void Intersect(CollisionBuffer &result, const Object *with, const ObjectVisualState &thisPos, const ObjectVisualState &withPos, int hierLevel=0, bool bPerson=false) const;

  //! Check if given two objects intersect. Use object current position.
  // TODO: collisions should occur in the same time frame
  void Intersect(CollisionBuffer &result, const Object *with) const {Intersect(result, with, FutureVisualState(), with->FutureVisualState());}

  // Be ware both  cNormal and cDeltaPos are in with model coord
  int FirstTouch(float &fDoneFrac, Vector3& cNormal, const ObjectVisualState &thisPos, const Object *with, const ObjectVisualState &withPos, Vector3Val cDeltaPos) const;
  
  //! Check if object at arbitrary position would intersect with a line.
  void IntersectLine(const ObjectVisualState &pos, CollisionBuffer &result,
    Vector3Par beg, Vector3Par end, float radius,
    ObjIntersect type=ObjIntersectFire, int hierLevel=0, unsigned int collisionFlags = 0) const;
  
  /// intersection with explicit level selection (cockpit view-geom. may be checked)
  void IntersectLine(const ObjectVisualState &pos, CollisionBuffer &result,
    Vector3Par beg, Vector3Par end, float radius,
    ObjIntersect type, const ConvexComponents *cc, int geomLevel, int hierLevel=0, unsigned int collisionFlags = 0) const;

  //! Check if object at current position intersects with line.
  void IntersectLine(VisualStateAge age, CollisionBuffer &result, Vector3Par beg, Vector3Par end, float radius, ObjIntersect type=ObjIntersectFire, unsigned int collisionFlags = 0) const;

  //! Reset all object properties to the default state.
  virtual void ResetStatus();
  //! Time skipped, react accordingly
  virtual void OnTimeSkipped() {}

  //! Position changed from outside (setPos), react accordingly
  virtual void OnPositionChanged() {}

  //! Check if status of the object need to be saved
  virtual bool MustBeSaved() const;

  //!\name SerializeClass style conforming
  //@{
  // Load/save object status.
  virtual LSError Serialize(ParamArchive &ar);
  static Object *CreateObject(ParamArchive &ar);
  static Object *LoadRef(ParamArchive &ar);
  LSError SaveRef(ParamArchive &ar);
  //@}

  //@{
  //! Access network id
  void SetNetworkId(NetworkId &id) {Fail("Cannot set network id");}
  NetworkId GetNetworkId() const;
  //@}

  //! Check if object is local
  bool IsLocal() const {return true;}
  //! Change object local/remote status 
  void SetLocal(bool local=true) {Fail("Object is always local");}

  //!\name NetworkObject implementation
  //@{
  // Network transfer interface
  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  static Object *CreateObject(NetworkMessageContext &ctx);
  void DestroyObject() override;
  TMError TransferMsg(NetworkMessageContext &ctx) override;
  float CalculateError(NetworkMessageContextWithError &ctx) override;
  virtual float CalculateErrorInitial(NetworkMessageClass cls) override;
  static float CalculateErrorInitialDamage(NetworkMessage *msg);
  Vector3 GetCurrentPosition() const override {return WorldPosition(FutureVisualState());}
  //@}

  // camera helpers
  //! Get external camera position (model space)
  virtual Vector3 ExternalCameraPosition( CameraType camType ) const;
  //! Get internal camera transformation matrix (world space)
  virtual Matrix4 InternalCameraTransform(Matrix4 &base, CameraType camType) const;
  //! Get group camera distance
  virtual float OutsideCameraDistance(CameraType camType ) const {return 20.0f;}
  //! Get tracking camera speed
  //!\note: Tracking camera is currently not implemented
  virtual float TrackingSpeed() const {return 15.0f;}

  struct Zoom : public SerializeClass
  {
    float _camFOV, _camFOVNoCont, _camFOVWanted;
    bool _zoomInsteadOfOptics;

    ///< toggle permanently
    bool _lookAroundToggleEnabled;
    /// result of both toggle and temporary
    bool _lookAroundEnabled; 
    /// commanding through DIK_SPACE in helicopter wants freeLook
    bool _forceLookAround;

    int _lookAroundLocked;

    Zoom()
    : _camFOV(0), _camFOVNoCont(0), _camFOVWanted(0), _zoomInsteadOfOptics(false),
    _lookAroundToggleEnabled(false), _lookAroundEnabled(false), _forceLookAround(false), _lookAroundLocked(0)

    {}
    void Update(InputInterface *input, float deltaT, Object *object, const ViewPars *viewPars=NULL, bool discrete = false);
    void ZoomIn(Object *object);
    void ZoomOut(Object *object);
    void Init(const ViewPars *init);
    void Init(const Object *object);
    void Init(float fov);
    LSError Serialize(ParamArchive &ar);
  };

  //! Get camera FOV (this replaced World::GetCameraFOV)
  virtual float GetCameraFOV() const {return 1.0f;}

  //! Get camera direction. This is used to change orientation of external camera.
  virtual Vector3 GetCameraDirection( CameraType camType ) const;
  //! Check if flares should be drawn with given camera
  virtual bool HasFlares( CameraType camType ) const;
  //! Check if camera is virtual (user can change camera direction)
  virtual bool IsVirtual( CameraType camType ) const;
  //! Check if camera is virtual in horizontal direction
  //! (user can change camera direction)
  virtual bool IsVirtualX( CameraType camType ) const;
  //! Check if camera is contiuous
  /*!
    Most parameters of contiuous camera (zoom, direction)
    can be adjusted continuosly and retain adjusted level.\n
    Discreet camera is used for looking to predefined directions / zoom
    and returns back to default state when controls are released.
  */
  virtual bool IsContinuous( CameraType camType ) const;

  /// Get camera field of view
  virtual float CamEffectFOV() const;
  /// get focus for depth of field effects
	virtual void CamEffectFocus(float &distance, float &blur) const;
  //! Simulate HUD 
  virtual void SimulateHUD(CameraType camType, float deltaT);
  //! Limit camera heading, dive and fov
  virtual void LimitVirtual(
    CameraType camType, float &heading, float &dive, float &fov
  ) const;
  //! Init camera heading, dive and fov
  virtual void InitVirtual(
    CameraType camType, float &heading, float &dive, float &fov
  ) const;

  //! Object may override actual cursor position.
  virtual void OverrideCursor(Vector3 &dir) const;
  //! Check if camera is in group leader mode
  virtual bool IsExternal( CameraType camType ) const;
  //! Get transformation of iternal camera
  virtual Matrix4 InsideCamera( CameraType camType ) const {return MIdentity;}
  //! Check which lod level should be used for internal view
  virtual int InsideLOD( CameraType camType ) const {return LOD_INVISIBLE;}
  //! Check which lod level should be used for internal view occlusion testing
  virtual int InsideViewGeomLOD( CameraType camType ) const;
  //! Check if weapon may be controlled with this camera
  virtual bool IsGunner( CameraType camType ) const;
  //! Chcek if turret picture should be drawn in given camera mode
  virtual bool IsTurret( CameraType camType ) const;
  //! Check if weapon aiming dot should be drawn
  virtual bool ShowAim(const TurretContextV &context, const WeaponModePtr &ptr, CameraType camType, Person *person) const;
  //! Check if aiming cursor should be drawn
  virtual bool ShowCursor(const TurretContextV &context, const WeaponModePtr &ptr, CameraType camType, Person *person) const;
  //! Check if aiming cursor should be drawn
  virtual Pair<Vector3,float> ShowDrivingCursor(CameraType camType, Person *person) const;
  //! Check if it is possible to give orders
  virtual bool IsCommander( CameraType camType ) const {return true;}

  //! Internal state changes when free-look mode changed
  virtual void FreelookChange(bool active) {}
  //! Internal state changes when camera change from internal to external or vice versa
  virtual void CamIntExtSwitched() {}

  //! Animate single point - model space
  virtual Vector3 AnimatePoint(const ObjectVisualState &vs, int level, int index ) const;
  /// check if given point is animated
  virtual bool IsPointAnimated(int level, int index) const;
  //! Animate matrix related to given bone
  virtual void AnimateBoneMatrix(Matrix4 &mat, const ObjectVisualState &vs, int level, SkeletonIndex boneIndex) const;

  Pair<Vector3,bool> DeanimatePoint(const ObjectVisualState &vs, int level, const BlendSkinning &skinning, Vector3Par pos);

  //! Flag to determine this object is marked to be deleted soon
  /** we do not want ToDelete to be virtual - causes serious performance hit */
  virtual bool IsMarkedToDelete() const {return false;}

  //@{ Access to world space transformation
  virtual Matrix4 WorldTransform(ObjectVisualState const& vs) const;
  virtual Matrix4 WorldInvTransform(ObjectVisualState const& vs) const;
  virtual Vector3 WorldSpeed() const;
  virtual Vector3 WorldPosition(ObjectVisualState const& vs) const;
  //@}

  //! Get world space proxy transform
  /*!
  \todo There may be several proxies using same Object.
  Use better proxy identification.
  */
  virtual Matrix4 ProxyWorldTransform(VisualStateAge age, const Object *obj) const;
  //! Get world space proxy inverse transform
  virtual Matrix4 ProxyInvWorldTransform(VisualStateAge age, const Object *obj) const;

  //@{ restorable link support
  virtual LinkId GetLinkId() const;

  virtual void LockLink() const;
  virtual void UnlockLink() const;


  virtual bool NeedsRestoring() const;

  static Object *RestoreLink(const LinkId &id);
  static bool RequestRestoreLink(const LinkId &id, bool noRequest=false);
  //@}

  //@{ Encapsulate unlock so that object may be released before unlock
  void UnlockLinkPrepare(UnlockLinkContext &context) const;
  static void UnlockLinkFinish(UnlockLinkContext &context);
  //@}
  
  //! Call to prepare skew matrix - called during binarization / ReplaceObjects
  virtual bool InitSkewReplace(Landscape *land, Matrix4 &trans);
  
  virtual bool InitSkew(Landscape *land, Matrix4 &pos) const;
  virtual void InitSkew(Landscape *land );


  virtual void DeSkew(const Landscape *land, Matrix4& trans, float& relHeight);
  
  // place in steady position
  virtual void PlaceOnSurface(Matrix4 &trans);
  /// check model coordinates of a reference point for purpose of placing onto a surface
  virtual Vector3 PlacingPoint() const;

  //! Returns true if one instance is allowed in instanced drawing
  virtual bool MustBeInstanced() {return false;}
  virtual void SetupInstances(int cb, const Shape &sMesh, const ObjectInstanceInfo *instances, int nInstances);
  USE_CASTING(base)
};

TypeIsSimple(Object*);

#if _VBS3_LARGE_OBJECTS
struct LargeObjectInfo
{
  int posX,posZ;
  VisitorObjId id;
  OLinkL(Object) object;
  LargeObjectInfo() {}
  LargeObjectInfo(Object *obj);
};
TypeContainsOLink(LargeObjectInfo)
//TypeIsMovableZeroed(LargeObjectInfo)
#endif

#include <Es/Memory/normalNew.hpp>

/// optional information about an object damage
class DamageRegions
{
  friend class Object;

  float _totalDamage;
  /// smoke from a damaged object
  OLinkPerm<Entity,SoftLinkTraits<Object> > _smoke;
  //! Structure to hold bullet holes for the object
  AutoArray<CraterProperties> _craters;
  
public:
  DamageRegions() :_totalDamage(0.0f) {}
  
  float GetTotalDamage() const {return _totalDamage;}
  void SetTotalDamage(float val) {_totalDamage = val; saturateMin(_totalDamage,1000.0f); }

  float Repair(float ammount);
  bool MustBeSaved() const {return _totalDamage > 0.0f;}

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>


template <class To,class From>
To *dyn_cast( From *from )
{
  DoAssert(To::_classId==sizeof(To)); // if size does not match, the class To did not define a USE_CASTING
  if( !from ) return NULL;
  if( from->IsClassId(&To::_classId) )
  {
    Assert(dynamic_cast<To *>(from));
    return static_cast<To *>(from);
  }
  return NULL;
}

#include <Es/Memory/normalNew.hpp>

//! very simple object
/*!
  This class is used for objects that use only default implementation.
  Main reason for existence of this class
  is different allocator used (USE_FAST_ALLOCATOR).
*/
class ObjectPlain: public Object
{
  typedef Object base;

  public:

  ObjectPlain(LODShape *shape, const CreateObjectId &id);
  ~ObjectPlain();
  USE_FAST_ALLOCATOR
};

//! similar to ObjectPlain, but instancing is disabled

class ObjectPlainNoInstancing: public Object
{
  typedef Object base;

  public:

  ObjectPlainNoInstancing(LODShape *shape, const CreateObjectId &id);
  ~ObjectPlainNoInstancing();

  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const {return false;}
  virtual bool CanBeInstancedShadow(int level) const {return false;}
  virtual bool CanObjDrawAsTask(int level) const {return false;}
  virtual bool CanObjDrawShadowAsTask(int level) const {return false;}
  USE_FAST_ALLOCATOR
};


/// parameters of the wind waving
class WindObjectType
{
private:
  /// frequency of waving in the wind (weak wind, 0 m/s)
  float _frequency0ms;
  /// frequency of waving in the wind (strong wind, 10 m/s)
  float _frequency10ms;
  /// amplitude of waving in the wind
  float _amplitude;
  /// how much is given object tilted by the wind
  float _tilt;
  /// some object need to have orientation applied last because of dammaging
  bool _setOrientationLast;

  /// precomputed constant to optimize EnlargeClippingInfo
  float _maxSkewFactor;
public:
  WindObjectType(
    float frequency0ms, float frequency10ms, float amplitude, float tilt, bool setOrientationLast
  );
  void AnimateOrigin(Matrix4 &origin, int seed, Vector3Par boundingCenter);
  float EnlargeClippingInfo(const Matrix4 &origin, Vector3 *minmax, float sphere) const;
};


/// object animated by the wind waving
class ObjectWindAnim: public Object
{
  typedef Object base;

public:

  ObjectWindAnim(LODShape *shape, const CreateObjectId &id);
  ~ObjectWindAnim();
  
  virtual WindObjectType GetWindType() const = 0;
  virtual void Draw(
    int cb, int forceLOD, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  ) override;
  virtual void DrawSimpleInstanced(
    int cb, int forceLOD, ClipFlags clipFlags, const DrawParameters &dp,
    ObjectInstanceInfo *instances, int instanceCount, float dist2, const LightList &lights,
    SortObject *oi, float coveredArea
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  );
	virtual Visible VisibleStyle() const {return ObjVisibleSlow;}
  virtual bool CanLimitClearance() const {return false;}
	float GetCollisionRadius(const ObjectVisualState &vs) const;
  float ClippingInfo(const ObjectVisualState &vs, Vector3 *minMax, ClippingType clip) const;
  float DensityRatio() const;
  virtual Color CraterColor() const {return Color(0.45f, 0.375f, 0.18f, 0.7f);}
  #if _ENABLE_CHEATS
    virtual bool CheckDrawToggle() const;
    virtual float CheckDrawScale() const;
  #endif
  USE_FAST_ALLOCATOR
};

//! ObjectColored class
/*!
  This class is used for plain objects that should be colored.
*\note USE_FAST_ALLOCATOR is used here. All classes deriving from ObjectColored
  must use USE_FAST_ALLOCATOR or USE_NORMAL_ALLOCATOR.
*/
class ObjectColored: public Object
{
  typedef Object base;

  Color _constantColor;
  int _specialOr;

  public:

  ObjectColored(LODShape *shape, const CreateObjectId &id);
  ~ObjectColored();

  // override virtual object functions
  virtual void SetConstantColor( ColorVal color ) {_constantColor=color;}
  virtual ColorVal GetConstantColor() const;

  // no instancing possible - we need per-object color information
  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const;

  int GetObjSpecial(int shapeSpecial) const {return _specialOr|shapeSpecial;}
  int GetSpecial() const {return base::GetSpecial()|_specialOr;}
  void SetSpecial( int spec ) {_specialOr = spec;}

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

//! Information about collision with object

struct CollisionInfo
{
  /// surface texture (Obsolete: use surface where possible) TODO: remove
  Texture *texture;
  /// surface material properties
  const SurfaceInfo *surface;

  /// position of collision (in world coord)
  Vector3 pos;
  /// direction outside (in world coord) or (in case of line-object collision) direction and size of the intersection
  Vector3 dirOut;
  /// direction not normalized (in world coord) or (in case of line-object collision) world space plane normal in the point of intersection
  Vector3 dirOutNotNorm;

  /// Which hierarchy level is the collision detected at, 0 = objects in landscape, >0 = proxy
  int hierLevel; 
  /// object,that we collide with (NULL if none), If hierLevel > 0 object is the proxy object
  Ref<Object> object;
  /// If hierLevel > 0 most parent of the proxy object
  Ref<Object> parentObject;

  /// how deep is contact (in world coord) or (in case of line-object collision) "time" of the intersection
  float under;
  /// index of component in corresponding geometry level
  int component;

  //is false if begining point was inside
  bool entry;
  //is false if end point was inside
  bool exit;

  /// which LOD level was the component found in - needed in case of secondary (typeSec) collisions
  int geomLevel;
#if _VBS3
  /// volume of intersection (but used to store modified impulse only)
  float underVolume;
#endif

  // if indestructible proxy is hit, assume that the parent object is hit instead
  Object *HitObject() const { return (parentObject && (!object || (object->GetDestructType() == DestructNo))) ? parentObject : object; }
  float MaxDeflectionAngleCos() const { return object ? object->MaxDeflectionAngleCos() : 0.0f; }
  float BulletPenetrability() const { return (object && surface) ? surface->_bulletPenetrability : 0.0f; }
  float Thickness() const { return (object && surface) ? surface->_thickness : -1.0f; }
  float DistInside() const { return dirOut.Size(); }
};

TypeIsMovableZeroed(CollisionInfo);

#if _HELISIM_ROTORLIB_SIM_THREAD

class CollisionBuffer
{
  typedef CollisionInfo Type;
  typedef AutoArray<Type, MemAllocDataStack<Type,128,AllocAlign16> > Container;
  typedef AutoArray<Type> ContainerEx;

  Container *_buffer;
  ContainerEx *_bufferEx;

public:
  CollisionBuffer(bool stackMemAlloc = true)
    : _buffer(stackMemAlloc ? new Container() : NULL),
    _bufferEx(stackMemAlloc ? NULL : new ContainerEx)
  {
  }
  ~CollisionBuffer() 
  { 
    if (_buffer)
      delete _buffer;

    if (_bufferEx)
      delete _bufferEx;
  }
  CollisionBuffer(const CollisionBuffer &src)
  {
    if (src._buffer)
      *_buffer = *src._buffer;
    else
      *_bufferEx = *src._bufferEx;
  }

  Vector3 GetContactPos(int fromIndx, int toIndx) const;

  //! Defined operator [] for modifying a value.
  __forceinline Type &operator [] (int i) {return (_buffer ? _buffer->Set(i) : _bufferEx->Set(i));}
  //! Defined operator [] for reading a value.
  __forceinline const Type &operator [] (int i) const {return (_buffer ? _buffer->Get(i) : _bufferEx->Get(i));}
  //! Retrieving a number of items used in the array.
  __forceinline int Size() const {return (_buffer ? _buffer->Size() : _bufferEx->Size());}
  //! Destruction and deallocation
  __forceinline void Clear()
  {
    if (_buffer) 
      _buffer->Clear();
    else
      _bufferEx->Clear();
  }
  //!  Adding of an item with resizing of the field.
  __forceinline int Add(const Type &src) { return (_buffer ? _buffer->Add(src) : _bufferEx->Add(src));}
  //! Adding of an item with resizing the field.
  __forceinline Type &Append() {return (_buffer ? _buffer->Append() : _bufferEx->Append());}
  //! Adding of an item with resizing the field.
  __forceinline Type &Append(const Type &val) {return (_buffer ? _buffer->Append(val) : _bufferEx->Append(val));}
  //! Returns the reference to array data for reading purpose.
  __forceinline const Type *Data() const {return (_buffer ? _buffer->Data() : _bufferEx->Data());}
  //! Returns the reference to array data for modifying purpose.
  __forceinline Type *Data() {return (_buffer ? _buffer->Data() : _bufferEx->Data());}
  //! Resizing of this array to n items.
  __forceinline void Resize(int n) 
  {
    if (_buffer)
      _buffer->Resize(n);
    else
      _bufferEx->Resize(n);
  }

private:
  CollisionBuffer &operator =( const CollisionBuffer &src);
};

#else

class CollisionBuffer: public AutoArray<CollisionInfo, MemAllocDataStack<CollisionInfo,128,AllocAlign16> >
{
public:
  Vector3 GetContactPos(int fromIndx, int toIndx) const;
};

#endif

/// identify cached animation matrices
struct MatrixCacheEntry
{
  //@{ ID of the cached animation
  const Object*	_obj;
  int _level;
  const Object*	_parent;
  //@}
  //@{ cached content
  int _matrices;
  int _matCount;
  //@}

  MatrixCacheEntry(): _obj(NULL), _level(0), _parent(NULL), _matrices(0), _matCount(0) {}
  MatrixCacheEntry(const Object* obj, int level, const Object* parent)
  : _obj(obj), _level(level), _parent(parent), _matrices(0), _matCount(0) {}
  MatrixCacheEntry& operator=(const MatrixCacheEntry& ce)
  {
    _obj = ce._obj;
    _level = ce._level;
    _parent = ce._parent;
    return *this;
  }

  bool operator==(const MatrixCacheEntry& ce) const
  {
    return _obj == ce._obj && _level == ce._level && _parent==ce._parent;
  }
  bool operator>(const MatrixCacheEntry& ce) const
  {
    if(_obj < ce._obj) return false;
    if(_obj > ce._obj) return true;
    if(_parent < ce._parent) return false;
    if(_parent > ce._parent) return true;
    return _level > ce._level;
  }
};

TypeIsMovableZeroed(MatrixCacheEntry);

/**
Matrix cache is cleared once per frame, during the frame any animated object is stored in it.
*/
struct MatrixCache
{
  AutoArray<MatrixCacheEntry> _entries;
  AutoArray<Matrix4>    _matrices;
  int         _cacheReused;
  int         _cacheMissed;

  MatrixCache(): _cacheMissed(0), _cacheReused(0)
  {
  }

  const MatrixCacheEntry& Get(int index) const
  {
    return _entries[index];
  }

  const Matrix4* GetMatrices(int off) const
  {
    return &_matrices.Data()[off];
  }

  /// search for the entry, return false if found, insert the new and return true when not found
  bool Insert(const Object* obj, int level, const Object *parentObject, int* index);
  /// search for the entry, return its index or -1 when not found
  int Find(const Object *obj, int level, const Object *parentObject) const;

  Matrix4 *AllocMatrices(int cacheindex, int count)
  {
    int oldSize = _matrices.Size();
    _entries[cacheindex]._matrices = oldSize;
    _entries[cacheindex]._matCount = count;
    _matrices.Resize(oldSize + count);
    return &_matrices.Data()[oldSize];
  }

  void Clear()
  {
    _entries.CompactIfNeeded();
    _matrices.CompactIfNeeded();
    _entries.Resize(0);
    _matrices.Resize(0);
    _cacheReused = 0;
    _cacheMissed = 0;
  }
};

extern MatrixCache GMatrixCache;

#endif

