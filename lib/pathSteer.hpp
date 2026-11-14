#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PATH_STEER_HPP
#define _PATH_STEER_HPP

//#include "loadSave.hpp"
#include "Network/networkObject.hpp"
#include "object.hpp"
#include "AI/aiTypes.hpp"
#include <El/Time/time.hpp>
#include <El/ParamArchive/serializeClass.hpp>

#include "pathAction.hpp"

#ifndef DECL_ASO_NODE_TYPE
#define DECL_ASO_NODE_TYPE
DECL_ENUM(ASONodeType)
#endif

#define ASO_NODE_TYPE_ENUM(type, prefix, XX) \
  XX(type, prefix, Road) \
  XX(type, prefix, Bridge) \
  XX(type, prefix, House) \
  XX(type, prefix, Grid) \
  XX(type, prefix, Error)

class RoadLink;

DECLARE_ENUM(ASONodeType, NT, ASO_NODE_TYPE_ENUM)

struct OperInfoResult
{
  Vector3 _pos;
  float _cost;
  OLink(Object) _house;
  const RoadLink *_road;
  int _index;
  Ref<const PathAction> _action;
  /// what field type we are
  ASONodeType _type;
  Clearance _clearance;

  OperInfoResult():_road(NULL),_clearance(ClearanceNormal){}

  LSError Serialize(ParamArchive &ar);
  static NetworkMessageFormat &CreateFormat
  (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
  TMError TransferMsg(NetworkMessageContext &ctx);
};
TypeContainsOLink(OperInfoResult)

/*
class IndicesPath;
*/

class Path: public AutoArray<OperInfoResult>, public SerializeClass 
{
  typedef AutoArray<OperInfoResult> base;

  /// when to start search a new operative plan
  float _costReplan;
  /// when the current operative plan is unusable
  float _costForceReplan;

  /// time when the path was constructed (Glob.time)
  Time _searchTime; 

  /// minimal cost returned by CostAtPos
  float _minCost;
  
  /// if the path leads into a cover, mark which one
  CoverInfo _cover;

  /// some paths lead into an "open" cover
  bool _openCover;
  
  /// once a path is completed, it cannot force us leaving a cover, fresh path is needed for this
  bool _completed;
  
  float Distance( int index, Vector3Par pos ) const;
  int FindNearest( Vector3Par pos ) const;

  public:
  Path();

  // keep the last cost
  void ResetMinCost() {_minCost = 0;}
  void SetMinCost(float cost) {_minCost = cost;}

  // corrected to be in pos plane
  Vector3 PosAtCost(
    float cost, float minDist, Vector3Par pos, float originalCost, Ref<const PathAction> *action = NULL, OLink(Object) *building = NULL, Clearance *clearance=NULL
  ) const;
  bool InHouseAtCost( float cost, Vector3Par pos ) const;

  // direct calculation
  float CostAtPos(Vector3Par pos, bool updateCost = false) const;
  float SpeedAtCost(float cost, bool isSoldier) const;

  float DistanceAtCost( float cost ) const;

  Vector3 PosAtCost( float cost ) const;
  Vector3 NearestPos( Vector3Par pos ) const;
  /// find nearest position in X/Z, but still make sure Y is reasonable
  Vector3 NearestPosXZ( Vector3Par pos ) const;

  /// return point further on the path
  Vector3 PosAtDist(Vector3Par pos, float dist);

  Vector3 Begin() const;
  Vector3 End() const;
  float EndCost() const;

  float GetReplanCost() const {return _costReplan;}
  void SetReplanCost(float cost) {_costReplan = cost;}

  float GetForceReplanCost() const {return _costForceReplan;}
  void SetForceReplanCost(float cost) {_costForceReplan = cost;}

  Time GetSearchTime() const {return _searchTime;}	
  void SetSearchTime( Time time ){_searchTime=time;}

  /// find the nearest next point on the path
  int FindNext(Vector3Par pos, bool minCost = false) const;

  /// find next for the given cost
  int FindNextCost(float cost) const;

  bool IsIntoCover() const {return _cover || _openCover;}

  const CoverInfo &GetCover() const {return _cover;}
  void SetCover(const CoverInfo &cover, bool openCover);
  
  bool GetOpenCover() const {return _openCover;}

  void SetCompleted() {_completed=true;}
  bool CheckCompleted() const {return _completed;}

  void Clear();

  LSError Serialize(ParamArchive &ar);

  void Optimize( EntityAI *vehicle ); // drop unnecessary points
  void AvoidCollision( EntityAI *vehicle ); // change path to avoid collisions
};


#endif

