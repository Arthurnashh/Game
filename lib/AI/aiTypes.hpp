#ifdef _MSC_VER
#pragma once
#endif

#ifndef _AI_TYPES_HPP
#define _AI_TYPES_HPP

#include <Es/Types/removeLinks.hpp>
#include "../object.hpp"

class AI;
class AILocker;

class OperField;
class LockField;

// const int OperItemRangeLog = 4;
const int OperItemRangeLog = 5;
const int OperItemRange = 1 << OperItemRangeLog;
const int OperItemRangeMask = OperItemRange - 1;
const float InvOperItemRange = 1.0f/OperItemRange;

// note: LandGrid and InvLandGrid are not constants, but macros accessing global functions
#define OperItemGrid      (LandGrid *InvOperItemRange)
#define InvOperItemGrid		(OperItemRange * InvLandGrid)

/// index of the strategic field (0 ... LandRange)
UNIQUE_TYPE(LandIndex, int);
/// index of the operative field (0 ... LandRange * OperItemRange)
UNIQUE_TYPE(OperMapIndex, int);
/// index of the operative field in the strategic field (0 ... OperItemRange)
UNIQUE_TYPE(OperFieldIndex, int);

// correct conversion of indices

#define SPLIT_INDICES(x, z, xLand, zLand, xField, zField) \
  LandIndex xLand((x) >> OperItemRangeLog); \
  LandIndex zLand((z) >> OperItemRangeLog); \
  OperFieldIndex xField((x) & OperItemRangeMask); \
  OperFieldIndex zField((z) & OperItemRangeMask);

// useful for roads handling - we do not need field coordinates there
#define LAND_INDICES(x, z, xLand, zLand) \
  LandIndex xLand((x) >> OperItemRangeLog); \
  LandIndex zLand((z) >> OperItemRangeLog);

inline void SplitIndices(OperMapIndex x, OperMapIndex z, LandIndex &xLand, LandIndex &zLand, OperFieldIndex &xField, OperFieldIndex &zField)
{
  xLand = LandIndex((x) >> OperItemRangeLog);
  zLand = LandIndex((z) >> OperItemRangeLog);
  xField = OperFieldIndex((x) & OperItemRangeMask);
  zField = OperFieldIndex((z) & OperItemRangeMask);
}

inline void LandIndices(OperMapIndex x, OperMapIndex z, LandIndex &xLand, LandIndex &zLand)
{
  xLand = LandIndex((x) >> OperItemRangeLog);
  zLand = LandIndex((z) >> OperItemRangeLog);
}

#if defined _MSC_VER && defined _M_PPC
// X360 bitfield layout is by default different (msb_to_lsb, common on PPC)
// we want the same as used on x86, i.e. lsb_to_msb
# pragma bitfield_order(push, lsb_to_msb)
#endif

/// describe properties of one terrain grid for strategic path finding
union GeographyInfo
{
	unsigned packed;
	struct
	{
		// note: some bug in MSVC 5.0
		// bool is not properly used in bit-field (in unions)
		unsigned minWaterDepth:2; // shallowest water
		//unsigned deepWater:1;
		//unsigned shallowWater:1;
		unsigned full:1;
		unsigned forest:1;
		unsigned road:1;	// fast road
		unsigned maxWaterDepth:2; // deepest water, when minWaterDepth>maxWaterDepth, old encoding is used
		unsigned howManyObjects:2; // count of hard obstacles (0..3)
		unsigned howManyHardObjects:2; // count of hard obstacles (0..3)
		unsigned gradient:3;
		unsigned someRoadway:1; // optimization - when true, RoadSurfaceY may be faster
		unsigned someObjects:1; // optimization - when true, UseObjects may be faster
	} u;
	
	GeographyInfo(){}
	explicit GeographyInfo(unsigned val):packed(val){}
	unsigned short GetPacked() const {return packed;}

  // in binarized world file-format 3, there was extra bitfield  forestInner before forest.
  void FromFormat3To7() { GeographyInfo o = *this; packed = packed >> 1; u.minWaterDepth = o.u.minWaterDepth; u.full = o.u.full;};
  void FromFormat7To3() { GeographyInfo o = *this; packed = packed << 1; u.minWaterDepth = o.u.minWaterDepth; u.full = o.u.full; u.forest = o.u.forest;};
};

#if defined _MSC_VER && defined _M_PPC
// undo bitfield_order above
# pragma bitfield_order(pop)
#endif

TypeIsSimple(GeographyInfo)

#include <Es/Containers/boolArray.hpp>

//@{ conversion between AIThreat storage and real (Threat) values
#define EXPOSURE_COEF     50.0F
#define INV_EXPOSURE_COEF (1.0F / EXPOSURE_COEF)
//@}

/// AICenter exposure maps
union AIThreat
{
	unsigned int packed;
	struct Unpacked
	{
		BYTE soft;
		BYTE armor;
		BYTE air;
		BYTE reserved;
    
    static AIThreat _null;
	} u;
  AIThreat() {packed = 0;}
  AIThreat(int soft, int armor, int air)
  {
    u.soft = soft;
    u.armor = armor;
    u.air = air;
  }
  
  /// useful when returning a const & value
  static const AIThreat &GetNull() {return Unpacked::_null;}
  /// average value access
  float Avg() const {return (u.soft+u.armor+u.air)*0.33f;}
  
  AIThreat operator + (const AIThreat &b) const;
};

TypeIsSimple(AIThreat)


//! entity event enum defined using enum factory
#define COVER_TYPE_ENUM(type,prefix,XX) \
  XX(type, prefix, None) /*no cover */ \
  XX(type, prefix, RightCornerHigh) /* high vertical edge, like a building edge - use standing + leaning*/ \
  XX(type, prefix, RightCornerMiddle) /* middle vertical edge, like a fence edge - use kneel + leaning*/ \
  XX(type, prefix, RightCornerLow) /* low vertical edge - use prone + sideways movement*/ \
  XX(type, prefix, LeftCornerHigh) /* high vertical edge, like a building edge - use standing + leaning*/ \
  XX(type, prefix, LeftCornerMiddle) /* middle vertical edge, like a fence edge - use kneel + leaning*/ \
  XX(type, prefix, LeftCornerLow) /* low vertical edge - use prone + sideways movement*/ \
  XX(type, prefix, EdgeMiddle) /* horizontal edge, like a fence - use kneel + stand */ \
  XX(type, prefix, EdgeLow) /* low horizontal edge, like a fence - use prone + kneel */ \
  XX(type, prefix, Wall) /* wall - total protection from one side, no fire over the obstacle */ \

#ifndef DECL_ENUM_COVER_TYPE
#define DECL_ENUM_COVER_TYPE
DECL_ENUM(CoverType)
#endif
DECLARE_ENUM(CoverType,Cover,COVER_TYPE_ENUM)

#if (_DEBUG || _PROFILE)
# define DIAG_FALSE_COVER 1
#else
# define DIAG_FALSE_COVER 0
#endif

/// information about a cover candidate as recorded in operative planning map
struct OperCover
{
  /// cover position
  Vector3 _pos;
  /// cover azimuth (pointing outside of the cover)
  /** _heading = FLT_MAX means invalid value - no cover */
  float _heading;

  //@{ coordinates inside of current OperField
  /** consider: create only a temporary array for this */
  signed char _x,_z;
  
  /// assumed type of the cover (left/right does not matter in OperCover)
  SizedEnum<CoverType> _type;
  
  #if DIAG_FALSE_COVER
    enum State
    {
      Unchecked,
      Valid,
      NoCollision,
      NoFreeSpace,
      BadHeight,
      /// unaccessible because of cost map
      Unaccessible 
    };
    InitVal<State,Unchecked> _valid;
  #endif

  static const float UpperGap() {return 0.2f;} // how much must the cover extend above the firing position
  /// some covers are more suitable than others (some are small, some are soft penetrable)
  /** we store here delay preferred over using this cover */
  unsigned char _coverScore;
  //@}
  OperCover():_heading(FLT_MAX),_pos(VZero),_type(CoverNone){}
  OperCover(Vector3Par pos, CoverType type, int score, float heading, int x, int z)
  :_pos(pos),_coverScore(score),_heading(heading),_type(type),_x(x),_z(z)
  {
    Assert(score>=0 && score<=UCHAR_MAX);
    Assert(x>=SCHAR_MIN && x<=SCHAR_MAX);
    Assert(z>=SCHAR_MIN && z<=SCHAR_MAX);
  }
  
  operator bool () const {return _heading<FLT_MAX;}
  
  float GetScore() const {return _coverScore*0.1f;}
  bool operator == (const OperCover &with) const {return _pos==with._pos && _heading==with._heading;}
};
TypeIsMovable(OperCover)

/// detailed runtime information about exact cover position

struct CoverVars: public RefCount, public SerializeClass
{
  /// cover mode currently in use
  CoverType _type;
  /// exact position of the cover
  Vector3 _pos;
  /// transformation matrix, pos gives a position on the ground
  Matrix4 _coverPos;
  //@{
  /**
  describe a box behind the cover point where we know the soldier can move freely
  The box is oriented based on _coverDir. The height of the box is assumed to be enough for the soldier height.
  */
  float _areaFreeLeft;
  float _areaFreeRight;
  float _areaFreeBack;
  //@}
  /// entry point into the cover
  Vector3 _entry;

  LSError Serialize(ParamArchive &ar);

  static CoverVars *CreateObject(ParamArchive &ar){return new CoverVars;}

  bool operator == (const CoverVars &with) const
  {
    // pos and dir is all we need to identify the cover
    // the rest is only additional information
    return _pos==with._pos && _coverPos.Direction()==with._coverPos.Direction();
  }
  __forceinline bool operator != (const CoverVars &with) const {return !operator ==(with);}
};

/// information about a cover returned by A* path searching
struct CoverInfo
{
  /// if not, we are hiding in an operative map cover
  OperCover _oper;
  /// payload - information about a cover, which we may need to know when using _oper
  Ref<CoverVars> _payload;
  
  operator bool() const {return _oper;} 
  Vector3Val Position() const {return _oper._pos;}
  
  /// check if both represent the same cover
  bool operator == (const CoverInfo &with) const
  {
    return _oper==with._oper;
  }

  CoverInfo(){}
  explicit CoverInfo(const OperCover &oper):_oper(oper){}
  explicit CoverInfo(bool open):_oper(){}
  
  void operator = (const OperCover &oper) {_oper=oper;_payload.Free();}
};

/// field cost and time are coupled
struct FieldCost
{
  float cost;
  float time;

  void operator += (const FieldCost &f) {cost += f.cost;time += f.time;}
  void operator *= (float f) {cost *= f;time *= f;}

#if _ENABLE_REPORT
  // make sure uninitialized values are detected
  FieldCost():cost(FLT_SNAN),time(FLT_SNAN){}
#else
  // by default there is no init
  FieldCost(){}
#endif
  explicit FieldCost(float c):cost(c),time(c){}
  FieldCost(float c, float t):cost(c),time(t){}
  FieldCost operator * (float f) const {FieldCost sum = *this;sum *= f;return sum;}
  FieldCost operator + (const FieldCost &f) const {FieldCost sum = *this;sum += f;return sum;}
};

///////////////////////////////////////////////////////////////////////////////
// visibility check interface
// 

#ifndef DECL_ENUM_TARGET_SIDE
#define DECL_ENUM_TARGET_SIDE
DECL_ENUM(TargetSide)
#endif

///////////////////////////////////////////////////////////////////////////////
// struct LockField

const int LockMaskVeh	= 0x0f;
const int LockMaskAll	= 0xf0;

#include <Es/Memory/normalNew.hpp>

/// locking fields to avoid two units occupying the same place or going through each other
/**
Locking is maintained in two layers (controlled by LockMaskVeh, LockMaskAll)

Both soldiers and vehicles create locks in both layers.
Locks with LockMaskAll are created with a lot smaller radius

Soldiers test locks only in the LockMaskAll layer
Vehicles test locks in both layers
*/

class LockField : public RefCount
{
protected:
	friend class LockCache;

	BYTE _locks[OperItemRange][OperItemRange];
	WORD _x;
	WORD _z;

private:
  /// field can be created only by the LockCache::GetField
  LockField(LandIndex x, LandIndex z);

public:
  ~LockField();

  LandIndex X() const {return LandIndex(_x);}
  LandIndex Z() const {return LandIndex(_z);}

	int IsLocked(OperFieldIndex x, OperFieldIndex z, bool soldier);
	void Lock(OperFieldIndex x, OperFieldIndex z, bool soldier, bool lock);
	
	USE_FAST_ALLOCATOR;
};
#include <Es/Memory/debugNew.hpp>

struct PlanPosition: public Vector3
{
  /// is this into a cover?
  bool _cover;
  
  PlanPosition(){}
  PlanPosition(Vector3Par pos, bool cover):Vector3(pos),_cover(cover){}
};


///////////////////////////////////////////////////////////////////////////////
// cache for operative map
// 

class IOperCache;

//! lock cache interface
class ILockCache
{
public:
	virtual ~ILockCache() {}

	virtual int IsLocked(OperMapIndex x, OperMapIndex z, bool soldier) = 0;
	virtual Ref<LockField> GetField(LandIndex x, LandIndex z, bool create) = 0; 
	virtual bool IsEmpty() const = 0;

private:
  friend class LockField;
  /// field is removed from the cache only in its destructor
  virtual void RemoveField(LandIndex x, LandIndex z) = 0;
};

ILockCache *CreateLockCache(Landscape *land);

// Message types for InGameUI
enum MessageType
{
	MessageSent,
	MessageReceived,
	MessageMission
};

// LifeState moved from AIBrain
enum LifeState
{
  LifeStateAlive,
  LifeStateDead,
  LifeStateAsleep,
  LifeStateUnconscious,
  NLifeStates
};

#define CLEARANCE_TYPE_ENUM(type,prefix,XX) \
  /* prone soldier only (50 cm) */ \
  XX(type, prefix, Low)  \
  /* crouched soldier only (<150 cm) */ \
  /* ClearanceMiddle,*/ \
  /* standing soldier, normal vehicle (170 cm)*/ \
  XX(type, prefix, Normal)

#ifndef DECL_ENUM_CLEARANCE
#define DECL_ENUM_CLEARANCE
DECL_ENUM(Clearance)
#endif
DECLARE_ENUM(Clearance, Clearance, CLEARANCE_TYPE_ENUM)

#endif
