#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VISIBILITY_HPP
#define _VISIBILITY_HPP

// sensor dirty unit is 2 m (max dirty is 20 m)

#include "vehicleAI.hpp"
#include "AI/ai.hpp"

enum {MaxSensorDirty=15};

template <class VehicleType>
class SensorInfo
{
  friend class SensorRow;
  friend class SensorList;

  OLinkO(VehicleType) _vehicle;
  /// position for which "dirty" was already computed and item sorted into the _toUpdate heap
  Vector3 _lastPos;

public:
  struct UnimportanceSrc
  {
    OLinkPermNO(AIBrain) brain; //  = from->CommanderUnit();
    bool unarmed; // fromCenter->GetSide()==TCivilian && !from->HasSomeWeapons()

    bool operator == (const UnimportanceSrc &with) const
    {
      return brain == with.brain;
    }

    void Init(EntityAI *veh)
    {
      if (!veh)
      {
        brain = NULL;
        unarmed = false;
        return;
      }
      brain = veh->CommanderUnit();
      unarmed = brain && brain->GetSide()==TCivilian && !veh->HasSomeWeapons();
    }
  };

  struct UnimportanceSrcFrom: UnimportanceSrc
  {
    typedef UnimportanceSrc base;

    bool fromCamera; // fromBrain->GetVehicle()!=GWorld->CameraOn()

    bool operator == (const UnimportanceSrcFrom &with) const
    {
      return UnimportanceSrc::operator ==(with) && fromCamera==with.fromCamera;
    }

    UnimportanceSrcFrom(){}
    void Init(Person *from)
    {
      UnimportanceSrc::Init(from);
      fromCamera = base::brain && base::brain->GetVehicle()!=GWorld->CameraOn();
    }
  };

  struct UnimportanceSrcTo: UnimportanceSrc
  {
   typedef UnimportanceSrc base;

    float visibleSize; // =to->VisibleSize(to->FutureVisualState());
    TargetSide targetSide; // to->GetTargetSide())
    bool alive; // toUnit && toUnit->LSIsAlive()
    LLink<const EntityAIType> type; // to->GetType()

    bool operator == (const UnimportanceSrcTo &with) const
    {
      DoAssert(type==with.type || type==NULL || with.type==NULL); // TODO: do not track, is constant
      return (
        UnimportanceSrc::operator ==(with) && visibleSize==with.visibleSize
        && targetSide==with.targetSide && alive==with.alive && type==with.type
      );
    }

    UnimportanceSrcTo(){visibleSize=0;}
    void Init(EntityAI *to)
    {
      UnimportanceSrc::Init(to);
      if (to)
      {
        visibleSize = to->VisibleSize(to->FutureVisualState());
        targetSide = to->GetTargetSide();
        alive = base::brain && base::brain->LSIsAlive();
        type = to->GetType();
      }
      else
      {
        type = NULL;
        visibleSize = 0;
      }
    }
  };

  SensorInfo():_lastPos(0,0,0){}
  LSError Serialize(ParamArchive &ar);
  bool IsDefaultValue(ParamArchive &ar) const {return false;}
  void LoadDefaultValues(ParamArchive &ar) {}

  ClassContainsOLink(SensorInfo);
};

class SensorCol: public SensorInfo<EntityAI>
{
  friend class SensorList;

  UnimportanceSrcTo _unimportanceSrc;

};

TypeContainsOLink(SensorCol)

/*
enum // note: this enum is never used
{
SPriorityTarget=128,
// AI vehicles update targets
SPriorityEnemy=32,
SPriorityFriendly=0,
// player update targets
SPriorityPlayerEnemy=64,
SPriorityPlayerFriendly=16,
SPriorityDead=8,
};
*/

/// Element of visibility matrix

struct SensorUpdate
{
  /// check result stored
  unsigned char vis8;
  /// set when item is present in SensorList::_toUpdate
  bool toUpdate;

  TimeSec lastVisible; //!< last time when visibility was > 0
  Time time; //!< last check time
  SensorPos rowPos; //!< sensor (observer) position
  SensorPos colPos; //!< target position
  // TODO: remove unimportance and defValue or compress it to reduce size
  float unimportance; //!< cached result of SensorList::Unimportance
  float defValue;

public:
  SensorUpdate();
  void Init();
  bool IsDefaultValue(ParamArchive &ar) const;
  void LoadDefaultValues(ParamArchive &ar);

  LSError Serialize(ParamArchive &ar);
  RString DiagText() const;

  __forceinline float GetVisibility() const {return vis8*(1.0f/255);}
  __forceinline void SetVisibility0(){vis8=0;}
  __forceinline void SetVisibility1(){vis8=255;}

  void SetVisibility(float vis);
  void SetLastVisible(Time vt){lastVisible=TimeSec(vt);}
  Time GetLastVisibilityTime() const;
};

TypeIsSimpleZeroed(SensorUpdate)

//! Row of visibility matrix

class SensorRow: public SensorInfo<Person>
{
  friend class SensorList;
  typedef SensorInfo<Person> base;

  AutoArray<SensorUpdate> _info;
  UnimportanceSrcFrom _unimportanceSrc;

public:
  SensorRow();
  void Init( Person *vehicle, int size );
  void Free();	

  LSError Serialize(ParamArchive &ar);
};

TypeContainsOLink(SensorRow);

/// Coordinates of the cell, used in the list of cells for smart update
struct SensorCell
{
  SensorRowID _row;
  SensorColID _col;
};
TypeIsSimple(SensorCell)

struct CellCoord
{
  OLinkPermNO(Person) from;
  OLinkPermNO(EntityAI) to;

  CellCoord(){}
  CellCoord(Person *from, EntityAI *to):from(from),to(to){}

  ClassContainsOLink(CellCoord)

  Time NextUpdate() const;
};

/*
template <>
struct HeapTraits<CellCoord>
{
  // default traits: Type is pointer to actual value
  static bool IsLess(const CellCoord &a, const CellCoord &b)
  {
    return a.NextUpdate() < b.NextUpdate();
  }

};
*/

//! Visibility matrix

class SensorList
{
  AutoArray<SensorCol> _cols; //!< columns - targets
  AutoArray<SensorRow> _rows; //!< rows - observers

  /// coordinates of cells waiting to be updated, listed by expected time to update
  FindArray<CellCoord> _toUpdate;

  
  SensorRowID _lastUpdateCellRow;
  SensorColID _lastUpdateCellCol;

  SensorRowID FindRowID( Person *veh );
  SensorColID FindColID( EntityAI *veh );

  void DeleteCol( SensorColID i ); // 
  void DeleteRow( SensorRowID i ); // 

public:
  SensorList();
  ~SensorList();

  SensorRowID AddRow( Person *veh ); // new ID
  SensorColID AddCol( EntityAI *veh ); // new ID
  void DeleteRow( Person *veh ); // 
  void DeleteCol( EntityAI *veh ); // 
  void Compact();

  void CheckPos();

  void ProcessRowToUpdate(SensorRowID r);
  void ProcessColToUpdate(SensorColID c);

  /// recalculate unimportance for all fields
  void RecalcUnimportance();

  //@{ all Updates return count of visibility checks
  int UpdateCell( SensorRowID r, SensorColID c, bool force=false ); // update single

  /// update single row
  int UpdateRow( SensorRowID id );
  /// update single column
  int UpdateCol( SensorColID id );
  //@}
  int UpdateAll();

  int SmartUpdateAll(); // update what is necessary, only some
  float GetVisibility( Person *from, EntityAI *to, float maxAge=FLT_MAX ) const;
  Time GetVisibilityTime( Person *from, EntityAI *to ) const;
  Time TimeToNextUpdate( Person *from, EntityAI *to ) const;

  RString DiagText( Person *from, EntityAI *to ) const;

  bool CheckStructure() const;
  static float Unimportance(EntityAIFull *from, EntityAI *to, float &defValue);

  LSError Serialize(ParamArchive &ar);
  static SensorList *CreateObject(ParamArchive &ar) {return new SensorList();}

  void UpdateUnimportanceRow(SensorRowID id);
  void UpdateUnimportanceCol(SensorColID id);

  void UpdateUnimportanceRow(Person *veh);
  void UpdateUnimportanceCol(EntityAI *veh);

protected:
  /// call UpdateCell, check row and col before (as in UpdateRow / UpdateCell)
  int CheckAndUpdateCell(SensorRowID r, SensorColID c);
};


#endif


