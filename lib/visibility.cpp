// visibility matrix
// row index is brain (sensor) handle
// column index is target handle

#include "wpch.hpp"
#include "vehicleAI.hpp"
#include "visibility.hpp"
#include "AI/ai.hpp"
#include "global.hpp"
#include "world.hpp"
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>
#include "landscape.hpp"
#include "paramArchiveExt.hpp"
#include "timeManager.hpp"

#define DIAGS 0

#if 0 // _ENABLE_CHEATS && _PROFILE
# pragma optimize("",off)
#endif

SensorRow::SensorRow()
{
}

void SensorRow::Init( Person *vehicle, int size )
{
  _info.Realloc(size);
  _info.Resize(size);
  for( int i=0; i<size; i++ )
  {
    _info[i].Init();
  }
  _vehicle=vehicle;
  _unimportanceSrc.Init(NULL); // force update
}

void SensorRow::Free()
{
  _info.Clear();
  _vehicle=NULL;
}

template <class VehicleType>
LSError SensorInfo<VehicleType>::Serialize(ParamArchive &ar)
{
  CHECK(ar.SerializeRef("Vehicle", _vehicle, 1))
  return LSOK;
}

void SensorUpdate::SetVisibility(float vis)
{
  int iVis = (int)(vis*255);
  saturate(iVis,0,255);
  vis8 = iVis;
}

SensorUpdate::SensorUpdate()
{
  toUpdate = false;
}

void SensorUpdate::Init()
{
  // full dirty
  rowPos.Init();
  colPos.Init();
  time = TIME_MIN;
  lastVisible = TIMESEC_MIN;
  vis8 = 0;
  toUpdate = false;
  unimportance = FLT_MAX;
  defValue = 0;
}

RString SensorUpdate::DiagText() const
{
  char buf[256];
  sprintf
  (
    buf,"%.2f (%.1fs, lv %.1fs, %.1fs)",
    GetVisibility(),Glob.time.Diff(time),
    Glob.time.Diff(lastVisible),Glob.time.Diff(GetLastVisibilityTime())
  );
  return buf;
}

DEF_RSB(lastVisible)
DEF_RSB(vis8)
DEF_RSB(time)
DEF_RSB(rowPos)
DEF_RSB(colPos)
DEF_RSB(rowPosV0)
DEF_RSB(rowPosV1)
DEF_RSB(rowPosV2)
DEF_RSB(rowPosV3)
DEF_RSB(colPosV0)
DEF_RSB(colPosV1)
DEF_RSB(colPosV2)
DEF_RSB(colPosV3)

bool SensorUpdate::IsDefaultValue(ParamArchive &ar) const
{
  if (Time(lastVisible)<Glob.time-120)
  {
    // it was not visible for very long time and can be considered never visible
    // - use default values
    return true;
  }
  return false;
}

void SensorUpdate::LoadDefaultValues(ParamArchive &ar)
{
  // if there are no values, use default
  lastVisible = ar.GetArVersion()>=12 ? TIMESEC_MIN : TIMESEC_MAX;
  vis8 = 0;
  time = Glob.time;
  rowPos.Init();
  colPos.Init();
}


/*!
\patch 1.50 Date 4/11/2002 by Ondra
- Optimized: Memory usage during save-game is now lower.
*/

LSError SensorUpdate::Serialize(ParamArchive &ar)
{
  // visibility matrix can be huge
  // many items in visibility matrix are "never visible"
  // we do not save such values
  // when loading them, we use some reasonable default
  if (ar.IsSaving())
  {
    if (Time(lastVisible)<Glob.time-120)
    {
      // it was not visible for very long time and can be considered never visible
      // - use default values
      return LSOK;
    }
  }
  TimeSec defLastVisible = ar.GetArVersion()>=12 ? TIMESEC_MIN : TIMESEC_MAX;
  CHECK(::Serialize(ar, RSB(lastVisible), lastVisible, 1, defLastVisible))
  // if there are no values, use default
  CHECK(ar.Serialize(RSB(vis8), vis8, 1, 0))
  // if not saved, load as MAX (most recent possible)
  CHECK(::Serialize(ar, RSB(time), time, 1, Glob.time))
  CHECK(::Serialize(ar, RSB(rowPos), rowPos.pos, 1, VZero))
  CHECK(::Serialize(ar, RSB(colPos), colPos.pos, 1, VZero))
  CHECK(ar.Serialize(RSB(rowPosV0), rowPos.optional[0], 1, 0))
  CHECK(ar.Serialize(RSB(rowPosV1), rowPos.optional[1], 1, 0))
  CHECK(ar.Serialize(RSB(rowPosV2), rowPos.optional[2], 1, 0))
  CHECK(ar.Serialize(RSB(rowPosV3), rowPos.optional[3], 1, 0))
  CHECK(ar.Serialize(RSB(colPosV0), colPos.optional[0], 1, 0))
  CHECK(ar.Serialize(RSB(colPosV1), colPos.optional[1], 1, 0))
  CHECK(ar.Serialize(RSB(colPosV2), colPos.optional[2], 1, 0))
  CHECK(ar.Serialize(RSB(colPosV3), colPos.optional[3], 1, 0))

  return LSOK;
}

LSError SensorRow::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.SerializeDef("Vis", _info, 1))
  return LSOK;
}

float SensorList::Unimportance(EntityAIFull *from, EntityAI *to, float &defValue)
{
  PROFILE_SCOPE_DETAIL_EX(visUn,vis);
  defValue = 1;
  if( !from ) return 1e10;
  AIBrain *fromBrain = from->EffectiveObserverUnit();
  Assert( fromBrain );
  if( !fromBrain ) return 1e10;

  // same center units are ignored - they are always seen and known
  AIBrain *toBrain = to->EffectiveObserverUnit();
  // calculate max. allowed dirty
  float unimportance=to->VisibleSize(to->FutureVisualState());
  saturateMax(unimportance,2);
  // sometimes we know checking is not necessary
  if (toBrain)
  {
  	//TODO: Sides are not used anymore. Remove all relevant code
/*    if (fromBrain->GetSide() == toBrain->GetSide())
    {
      if (fromBrain->GetVehicle() != GWorld->CameraOn())
      {
        // only player needs to know the visibility to friendlies
        return 1e10;
      }
      // for camera..friendly we need frequent updates because of sound and peripheral vision

      // friendly are less important than enemy 
      unimportance *= 2;
    }
    else if (fromBrain->GetSide() == TCivilian && !from->HasSomeWeapons())
    {
      // we do not care much what unarmed civilian see or do not see
      unimportance *= 10;
    }
    else if (toBrain->GetSide() == TCivilian && !to->HasSomeWeapons())
    {
      // we do not care if we see unarmed civilian or not
      unimportance *= 10;
    }*/
  }
  else if (to->GetType()->GetLaserTarget() || to->GetType()->GetNvTarget())
  { // laser targets are important, as long as they are the ones we are interested in
    if (!fromBrain->IsEnemy(to->GetTargetSide()))
    {
      // friendly laser targets are very little important
      // we are never targeting them
      unimportance = 100;
    }
    else
    {
      // for purpose of this calculation laser target needs to be considered small
      // even a small movement is important and should trigger recalculation
      unimportance = 2;
    }
  }
  return unimportance;
}

SensorList::SensorList()
 : _lastUpdateCellRow(SensorRowID(0)),_lastUpdateCellCol(SensorRowID(0))
{
}

SensorList::~SensorList()
{
}

SensorRowID SensorList::FindRowID( Person *veh )
{
  for( int i=0; i<_rows.Size(); i++ )
  {
    if( _rows[i]._vehicle==veh ) return SensorRowID(i);
  }
  return SensorRowID(-1);
}
SensorColID SensorList::FindColID( EntityAI *veh )
{
  for( int i=0; i<_cols.Size(); i++ )
  {
    if( _cols[i]._vehicle==veh ) return SensorColID(i);
  }
  return SensorColID(-1);
}

SensorRowID SensorList::AddRow( Person *veh )
{
  //DoAssert(veh->GetType()->ScanTargets() || veh==GWorld->CameraOn());
  if  (!veh->Brain() || veh->Brain()->GetLifeState()==LifeStateDead)
  {
    LogF("Add dead sensor %s",(const char *)veh->GetDebugName());
    // error:
    return (SensorRowID)-1;
  }
  if (veh->IsDamageDestroyed())
  {
    LogF("Add destroyed sensor %s",(const char *)veh->GetDebugName());
    return (SensorRowID)-1;
  }
  
  // search for some free slot
  SensorRowID rowID = FindRowID(NULL);
  if (rowID < 0) rowID = SensorRowID(_rows.Add());
  else if (_rows[rowID]._info.Size() > 0)
  {
    // row cleaned up because of deleted vehicle, make sure no duplicate _cells will appear
    DeleteRow(rowID);
  }
  _rows[rowID].Init(veh,_cols.Size());

  veh->SetSensorRowID(rowID);
  return rowID;
}

SensorColID SensorList::AddCol( EntityAI *veh )
{
  DoAssert(veh->GetType()->ScannedAsTarget());
  if (!veh->IsInLandscape())
  {
    Fail("BUG: Adding in-vehicle target.");
  }
  SensorColID colID=FindColID(NULL);
  if( colID<0 )
  {
    colID=SensorColID(_cols.Add());
    // add this ID to all rows
    for( int row=0; row<_rows.Size(); row++ ) if( _rows[row]._vehicle!=NULL ) 
    {
      _rows[row]._info.Access(colID);
    }

  }
  for( int row=0; row<_rows.Size(); row++ ) if( _rows[row]._vehicle!=NULL ) 
  {
    _rows[row]._info[colID].Init();
  }
  _cols[colID]._vehicle=veh;
  veh->SetSensorColID(colID);
  _cols[colID]._unimportanceSrc.Init(NULL); // force update
  return colID;
}
void SensorList::DeleteRow( SensorRowID i )
{
  Person *veh = _rows[i]._vehicle;
  if (veh) //FIX: after load veh was sometimes NULL
    veh->SetSensorRowID(SensorRowID(-1)); // no longer used as sensor
  _rows[i].Free(); // delete
}


void SensorList::DeleteRow( Person *veh )
{
  SensorRowID index = veh->GetSensorRowID();
  if (index < 0) return;
  #if !_RELEASE
    Log
    (
      "Removed sensor row %s",
      (const char *)veh->GetDebugName()
    );
  #endif
  Assert( index==FindRowID(veh) );
  DeleteRow(index);
}

void SensorList::DeleteCol( SensorColID i )
{
  EntityAI *veh = _cols[i]._vehicle;
  veh->SetSensorColID(SensorColID(-1)); // no longer used as sensor
  _cols[i]._vehicle = NULL; // delete
}

void SensorList::DeleteCol( EntityAI *veh )
{
  SensorColID index=veh->GetSensorColID();
  if( index<0 ) return;
  #if !_RELEASE
    Log
    (
      "Removed sensor column %s",
      (const char *)veh->GetDebugName()
    );
  #endif
  Assert( index==FindColID(veh) );
  DeleteCol(index);
}

void SensorList::UpdateUnimportanceRow(SensorRowID id)
{
  SensorRow &row = _rows[id];
  for (int i=0; i<row._info.Size(); i++)
  {
    const SensorCol &col = _cols[i];
    SensorUpdate &info = row._info[i];
    if (col._vehicle)
    {
      info.unimportance = Unimportance(row._vehicle,col._vehicle,info.defValue);
    }
  }
}
void SensorList::UpdateUnimportanceCol(SensorColID id)
{
  const SensorCol &col = _cols[id];
  for (int i=0; i<_rows.Size(); i++)
  {
    SensorRow &row = _rows[i];
    if (row._info.Size()<=id)
    {
      DoAssert(row._info.Size()==0);
      DoAssert(!row._vehicle);
      continue;
    }
    SensorUpdate &info = row._info[id];
    if (row._vehicle)
    {
      info.unimportance = Unimportance(row._vehicle,col._vehicle,info.defValue);
    }
  }

}

void SensorList::UpdateUnimportanceRow(Person *veh)
{
  SensorRowID index = veh->GetSensorRowID();
  if (index < 0) return;
  Assert( index==FindRowID(veh) );
  UpdateUnimportanceRow(index);
}
void SensorList::UpdateUnimportanceCol(EntityAI *veh)
{
  SensorColID index=veh->GetSensorColID();
  if( index<0 ) return;
  Assert( index==FindColID(veh) );
  UpdateUnimportanceCol(index);
}


void SensorList::Compact()
{
  // delete any NULL entries from the end of lists to compact the list
  while (_cols.Size()>0 && _cols.Last()._vehicle.IsNull())
  {
    _cols.Delete(_cols.Size()-1);
}
  while (_rows.Size()>0 && _rows.Last()._vehicle.IsNull())
  {
    _rows.Delete(_rows.Size()-1);
  }
}


Time CellCoord::NextUpdate() const
{
  Time nextUpdate = GWorld->GetSensorList()->TimeToNextUpdate(from,to);
  return nextUpdate;
}

Time SensorList::TimeToNextUpdate( Person *fromDriver, EntityAI *to ) const
{
  AIBrain *fromUnit = fromDriver->EffectiveObserverUnit();
  if (!fromUnit) return TIME_MAX;
  EntityAIFull *from = fromUnit->GetVehicle();
  if (!from) return TIME_MAX;
  const SensorRow &row = _rows[fromDriver->GetSensorRowID()];
  //const SensorCol &col = _cols[to->GetSensorColID()];
  const SensorUpdate &info = row._info[to->GetSensorColID()];

  float dist2 = from->FutureVisualState().Position().Distance2(to->FutureVisualState().Position());

  float unimportance = info.unimportance;
  float toMoved=to->DistanceFrom(info.colPos);
  float fromMoved=from->DistanceFrom(info.rowPos);
  // when out of common engagement range, we may afford much slower reaction time
  if( dist2>Square(200.0f) )
  {
    unimportance *= 2.0f;
    if( dist2>Square(600.0f) ) unimportance*=2.0f; // cannot be targeted
  }
  else if (dist2<Square(50.0f))
  { // for near enemy we need a lot faster reaction
    unimportance *= 0.25f;
  }

  // sophisticated check (4D: 3D + time)
  
  Time nextUpdate = info.time + floatMax(unimportance-toMoved-fromMoved,0)/0.1f;
  return nextUpdate;
}

int SensorList::UpdateCell( SensorRowID r, SensorColID c, bool force )
{
  PROFILE_SCOPE_DETAIL_EX(visUC,vis);

  #if DIAGS>=20
    SectionTimeHandle start = StartSectionTime();
  #endif
  SensorRow &row=_rows[r];
  Person *fromDriver=row._vehicle;
  if( !fromDriver ) return 0;
  SensorCol &col=_cols[c];
  EntityAI *to=col._vehicle;
  if( !to ) return 0;
  // 
  AIBrain *fromUnit = fromDriver->EffectiveObserverUnit();
  if (!fromUnit) return 0;

  EntityAIFull *from = fromUnit->GetVehicle();

#if defined SERVER_CORPSE_PERFORMANCE_PATCH
  //@NOTE: not verified that it is necessary
  //if (to->IsDamageDestroyed())
  //  return 0;
  //if (from->IsDamageDestroyed())
  //  return 0;
#endif

  // update single
  SensorUpdate &info=row._info[c];
  if( from==to )
  {
    info.SetVisibility1();
    info.SetLastVisible(Glob.time);
    from->GetSensorInfo(info.rowPos);
    to->GetSensorInfo(info.colPos);
    info.time=Glob.time;
    return 0;
  }
  float dist2 = from->FutureVisualState().Position().Distance2(to->FutureVisualState().Position());
  float irRange = 0;
  if (
    to->GetType()->GetLaserTarget() && from->GetType()->GetLaserScanner() ||
    to->GetType()->GetIRTarget() || to->GetType()->GetNvTarget()
  )
  {
    irRange = from->GetType()->GetIRScanRange();
  }
  float sensorLimit=floatMax(irRange,TACTICAL_VISIBILITY);
  if( dist2<Square(sensorLimit) )
  {
    float defValue=info.defValue;
    float unimportance = info.unimportance;
    #if 0 // _DEBUG // triggers after camera is switched
    {
      float defValueT = 0;
      float unimportanceT=Unimportance(from,to,defValueT);
      Assert(fabs(info.unimportance-unimportanceT)<1e-3f);
      Assert(info.defValue==defValueT);
    }
    #endif


    // unimportant: assume default value
    if( unimportance>1e5 )
    {
      info.SetVisibility(defValue);
      from->GetSensorInfo(info.rowPos);
      to->GetSensorInfo(info.colPos);
      info.time=Glob.time-30;
      if (defValue>=0.02f)
      {
        info.SetLastVisible(Glob.time);
      }
      // set time in past 
      // to guarantee quick evaluation when unimportance would drop down
      // (like in case of death)
      return 0;
    }

    CellCoord cell(fromDriver,to);
    Time nextUpdate = cell.NextUpdate();
    
    // calculate dirty value
    float toMoved=to->DistanceFrom(info.colPos);
    float fromMoved=from->DistanceFrom(info.rowPos);
    float timeMoved=Glob.time.Diff(info.time);
    // when out of common engagement range, we may afford much slower reaction time
    if( dist2>Square(200.0f) )
    {
      unimportance *= 2.0f;
      if( dist2>Square(600.0f) ) unimportance*=2.0f; // cannot be targeted
    }
    else if (dist2<Square(50.0f))
    { // for near enemy we need a lot faster reaction
      unimportance *= 0.25f;
    }

    // sophisticated check (4D: 3D + time)
    float dirty = 0;
    if (!force)
    {
      dirty=toMoved+fromMoved+timeMoved*0.1;
      if( dirty<unimportance )
      {
        return 0;
      }
      Assert(Glob.time>=nextUpdate);
    }

    PROFILE_SCOPE_DETAIL_EX(visUD,vis);
    #if _PROFILE
    if (PROFILE_SCOPE_NAME(visUD).IsActive())
    {
      PROFILE_SCOPE_NAME(visUD).AddMoreInfo(Format("%s-%s",cc_cast(from->GetDebugName()),cc_cast(to->GetDebugName())));
    }
    #endif

    // vehicle/person equipped with Ti optics see through smokes - ignore cloudlets visibility calculation
    bool cloudlets = !from->HasTiOptics();      
    
    // check from/to camera/aiming positions
    float visibility=GLandscape->Visible(from, to, 1.0f, ObjIntersectView, cloudlets);

    /*
    float aboveFrom = GLandscape->AboveSurface(from->CameraPosition());
    float aboveTo = GLandscape->AboveSurface(to->AimingPosition());
    LogF("  from %s: above %.1f",(const char *)from->GetDebugName(),aboveFrom);
    LogF("  to   %s: above %.1f",(const char *)to->GetDebugName(),aboveTo);
    */

    #if DIAGS>=20
      float duration = GetSectionTime(start);
      LogF
      (
        "Cell update %s to %s, dist %.1f %.2f, dirty %.1f>%.1f%s, age %.2f, duration %.2f",
        (const char *)from->GetDebugName(),(const char *)to->GetDebugName(),
        sqrt(dist2),visibility,
        dirty,unimportance,force ? " Forced" : "",Glob.time-info.time,duration*1000
      );
    #endif
    
    info.SetVisibility(visibility);
    from->GetSensorInfo(info.rowPos);
    to->GetSensorInfo(info.colPos);
    info.time=Glob.time;
    if (visibility>0.02f)
    {
      info.SetLastVisible(Glob.time);
    }
    return 1;
  }
  else
  {
    info.SetVisibility0();
    from->GetSensorInfo(info.rowPos);
    to->GetSensorInfo(info.colPos);
    info.time=Glob.time;
    return 0;
  }
}

#define DIAG_UPDATE_CELL 0

int SensorList::UpdateRow( SensorRowID id )
{
  int countUpdates=0;
  SensorRow &row=_rows[id];
  Person *fromDriver=row._vehicle;
  Assert( fromDriver );
  AIBrain *fromUnit = fromDriver->EffectiveObserverUnit();
  Assert(fromUnit==fromDriver->Brain()); // commander of a person is always its brain
  //EntityAI *from = fromUnit->GetVehicle();
  if (!fromUnit)
  {
    ErrF("No unit sensor row %s",(const char *)fromDriver->GetDebugName());
    DeleteRow(id); // delete - it will not be needed for a long time
    return 0;
  }
  if ( fromUnit->IsInCargo())
  {
    // TODO: BUG: assert failed
    //Fail("Cargo unit sensor present");
    // unit loaded in cargo
    #if DIAGS>=30
      LogF("Removed cargo sensor row %s",(const char *)fromDriver->GetDebugName());
    #endif
    DeleteRow(id); // delete - it will not be needed for a long time
    return 0;
  }

#if DIAG_UPDATE_CELL
int timeTotal = 0, timeMax = 0;
#endif
  for( int i=0; i<_cols.Size(); i++ )
  {
#if DIAG_UPDATE_CELL
__int64 start = ProfileTime();
#endif
    countUpdates+=UpdateCell(id,SensorColID(i));
#if DIAG_UPDATE_CELL
__int64 spent = ProfileTime() - start;
int time = ((10 * spent) >> PROF_ACQ_SCALE_LOG) / ProfileScale();
timeTotal += time;
saturateMax(timeMax, time);
#endif
  }
#if DIAG_UPDATE_CELL
LogF("SensorList::UpdateRow columns %d, total %d us, max %d us", _cols.Size(), timeTotal, timeMax);
#endif

  #if DIAGS>=10
    if( countUpdates>0 )
    {
      LogF
      (
        "Update sensor row %d,%s %d updates",
        id,(const char *)fromUnit->GetDebugName(),countUpdates
      );
    }
  #endif
  return countUpdates;
}

int SensorList::UpdateCol( SensorColID id )
{
  //LogF("UpdateCol %d",id);
  // last column pos. change is col._posUpdatedTime
  int countUpdates=0;
  //SensorCol &col=_cols[id];
  //EntityAI *to=;
  Assert( _cols[id]._vehicle );
  //bool someDirty=false;

#if DIAG_UPDATE_CELL
int timeTotal = 0, timeMax = 0;
#endif
  for( int i=0; i<_rows.Size(); i++ )
  {
#if DIAG_UPDATE_CELL
__int64 start = ProfileTime();
#endif
    countUpdates+=UpdateCell(SensorRowID(i),id);
#if DIAG_UPDATE_CELL
__int64 spent = ProfileTime() - start;
int time = ((10 * spent) >> PROF_ACQ_SCALE_LOG) / ProfileScale();
timeTotal += time;
saturateMax(timeMax, time);
#endif
  }
#if DIAG_UPDATE_CELL
LogF("SensorList::UpdateCol rows %d, total %d us, max %d us", _rows.Size(), timeTotal, timeMax);
#endif

  #if DIAGS>=10
    if( countUpdates>0 )
    {
      LogF
      (
        "Update target col %d,%s  %d updates",
        id,cc_cast(_cols[id]._vehicle->GetDebugName()),
        countUpdates
      );
    }
  #endif
  return countUpdates;
}

static bool UseDoUpdate = true;

LSError SensorList::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("Cols", _cols, 1))
  CHECK(ar.Serialize("Rows", _rows, 1))
  CHECK(ar.Serialize("lastUpdateCellRow", _lastUpdateCellRow, 1, SensorRowID(0)))
  CHECK(ar.Serialize("lastUpdateCellCol", _lastUpdateCellCol, 1, SensorColID(0)))

  return LSOK;
}


void SensorList::ProcessRowToUpdate(SensorRowID r)
{
  SensorRow &row = _rows[r];
  for (int c=0; c<row._info.Size(); c++)
  {
    if (!_cols[c]._vehicle) continue;

#if defined SERVER_CORPSE_PERFORMANCE_PATCH
    if (_cols[c]._vehicle->IsDamageDestroyed())
      continue;
#endif

    SensorUpdate &info = row._info[c];
    if (info.toUpdate) continue;
    if (TimeToNextUpdate(row._vehicle,_cols[c]._vehicle)<=Glob.time)
    {
      info.toUpdate = true;
      _toUpdate.Add(CellCoord(row._vehicle,_cols[c]._vehicle));
    }
  }
}

void SensorList::ProcessColToUpdate(SensorColID c)
{
  for (int r=0; r<_rows.Size(); r++)
  {
    SensorRow &row = _rows[r];
    if (!row._vehicle) continue;
    SensorUpdate &info = row._info[c];
    if (info.toUpdate) continue;
    if (TimeToNextUpdate(row._vehicle,_cols[c]._vehicle)<=Glob.time)
    {
      info.toUpdate = true;
      _toUpdate.Add(CellCoord(row._vehicle,_cols[c]._vehicle));
    }
  }
}

void SensorList::CheckPos()
{
  PROFILE_SCOPE_EX(visRP,*);
  const float moveTHold = 0.5f; // how significant position change do we require to process the column/row immediately?
  // verify no targets are inside the vehicle
  // if they are, their position is invalid
  for (int i=0; i<_cols.Size(); i++)
  {
    SensorCol &col = _cols[i];
    EntityAI *veh = col._vehicle;
    if (!veh) continue;
    if(veh->IsSimulationDisabled())
      continue;
    if (UseDoUpdate)
    {
      Vector3Val pos = veh->FutureVisualState().Position();
      float moved2 = col._lastPos.Distance2(pos);
      if (moved2>Square(moveTHold))
      {
        col._lastPos = pos;
        ProcessColToUpdate(SensorColID(i));
        // recompute 
      }
    }
    // vehicle may be person that is inside of another vehicle
    // TODO: make function IsInLandscape better
    if (!veh->IsInLandscape())
    {
      // TODO: report this situation
      LogF("Removing in-vehicle target %s",(const char *)veh->GetDebugName());
      DeleteCol(SensorColID(i));
      // target should be removed when getting in the vehicle
    }
  }
  // remove dead sensors
  int rowsInOneStep = (_rows.Size()+9)/10;
  if (UseDoUpdate)
  {
    _lastUpdateCellRow += rowsInOneStep;
    if (_lastUpdateCellRow>=_rows.Size())
    {
      _lastUpdateCellRow = SensorRowID(0);
    }
  }

  for (int i=0; i<_rows.Size(); i++)
  {
    SensorRow &row = _rows[i];
    Person *veh = row._vehicle;
    if (!veh) continue;
    if (veh->IsDamageDestroyed() && !veh->EffectiveObserverUnit())
    {
      DeleteRow(SensorRowID(i));
      LogF("Removing dead sensor %s",(const char *)veh->GetDebugName());
    }
    else
    {
      if (UseDoUpdate)
      {
        AIBrain *fromUnit = veh->EffectiveObserverUnit();
        if (!fromUnit) continue;
        EntityAIFull *from = fromUnit->GetVehicle();
        if (!from) continue;
        if(from->IsSimulationDisabled())
          continue;
        Vector3Val pos = from->FutureVisualState().Position();
        float moved2 = row._lastPos.Distance2(pos);
        if (moved2>Square(moveTHold))
        {
          row._lastPos = pos;
          ProcessRowToUpdate(SensorRowID(i));
        }
        else if (i>=_lastUpdateCellRow && i<_lastUpdateCellRow+rowsInOneStep)
        { // periodical update to make sure timing out is considered as well
          row._lastPos = pos;
          ProcessRowToUpdate(SensorRowID(i));
        }
      }
    }
    
  }
}

void SensorList::RecalcUnimportance()
{
  PROFILE_SCOPE_EX(visRU,*);
  for (int r=0; r<_rows.Size(); r++)
  {
    if (!_rows[r]._vehicle) continue;

    SensorRow::UnimportanceSrcFrom src;
    src.Init(_rows[r]._vehicle);
    if (!(src==_rows[r]._unimportanceSrc))
    {
      _rows[r]._unimportanceSrc = src;
      if (_rows[r]._vehicle) UpdateUnimportanceRow(SensorRowID(r));
    }
  }
  for (int c=0; c<_cols.Size(); c++)
  {
    if (!_cols[c]._vehicle)
      continue;
#if defined SERVER_CORPSE_PERFORMANCE_PATCH
    if (_cols[c]._vehicle->IsDamageDestroyed())
      continue;
#endif

    SensorCol::UnimportanceSrcTo src;
    src.Init(_cols[c]._vehicle);
    if (!(src==_cols[c]._unimportanceSrc))
    {
      _cols[c]._unimportanceSrc = src;
      if (_cols[c]._vehicle) UpdateUnimportanceCol(SensorColID(c));
    }

  }
}

int SensorList::CheckAndUpdateCell(SensorRowID r, SensorColID c)
{
  if (r >= _rows.Size()) return 0;
  SensorRow &row = _rows[r];
  if (c >= _cols.Size()) return 0;
  SensorCol &col = _cols[c];

  // check the column
  if (col._vehicle == NULL) return 0;

  // check the row
  Person *from = row._vehicle;
  if (!from) return 0;
  if(from->IsSimulationDisabled())
    return 0;

  AIBrain *fromUnit = from->EffectiveObserverUnit();
  Assert(fromUnit==from->Brain()); // commander of a person is always its brain
  if (!fromUnit)
  {
    ErrF("No unit sensor row %s", (const char *)from->GetDebugName());
    DeleteRow(r); // delete - it will not be needed for a long time
    return 0;
  }
  if ( fromUnit->IsInCargo())
  {
    // TODO: BUG: assert failed
    if (fromUnit->GetVehicle()->EffectiveObserverUnit()==fromUnit)
    {
      // thousands of the same messages in rpt, allow 1 message per 60
      static int MsgCounter = 0;
      static Time LastFailMsgTimestamp(0);

      if (Glob.time > LastFailMsgTimestamp)
      {
        RptF("Observer %s in cargo of %s; message was repeated in last 60 sec: %d",cc_cast(fromUnit->GetDebugName()),cc_cast(fromUnit->GetVehicle()->GetDebugName()),MsgCounter);
        LastFailMsgTimestamp = Glob.time + 60.f;
        MsgCounter = 0;
    }
    else
        MsgCounter++;
    }
    else
    {
    // unit loaded in cargo
  #if DIAGS>=30
    LogF("Removed cargo sensor row %s",(const char *)fromDriver->GetDebugName());
  #endif
      // it is a commander - do not delete, important for visibility testing
    DeleteRow(r); // delete - it will not be needed for a long time
    return 0;
  }
  }

  return UpdateCell(r, c);
}

/*!
\patch 1.30 Date 11/06/2001 by Ondra.
- Fixed: Random crash in visibility matrix update.
*/

int SensorList::SmartUpdateAll()
{
  #if DIAGS>=10
    SectionTimeHandle start = StartSectionTime();
  #endif
#if DIAG_UPDATE_CELL
  __int64 start = ProfileTime();
  LogF("SmartUpdateAll started");
#endif
  int sum = 0;
  PROFILE_SCOPE_GRF_EX(visUA,*,SCOPE_COLOR_RED);
  
  if (PROFILE_SCOPE_NAME(visUA).IsActive())
  {
    PROFILE_SCOPE_NAME(visUA).AddMoreInfo(Format("~~ %.2f ms, %dx%d",GTimeManager.TimeLeft(TCVisUpdate)*1000,_rows.Size(),_cols.Size()));
  }

  {
    #if _PROFILE
    int toUpdateStart = _toUpdate.Size();
    #endif
    TimeManagerScope time(TCVisUpdate);
    RecalcUnimportance();
    CheckPos();

    if (UseDoUpdate)
    {
      // use time manager scope external to the loop, as a lot of time is spent in loop infrastructure
      PROFILE_SCOPE_DETAIL_EX(visUU,vis);

      // process a few items to to be updated
      const int timeLeftEach = 200;
      int timeLeft = 0;
      int i;
      for (i=0; i<_toUpdate.Size(); i++)
      {
        const CellCoord &cc = _toUpdate[i];
        // entities may have been destroyed or removed from sensor list meanwhile
        if(!cc.from || !cc.to)
          continue;
        if(cc.from->GetSensorRowID()<0 || cc.to->GetSensorColID()<0)
          continue;
        SensorRow &row = _rows[cc.from->GetSensorRowID()];
        if (row._info.Size()<=cc.to->GetSensorColID())
        {
          DoAssert(row._info.Size()==0);
          DoAssert(row._vehicle==NULL);
          continue;
        }
        SensorUpdate &info = row._info[cc.to->GetSensorColID()];
        if (timeLeft--<=0 && (timeLeft=timeLeftEach,GTimeManager.TimeLeft(time)<0))
          break;
        info.toUpdate = false; 
        // update one cell
        // caution: CheckAndUpdateCell may destroy the cell (DeleteRow may be called)
        int checked = CheckAndUpdateCell(cc.from->GetSensorRowID(), cc.to->GetSensorColID());
        if (checked) timeLeft = 0; // force TimeLeft to be called in next iteration
        sum += checked;
      }
      // delete all items we have processes
      #if _PROFILE
      if (PROFILE_SCOPE_NAME(visUU).IsActive())
      {
        PROFILE_SCOPE_NAME(visUU).AddMoreInfo(Format("%d +%d, -%d",toUpdateStart,_toUpdate.Size()-toUpdateStart,i));
      }
      //DIAG_MESSAGE(100,"%4d +%4d, -%4d (%4d)",toUpdateStart,_toUpdate.Size()-toUpdateStart,i,sum);
      //ADD_COUNTER(visTU,sum);
      #endif

      _toUpdate.DeleteAt(0,i);
    }
    else
    {
      // use time manager scope external to the loop, as a lot of time is spent in loop infrastructure
      TimeManagerScope time(TCVisUpdate);

      Compact();

      int maxCellChecks = _rows.Size()*_cols.Size();
      // items for which intersection is not computed are cheap, avoid calling TimeLeft for each of them
      const int timeLeftEach = 200;
      int timeLeft = timeLeftEach;
      for(;;)
      {
        if (timeLeft--<=0 && (timeLeft=timeLeftEach,GTimeManager.TimeLeft(time)<0))
          break;
        if (maxCellChecks <= 0)
          break;
        if (_lastUpdateCellCol>=_cols.Size())
        { // wrap around a line end
          _lastUpdateCellCol = SensorColID(0);
          _lastUpdateCellRow++;
        }
        if (_lastUpdateCellRow>=_rows.Size())
        { // wrap around an end
          _lastUpdateCellRow = SensorRowID(0);
          _lastUpdateCellCol = SensorColID(0);
        }
        // update one cell
        int checked = CheckAndUpdateCell(_lastUpdateCellRow, _lastUpdateCellCol);
        if (checked) timeLeft = 0; // force TimeLeft to be called in next iteration
        sum += checked;
        // proceed to the next item
        SensorColID i = SensorColID(_lastUpdateCellCol+1);
        // quick skip any empty cells, to prevent paying loop and function call overhead for them
        while (i<_cols.Size() && _cols[i]._vehicle.IsNull() ) i++;
        int advanced = i-_lastUpdateCellCol;
        _lastUpdateCellCol = i;
        // avoid testing each cell more then once in one frame
        maxCellChecks -= advanced;
      }
      #if _PROFILE
      // DIAG_MESSAGE(100,"%(%4d)",sum);
      // ADD_COUNTER(visTU,sum);
      #endif
    }

    #if DIAGS>=10
    if( sum>0 )
    {
      float duration = GetSectionTime(start);
      LogF("%6.2f: Visibility: %d updates, %.2f ms",Glob.time-Time(0),sum,duration*1000);
    }
    #endif
    //ADD_COUNTER(sCell,sum);
  }
  GTimeManager.Finished(TCVisUpdate);
#if DIAG_UPDATE_CELL
  __int64 spent = ProfileTime() - start;
  int time = ((10 * spent) >> PROF_ACQ_SCALE_LOG) / ProfileScale();
  LogF("SmartUpdateAll finished in %d us, %d cells updated", time, sum);
#endif
  return 0;
}

int SensorList::UpdateAll()
{	
  RecalcUnimportance();
  CheckPos();

  // reset all items to be updated
  for (int i=0; i<_toUpdate.Size(); i++)
  {
    const CellCoord &cc = _toUpdate[i];
    if (!cc.from || !cc.to) continue;
    if (cc.from->GetSensorRowID()<0 || cc.to->GetSensorColID()<0) continue;
    SensorRow &row = _rows[cc.from->GetSensorRowID()];
    SensorUpdate &info = row._info[cc.to->GetSensorColID()];
    info.toUpdate = false;
  }
  _toUpdate.Clear();


  int sum=0;
  int index;
  for( index=0; index<_rows.Size(); index++ ) if( _rows[index]._vehicle!=NULL )
  {
    sum+=UpdateRow(SensorRowID(index));
  }
  for( index=0; index<_cols.Size(); index++ ) if( _cols[index]._vehicle!=NULL )
  {
    sum+=UpdateCol(SensorColID(index));
  }
  #if DIAGS>=10
    LogF("Visibility: %d updates",sum);
  #endif
  //ADD_COUNTER(sCell,sum);
  return sum;
}


float SensorList::GetVisibility( Person *from, EntityAI *to, float maxAge ) const
{
  if( from->GetSensorRowID()<0 )
  {
    #if DIAGS>=30
      LogF("Added sensor row %s",(const char *)from->GetDebugName());
    #endif
    SensorRowID id = const_cast<SensorList *>(this)->AddRow(from);
    if (id<0) return 0;
  }
  if( to->GetSensorColID()<0 )
  {
    #if DIAGS>=30
      LogF("Added sensor col %s",(const char *)to->GetDebugName());
    #endif
    SensorColID id = const_cast<SensorList *>(this)->AddCol(to);
    if (id<0) return 0;
  }
  if (from->GetSensorRowID()>=_rows.Size())
  {
    ErrF
    (
      "%s: bad sensor row %d (>=%d)",
      (const char *)from->GetDebugName(),from->GetSensorRowID(),_rows.Size()
    );
    return 0;
  }
  const SensorRow &row=_rows[from->GetSensorRowID()];	
  if (to->GetSensorColID()>=row._info.Size())
  {
    ErrF
    (
      "%s: bad sensor col %d (>=%d)",
      (const char *)to->GetDebugName(),to->GetSensorColID(),row._info.Size()
    );
    return 0;
  }
  const SensorUpdate &info = row._info[to->GetSensorColID()];
  if (maxAge<FLT_MAX)
  {
    if (info.time==TIME_MIN || Glob.time-info.time>maxAge)
    {
      // update is needed - force it
      const_cast<SensorList *>(this)->UpdateCell(
        from->GetSensorRowID(),to->GetSensorColID(),true
      );
    }
    
  }
  return info.GetVisibility();
}


Time SensorUpdate::GetLastVisibilityTime() const
{
  if(lastVisible<=TIMESEC_MIN)
  {
    return TIME_MIN;
  }
  // optimistic: if there was no check done yet, assume target is visible
  // otherwise check lastVisible value
  if (time<TIME_MIN+1 || Time(lastVisible)>=time-1)
  {
    return Glob.time;
  }
  return lastVisible;
}

Time SensorList::GetVisibilityTime( Person *from, EntityAI *to ) const
{
  // this function is used to perform safety check on AI targets
  // it is therefore optimistic and if real value is not know, it assumes
  // target is visible now
  if( from->GetSensorRowID()<0 ) return Glob.time;
  if( to->GetSensorColID()<0 ) return Glob.time;
  if (from->GetSensorRowID()>=_rows.Size()) return Glob.time;
  const SensorRow &row=_rows[from->GetSensorRowID()];	
  if (to->GetSensorColID()>=row._info.Size()) return Glob.time;
  const SensorUpdate &upd = row._info[to->GetSensorColID()];
  // if time of last visibility is same as time of last check
  // return it is visible now
  return upd.GetLastVisibilityTime();
}

RString SensorList::DiagText( Person *from, EntityAI *to ) const
{
  char buf[256];
  if( from->GetSensorRowID()<0 )
  {
    return "No row";
  }
  if( to->GetSensorColID()<0 )
  {
    return "No col";
  }
  const SensorRow &row=_rows[from->GetSensorRowID()];	
  //return row._vis[to->GetSensorColID()]*(1.0/255);
  const SensorUpdate &info=row._info[to->GetSensorColID()];
  sprintf(buf,"Cell %d,%d: ",from->GetSensorRowID(),to->GetSensorColID());
  strcat(buf,info.DiagText());
  return buf;
}

bool SensorList::CheckStructure() const
{
  return true;
}


