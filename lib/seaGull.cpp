// Poseidon - vehicle class
// (C) 1997, Suma

#define DEBUG_SEAGULL 0
#define DEBUG_SWITCH_CAMERA 0

#if DEBUG_SEAGULL || DEBUG_SWITCH_CAMERA
volatile int debugID=-1;
#endif
#if DEBUG_SEAGULL
volatile bool debugBreaking=false;
volatile bool debugLanding=false;
volatile float posCorrectDebug=0.01f;
volatile float debugSetMaxSpeed=1.0f; 
volatile float debugSetAcceleration=1.0f; 
volatile float debugMultFriction=1.0f; 
#endif

#include "wpch.hpp"

#include "seaGull.hpp"
#include "camera.hpp"
#include "landscape.hpp"
#include "keyInput.hpp"
#include "scene.hpp"
#include "world.hpp"
#include <Es/Common/delegate.hpp>
#include <El/Common/randomGen.hpp>
#include <El/Common/perfProf.hpp>
#include "diagModes.hpp"

#include "AI/ai.hpp"
#include "fsmEntity.hpp"

#include "Network/network.hpp"

SeaGullType::SeaGullType(ParamEntryPar param)
:base(param)
{
  _scopeLevel = 1;
}

SeaGullType::~SeaGullType()
{
}

void SeaGullType::Load(ParamEntryPar param, const char *shapeName)
{
  base::Load(param,shapeName);
  _minHeight = param>>"minHeight";
  _avgHeight = param>>"avgHeight";
  _maxHeight = param>>"maxHeight";
  _maxSpeed = param>>"maxSpeed";
  _minSpeed = param>>"minSpeed";
  _straightDistance = param>>"straightDistance";
  _acceleration = param>>"acceleration";
  _turning = param>>"turning";
  GetValue(_fricCoef2,param>>"airFriction2");
  GetValue(_fricCoef1,param>>"airFriction1");
  GetValue(_fricCoef0,param>>"airFriction0");

  // the better acceleration, the better precision
  // check distance from max speed to zero
  
  //float timeToStop = _maxSpeed/_acceleration;
  //float distToStop = _maxSpeed*0.5*timeToStop;
  float distToStop = Square(_maxSpeed)*0.5/_acceleration;
  _precision = distToStop*0.035f;
#if DEBUG_SEAGULL
  LogF("*** Precision of %s is %f (maxSpeed=%f, acceleration=%f)", shapeName, _precision, _maxSpeed, _acceleration);
#endif
  
  _canBeShot = param>>"canBeShot";
  
  GetValue(_singSound, param>>"singSound");
  GetValue(_flySound, param>>"flySound");

  ConstParamEntryPtr entry = param.FindEntry("Moves");
  if (entry)
  {
    RString moves = *entry;
    if (moves.GetLength() > 0) SetHardwareAnimation(param);
  }
}

void SeaGullType::InitShape()
{
  base::InitShape();

  ConstParamEntryPtr entry = _par->FindEntry("Moves");
  if (entry)
  {
    RString moves = *entry;
    if (moves.GetLength() > 0) 
    {
      // create a new skeleton
      _movesType = MovesTypesBase.New(MovesTypeName(moves));
      _shape->SkeletonAssigned(_movesType->GetSkeleton());
    }
  }
}

void SeaGullType::DeinitShape()
{
  if (_movesType)
  {
    _shape->SkeletonUnassigned(_movesType->GetSkeleton());
    _movesType = NULL;
  }

  base::DeinitShape();
}


Object* SeaGullType::CreateObject(bool unused) const
{ 
  return new SeaGullAuto(this);
}


FSMEntityConditionFunc *SeaGullType::CreateEntityConditionFunc(RString name)
{
  SEAGULL_FSM_CONDITIONS(REGISTER_ENTITY_FSM_CONDITION)
  return base::CreateEntityConditionFunc(name);
}

FSMEntityActionFunc *SeaGullType::CreateEntityActionInFunc(RString name)
{
  SEAGULL_FSM_ACTIONS(REGISTER_ENTITY_FSM_ACTION_IN)
  return base::CreateEntityActionInFunc(name);
}
FSMEntityActionFunc *SeaGullType::CreateEntityActionOutFunc(RString name)
{
  SEAGULL_FSM_ACTIONS(REGISTER_ENTITY_FSM_ACTION_OUT)
  return base::CreateEntityActionOutFunc(name);
}

SeaGullVisualState::SeaGullVisualState(const SeaGullType & type)
:base(type)
{

}
void SeaGullVisualState::Interpolate(Object *obj, const SeaGullVisualState & t1state, float t, SeaGullVisualState& interpolatedResult) const
{
  base::Interpolate(obj,t1state,t,interpolatedResult);
  _moves.Interpolate(t1state._moves,t,interpolatedResult._moves);
}

//#if !_RELEASE
  #define ARROWS 0
//#endif

SeaGull::SeaGull(const EntityType *type)
:base(NULL,type,CreateObjectId()),
_mainRotor(1.5),_mainRotorWanted(1.5),

_rpm(1),_rpmWanted(1), // landing control

_cyclicForwardWanted(0),
_cyclicAsideWanted(0),
_wingDive(0),_wingDiveWanted(0),

_thrust(0),_thrustWanted(0),

_nextCreek(Glob.time+10),

_landContact(false),

_wingPhase(0),_wingSpeed(0.5),_wingBase(0),
_fsmFlags(0), _breakingFactor(0.0f),
_zoom()
{
  SetSimulationPrecision(1.0f/15,1.0f/15);  
  RandomizeSimulationTime();
  SetTimeOffset(1.0/15);

  _zoom.Init(this);

  // load directly from entity config?
  #if ENABLE_BIRD_TRACKING
    _cloudlets.Load(Pars >> "CfgCloudlets" >> "CloudletsDebugTrace");
  #endif
}

SeaGull::~SeaGull()
{
}

void SeaGull::Sound(bool inside, float deltaT)
{
  if (_ambient && GetTotalDamage()>=1)
    return;
  const VisualState &vs = RenderVisualState();
  //float vol = _speed.Size()/Type()->_maxSpeed;
  float vol = 1;
  float freq = 1;
  //inside |= GWorld->IsCameraInsideVeh();
  //static float InternalVolumeFactor = 0.1f;
  if (vol>0.01 )
  {
    const SoundPars &sound=Type()->_flySound;
    if( !_flySound && sound.name.GetLength()>0)
      _flySound=GSoundScene->OpenAndPlay(sound.name,NULL,false,vs.Position(),vs.Speed(), vol, freq, sound.distance);
    if( _flySound )
    {
      // check obstruction level
      freq*=sound.freq;
      vol*=sound.vol;
      //if (inside) vol *= Type()->_insideSoundCoef;

      float obstruction=1,occlusion=1;
      // handle the sound as an external one
      GWorld->CheckSoundObstruction(NULL,false,obstruction,occlusion);
      _flySound->SetObstruction(obstruction,occlusion,-1.0f/*deltaT*/);

      _flySound->SetVolume(vol,freq); // volume, frequency
      _flySound->SetPosition(vs.Position(),vs.Speed());
      _flySound->Set3D(!inside);
    }
  }
  else
    _flySound.Free();

  // singing / creeks

  if (_nextCreek<Glob.time)
  {
    _nextCreek=Glob.time+GRandGen.RandomValue()*30+3;

    const SoundPars &pars = Type()->_singSound;
    if (IsLocal() && pars.name.GetLength()>0)
    {
      const VisualState &vs = RenderVisualState();
      bool doSound = false;
      if( !_sound )
      { // load sound if necessary
        _sound=GSoundScene->Open(pars.name);
        doSound = true;
      }
      float volume = pars.vol*0.5;
      float freq = pars.freq;
      if( _sound )
      {
        float obstruction=1,occlusion=1;
        const VisualState &vs = RenderVisualState();
        // handle the sound as an external one
        GWorld->CheckSoundObstruction(NULL,false,obstruction,occlusion);
        _sound->SetObstruction(obstruction,occlusion,-1.0f);
        _sound->SetPosition(vs.Position(),vs.Speed());
        _sound->SetVolume(volume,freq);
        _sound->SetMaxDistance(pars.distance);
        _sound->Repeat(1);
        if (_sound->IsTerminated())
        {
          _sound->Restart();
          doSound = true;
        }
      }
      if (doSound && !_ambient)
        GetNetworkManager().PlaySound(pars.name, vs.Position(), vs.Speed(), volume, freq, _sound);
    }
  }
}

void SeaGull::UnloadSound()
{
  _sound.Free();
  _flySound.Free();
}

Vector3 SeaGull::ExternalCameraPosition(CameraType camType) const
{
  return Vector3(0,0.3,-3);
}

Matrix4 SeaGullAuto::InsideCamera(CameraType camType) const
{
  Vector3 headDir = Matrix3(MRotationY,_headYRot) * Matrix3(MRotationX,-_headXRot).Direction();
  Matrix3 rot(MDirection,headDir,VUp);
  Matrix4 transf;
  transf.SetOrientation(rot);
  transf.SetPosition(VZero);
  return transf;
  
}

Matrix4 SeaGullAuto::InternalCameraTransform(Matrix4 &base, CameraType camType) const
{
  base = RenderVisualState().Transform();
  return InsideCamera(camType);
}

Vector3 SeaGullAuto::GetCameraDirection(CameraType camType) const
{
  const VisualState &vs = RenderVisualState();
  Vector3 headDir = Matrix3(MRotationY,_headYRot) * Matrix3(MRotationX,-_headXRot).Direction();
  return vs.DirectionModelToWorld(headDir);
}

#if DEBUG_SEAGULL
volatile static float multfriction = 1.0f;
#endif
Vector3 SeaGull::BodyFriction(Vector3Val oSpeed)
{
//  Vector3 fricCoef2(25.0f,12.0f,2.5f);
//  Vector3 fricCoef1(1500.0f,700.0f,100.0f);
//  Vector3 fricCoef0(40.0f,20.0f,60.0f);
  Vector3 fricCoef2 = Type()->_fricCoef2;
  Vector3 fricCoef1 = Type()->_fricCoef1;
  Vector3 fricCoef0 = Type()->_fricCoef0;
  Vector3 friction;
  friction.Init();
  Vector3 speed=oSpeed*7.5;
  friction[0]=speed[0]*fabs(speed[0])*fricCoef2[0]+speed[0]*fricCoef1[0]+fSign(speed[0])*fricCoef0[0];
  friction[1]=speed[1]*fabs(speed[1])*fricCoef2[1]+speed[1]*fricCoef1[1]+fSign(speed[1])*fricCoef0[1];
  friction[2]=speed[2]*fabs(speed[2])*fricCoef2[2]+speed[2]*fricCoef1[2]+fSign(speed[2])*fricCoef0[2];
#if DEBUG_SEAGULL
  friction[0]*=multfriction;
#endif
  return friction;
}

static Vector3 WingUpForce(Vector3Val oSpeed, float coef, float wingDive)
{
  // optimized calculation

  float angle = -wingDive;
  // assume wingDive is small (-0.4 ... +0.4)
  // sin wingDive =.= wingDive (sin 0.4=0.39)
  // cos wingDive =.= 1-wingDive^2/2
  //DoAssert(fabs(angle)<=0.4);
  saturate(angle, -0.4, 0.4);
#if DEBUG_SEAGULL
  if (fabs(angle)>0.4) LogF("### wingDiveAngle = %f", angle);
#endif
  float s=angle,c=1-angle*angle*0.5;
  Matrix3 wingRot;
  wingRot.SetIdentity();
  wingRot(1,1)=+c,wingRot(1,2)=-s;
  wingRot(2,1)=+s,wingRot(2,2)=+c;

  Matrix3 wingRotInv;
  wingRotInv.SetIdentity();
  wingRotInv(1,1)=+c,wingRotInv(1,2)=+s;
  wingRotInv(2,1)=-s,wingRotInv(2,2)=+c;

  Vector3 speed=wingRotInv*(0.04*oSpeed);
  float forceUp=0;

  float zSpeed=fabs(oSpeed[2])-2;
  float spd1=speed[1]+6-36*coef;
  forceUp+=spd1*fabs(spd1)*-50+spd1*-5000;
  forceUp+=zSpeed*20000;
  Vector3 ret = wingRot.DirectionUp()*(forceUp*1.2);

  /*
  LogF
  (
    "forceUp=%.1f, zSpeed=%.1f, speed[1]=%.1f, coef=%.1f, wingDive=%.1f, ret=(%.2f,%.2f,%.2f)",
    forceUp,zSpeed,speed[1],coef,wingDive,ret[0],ret[1],ret[2]
  );
  */

  return ret;
}

/** Function recursively searches for a subset of dirs. In subset must be one dir from every set. Any two dirs from
subset must have positive dot product. And the result repairMove build from dirs must be the shortest.
@param collisions - (in) array with collision points
@param repairMoveSets - (in) index of the sets last points.
@param actRepairMove - (in) actual repair move (build from directions in actualSubSet)
@param iSet - (in) set in which function must search for new dir. 
@param actualSubSet - (in/out) dirs chosen till now.
@param minRepairMove - (in/out) the smallest repair move found till now.
@param minRepairMoveSize - (in/out) size of the smallest repair move found till now.
@see #ValidatePotentialState( const Frame& , bool, float, Vector3 *, bool, ObjectArray * )  
*/
void FindConvexSubSet(const CollisionBuffer& collisions, const AutoArray<int>& repairMoveSets, 
  Vector3Par actRepairMove, const int iSet, AutoArray<Vector3>& actualSubSet, Vector3& minRepairMove, float& minRepairMoveSize)
{
  for(int iPosInSet = repairMoveSets[iSet - 1]; iPosInSet <  repairMoveSets[iSet]; iPosInSet++)
  {
    const CollisionInfo& info = collisions[iPosInSet];

    if (info.under == 0)
      continue;

    // check if current info is convex with allready choosen.
    int i = 0;
    for(; i < iSet - 1; i++) 
    {
      if (info.dirOut * actualSubSet[i] < -0.01)
        break;
    }

    if (i == iSet - 1)
    {
      // convex
      float under = info.under - info.dirOut * actRepairMove;
      Vector3 cNewActRepairMove = actRepairMove;
      if (under > 0)
        cNewActRepairMove += info.dirOut * under;                

      if (cNewActRepairMove.Size() < minRepairMoveSize)
      {
        // still interesting
        if (iSet == repairMoveSets.Size() - 1)
        {
          //end
          minRepairMove = cNewActRepairMove;
          minRepairMoveSize = cNewActRepairMove.Size();
        }
        else
        {
          // continue with next set
          actualSubSet[iSet - 1] = info.dirOut;
          FindConvexSubSet(collisions,  repairMoveSets, cNewActRepairMove, iSet + 1, actualSubSet, minRepairMove, minRepairMoveSize);
        }
      }
    }
  }
}

#define MAX_REPAIR_SPEED 15.0f
#define MAX_REPAIR_STEP(repairMove, speed, deltaT) \
  (((speed).SquareSize() > 100.0f) ? \
  (MAX_REPAIR_SPEED - 10 * (repairMove) * (speed) / (repairMove).Size() / (speed).Size()) * (deltaT) \
  : (MAX_REPAIR_SPEED - (repairMove) * (speed) / (repairMove).Size()) * (deltaT))

typedef struct 
{
  int iComp;
  Ref<Object> obj;
} CollComponentID;

TypeIsMovableZeroed(CollComponentID);

/** Function detects collisions with objects. Futher, if requested, it searches for "repair move". 
Repair move is difference to transformation. In ideal case repaired transformation will be collision free.
@param moveTrans - investigated man position and orientation.
@param repairMove - result repair move.
@return collision volume
*/
float SeaGull::ValidatePotentialState(const SeaGullVisualState& moveTrans, Vector3* repairMove)
{
  //PROFILE_SCOPE(manval);

#if _ENABLE_CHEATS
/*  if (CHECK_DIAG(DEManCollision) && _landContact)
  {
    Vector3 cLandNormal(-_landDX,1,-_landDZ);
    cLandNormal.Normalize(); 

    AddForce(_landContactPos, cLandNormal * 2, Color(0,0,0));
    AddForce(moveTrans.Position(), cLandNormal * 2, Color(1,1,1));
    if (_freeFall)
      AddForce(moveTrans.Position(), cLandNormal * 10, Color(1,1,1));
  }*/
#endif

#define SHOW_STATIC_COL 0
#if SHOW_STATIC_COL  
  AddForce(moveTrans.Position(), Vector3(0,2,0), Color(0,0,1));  
#endif
  
  // do it just for local ones and with geometry. 
  if (!IsLocal() || !HasGeometry())
    return 0;

  //ADD_COUNTER(manval2,1);  

  CollisionBuffer collision; 

  {
    //PROFILE_SCOPE(gullCl);
    GLandscape->ObjectCollision( collision, this, moveTrans, false, true);
  }

  if (collision.Size() == 0)
    return 0;

  //Vector3 cLandNormal(-_landDX,1,-_landDZ);
  //cLandNormal.Normalize();

  //const Mapping& bottomComp = Type()->GetBottomGeomComp();

  // The following code is searching for collisions that will be ignored. 
  // Ignored will be collisions with objects, where is reachable roadway.
  // It allows as to reach these roadways with landcontact and continue on them.

  // Ignored collisions are marked as forbidden. Forbidden is combination of two convex componetns
  // where their intersection is lying between landcontact and roadway. Roadway must maximaly MAX_STEP_UP over landcontact.

  bool bFirst = true;
  CollComponentID actComp;
  AutoArray<CollComponentID> cForbiddenComponents(collision.Size());

 /* if (!_freeFall)
  {
    bool bRoadWay= false;
    bool bForbidden = true; 
    bool bSureForbidden = false;

    for( int i=0; i<collision.Size(); i++ )
    {
      // info.pos is relative to object
      CollisionInfo &info=collision[i];                 

      if (!info.object) continue;
      if (info.object->IsPassable()) continue;

      // In collision array are collision points ordered. The first come points from
      // collision between man and the first object, then from the man and the second object etc...
      // Futher they are oredered according to convex components.

      if (bFirst)
      {
        actComp.obj = info.object;
        actComp.iComp = info.component;                       
        bFirst = false;

        if (info.object->GetShape()->FindRoadwayLevel()>=0 )  
        {
          bRoadWay = true;                        
        }              

        bSureForbidden = (_ignoreLowPriorituContact && (!info.object->Static() || info.hierLevel > 0));          
      }
      else
      {
        if (actComp.obj != info.object || actComp.iComp != info.component)
        {
          // new component
          if ((bRoadWay && bForbidden) || bSureForbidden)                   
            cForbiddenComponents.Add(actComp);

          bRoadWay = false;
          bForbidden = true; 
          bSureForbidden = (_ignoreLowPriorituContact && (!info.object->Static() || info.hierLevel > 0));     

          actComp.obj = info.object;
          actComp.iComp = info.component;  

          if (info.object->GetShape()->FindRoadwayLevel()>=0 ) 
          {
            bRoadWay = true;
          }
        }
      }

      Vector3 worldPos = (info.hierLevel == 0) ? info.object->PositionModelToWorld(info.pos)
        : info.parentObject->PositionModelToWorld(info.pos) ;      

      // Check iF collision point is under landcontact and land contact is maximaly MAX_STEP_UP, over landcontact.
      // Here is a bit of simplification, we do not have all collision points in array collision.  
      if (bRoadWay && bForbidden && !bSureForbidden)
      {
        float under = GLandscape->UnderRoadSurface(info.object,info.pos,0, Landscape::ObjDebuggingContext());
        if (under > -0.01 //numerical barier
            ) 
        {          
          bForbidden = _landContact ? (worldPos + Vector3(0,under,0) - _landContactPos) * cLandNormal < MAX_STEP_UP  * cLandNormal[1]:
        under < MAX_STEP_UP;                                            
        }
        else 
          bForbidden = false;

      }                           

      info.pos = worldPos;                                  
    }

    if ((bRoadWay && bForbidden) || bSureForbidden)                  
      cForbiddenComponents.Add(actComp);
  }*/

  // Now collision points will be moved into wipedCollision array, but forbidden points will be omited.  
  CollisionBuffer wipedCollision;

  // Find repair move sets.
  // repairMoveSets array will contain 1-based indexes into array wipedCollision.
  // It will be indexes of the last points from a collision between two convex components.
  AutoArray<int> repairMoveSets(128);
  repairMoveSets.Add(0); // Terminator   

  bFirst = true;  

  for( int i=0; i<collision.Size(); i++ )
  {
    // info.pos is relative to object
    CollisionInfo &info=collision[i];

    if (!info.object) continue;
    if (info.object->IsPassable()) continue;

    // If object contact is under land contact, iqnore it. It must be solved by land contact
    int j = 0;
    for(; j < cForbiddenComponents.Size(); j++)
    {
      if (info.component == cForbiddenComponents[j].iComp && info.object == cForbiddenComponents[j].obj)
        break;
    }

    if (j != cForbiddenComponents.Size())
      continue;

    if (bFirst)
    {
      actComp.obj = info.object;
      actComp.iComp = info.component;        
      
      bFirst = false;
    }
    else
    {
      if (actComp.obj != info.object || actComp.iComp != info.component)
      {
        actComp.obj = info.object;
        actComp.iComp = info.component;               

        repairMoveSets.Add(wipedCollision.Size());
      }
    }       
   
    wipedCollision.Add(info);
#if _ENABLE_CHEATS
    /*if(CHECK_DIAG(DEManCollision))
      AddForce(info.pos, info.dirOut * info.under * 10, Color(0,0,1));
      */
#endif
  }

  if (bFirst)
    // there are not a valid collision in the result;
    return 0;

  repairMoveSets.Add(wipedCollision.Size()); // Terminator      

  // Now find the shortes penetration depth in each repairMoveSets. The collision volume is the largest value from those values.
  float fMaxCollVol = 0;

  for( int iSet = 1; iSet < repairMoveSets.Size(); iSet++ )
  {
    float fMinCollVol = 1000000.0f; // Maximum        

    for(int i = repairMoveSets[iSet - 1]; i < repairMoveSets[iSet]; i ++)
    {
      // info.pos is relative to object
      const CollisionInfo &info=wipedCollision[i];

      if(fMinCollVol > info.under)
        fMinCollVol = info.under;                 
    }

    if (fMinCollVol > fMaxCollVol)
      fMaxCollVol = fMinCollVol;
  }

  if (!repairMove)
    return fMaxCollVol; // State value

  bool bFoundRepair = false;

  // Now, repairMove must be from the plane perpendicular to LandNomrmal. Recalculate all direction to this plane. 
 /* if (_landContact)
  {                     
    for(int iSet = 1; iSet < repairMoveSets.Size(); iSet++)
    {
      bool bValid = false;
      for(int i = repairMoveSets[iSet - 1]; i < repairMoveSets[iSet]; i ++)
      {
        CollisionInfo& info = wipedCollision[i];

        if ( cLandNormal * info.dirOut != 0) 
        {                
          Vector3 cOld = info.dirOut;

          info.dirOut -= cLandNormal * (cLandNormal * info.dirOut);
          float fSize = info.dirOut.Size();                                                          

          if (fSize <= 0.1f) // numerical zero
          {
            // this move cannot be used
            info.dirOut = VZero;
            info.under = 0;
          }
          else
          {
            info.dirOut /= fSize;
            info.under /= fSize;
            bValid = true;
          }
        }
        else
          bValid = true;
      }

      if (!bValid)
      {
        // we cannot solve this set
        repairMove = VZero;
        return fMaxCollVol;
      }                  
    }
  }*/

  *repairMove = VZero;  

  // Find smallest convex path. Find set of dirs (one from each repairMoveSet) that have positive dot product and 
  // forms the smallest repair move.
  if (!bFoundRepair)
  {
    // if there is too many repairMoveSets algorythmus is very unefective
    if (repairMoveSets.Size() < 8)
    {    
      AutoArray<Vector3> actualSubSet(repairMoveSets.Size());

      //Init array       
      for(int i = 0; i < repairMoveSets.Size(); i++)
        actualSubSet.Add(VZero);

      float minRepairMoveSize = 100000.0f;    

      FindConvexSubSet( wipedCollision, repairMoveSets, 
        VZero, 1, actualSubSet, 
        *repairMove, minRepairMoveSize);

      bFoundRepair = (*repairMove != VZero);      
    }   
  }

  // If convex path does not exist, take the smallest from each set. There is no garanty, that result repairMove, 
  // will be right correction.
  if (!bFoundRepair)
  {
    AutoArray<int> cMinMoves(repairMoveSets.Size());

    // Find Min Moves in each set
    for( int iSet = 1; iSet < repairMoveSets.Size(); iSet++ )
    {
      float fMinCollVol = 1000000.0f; // Maximum  
      int iMinIndex = 0;

      for(int i = repairMoveSets[iSet - 1]; i < repairMoveSets[iSet]; i ++)
      {                
        // info.pos is relative to object
        const CollisionInfo &info=wipedCollision[i];

        if (info.under != 0)
        {                    
          if(fMinCollVol > info.under)
          {        
            fMinCollVol = info.under;  
            iMinIndex = i;
          }            
        }
      }

      cMinMoves.Add(iMinIndex);
    }

    for(int i = 0; i < cMinMoves.Size(); i++)
    {        
      const CollisionInfo &info = wipedCollision[cMinMoves[i]];
      float under = info.under - (*repairMove) * info.dirOut;

      if (under > 0)
      {                        
        if (*repairMove * info.dirOut < 0)
        {
          // not complex part.
          // take perpendicular dir to dirOut
          Vector3 repairMoveNorm = repairMove->Normalized();
          Vector3 dir = info.dirOut - repairMoveNorm * (repairMoveNorm * info.dirOut);

          float fDot = dir * info.dirOut;
          if (fDot < 0.1)
          {
            // numerical zero (we can't find any dir)
            *repairMove = VZero;
            return fMaxCollVol;
          }
          *repairMove += dir * (under / (dir * info.dirOut));                                  
        }
        else
          // complex part
          *repairMove += info.dirOut * under;                    
      }
    }
    bFoundRepair = true;                        
  }

  //Enlarge repairMove a bit.
  if (repairMove->SquareSize() != 0)            
    *repairMove += (*repairMove) / repairMove->Size() * 0.01;

  return fMaxCollVol;
}

SeaGullVisualState SeaGull::SimulateMove(float deltaT, SimulationImportance prec )
{
  // sitting seaGull is inactive
  if (_landContact)
  {
    SeaGullVisualState trans = FutureVisualState();
    trans.SetUpAndDirection(VUp, FutureVisualState().Direction());
    trans.SetPosition(FutureVisualState().Position());
    return trans;
  }

  // calculate new position
  SeaGullVisualState oldPos = FutureVisualState();
  //float collOld = ValidatePotentialState(*this, false, Vector3());

  SeaGullVisualState trans = oldPos;
  trans.SetTransform(ApplySpeed(deltaT));

  ApplyRemoteStateAdjustSpeed(deltaT, trans);

  if (prec > SimulateVisibleNear || !IsLocal() || !GetManual())
    return trans;

  //Check line if the speed is to high
  if (FutureVisualState()._speed.SquareSize() * deltaT * deltaT > 0.06f)
  {  
    CollisionBuffer retVal;
    GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),retVal, Landscape::FilterIgnoreOne(this), oldPos.Position(), trans.Position(), 0, ObjIntersectGeom);
    // find closest intersection
    float minDist = 1;  
    for(int i = 0; i < retVal.Size(); i++)
    {
      if (retVal[i].object->IsPassable()) continue;
      saturateMin(minDist, retVal[i].under);    
    }

    Vector3 resPos = oldPos.Position()* (1 - minDist) + minDist * trans.Position();
    trans.SetPosition(resPos);
  }

  // do real collision detection at the end position
  Vector3 repairMove;
  float collnew = ValidatePotentialState(trans, &repairMove);

  if (collnew > 0)
  {
    if (repairMove.SquareSize() > 0)
    {    
      // repair it
      SeaGullVisualState moveTransRepaired = trans;
      moveTransRepaired.SetPosition(trans.Position()+repairMove);

      float collRep = ValidatePotentialState(moveTransRepaired);
      if (collRep < collnew)
      {
        trans.SetTransform(moveTransRepaired);
        collnew = collRep;
      }
    }

    if (collnew > 0)
    {
      // compare it with original value
      float collOrig = ValidatePotentialState(oldPos);
      if (collOrig * 1.1 < collnew)
        //move must be forbidden
        trans.SetTransform(oldPos);
    }
    FutureVisualState()._speed = (trans.Position() - oldPos.Position()) / deltaT;
  }
  return trans;
}

void SeaGull::Simulate(float deltaT, SimulationImportance prec)
{
  #if ENABLE_BIRD_TRACKING
  // in each simulation step drop a trace particle
    Cloudlet *cloudlet = _cloudlets.Drop(Position(),VZero,1.0);
    if (cloudlet)
      GWorld->AddCloudlet(cloudlet);
  #endif
  
  // integrate movement + CD
  SeaGullVisualState moveTrans = SimulateMove(deltaT, prec);

  // calculate all forces, frictions and torques
  Vector3Val speed = FutureVisualState().ModelSpeed();
  Vector3 force(VZero),friction(VZero);
  Vector3 torque(VZero),torqueFriction(VZero);
  //LogF("ModelSpeed (%f %f %f), prec %f, deltaT %f", speed[0], speed[1], speed[2], SimulationPrecision(), deltaT); 
  
  // world space center of mass
  Vector3 wCenter(VFastTransform,FutureVisualState().ModelToWorld(),GetCenterOfMass());

  // we consider following forces
  // gravity
  // main rotor (in direction of rotor axis, changed by cyclic, controlled by thrust)
  // back rotor (in direction of rotor axis - controlled together with cyclic)
  // body aerodynamics (air friction)
  // main rotor aerodynamics (air friction and "up" force)
  // body and rotor torque friction (air friction)

  Vector3 pForce(VZero); // partial force
  Vector3 pCenter(VZero); // partial force application point

  float delta;

  float oldRpm=_rpm;

  float rpmWanted = _rpmWanted;
  if( !_landContact && !GetManual()) rpmWanted=1;
  delta=rpmWanted-_rpm;
  Limit(delta,-2*deltaT,+2*deltaT);
  _rpm+=delta;
  Limit(_rpm,0,1);


  if( _rpm<0.9 )
  {
    if (_ambient && GetTotalDamage()>=1)
    {
      // we died
      _wingBase = 0;
      const float wingDead = 0.97f;
      if (_wingPhase<wingDead)
      {
        const float wingDeadSpeed = 1.0f;
        _wingPhase += wingDeadSpeed*deltaT;
        if (_wingPhase>=wingDead)
          _wingPhase = wingDead;
      }
      else
        _wingPhase = wingDead;
    }
    else
    {
      // sitting on the ground
      _wingBase=1,_wingPhase=(1-_rpm)*0.5;
      _cyclicForwardWanted=0;
      _cyclicAsideWanted=0;
      _wingDiveWanted=0;
      _mainRotorWanted=0;
      _wingDiveWanted=0;
      _thrustWanted=0;
    }
  }
  else
  {
    if( oldRpm<0.9f )
      QuickStart();
    const float wingFree=0.97f;
    float wingSpeed=_mainRotor*2.5f;
    if( _wingPhase<wingFree ) saturateMax(wingSpeed,0.5f);
    _wingPhase+=_wingSpeed*wingSpeed*deltaT;
    if( _wingPhase>=1.0f )
    {
      float randomize=GRandGen.RandomValue()*0.2f+0.9f;
      _wingSpeed=randomize;
      _wingPhase=fastFmod(_wingPhase,1.0f);
    }
    _wingBase=0;
  }

#if DEBUG_SEAGULL
  if (debugID==ID()) 
    LogF("     dThrust=%f, dRotor=%f, dDive=%f", _thrustWanted-_thrust, _mainRotorWanted-_mainRotor, _wingDiveWanted-_wingDive);
#endif
  // main rotor thrust
  // change main rotor force
  delta=_mainRotorWanted-_mainRotor;
  Limit(delta,-2*deltaT,+2*deltaT);
  _mainRotor+=delta;
  Limit(_mainRotor,0,2);

  delta=_thrustWanted-_thrust;
  Limit(delta,-1.5*deltaT,+1.5*deltaT);
  _thrust+=delta;
  Limit(_thrust,-1,1);
  
  delta=_wingDiveWanted-_wingDive;
  Limit(delta,-4*deltaT,4*deltaT);
  _wingDive+=delta;
  Limit(_wingDive,-1,1);

  float posCorrect = GetShape()->GeometrySphere()*0.068f;
#if DEBUG_SEAGULL  
  //posCorrect = posCorrectDebug;
#endif 

  // force applied to the center of the rotor
  // it is forced to be aligned with center of mass
  //Vector3 sideOffset(VZero);
  pCenter[0]=_cyclicAsideWanted*0.8*posCorrect * 25 * Type()->_turning;
  pCenter[2]=_cyclicForwardWanted*-0.8*posCorrect * 20 * Type()->_turning;
  pCenter[1]=0; // note: this line was missing - causing QNAN in torque
  //sideOffset[1]=_rotorH.Center()[1]-CenterOfMass()[1];
  // apply aerodynamics of the main rotor

  pForce=GetMass()*(1.0/30000)*WingUpForce(speed,_mainRotor,_wingDive);

#if ARROWS
  AddForce(/*DirectionModelToWorld(pCenter)+*/wCenter, DirectionModelToWorld(pForce)*InvMass(), Color(1,0,0));
#endif
  
  // add simple forward/backward acceleration (1/4 G)

  // empirical: 2.5m/ss corresponds to 7 m/ss total acceleration
  float coef = Type()->_acceleration*(2.5f/7.0f);
  pForce+=VForward*(GetMass()*_thrust*coef);
  force+=pForce;
#if ARROWS
  AddForce(/*DirectionModelToWorld(pCenter)+*/wCenter, FutureVisualState().DirectionModelToWorld(VForward)*_thrust*coef, Color(1,0,0));
#endif

  // torque - turning based on "cyclic"
  pForce=Vector3(0,2.94f*GetMass(),0);
  #if ARROWS
    AddForce(FutureVisualState().DirectionModelToWorld(pCenter)+wCenter, FutureVisualState().DirectionModelToWorld(pForce)*InvMass());
  #endif

  torque += pCenter.CrossProduct(pForce);

  // side bank causes torque - rotate
  FutureVisualState().DirectionModelToWorld(torque,torque);

  float bank=FutureVisualState().DirectionAside().Y();

  Vector3 torqueDir = _invAngInertia.InverseGeneral() * VUp; // Maybe it's slow but important to set really good dir for ang momentum

  torqueDir.Normalize();
  torque -= torqueDir * 9180 * fabs(bank)*bank * posCorrect * Type()->_turning;
  #if ARROWS
    AddForce(FutureVisualState().DirectionModelToWorld(pCenter)+wCenter, FutureVisualState().DirectionModelToWorld(pForce)*InvMass(), Color(1,0,0));
  #endif
  
  // convert forces to world coordinates

  //DirectionModelToWorld(torque,torque);
  FutureVisualState().DirectionModelToWorld(force,force);
  
  // angular velocity causes also some angular friction
  // this should be simulated as torque
  //torqueFriction=_angMomentum*(0.05+rotorEff);
  torqueFriction=_angMomentum*30.0;
  
  // body air friction
  FutureVisualState().DirectionModelToWorld(friction,BodyFriction(speed)*GetMass()*(1.0/7000));
  if ( _fsmFlags & FSM_BREAKING )
  {
    friction*=_breakingFactor;
    //if (debugLanding) friction[1]/=debugMultFriction;
  }
  if( _rpm<1 )
    friction*=10*(1-_rpm);
  #if ARROWS
    AddForce(wCenter,-friction*InvMass(), Color(0,1,0));
  #endif
  
  // gravity - no torque
  pForce=Vector3(0,-1,0)*(GetMass()*G_CONST);
  Assert( pForce.IsFinite() );
  force+=pForce;
  #if ARROWS
    AddForce(wCenter,pForce*InvMass());
  #endif  

  // avoid going underground
  Point3 mPos=moveTrans.Position();
  //Log("Seagull mass %f pos %f,%f",GetMass(),mPos[0],mPos[2]);
  float minAbove=0.08; // to keap seeGull over surface
  minAbove = GetShape()->BoundingSphere()/100; //TODO: this should be BoundingSphere()/5, but butterfly has large geometry lod
  float minY=GLOB_LAND->SurfaceY(moveTrans.Position() + VUp * 0.1f);
#if DEBUG_SEAGULL
  if (debugID==ID())
  {
    float surfaceY = GLandscape->SurfaceY(Position());
  }
#endif
  //LogF("minY %f", minY);
  saturateMax(mPos[1],minY+minAbove);
  moveTrans.SetPosition(mPos);

  _landContact=( mPos[1]<=minY+minAbove+0.01 );

  // apply all forces
  //LogF("torque(%f, %f, %f)", torque[0], torque[1], torque[2]);
  Assert(_angMomentum.SquareSize()<1e20);
  ApplyForces(deltaT,force,torque,friction,torqueFriction);
  Assert(_angMomentum.SquareSize()<1e20);

  if( _landContact) 
  {
    if (_rpmWanted<0.9 )
    {
      // make stopped
      FutureVisualState()._speed = VZero;
      _angVelocity = VZero;
      _angMomentum = VZero;
      mPos[1]=minY;
    }
    else
      _landContact = false;
  }


  DoAssert(FutureVisualState().Transform().IsFinite());
  DoAssert(moveTrans.IsFinite());

  Move(moveTrans);
  // FIX
  DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);

  base::Simulate(deltaT,prec);
}


AnimationStyle SeaGull::IsAnimated( int level ) const {return AnimatedGeometry;}

bool SeaGull::CanObjDrawAsTask(int level) const
{
  return _shape->GetAnimationType()!=AnimTypeSoftware;
}

bool SeaGull::CanObjDrawShadowAsTask(int level) const
{
  return _shape->GetAnimationType()!=AnimTypeSoftware;
}

void SeaGull::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  ShapeUsed shape=_shape->Level(level);
  if( shape.IsNull() ) return;
  if (!setEngineStuff)
    shape->SetPhase(animContext, _wingPhase + _wingBase, _wingBase);
}

void SeaGull::Deanimate(int level, bool setEngineStuff)
{
/*
  ShapeUsed shape=_shape->Level(level);
  if( shape.IsNull() ) return;
  if (!setEngineStuff)
  {
    shape->SetPhase(0,0);
  }
*/
}

DEFINE_CASTING(SeaGull)
DEFINE_CASTING(SeaGullAuto)

/*%FSM<COMPILE "C:\bis\fsmeditor\globalSwitchFSM.cfg, SeaGullAutopilot">*/
/*%FSM<HEAD>*///{{AFX
/*
item0[] = {"alignTest",4,218,-232.930984,-127.586197,-142.930984,-77.586151,1.000000,"alignTest"};
item1[] = {"Far",2,250,14.655178,-233.620743,104.655205,-183.620682,0.000000,"Far"};
item2[] = {"farTest",4,218,-109.655121,-234.482697,-19.655121,-184.482697,0.000000,"farTest"};
item3[] = {"Near",0,250,-231.896439,-234.482681,-141.896545,-184.482758,0.000000,"Near"};
item4[] = {"Align",2,250,-239.655121,-11.206888,-149.655151,38.793121,0.000000,"Align"};
item5[] = {"Reached",2,250,8.620697,-11.139172,98.620575,38.860836,0.000000,"Reached"};
item6[] = {"reachedTest",4,218,-116.551704,-11.206901,-26.551701,38.793098,0.000000,"reachedTest"};
item7[] = {"reached",4,218,96.379364,88.793106,186.379364,138.793106,0.000000,"reached"};
item8[] = {"",7,210,49.448280,96.794350,57.448277,104.794350,0.000000,""};
item9[] = {"aligned",4,218,-123.827576,75.310349,-33.827583,125.310425,1.000000,"aligned"};
item10[] = {"",7,210,-199.621933,95.273361,-191.621933,103.273361,0.000000,""};
item11[] = {"stillAligned",4,218,-365.689667,-11.206898,-275.689667,38.793106,1.000000,"stillAligned"};
item12[] = {"near",4,218,-131.206818,-127.586174,-41.206848,-77.586197,2.000000,"near"};
item13[] = {"nearTest",4,218,-332.931000,-127.586174,-242.931000,-77.586189,2.000000,"nearTest"};
item14[] = {"",7,210,55.550476,-288.482727,63.550480,-280.482727,0.000000,""};
item15[] = {"farTonear",4,218,-109.172401,-310.034485,-19.172413,-260.034485,1.000000,"farTonear"};
item16[] = {"",7,210,-190.274597,-288.550415,-182.274597,-280.550415,0.000000,""};
link0[] = {0,4};
link1[] = {1,14};
link2[] = {2,1};
link3[] = {3,0};
link4[] = {3,2};
link5[] = {4,6};
link6[] = {4,11};
link7[] = {4,13};
link8[] = {5,7};
link9[] = {5,8};
link10[] = {5,12};
link11[] = {6,5};
link12[] = {7,5};
link13[] = {8,9};
link14[] = {9,10};
link15[] = {10,4};
link16[] = {11,4};
link17[] = {12,3};
link18[] = {13,3};
link19[] = {14,15};
link20[] = {15,16};
link21[] = {16,3};
globals[] = {0.000000,1,0,1,16777088,640,480,1,21,6316128,1,-401.881256,244.984634,197.087051,-362.312592,673,582,1};
window[] = {0,-1,-1,-1,-1,838,88,1048,116,1,691};
*//*%FSM</HEAD>*///}}AFX

/////////////////////////////////////////////////////////////////////////////////
//Global FSM functions and variables

// FSM: SeaGullAutopilot
//
//{{AFX

//}}AFX


/////////////////////////////////////////////////////////////////////////////////
//enumerate States of SeaGullAutopilot
//
//{{AFX
DEFINE_ENUM_BEG(SeaGullAutopilotState)
  SeaGullAutopilotNear,
  SeaGullAutopilotFar,
  SeaGullAutopilotAlign,
  SeaGullAutopilotReached,
DEFINE_ENUM_END(SeaGullAutopilotState)
//}}AFX

/////////////////////////////////////////////////////////////////////////////////
//Main FSM function for SeaGullAutopilot
//
//{{AFX
//}}AFX
/*%FSM<FSMFUNCTIONDECLARATION>*/
void SeaGullAuto::Autopilot
(
 Vector3Par target, Vector3Par tgtSpeed, // target
 Vector3Par direction, Vector3Par speed // wanted values
 )
/*%FSM</FSMFUNCTIONDECLARATION>*///{{AFX
{
  //}}AFX
  /*%FSM<FSMFUNCTIONPREFIX>*/
    const SeaGullType *type = Type();
    // point we would like to reach
    float avoidGround=0;
    Vector3Val position=FutureVisualState().Position();
    Vector3 absDistance=target-position;
    //Vector3 absDirection=absDistance+tgtSpeed*4; // "lead target" - est. target position
    Vector3 distance=DirectionWorldToModel(absDistance);

    float tgtSurfaceY = GLandscape->SurfaceY(target+ VUp * 0.1f);

  #if DEBUG_SEAGULL
    if (ID()==debugID)
    {
      SeaGullType *type2 = const_cast<SeaGullType *>(Type());
      type2->_maxSpeed=debugSetMaxSpeed;
      type2->_acceleration=debugSetAcceleration;
      float distToStop = Square(type2->_maxSpeed)*0.5/type2->_acceleration;
      type2->_precision = distToStop*0.035f;
    }
  #endif
    float precision = type->_precision;
    float precision2 = Square(precision);
    //float size2=distance.SquareSize();
    float sizeXZ2=distance[0]*distance[0]+distance[2]*distance[2];          
  /*%FSM</FSMFUNCTIONPREFIX>*///{{AFX
  switch (_state)
  {
    /*%FSM<STATE "Far">*/
    case SeaGullAutopilotFar:
    {
      //}}AFX
      /*%FSM<STATEINIT>*/
      /*%FSM</STATEINIT>*///{{AFX
      //}}AFX
      /*%FSM<STATEPRECONDITION>*/
      Vector3 absDirection=absDistance+tgtSpeed*5; // "lead target" - est. target position
      _pilotHeading=atan2(absDirection.X(),absDirection.Z());
      avoidGround = type->_avgHeight;
      _pilotHeight=target.Y()-tgtSurfaceY+avoidGround;

      _pilotSpeed[0]=0; // no side slips
      _pilotSpeed[1]=0; // vertical speed is ignored anyway
      _pilotSpeed[2]=type->_maxSpeed;
      _rpmWanted=1;
      // target height                              
      /*%FSM</STATEPRECONDITION>*///{{AFX

      /*%FSM<LINK "farTonear">*/
      //}}AFX
      /*%FSM<CONDPRECONDITION>*/
      /*%FSM</CONDPRECONDITION>*///{{AFX
      if (
        //}}AFX
        /*%FSM<CONDITION>*/
        sizeXZ2<50*50*precision2                                    
        /*%FSM</CONDITION>*///{{AFX
      )
      {
        //}}AFX
        /*%FSM<ACTION>*/
        /*%FSM</ACTION>*///{{AFX
        _state=SeaGullAutopilotNear;
        break;
      }
      /*%FSM</LINK>*/
    }
    break;
    /*%FSM</STATE>*/
    /*%FSM<STATE "Near">*/
    case SeaGullAutopilotNear:
    {
      //this is start state
      //}}AFX
      /*%FSM<STATEINIT>*/
      /*%FSM</STATEINIT>*///{{AFX
      //}}AFX
      /*%FSM<STATEPRECONDITION>*/
      Vector3 absDirection=absDistance+tgtSpeed*5; // "lead target" - est. target position
      // slow down near the target
      _pilotHeading=atan2(absDirection.X(),absDirection.Z());
      float fast=sqrt(sizeXZ2)*(1.0/50)/precision;
      Limit(fast,0,1);
      float y=0;
      saturateMax(y,target.Y()-tgtSurfaceY);
      avoidGround=(fast*0.2f+0.1f)*type->_avgHeight;
      _pilotHeight=y+avoidGround;
      _pilotSpeed[0]=0; // no side slips
      _pilotSpeed[1]=0; // vertical speed is ignored anyway
      _pilotSpeed[2]=(0.9f*fast+0.1f)*type->_maxSpeed*0.5f;
      _rpmWanted=1;                              
      /*%FSM</STATEPRECONDITION>*///{{AFX

      /*%FSM<LINK "alignTest">*/
      //}}AFX
      /*%FSM<CONDPRECONDITION>*/
      /*%FSM</CONDPRECONDITION>*///{{AFX
      if (
        //}}AFX
        /*%FSM<CONDITION>*/
        sizeXZ2<20*20*precision2
        && speed.Distance2(tgtSpeed)<Square(type->_maxSpeed*0.1f)                              
        /*%FSM</CONDITION>*///{{AFX
      )
      {
        //}}AFX
        /*%FSM<ACTION>*/
        /*%FSM</ACTION>*///{{AFX
        _state=SeaGullAutopilotAlign;
        break;
      }
      /*%FSM</LINK>*/

      /*%FSM<LINK "farTest">*/
      //}}AFX
      /*%FSM<CONDPRECONDITION>*/
      /*%FSM</CONDPRECONDITION>*///{{AFX
      if (
        //}}AFX
        /*%FSM<CONDITION>*/
        distance[2]>fabs(distance[0])*2 && sizeXZ2>60*60*precision2                                    
        /*%FSM</CONDITION>*///{{AFX
      )
      {
        //}}AFX
        /*%FSM<ACTION>*/
        /*%FSM</ACTION>*///{{AFX
        _state=SeaGullAutopilotFar;
        break;
      }
      /*%FSM</LINK>*/
    }
    break;
    /*%FSM</STATE>*/
    /*%FSM<STATE "Align">*/
    case SeaGullAutopilotAlign:
    {
      //}}AFX
      /*%FSM<STATEINIT>*/
      /*%FSM</STATEINIT>*///{{AFX
      //}}AFX
      /*%FSM<STATEPRECONDITION>*/
      float topSpeed2 = Square(type->_maxSpeed*0.2f);
      float sizeXZ=sqrt(sizeXZ2);
      _pilotHeading=atan2(direction.X(),direction.Z());
      float high=Interpolativ(sizeXZ,type->_maxSpeed*0.05f,type->_maxSpeed*0.5f,0,type->_avgHeight*0.1f);
      float highSpeed=Interpolativ(FutureVisualState()._speed.Distance2(speed),0,topSpeed2,0,type->_maxSpeed*0.05f);
      saturateMax(high,highSpeed);
      _pilotHeight=target.Y()-tgtSurfaceY+high;
      _pilotSpeed = distance*0.5f; // distance-speed conversion depends on acceleration abilities only

      avoidGround=high;                              
      /*%FSM</STATEPRECONDITION>*///{{AFX

      /*%FSM<LINK "nearTest">*/
      //}}AFX
      /*%FSM<CONDPRECONDITION>*/
      /*%FSM</CONDPRECONDITION>*///{{AFX
      if (
        //}}AFX
        /*%FSM<CONDITION>*/
        sizeXZ2>25*25*precision2 || tgtSpeed.Distance2(speed)>=topSpeed2                                    
        /*%FSM</CONDITION>*///{{AFX
      )
      {
        //}}AFX
        /*%FSM<ACTION>*/
        /*%FSM</ACTION>*///{{AFX
        _state=SeaGullAutopilotNear;
        break;
      }
      /*%FSM</LINK>*/

      /*%FSM<LINK "stillAligned">*/
      //}}AFX
      /*%FSM<CONDPRECONDITION>*/
      /*%FSM</CONDPRECONDITION>*///{{AFX
      if (
        //}}AFX
        /*%FSM<CONDITION>*/
        high>type->_avgHeight*0.02f                                    
        /*%FSM</CONDITION>*///{{AFX
      )
      {
        //}}AFX
        /*%FSM<ACTION>*/
        if( _rpm<0.5 )
        {
          QuickStart();
          _rpm=_rpmWanted=1;
        }                                        
        /*%FSM</ACTION>*///{{AFX
        _state=SeaGullAutopilotAlign;
        break;
      }
      /*%FSM</LINK>*/

      /*%FSM<LINK "reachedTest">*/
      //}}AFX
      /*%FSM<CONDPRECONDITION>*/
      /*%FSM</CONDPRECONDITION>*///{{AFX
      if (
        //}}AFX
        /*%FSM<CONDITION>*/
        high<type->_avgHeight*0.01f                                    
        /*%FSM</CONDITION>*///{{AFX
      )
      {
        //}}AFX
        /*%FSM<ACTION>*/
        _rpmWanted=0; // turn off engine (when on ground)
        _pilotSpeed=VZero;
        _pilotHeight -= type->_avgHeight*0.02f;                                        
        /*%FSM</ACTION>*///{{AFX
        _state=SeaGullAutopilotReached;
        break;
      }
      /*%FSM</LINK>*/
    }
    break;
    /*%FSM</STATE>*/
    /*%FSM<STATE "Reached">*/
    case SeaGullAutopilotReached:
    {
      //}}AFX
      /*%FSM<STATEINIT>*/
      /*%FSM</STATEINIT>*///{{AFX
      //}}AFX
      /*%FSM<STATEPRECONDITION>*/
      float topSpeed2 = Square(type->_maxSpeed*0.2f);
      float sizeXZ=sqrt(sizeXZ2);
      _pilotHeading=atan2(direction.X(),direction.Z());
      float high=Interpolativ(sizeXZ,type->_maxSpeed*0.05f,type->_maxSpeed*0.5f,0,type->_avgHeight*0.1f);
      float highSpeed=Interpolativ(FutureVisualState()._speed.Distance2(speed),0,topSpeed2,0,type->_maxSpeed*0.05f);
      saturateMax(high,highSpeed);
      _pilotHeight=target.Y()-tgtSurfaceY+high;
      _pilotSpeed = distance*0.5f; // distance-speed conversion depends on acceleration abilities only

      avoidGround = type->_avgHeight*-0.05f;                              
      /*%FSM</STATEPRECONDITION>*///{{AFX

      /*%FSM<LINK "near">*/
      //}}AFX
      /*%FSM<CONDPRECONDITION>*/
      /*%FSM</CONDPRECONDITION>*///{{AFX
      if (
        //}}AFX
        /*%FSM<CONDITION>*/
        sizeXZ2>25*25*precision2 || tgtSpeed.Distance2(speed)>=topSpeed2                                    
        /*%FSM</CONDITION>*///{{AFX
      )
      {
        //}}AFX
        /*%FSM<ACTION>*/
        /*%FSM</ACTION>*///{{AFX
        _state=SeaGullAutopilotNear;
        break;
      }
      /*%FSM</LINK>*/

      /*%FSM<LINK "aligned">*/
      //}}AFX
      /*%FSM<CONDPRECONDITION>*/
      /*%FSM</CONDPRECONDITION>*///{{AFX
      if (
        //}}AFX
        /*%FSM<CONDITION>*/
        high>type->_avgHeight*0.02f                                    
        /*%FSM</CONDITION>*///{{AFX
      )
      {
        //}}AFX
        /*%FSM<ACTION>*/
        if( _rpm<0.5 )
        {
          QuickStart();
          _rpm=_rpmWanted=1;
        }                                        
        /*%FSM</ACTION>*///{{AFX
        _state=SeaGullAutopilotAlign;
        break;
      }
      /*%FSM</LINK>*/

      /*%FSM<LINK "reached">*/
      //}}AFX
      /*%FSM<CONDPRECONDITION>*/
      /*%FSM</CONDPRECONDITION>*///{{AFX
      if (
        //}}AFX
        /*%FSM<CONDITION>*/
        high<type->_avgHeight*0.01f                                    
        /*%FSM</CONDITION>*///{{AFX
      )
      {
        //}}AFX
        /*%FSM<ACTION>*/
        _rpmWanted=0; // turn off engine (when on ground)
        _pilotSpeed=VZero;
        _pilotHeight -= type->_avgHeight*0.02f;                                        
        /*%FSM</ACTION>*///{{AFX
        _state=SeaGullAutopilotReached;
        break;
      }
      /*%FSM</LINK>*/
    }
    break;
    /*%FSM</STATE>*/
  }
  //}}AFX
  /*%FSM<FSMFUNCTIONSUFIX>*/
  _pilotSpeed+=speed;
  Limit(_pilotHeight,0,type->_maxHeight);
  Limit(_pilotSpeed[0],-type->_maxSpeed*0.33f,type->_maxSpeed*0.33f);
  Limit(_pilotSpeed[1],-type->_maxSpeed*0.5f,type->_maxSpeed*0.5f);
  Limit(_pilotSpeed[2],type->_minSpeed,type->_maxSpeed);
  if( avoidGround>0 ) AvoidGround(avoidGround);            
  /*%FSM</FSMFUNCTIONSUFIX>*///{{AFX
}
//}}AFX
/*%FSM</COMPILE>*/

SeaGullAuto::SeaGullAuto( const EntityType *type )
:base(type),
_pilotHeight(0),_pilotSpeed(VZero),_pilotHeading(0),_pilotHeadingSet(false),
_pressedForward(false),_pressedBack(false),
_pressedUp(false),_pressedDown(false),
_lastPilotTime(0),
_state(SeaGullAutopilotNear),
_player(AI_PLAYER),
_waitUntil(Glob.time),
_moves(0), // 0 is default upDegree
_headXRotWanted(0), _headYRotWanted(0), _headXRot(0), _headYRot(0), _headYRotWantedCont(0),
_mouseDive(0)
{
  ResetAutopilot();
  SwitchMove(); //set default move (if any)
}

const float MinSpeed=-0.5;

DEF_RSB(default)

void SeaGullAuto::Simulate( float deltaT, SimulationImportance prec )
{
  PROFILE_SCOPE_EX(simSg,sim)
  // Simulate animations
  const MovesTypeBase *movesType = GetMovesType();
  if (movesType)
  {
    // patch if no move is played
    int upDegree = 0;
    
    MoveId defaultMove = movesType->GetMove(upDegree, RSB(default)).id;
    _moves.PatchNoMove(FutureVisualState()._moves,MotionPathItem(defaultMove));

    // apply current animation
    float adjustTimeCoef = 1.0f;
    AdvanceQueueInfo info;
    bool done = false;
    _moves.AdvanceMoveQueue(FutureVisualState()._moves, movesType, deltaT, adjustTimeCoef, info, done);

    // animation streaming
    _moves.LoadAnimationPhases(FutureVisualState()._moves, movesType, _timeOffset);
  }

  // VoN position to hear Direct Speech for players respawned into seagull
  if( _manual ) 
  {
    Camera *cam = GScene->GetCamera();
    if (cam) GetNetworkManager().SetVoNPlayerPosition(cam->Position(), FutureVisualState().Speed());
  }
  else if (_player!=AI_PLAYER)
  { // we need to set obstruction, occlusion and accomodation
    float obstruction = 1, occlusion = 1;
    GetNetworkManager().SetVoNObstruction(_player, obstruction,occlusion,0);
    float accomodation = GSoundScene->GetEarAccomodation();
    float volume = 1.0f;
    GetNetworkManager().UpdateVoNVolumeAndAccomodation(_player, volume, accomodation);
    // There is no need to set their position for VoN purposes as it is updated by UpdateClientInfo
    // GetNetworkManager().SetVoNPosition(_player, Position(), Speed());
  }

  Matrix3 inv = InvInertia();

  // autopilot: convert simple _pilot control
  // to advanced simulation model

  if (prec>SimulateVisibleFar)
  { // sea-gulls are visible only near (are very small)
    if( Glob.time>_lastPilotTime )
    {
      // avoid calculating pilot too often
      // pilot calculation can be quite time consuming
      //_dirCompensate=0.5;  // how much we compensate for estimated change
      if (IsLocal())
        CamControl(deltaT);
      _lastPilotTime=Glob.time+2.0+GRandGen.RandomValue();
    }
  
    // if sea-gull is invisible simulate it very roughly
    // just linear movement with constant speed, no animation
    Vector3 position=FutureVisualState().Position();
    float surfaceY = GLandscape->SurfaceY(position);
    FutureVisualState().DirectionModelToWorld(FutureVisualState()._speed,_pilotSpeed);
    position += FutureVisualState()._speed*deltaT;
    position[1] = _pilotHeight+surfaceY;
    Matrix4 trans;
    trans.SetRotationY(-_pilotHeading);
    trans.SetPosition(position);
    Move(trans);
    // FIX
    DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
    //Assert(fabs(_pilotSpeed, FutureVisualState()._modelSpeed)<0.1);
    _wingPhase=0;
    _wingBase=0;
    _mainRotor=0.5;
    _wingDive=0;
    _rpmWanted=1;
    _rpm=1;
    Entity::Simulate(deltaT,prec);
    return;
  }
  
  if( _manual )
  {
    //_dirCompensate=0.9; // heading compensation
    KeyboardPilot(NULL, deltaT);
  }
  else if (!IsLocal())
  {
    // no pilot for remote animals
  }
  else 
  {
    //_dirCompensate=0.9;  // how much we compensate for estimated change
    CamControl(deltaT);
    _lastPilotTime=Glob.time;
  }


  if (_ambient && GetTotalDamage()>=1)
  {
    // seagull is dead - simulate it
    _mainRotorWanted = 0;
    _thrustWanted = 0;
    _rpmWanted = 0;
    _cyclicAsideWanted = 0.5;
    _cyclicForwardWanted = 0;
    _wingDiveWanted = 0;
#if DEBUG_SEAGULL
    if (ID()==debugID)
    {
      LogF("SPEED of dead body: %s = %f, deltaT = %f", GetDebugName().Data(), sqrt(_modelSpeed.SquareSize()), deltaT);
      LogF("     _modelSpeed == %f, %f, %f",_modelSpeed.X(), _modelSpeed.Y(), _modelSpeed.Z());
    }
#endif
    base::Simulate(deltaT,prec);
    return;
  }
  
  // use acceleration to estimate change of position
  #define EST_DELTA 1.0
  // estimate vertical acceleration
  //float estY=Position.Y()+_pilotSpeed*EST_DELTA+_acceleration*EST_DELTA*EST_DELTA*0.5;
  // wanted acceleration is a
  float surfaceY = GLandscape->SurfaceY(FutureVisualState().Position() + VUp * 0.1f);

  float aboveHeight = _pilotHeight+surfaceY-FutureVisualState().Position().Y();
  if (GetManual())
  {
    // avoid ground in advance
    for (int i=0; i<2; i++)
    {
      float estT = i;
      Vector3 estPos = FutureVisualState().Position()+FutureVisualState()._speed*estT;
      float estSurfY = GLandscape->SurfaceY(estPos + VUp * 0.1f);
      float estAboveHeight = _pilotHeight+estSurfY-FutureVisualState().Position().Y();
      saturateMax(aboveHeight,estAboveHeight);
    }
  }
  float wantedAY=(aboveHeight-FutureVisualState()._speed.Y()*EST_DELTA)*(1/(0.5*EST_DELTA*EST_DELTA));
  float changeAY=wantedAY-FutureVisualState()._acceleration.Y();
  
  Vector3 absSpeedWanted(VMultiply,FutureVisualState().DirModelToWorld(),_pilotSpeed);
  Vector3 changeAccel=(absSpeedWanted-FutureVisualState()._speed)*(1/EST_DELTA)-FutureVisualState()._acceleration;

  _thrustWanted = (_pilotSpeed[2]-FutureVisualState().ModelSpeed()[2])*2;
  
  // if we need to accelerate forward, move cyclic forward
  changeAccel = DirectionWorldToModel(changeAccel);
  {
    static float accelC = 0.1f;
    float deltaMR = changeAY*accelC;
    // it has no sense to change mainRotorWanted too fast - the simulation will not react fast anyway
    // too abrupt changes cause a lot of data to be transmitted
    Limit(deltaMR,-2*deltaT,+2*deltaT);
    _mainRotorWanted = deltaMR+_mainRotor;
  }

  // get simple approximations of heading and dive
  // we must consider current angular velocity
  //TODO: estT = deltaT; should be much better than the commented version
  float estT = deltaT;
/*
  float angVelocitySS = _angVelocity.SquareSize()/9; 
  float estT = 0.3f * min(1,angVelocitySS);  // experimentally tested fact for lower velocities 
                                             // it's better t use lower estimation time
*/
  
  Matrix3Val orientation=FutureVisualState().Orientation();
  Matrix3Val derOrientation=_angVelocity.Tilda()*orientation;
  Matrix3 estOrientation= orientation+derOrientation*estT;  

  estOrientation.Orthogonalize();

  Vector3 direction=estOrientation.Direction();

  float curHeading=atan2(direction[0],direction[2]);
  float changeHeading=AngleDifference(_pilotHeading,curHeading);
  //LogF("curHeading: %f, _pilotHeading %f, changeHeading %f",  curHeading, _pilotHeading, changeHeading);
  
  float bank = FutureVisualState().DirectionAside().Y();  

#if DEBUG_SEAGULL
  static volatile bool testLog=true;
  if (testLog && (ID()==debugID))
  {
    Vector3 temp = -Direction().CrossProduct(VUp);
    temp.Normalize();
    float bank2 = temp.CrossProduct(DirectionAside()) * Direction();
    Vector3 dirct = Direction();
    Vector3 dirctAside = DirectionAside();
    Vector3 dirctUp = DirectionUp();
    LogF("     bank=%f bank2=%f direction=%f,%f,%f directionAside=%f,%f,%f derictionUp=%f,%f,%f",
          bank, bank2, dirct[0],dirct[1],dirct[2],dirctAside[0],dirctAside[1],dirctAside[2],
          dirctUp[0],dirctUp[1],dirctUp[2],Direction().X(),Direction().Y(),Direction().Z());
  }
#endif

  float dive=estOrientation.Direction().Y();
  float fastDive=fabs(_pilotSpeed[2])*(1.0/9);
  Limit(fastDive,0.0,0.7);
  const float mouseDiveSpeed = 0; //mouse fwd/back doesn't affect dive, but pilotHeight
  _wingDiveWanted = (
    // when moving slow, dive corresponds to acceleration
    (dive+changeAccel[2])*(-1.0/75)*(1-fastDive)+
    _mouseDive*mouseDiveSpeed*fastDive +
    // when moving fast, dive corresponds to speed
    _pilotSpeed[2]*(-0.5/12)*fastDive
  );
  float diveWanted=0; // never dive (except for loosing altitude?)
  if( aboveHeight>5 ) diveWanted=0.3;
  else if( aboveHeight<-5 ) diveWanted=-0.3;
  else if( aboveHeight<-10 ) diveWanted=-0.6;
  float bankWanted = -changeHeading;
  //float bankWanted = -changeHeading*0.5; //*fabs(changeHeading);
  //float bankWanted = -changeHeading*fabs(changeHeading);
  // bank is limited
#if DEBUG_SEAGULL
  float bankWantedOld = bankWanted;
#endif
  static float BankLimit=0.8;
  Limit(bankWanted,-BankLimit,BankLimit);
  Limit(_wingDiveWanted,-0.4,0.4);
  //DoAssert(fabs(bank)<0.7);  //assert fails too often

  //LogF("bankWanted: %f, bank %f, oldbank %f",  bankWanted, bank, estOrientation.DirectionAside().Y());
  //LogF("diveWanted: %f, dive %f, thrustWanted %f",  diveWanted, dive,_thrustWanted);

  _cyclicAsideWanted=+(bankWanted-bank)*2 + fabs(bankWanted-bank) * (bankWanted-bank) * 2;
  _cyclicForwardWanted=-(diveWanted-dive)*2;  

#if DEBUG_SEAGULL
  if (debugID==ID()) 
    LogF("     thrustW=%f, estT=%f, bankWOld=%f, bankW=%f, cyclicAsideW=%f, cyclicFwdW=%f",
         _thrustWanted, estT, bankWantedOld, bankWanted, _cyclicAsideWanted, _cyclicForwardWanted);

  static volatile bool logTest=false;
  debugBreaking=logTest;
  if ((logTest && ID()==debugID) || (_fsmFlags & FSM_BREAKING))
  {
    debugBreaking=true;
    _wingDiveWanted = 0;
    _cyclicAsideWanted = -bank*2 - fabs(bank)*bank*2;
    _cyclicForwardWanted =  2*dive;
    _rpmWanted = 0;
    _thrustWanted = 0;
    _mainRotorWanted = (-2*_speed.Y()-_acceleration.Y())*0.2+_mainRotor;
  }
  static volatile bool logTest2=false;
  debugLanding=logTest2;
  if ((logTest2 && ID()==debugID) || (_fsmFlags & FSM_LANDING))
#else
  if (_fsmFlags & FSM_LANDING)
#endif 
  {
    _wingDiveWanted = 0;
    _cyclicAsideWanted = -bank*2 - fabs(bank)*bank*2;
    _cyclicForwardWanted =  2*dive;
    _rpmWanted = 0;
    //speedZ~height/2; //at _maxHeight, maxSpeed wanted
    float speedMultConst = 2*Type()->_maxSpeed/Type()->_maxHeight;
    float myHeight = FutureVisualState().Position().Y()-surfaceY;
    float speedWanted = 0.5f*speedMultConst*myHeight;
    //float speedYWanted = -speedMultConst*myHeight*myHeight;
    _thrustWanted = (speedWanted-FutureVisualState().ModelSpeed().Z())*2;
    static volatile float multiply=4.0f;
    static volatile float multiply2=0.5f;
    static volatile float multiply3=0.4f;
    _mainRotorWanted = (((surfaceY-FutureVisualState().Position().Y())*multiply3-FutureVisualState()._speed.Y())*multiply-FutureVisualState()._acceleration.Y())*multiply2+_mainRotor;
    //_mainRotorWanted = ((speedYWanted-_speed.Y())-_acceleration.Y())*0.5+_mainRotor;
#if DEBUG_SEAGULL
    if (ID()==debugID) LogF("rotorW=%f, thrustW=%f, speedWanted=%f, myHeight=%f", _mainRotorWanted, _thrustWanted, speedWanted, myHeight);
#endif
  }
  // perform advanced simulation
  base::Simulate(deltaT,prec);
}

void SeaGullAuto::AvoidGround( float minHeight )
{
  Point3 estimate=FutureVisualState().Position();
  for( int i=0; i<4; i++ )
  {
    float estY=GLOB_LAND->SurfaceY(estimate);
    estY+=minHeight;
    float estUnder=estY-estimate.Y();
    if( estUnder>0 && i==1 )
    {
      float maxSpeed=Interpolativ(estUnder,0,10,20,0);
      Limit(_pilotSpeed[2],MinSpeed,maxSpeed);
    }
    estimate+=FutureVisualState()._speed*3.0;
  }
}

void SeaGullAuto::ResetAutopilot()
{
  // We set state to near. It will go to far automatically (if necessary).
  _state=SeaGullAutopilotNear;
}


bool SeaGullAuto::IsVirtualX( CameraType camType ) const
{
  if (GetManual())
  {
    return camType!=CamInternal && camType!=CamExternal;
  }
  return true;
}

/*!
\patch 1.01 Date 6/25/2001 by Ondra.
- Fixed: Seagull mouse control. Improved seagull keyboard control.
*/

void SeaGullAuto::FSMRelativeMoveIn(FSMEntity *fsm, const float *params, int paramsCount)
{
  // parameters
  Vector3 move = VZero;
  float varX = 0;
  float varZ = 0;
  float varY = 0;

  switch (paramsCount)
  {
  default:
    varY = params[5];
    // continue
  case 5:
    move[1] = params[4];
    // continue
  case 4:
    varZ = params[3];
    // continue
  case 3:
    move[2] = params[2];
    // continue
  case 2:
    varX = params[1];
    // continue
  case 1:
    move[0] = params[0];
    // continue
  case 0:
    break;
  }

  move[0] += 2*varX*(GRandGen.RandomValue()-0.5f);
  move[2] += 2*varZ*(GRandGen.RandomValue()-0.5f);
  move[1] += 2*varY*(GRandGen.RandomValue()-0.5f);

  Matrix3 rot(MUpAndDirection, VUp, FutureVisualState().Direction());
  Vector3 moveAbs = move * rot;
  _setPars._camPos = FutureVisualState().Position() + moveAbs;
}

void SeaGullAuto::FSMRelativeMoveOut(FSMEntity *fsm, const float *params, int paramsCount)
{}

void SeaGullAuto::FSMSetNoBackwardsIn(FSMEntity *fsm, const float *params, int paramsCount)
{
  if (paramsCount>0) 
  {
    if (toInt(params[0])==0) _fsmFlags &= ~FSM_NO_BACKWARDS;
    else _fsmFlags |= FSM_NO_BACKWARDS;
  }
}
void SeaGullAuto::FSMSetNoBackwardsOut(FSMEntity *fsm, const float *params, int paramsCount)
{}

void SeaGullAuto::FSMRandomMoveIn(FSMEntity *fsm, const float *params, int paramsCount)
{
  RandomMoveFunc(params, paramsCount, false);
}
void SeaGullAuto::FSMRandomMoveOut(FSMEntity *fsm, const float *params, int paramsCount)
{}

void SeaGullAuto::FSMRandomMoveLandIn(FSMEntity *fsm, const float *params, int paramsCount)
{
  RandomMoveFunc(params, paramsCount, true);
}
void SeaGullAuto::FSMRandomMoveLandOut(FSMEntity *fsm, const float *params, int paramsCount)
{}

void SeaGullAuto::RandomMoveFunc(const float *params, int paramsCount, bool land)
{
  _fsmFlags &= ~(FSM_LANDED | FSM_BREAKING | FSM_LANDING);
  // parameters
  const SeaGullType *type = Type();
  float straightDistance = type->_straightDistance;
  float minHeight = type->_minHeight;
  float avgHeight = type->_avgHeight;
  float maxHeight = type->_maxHeight;
  switch (paramsCount)
  {
  default:
    maxHeight = params[3];
    // continue
  case 3:
    avgHeight = params[2];
    // continue
  case 2:
    minHeight = params[1];
    // continue
  case 1:
    straightDistance = params[0];
    // continue
  case 0:
    break;
  }

  float dist = straightDistance;
  float rX = GRandGen.RandomValue()*dist*2-dist;
  float rZ = GRandGen.RandomValue()*dist*2-dist;
  float rY = (land ? 0.0 : GRandGen.Gauss(minHeight, avgHeight, maxHeight));
  float posX = FutureVisualState().Position().X()+rX;
  float posZ = FutureVisualState().Position().Z()+rZ;
  float posY = GLandscape->SurfaceYAboveWater(posX,posZ)+rY;
  _setPars._camPos = Vector3(posX,posY,posZ);
  //ResetAutopilot();
}

bool SeaGullAuto::UpdateMoveOut()
{
  if (!_ambient) return false;
  const SeaGullType *type = Type();
  float avgHeight = type->_avgHeight;
  float straightDistance = type->_straightDistance;
  const Camera &camera = *GScene->GetCamera();
  float moveOut = _ambient->CheckMoveOut(camera,this);
  if (moveOut>1e-4f)
  {
    float posX = FutureVisualState().Position().X();
    float posZ = FutureVisualState().Position().Z();
    // if the unit should be moving out, we need to apply some systematic movement
    // we move away from the camera
    // TODO: handle moving camera better
    Vector3 moveOutDirection = FutureVisualState().Position()-camera.Position();
    // we are not interested about vertical component
    moveOutDirection[1] = 0;
    if (moveOutDirection.SquareSize()<1e-3)
      // handle singular case - unit at the camera position
      moveOutDirection = VForward;
    else
      moveOutDirection.Normalize();
    Vector3 moveOutDistance = moveOutDirection*(moveOut*straightDistance*2.0f);
    posX += moveOutDistance.X();
    posZ += moveOutDistance.Z();
    float posY = GLandscape->SurfaceYAboveWater(posX,posZ)+avgHeight;
    _setPars._camPos = Vector3(posX,posY,posZ);
    return true;
  }
  return false;
}

void SeaGullAuto::FSMWaitIn(FSMEntity *fsm, const float *params, int paramsCount)
{
  float time = 0;
  if (paramsCount >= 1) time = params[0];
  if (paramsCount >= 2) time += (params[1]-params[0])*GRandGen.RandomValue();
  _waitUntil = Glob.time + time;
}
void SeaGullAuto::FSMWaitOut(FSMEntity *fsm, const float *params, int paramsCount)
{}

void SeaGullAuto::FSMStopIn(FSMEntity *fsm, const float *params, int paramsCount)
{
  _setPars._camPos = FutureVisualState().Position();
}
void SeaGullAuto::FSMStopOut(FSMEntity *fsm, const float *params, int paramsCount)
{}

void SeaGullAuto::FSMBreakIn(FSMEntity *fsm, const float *params, int paramsCount)
{
  if (paramsCount >= 1) _breakingFactor = params[0];
  else _breakingFactor=10.0f;
  _fsmFlags |= FSM_BREAKING;
}
void SeaGullAuto::FSMBreakOut(FSMEntity *fsm, const float *params, int paramsCount)
{}

void SeaGullAuto::FSMLandIn(FSMEntity *fsm, const float *params, int paramsCount)
{
  _fsmFlags |= FSM_LANDING;
}
void SeaGullAuto::FSMLandOut(FSMEntity *fsm, const float *params, int paramsCount)
{}

void SeaGullAuto::FSMSetTimerIn(FSMEntity *fsm, const float *params, int paramsCount)
{
  float time = 0;
  if (paramsCount >= 1) time = params[0];
  if (paramsCount >= 2) time += (params[1]-params[0])*GRandGen.RandomValue();
  _timer = Glob.time + time;
}
void SeaGullAuto::FSMSetTimerOut(FSMEntity *fsm, const float *params, int paramsCount)
{}

void SeaGullAuto::FSMSwitchActionIn(FSMEntity *fsm, const float *params, int paramsCount)
{
  // read action index from parameters
  int action = 0;
  if (paramsCount >= 1) action = toInt(params[0]);
  SwitchMove(action);
}
void SeaGullAuto::FSMSwitchActionOut(FSMEntity *fsm, const float *params, int paramsCount)
{}

void SeaGullAuto::SwitchMove(int action)
{
  const MovesTypeBase *movesType = GetMovesType();
  if (!movesType) return;
  // find move for given action
  MoveId move = MoveIdNone;
  const ActionMap *map = movesType->GetActionMap(_moves.GetPrimaryMove(FutureVisualState()._moves).id, MotionType::NoSubclassIx);
  if (map)
    move = map->GetAction((ActionMap::Index)action).id;
  else
  {
    int upDegree = _moves.GetActUpDegree(FutureVisualState()._moves,movesType);
    move = movesType->GetMove(upDegree, (ActionMap::Index)action).id;
  }

  // switch move
  if (move != MoveIdNone)
    _moves.SwitchMove(FutureVisualState()._moves, move, NULL);
}

float SeaGullAuto::FSMMoveCompleted(const FSMEntity *fsm, const float *params, int paramsCount) const
{
  const SeaGullType *type = Type();
  float precision = type->_precision;
  if (_setPars._camPos.DistanceXZ2(FutureVisualState().Position()) < Square(10 * precision) ||
    _state==SeaGullAutopilotReached || _state==SeaGullAutopilotAlign) return 1.0f;
  else return 0.0f;
}

float SeaGullAuto::FSMMoveCompletedVertical(const FSMEntity *fsm, const float *params, int paramsCount) const
{
  const SeaGullType *type = Type();
  float precision = type->_precision;
  Vector3 position = FutureVisualState().Position();
  if (_setPars._camPos.DistanceXZ2(position) < Square(10 * precision) ||
    _state==SeaGullAutopilotReached || _state==SeaGullAutopilotAlign) 
  {
    if (fabs(position.Y()-_setPars._camPos.Y()) < 15*precision)
    {
      _fsmFlags |= FSM_LANDED;
      return 1.0f;
    }
  }
  return 0.0f;
}

float SeaGullAuto::FSMWaitCompleted(const FSMEntity *fsm, const float *params, int paramsCount) const
{
  return Glob.time >= _waitUntil ? 1.0f : 0.0f;
}

float SeaGullAuto::FSMTimeElapsed(const FSMEntity *fsm, const float *params, int paramsCount) const
{
  return Glob.time >= _timer ? 1.0f : 0.0f;
}

void SeaGullAuto::CamControl(float deltaT )
{
  if (_ambient && GetTotalDamage()>=1)
  {
    // seagull is dead - no AI
    return;
  }
  #if _DEBUG
    static bool freeze = false;
    if (freeze)
    {
      Autopilot(FutureVisualState().Position(),VZero,FutureVisualState().Direction(),VZero);
    }
  #endif
  //check move out
  if (!UpdateMoveOut())
  {
    // get camera target - go there
    // decide where to fly to
    if (_ambient && (_fsmFlags & FSM_LANDED))
    {
      Autopilot(FutureVisualState().Position(),VZero,FutureVisualState().Direction(),VZero);
      FutureVisualState()._speed = VZero;
    }
    else
    {
#if DEBUG_SWITCH_CAMERA
    {
      static volatile bool logtest;
      if (logtest)
      {
        debugID=ID();
        LogF("Debug_Seagull: %s", cc_cast(GetDebugName()));
        GWorld->SwitchCameraTo(this, CamInternal, true);
      }
    }
    { //debug: testing, why butterfly do not land to the ground, but levitate at about 8cm above
      static volatile bool setposition = false;
      if (setposition)
      {
        Matrix4 trans = Transform();
        Vector3 pos = trans.Position();
        float yOffset = 0;
        LODShape *shape = GetShape();
        if (shape)
        {
          Shape *geom = shape->LandContactLevel();
          if (!geom) geom = shape->GeometryLevel();
          if (geom)
          {
            yOffset = geom->Min().Y();
          }
          else if (shape->NLevels()>0)
          {
            ShapeUsed level0 = shape->Level(0);
            yOffset = level0->Min().Y();
          }
          else
          {
            yOffset = shape->Min().Y();
          }
        }
        pos[1] = yOffset + GLandscape->SurfaceYAboveWater(pos.X(), pos.Z());
        trans.SetPosition(pos);
        MoveNetAware(trans);
        OnPositionChanged();
        GScene->GetShadowCache().ShadowChanged(this);
      }
    }
#endif
#if DEBUG_SEAGULL 
      static volatile bool logtest;
      if (logtest)
      {
        const SeaGullType *type = Type();
        debugID=ID();
        //LogF("Debug_Seagull: %s", cc_cast(GetDebugName()));
        //GWorld->SwitchCameraTo(this, CamInternal);
        SetSimulationPrecision(1/60.0f);
        debugSetAcceleration=type->_acceleration;
        debugSetMaxSpeed=type->_maxSpeed;
        debugMultFriction=1.0f;
      }
      if (ID()==debugID)
      {
        RString info=" ";
        if (debugBreaking) info=info + RString("breaking");
        LogF("SPEED of %s = %f, deltaT = %f %s", GetDebugName().Data(), sqrt(_modelSpeed.SquareSize()), deltaT, info.Data());
        LogF("     _modelSpeed == %f, %f, %f",_modelSpeed.X(), _modelSpeed.Y(), _modelSpeed.Z());
        static bool test=false;
        if (test)
        {
          LogF("     _maxSpeed = %f, _minSpeed = %f, _acceleration = %f", Type()->_maxSpeed, Type()->_minSpeed, Type()->_acceleration);
        }
      }
#endif
      if (_setPars._camPos.SquareSize() > 0.5 && Glob.time >= _waitUntil)
        Autopilot(_setPars._camPos,VZero,FutureVisualState().Direction(),VZero);
      else
      {
        Vector3 pos = FutureVisualState().Position();
        if ( _fsmFlags & (FSM_LANDING | FSM_LANDED) )
          pos[1] = GLandscape->SurfaceYAboveWater(pos.X(), pos.Z()); //allow proper landing
        Autopilot(pos,VZero,FutureVisualState().Direction(),VZero);
        if (_fsmFlags & FSM_NO_BACKWARDS)
        {
          Vector3 modspeed = DirectionWorldToModel(FutureVisualState()._speed);
          if (modspeed.Z()<0)
          {
            modspeed[2]=0;
            FutureVisualState().DirectionModelToWorld(FutureVisualState()._speed,modspeed);
          }
        }
      }
    }
#if DEBUG_SEAGULL
    if (ID()==debugID)
    {
      LogF("     _pilotSpeed of %s = %f (%f, %f, %f)", GetDebugName().Data(), 
           sqrt(_pilotSpeed.SquareSize()), _pilotSpeed[0],_pilotSpeed[1],_pilotSpeed[2]);
    }
#endif
  }
  if (_ambient)
  {
    if (_fsm.Size())
    {
      for (int i=0, siz=_fsm.Size();  i<siz; i++)
      {
        if (_fsm[i]!=NULL && _fsm[i]->Update(this))
          // done
          _fsm[i] = NULL;
      }
    }
    else
    {
      if (FSMMoveCompleted(NULL,NULL, 0) >= 0.5)
        FSMRandomMoveIn(NULL,NULL, 0);
    }
  }
}

bool SeaGullAuto::HasGeometry() const
{
  // no geometry collision testing for birds and insects
  // they have incorrect mass anyway, co collision testing would be wrong
  return false;
}

bool SeaGull::OcclusionFire() const
{
  return Type()->_canBeShot;
}

bool SeaGullAuto::IsPassable() const
{
  // birds/insects are too light to cause any collision
  return true;
}

void SeaGullAuto::SetAmbient(IAmbLifeArea *area, Vector3Par camPos)
{

  base::SetAmbient(area,camPos);
  
  const SeaGullType *type = Type();
  
  float ry = GRandGen.Gauss(type->_minHeight,type->_avgHeight,type->_maxHeight);
  
  Vector3 pos = FutureVisualState().Position();
  pos[1] += ry;
  Move(pos);
  
  SetDestructType(DestructMan);
  MakeAirborne(ry, false);

  float dist2 = pos.DistanceXZ2(camPos);
  // avoid too many iterations
  float tgtX=0,tgtZ=0;
  for(int i=0; i<5; i++)
  {
    float dist = type->_straightDistance;
    float rtx = GRandGen.RandomValue()*dist*2-dist;
    float rtz = GRandGen.RandomValue()*dist*2-dist;
    tgtX = pos.X()+rtx;
    tgtZ = pos.Z()+rtz;
    // first target should be closer to camera than current position
    float tgtDist2 = Vector3(tgtX,0,tgtZ).DistanceXZ2(camPos);
    if (tgtDist2<dist2) break;
  }
  float rty = GRandGen.Gauss(type->_minHeight,type->_avgHeight,type->_maxHeight);
  float tgtY = GLandscape->SurfaceYAboveWater(tgtX,tgtZ)+rty;
  SetPos(Vector3(tgtX,tgtY,tgtZ));
}

bool SeaGullAuto::MoveHead(float deltaT, float aimX, float aimY)
{
  bool forceRecalcMatrix = false;
  //move head using aimX, aimY first
  float headingSpeed = aimX;
  saturate(headingSpeed, -2, 2);
  float diveSpeed = aimY;
  saturate(diveSpeed, -2, 2);
  _headXRotWanted += diveSpeed * deltaT;
  _headYRotWanted -= headingSpeed * deltaT;
  saturate(_headXRotWanted, -0.5*H_PI+0.1f, 0.5*H_PI-0.1f);
  saturate(_headYRotWanted, -0.66*H_PI, 0.66*H_PI);
  
  //move head using other actions
  float headXRotWanted=_headXRotWanted;
  float headYRotWanted=_headYRotWanted;
  //combine movement with Continuous wanted values
  float headYRotWantedCont = H_PI*(GInput.GetAction(UALookLeftCont,false) - GInput.GetAction(UALookRightCont,false));
  float headXRotWantedCont = H_PI*(GInput.GetAction(UALookUpCont,false) - GInput.GetAction(UALookDownCont,false));
  headXRotWanted += headXRotWantedCont;
  headYRotWanted += headYRotWantedCont + _headYRotWantedCont;
  //LimitHeadRot(headXRotWanted, headYRotWanted);  
  saturate(headXRotWanted, -0.5*H_PI+0.1f, 0.5*H_PI-0.1f);
  saturate(headYRotWanted, -0.66*H_PI, 0.66*H_PI);
  //if (_isDead)
  if (GetTotalDamage()>=1) headXRotWanted = headYRotWanted = 0;

  if (headXRotWanted != _headXRot || headYRotWanted != _headYRot)
  {
    // Get the wanted - actual difference
    float headXRotChange = headXRotWanted-_headXRot;
    float headYRotChange = headYRotWanted-_headYRot;

    // Get the real change (based on difference size, V and deltaT)
    static float maxV = 2.0f; ///TODO
    const float deltaV = deltaT * maxV;
    saturate(headXRotChange,-deltaV, deltaV);
    saturate(headYRotChange,-deltaV, deltaV);
//    headXRotChange *= deltaV;
//    headYRotChange *= deltaV;
    if (fabs(headXRotChange)>1e-6) forceRecalcMatrix = true;
    else headXRotChange = 0;
    if (fabs(headYRotChange)>1e-6) forceRecalcMatrix = true;
    else headYRotChange = 0;

    _headXRot += headXRotChange;
    _headYRot += headYRotChange;
  }
  return forceRecalcMatrix;
}

/*!
\patch_internal 1.01 Date 6/25/2001 by Ondra.
- Fixed: seagull handling.
*/

bool SeaGullAuto::KeyboardUsedInFrame() const
{
  if (_inputFrameID == GInput.frameID) return true; // avoid processing a keyboard twice in a frame
  _inputFrameID = GInput.frameID;
  return false;
}

void SeaGullAuto::KeyboardPilot(AIBrain *unit, float deltaT )
{
  InputInterface *input = &GInput; // TODO: get proper input interface

  if (KeyboardUsedInFrame()) return;

  _pilotSpeed[1]=0; // maintain height
  _pilotSpeed[0]=0; // no side slip

  float forward = 0;

  {
    Vector3Val direction=FutureVisualState().Direction();
    float turn = 0;
    float aimX(0), aimY(0);
    _mouseDive = 0;
    if (!GWorld->HasMap())
    {
      // TODO: Introduce Object::Zoom
      if (unit && unit->LookAroundEnabled())
      {
        // aim and turn with aiming actions
        aimX = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UAAimRight, UAAimLeft);
        aimY = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UAAimUp, UAAimDown);
      }
      else
        _headYRotWanted = 0.0f;
      //moving actions should be processed even in LookAroundEnabled mode 
      //actions should differ, or read exclusively
      static float seagullMouseHeadingCoef=0.8f;
      turn = (GInput.GetAction(UAHeliRudderRight)-GInput.GetAction(UAHeliRudderLeft))*2;
      float trn = GInput.GetActionWithCurve(InputInterface::Curve1Mouse,UAHeliCyclicRight,UAHeliCyclicLeft);
      float mouseLR = (GInput.GetAction(UAHeliRight)-GInput.GetAction(UAHeliLeft))*Input::MouseRangeFactor;
      turn += (trn + mouseLR)*seagullMouseHeadingCoef;
      saturate(turn,-3,3);
      forward = GInput.GetAction(UASeagullForward)*0.5*Input::MouseRangeFactor
        +GInput.GetActionSeagullFastForward()
        -GInput.GetAction(UASeagullBack)*0.25*Input::MouseRangeFactor;
      //mouse fwd/back affects dive
      _mouseDive = GInput.GetActionWithCurve(InputInterface::Curve1Mouse, UAAimUp, UAAimDown);
    }
    float diveChangeWanted = 0;
    extern void GetHeadLookAroundActions(InputInterface *input, float deltaT, float &headingChangeWanted, float &diveChangeWanted);
    GetHeadLookAroundActions(input, deltaT, _headYRotWantedCont, diveChangeWanted);
    _headXRotWanted += diveChangeWanted;
    //if (_headXRotWanted!=0 || _headYRotWanted+_headYRotWantedCont!=0) LogF("HeadXRotWanted=%.2f HeadYRotWanted=%.2f", _headXRotWanted, _headYRotWanted+_headYRotWantedCont);
    MoveHead(deltaT, aimX, aimY);

    _pilotHeading=atan2(direction[0],direction[2])+turn;

    if (fabs(turn)>0.001)
    {
      _pilotHeading=atan2(direction[0],direction[2])+turn;
      _pilotHeadingSet = false;
    }
    else if (!_pilotHeadingSet)
    {       
      _pilotHeadingSet = true;
      // estimate direction in advance
      // player expects some inertia

      float estT = 0.2f;
      Matrix3Val orientation=FutureVisualState().Orientation();
      Matrix3Val derOrientation=_angVelocity.Tilda()*orientation;
      Matrix3Val estOrientation=orientation+derOrientation*estT;
      Vector3 estDirection=estOrientation.Direction();

      _pilotHeading=atan2(estDirection[0],estDirection[2]);       
    }
  }

  _pilotSpeed[2] = forward * 15;  
  _pilotSpeed[0]=5*(GInput.GetAction(UAHeliCyclicRight)-GInput.GetAction(UAHeliCyclicLeft))*Input::MouseRangeFactor;
  float mouseLR = (GInput.GetAction(UAHeliLeft)-GInput.GetAction(UAHeliRight))*Input::MouseRangeFactor;
  _pilotSpeed[0] += 5*mouseLR;

  Vector3Val position=FutureVisualState().Position();
  float surfaceY = GLandscape->SurfaceY(FutureVisualState().Position() + VUp * 0.01f);

  float curHeight = position.Y()-surfaceY;
  if( GInput.GetAction(UASeagullUp))
  {
    if( !_pressedUp ) _pilotHeight=curHeight,_pressedUp=true;
    float sgUp = 10*GInput.GetAction(UASeagullUp)*Input::MouseRangeFactor; 
    //deadzone (mouse is analog now)
    static float treshold = 1.0f;
    saturateMax(sgUp,treshold); sgUp -=treshold;
    saturateMin(sgUp,1.5f);
    _pilotHeight+=deltaT*10*sgUp;
  }
  else
  {
    if( _pressedUp ) _pilotHeight=curHeight+FutureVisualState()._speed[1]*1.0,_pressedUp=false;
  }
  if( GInput.GetAction(UASeagullDown))
  {
    if( !_pressedDown ) _pilotHeight=curHeight,_pressedDown=true;
    float sgDown = 10*GInput.GetAction(UASeagullDown)*Input::MouseRangeFactor;
    //deadzone (mouse is analog now)
    static float treshold = 1.0f;
    saturateMax(sgDown,treshold); sgDown -=treshold;
    saturateMin(sgDown,1.5f);
    _pilotHeight-=deltaT*10*sgDown;
  }
  else
  {
    if( _pressedDown ) _pilotHeight=curHeight+FutureVisualState()._speed[1]*1.0,_pressedDown=false;
  }
  if( GInput.GetActionToDo(UAFire))
  {
    if (_nextCreek>Glob.time+1)
      _nextCreek=Glob.time+0.1;
  }
  //float positionY=GLOB_LAND->SurfaceYAboveWater(position[0],position[2]);
  Limit(_pilotSpeed[2],-0.5,+20);
  float noland = floatMax(FutureVisualState().ModelSpeed().Size(),_pilotSpeed.Size())*0.33-1;
  saturateMin(noland,2);
  Limit(_pilotHeight,-0.05+noland,250);
  if (_pilotHeight<0.2 && curHeight<0.5 )
  {
    _rpmWanted = 0;
    _pilotSpeed = VZero;
  }
  else if (_pilotHeight>0.4 && _rpmWanted<0.5 )
  {
    _rpmWanted = 1;
    _pilotSpeed = Vector3(0,0,1);
  }
  
}


void SeaGullAuto::DrawDiags()
{
  if (!CHECK_DIAG(DEAmbient)) return;
  #define DRAW_OBJ(obj) GScene->DrawObject(obj)
  {
    Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),VISITOR_NO_ID);
    float scale=0.5;
    obj->SetPosition(_setPars._camPos);
    Color color(0,1,1);
    obj->SetScale(scale);
    obj->SetConstantColor(PackedColor(color));
    DRAW_OBJ(obj);
  }
  // draw pilot heading
  LODShape *forceArrow=GScene->ForceArrow();
  {
    Matrix3 rot(MRotationY,-_pilotHeading);
    Vector3Val dir = rot.Direction();
    Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);

    float size=0.1;
    arrow->SetPosition(FutureVisualState().Position());
    arrow->SetOrient(dir,VUp);
    arrow->SetPosition(arrow->FutureVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size));
    arrow->SetScale(size);
    arrow->SetConstantColor(PackedColor(Color(1,1,0,0.5)));

    DRAW_OBJ(arrow);
  }
}

bool SeaGullAuto::PrepareShapeDrawMatrices(Matrix4Array &matrices, const ObjectVisualState &vs, LODShape *shape, int level)
{
  const MovesTypeBase *movesType = GetMovesType();
  if (movesType)
  {
    BlendAnims blend;
    _moves.GetBlendAnims(vs.cast<SeaGullAuto>()._moves, blend,movesType);
    _moves.PrepareMatrices(movesType, matrices, shape, level, blend);
  }
  else
    base::PrepareShapeDrawMatrices(matrices, vs, shape, level);
  return true;
}

void SeaGullAuto::MakeLanded()
{
  _wingPhase=0.5;
  _wingBase=1;
  _mainRotor=_mainRotorWanted=0;
  _cyclicAsideWanted=0;
  _cyclicForwardWanted=0;
  _wingDive=_wingDiveWanted=0;
  _thrust=_thrustWanted=0;
  _rpm=_rpmWanted=0;
  _state=SeaGullAutopilotReached;
  _pilotHeight=0;
  _pilotSpeed=VZero;
}

void SeaGull::QuickStart()
{
  _wingPhase=0;
  _wingBase=0;
  _mainRotor=_mainRotorWanted=1.5;
  _cyclicAsideWanted=0;
  _cyclicForwardWanted=0;
  _wingDive=_wingDiveWanted=0;
  _thrust=_thrustWanted=0;
  // quickstart
  _mainRotor=_mainRotorWanted=2;
}

void SeaGullAuto::MakeAirborne( float height, bool playerIn )
{
  _wingPhase=0;
  _wingBase=0;
  _mainRotor=_mainRotorWanted=1.5;
  _cyclicAsideWanted=0;
  _cyclicForwardWanted=0;
  _pilotHeight = height;
  _wingDive=_wingDiveWanted=0;
  _thrust=_thrustWanted=0;
  _rpm=_rpmWanted=1;
}

void SeaGullAuto::Command( RString mode )
{
  if( !strcmpi(mode,"landed") )
    MakeLanded();
  else if( !strcmpi(mode,"airborne") )
    MakeAirborne(_pilotHeight, false);
  else
    base::Command(mode);
}

void SeaGullAuto::Commit( float time )
{
  // CameraHolder implementation
  // set movement target
}
void SeaGullAuto::CommitPrepared( float time )
{
}


#define SEAGULL_MSG_LIST(XX) \
  XX(Create, CreateSeagull) \
  XX(UpdateGeneric, UpdateSeagull) \
  XX(UpdatePosition, UpdatePositionSeagull)

DEFINE_NETWORK_OBJECT(SeaGullAuto, base, SEAGULL_MSG_LIST)

#define CREATE_SEAGULL_MSG(MessageName, XX) \
  XX(MessageName, int, player, NDTInteger, int, NCTNone, DEFVALUE(int, 0), DOC_MSG("ID of attached player"), TRANSF)

DECLARE_NET_INDICES_EX(CreateSeagull, CreateVehicle, CREATE_SEAGULL_MSG)
DEFINE_NET_INDICES_EX(CreateSeagull, CreateVehicle, CREATE_SEAGULL_MSG)

#define UPDATE_SEAGULL_MSG(MessageName, XX) \
  XX(MessageName, Vector3, pilotSpeed, NDTVector, Vector3, NCTNone, DEFVALUE(Vector3, VZero), DOC_MSG("Wanted speed"), TRANSF, ET_ABS_DIF, 1) \
  XX(MessageName, float, pilotHeading, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Wanted heading"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, pilotHeight, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Wanted height"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, int, state, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, 0), DOC_MSG("Autopilot state"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE)

DECLARE_NET_INDICES_EX_ERR(UpdateSeagull, UpdateVehicle, UPDATE_SEAGULL_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateSeagull, UpdateVehicle, UPDATE_SEAGULL_MSG, NoErrorInitialFunc)

#define UPDATE_POSITION_SEAGULL_MSG(MessageName, XX) \
  XX(MessageName, float, rpmWanted, NDTFloat, float, NCTFloat0To2, DEFVALUE(float, 0), DOC_MSG("Wanted RPM"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, mainRotorWanted, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Wanted main rotor state"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, cyclicForwardWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted forward cyclic"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, cyclicAsideWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted aside cyclic"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, wingDiveWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted wing dive"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, thrustWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted thrust"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR)

DECLARE_NET_INDICES_EX_ERR(UpdatePositionSeagull, UpdatePositionVehicle, UPDATE_POSITION_SEAGULL_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdatePositionSeagull, UpdatePositionVehicle, UPDATE_POSITION_SEAGULL_MSG, NoErrorInitialFunc)

NetworkMessageFormat &SeaGullAuto::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCCreate:
    base::CreateFormat(cls, format);
    CREATE_SEAGULL_MSG(CreateSeagull, MSG_FORMAT)
    break;
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_SEAGULL_MSG(UpdateSeagull, MSG_FORMAT_ERR)
    break;
  case NMCUpdatePosition:
    base::CreateFormat(cls, format);
    UPDATE_POSITION_SEAGULL_MSG(UpdatePositionSeagull, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

SeaGullAuto *SeaGullAuto::CreateObject(NetworkMessageContext &ctx)
{
  Entity *veh = Entity::CreateObject(ctx);
  SeaGullAuto *seagull = dyn_cast<SeaGullAuto>(veh);
  if (!seagull) return NULL;
  seagull->TransferMsg(ctx);
  return seagull;
}

TMError SeaGullAuto::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCCreate:
    if (ctx.IsSending())
    {
      TMCHECK(base::TransferMsg(ctx))
    }
    {
      PREPARE_TRANSFER(CreateSeagull)
      TRANSF(player)
    }
    break;
  case NMCUpdateGeneric:
    {
      TMCHECK(base::TransferMsg(ctx))

      PREPARE_TRANSFER(UpdateSeagull)

      TRANSF(pilotSpeed)
      TRANSF(pilotHeading)
      TRANSF(pilotHeight)
      TRANSF_ENUM(state)
    }
    break;
  case NMCUpdatePosition:
    {
      TMCHECK(base::TransferMsg(ctx))

      PREPARE_TRANSFER(UpdatePositionSeagull)

      TRANSF(rpmWanted)
      TRANSF(mainRotorWanted)
      TRANSF(cyclicForwardWanted)
      TRANSF(cyclicAsideWanted)
      TRANSF(wingDiveWanted)
      TRANSF(thrustWanted)
    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

float SeaGullAuto::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    {
      error += base::CalculateError(ctx);

      PREPARE_TRANSFER(UpdateSeagull)

      ICALCERR_DIST(pilotSpeed, 1)
      ICALCERR_ABSDIF(float, pilotHeading, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, pilotHeight, ERR_COEF_VALUE_MAJOR)
      ICALCERR_NEQ(int, state, ERR_COEF_MODE)
    }
    break;
  case NMCUpdatePosition:
    {
      error +=  base::CalculateError(ctx);
      
      PREPARE_TRANSFER(UpdatePositionSeagull)

      ICALCERR_ABSDIF(float, rpmWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, mainRotorWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, cyclicForwardWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, cyclicAsideWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, wingDiveWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, thrustWanted, ERR_COEF_VALUE_MAJOR)
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}
