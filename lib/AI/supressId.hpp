#ifdef _MSC_VER
#pragma once
#endif

#ifndef _AI_SUPPRESS_MAP_HPP
#define _AI_SUPPRESS_MAP_HPP

#include "../vehicleAI.hpp"

/// target info needed for suppress cost calculation
struct SuppressTargetInfo
{
  /// target identification
  ObjectId _object;
  /// how dangerous are shots from this target
  float _costCoef;

  SuppressTargetInfo(){}
  SuppressTargetInfo(ObjectId object, float costCoef) : _object(object), _costCoef(costCoef) {}
};
TypeIsSimple(SuppressTargetInfo);

/// traits for key comparison and searching
struct SuppressTargetTraits
{
  typedef ObjectId KeyType;
  /// check if two keys are equal
  static bool IsEqual(KeyType a, KeyType b) {return a == b;}
  /// get a key from an item
  static KeyType GetKey(const SuppressTargetInfo &a) {return a._object;}
};

/// list of targets needed for suppress cost calculation
class SuppressTargetList : public FindArrayKey<SuppressTargetInfo, SuppressTargetTraits, MemAllocLocal<SuppressTargetInfo, 256> >
{
public:
  // create a snapshot of unit's target list usable for suppress cost calculations
  void CollectTargets(AIBrain *unit);
};

enum SuppressSourceState
{
  /// no
  SupSrcNone,
  SupSrcDisabled,
  SupSrcActive,
};

/// ID used to identify one layer in the SuppressFieldFull
struct SourceId
{
  SuppressSourceState _state;
  ObjectId _source;
  Vector3 _srcPos;
  Vector3 _srcDir;
  /// time when last shot was fired in this layer
  Time _time;
  /// the only thing we care about ammo is its strength
  float _ammoHit;
  /// relative importance of this ID
  float _priority;

  float Priority() const {return _priority;}
  
  /// get cost tied to an individual source and unit
  float GetCost(const SuppressTargetList &list) const;
};


/// list of suppression IDs
class SupressIdList
{
  public:
  static const int MaxSources = 32;
  
  protected:
  
  SourceId _sources[MaxSources];

  public:
  SupressIdList();
  
  /// add a new ID - may discard some old one if required
  int AddId(EntityAI *source, Vector3Par srcPos, Vector3Par srcDir, float ammoHit, float ammoPrior);
  
  /// callback when ID is replaced by another one
  virtual void ClearLayer(int id){}
  
  float GetTotalCost(const SuppressTargetList &list) const;
};


#endif
