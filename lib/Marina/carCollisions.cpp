// Poseidon - includes
#include "../wpch.hpp"
#include "../AI/ai.hpp"
#include "../car.hpp"

#include "carCollisions.hpp"

const float MAXIMAL_DYN_SPEED = 0.000001f;

const unsigned int MAX_ALLOWED_POINTS = 9;

void CarCollisionsContactResolver::Init( marina_real fSimulationTime,
                                         EntityAI * pcBody1,
                                         const CollisionBuffer& cCollBuffer,
                                         const unsigned int iFrom,
                                         const unsigned int iTo)
{

  Assert(pcBody1->IsLocal());
  Assert(iFrom <= iTo);
  Assert(pcBody1 != NULL);
  Assert(fSimulationTime > 0.0f);

  _fSimulationTime = fSimulationTime;    

  Object * obj = (cCollBuffer[iFrom].hierLevel == 0) ? cCollBuffer[iFrom].object : cCollBuffer[iFrom].parentObject;

  EntityAI * pcBody2 = dyn_cast<EntityAI,Object>(obj);   

  float fRestitutionCoef = pcBody1->RigidityCoef();
  _fNormalizedMass = pcBody1->GetMass();
  if (pcBody2)
  {    
    fRestitutionCoef += pcBody2->RigidityCoef();
    fRestitutionCoef /= 2;
    //_fNormalizedMass += pcBody2->GetMass();
    //_fNormalizedMass /=2; // TODO: put here min
    _fNormalizedMass = _fNormalizedMass > pcBody2->GetMass() ? pcBody2->GetMass() : _fNormalizedMass;
  }    

  if (obj->Static())
  {
    pcBody2 = NULL;
  }

  _nNumberOfBodies = 2;

  // Following trick is very dangerouse !!!!!. 
  // It can be applied just because we know, that body will  be futher processed by penetration forces.
  // If there is too much points, we select some arbitrary representants. 
  // That means we are not sure that bodies are going away after collision.
  // It is here for optimalization.

  unsigned int nStep = 1;
  unsigned int nSteps = (iTo - iFrom);
  unsigned int nMod = 0;

  if ((iTo - iFrom) > MAX_ALLOWED_POINTS)
  {
    nStep = (iTo - iFrom) / MAX_ALLOWED_POINTS;
    nSteps = MAX_ALLOWED_POINTS;
    nMod = (iTo - iFrom) % MAX_ALLOWED_POINTS;
  }

  _cContactPoints.Init(nSteps);
  for(unsigned int i = 0; i < nSteps; i++)
  {
    const CollisionInfo &info = cCollBuffer[iFrom + i * nStep + (i + 1) * nMod / nSteps];
    
    Vector3 tWorldPos = info.pos; 
    Vector3 tWorldDirOut = info.dirOut;

    int j = 0;
    for(; j < _cContactPoints.Size(); j++)
    {
      const CC_ContactPoint& tContactPoint2 = _cContactPoints[j];
      if (tWorldDirOut * tContactPoint2.dir[0] < -0.8)
      {
        Vector3 cDiff = tWorldPos - tContactPoint2.pos;
        if ((cDiff.CrossProduct(tWorldDirOut)).SquareSize() < 1.01)           
          break;            
      }
    }

    if (j != _cContactPoints.Size())
      continue;

    CC_ContactPoint& tContactPoint = _cContactPoints.Append();
    memset(&tContactPoint, 0x0, sizeof(tContactPoint));

    tContactPoint.fRestitutionCoef = fRestitutionCoef;
#define FRICTION_COEF 0.4f
    tContactPoint.fSlidingFrictionCoef = FRICTION_COEF;

    tContactPoint.pcBody1 = pcBody1;
    tContactPoint.pcBody2 = pcBody2;
    tContactPoint.pcObjBody2 = obj;
    tContactPoint.pos = tWorldPos;
    tContactPoint.dir[0] = tWorldDirOut;

    // Find pedestal axes
    tContactPoint.dir[0].Normalize();
    tContactPoint.dir[1] = tContactPoint.dir[0].CrossProduct( Vector3(1,0,0));
    if (tContactPoint.dir[1].Size() < 0.1f) // is new vector correct
      tContactPoint.dir[1] = tContactPoint.dir[0].CrossProduct(Vector3(0,1,0));

    tContactPoint.dir[2] = tContactPoint.dir[0].CrossProduct(tContactPoint.dir[1]);

    tContactPoint.dir[1].Normalize();
    tContactPoint.dir[2].Normalize();

    //_cContactPoints.Add(tContactPoint);
  }
}

int CarCollisionsContactResolver::Initialize(BOOL &bNothingToDo)
{	
  unsigned int nNumberOfPoints =_cContactPoints.Size();
  _nNumberOfForces = 3 * nNumberOfPoints;

  _pcColumnHeaders = new AutoArray<TCOLUMNHEADER>(_nNumberOfForces);

  for(unsigned int i = 0; i < nNumberOfPoints; i++)
  {
    //_cContactPedestals[0].piActiveIndexes[i] = i;

    TCOLUMNHEADER& tHeaderNormal = (*_pcColumnHeaders).Append();
    TCOLUMNHEADER& tHeaderFriction1 = (*_pcColumnHeaders).Append();
    TCOLUMNHEADER& tHeaderFriction2 = (*_pcColumnHeaders).Append();

    memset(&tHeaderNormal, 0x0, sizeof(tHeaderNormal));
    memset(&tHeaderFriction1, 0x0, sizeof(tHeaderFriction1));
    memset(&tHeaderFriction2, 0x0, sizeof(tHeaderFriction2));

    /*tHeaderFriction2.iContactPedestalID = tHeaderFriction1.iContactPedestalID = 
    tHeaderNormal.iContactPedestalID = 0;*/
    tHeaderFriction2.iContactPointID = tHeaderFriction1.iContactPointID = 
      tHeaderNormal.iContactPointID = i;


    tHeaderNormal.eForceType = EFT_STATIC;		
    tHeaderNormal.iMatrixAIndex = 3 * i;
    tHeaderNormal.iSourceMatrixAIndex = 3 * i;
    tHeaderNormal.iDirIndex = 0;
    tHeaderNormal.pBrotherForcesHeaders[0] = &tHeaderFriction1;
    tHeaderNormal.pBrotherForcesHeaders[1] = &tHeaderFriction2;


    tHeaderFriction1.eForceType = EFT_FRICTION;		
    tHeaderFriction1.iMatrixAIndex = 3 * i + 1;
    tHeaderFriction1.iSourceMatrixAIndex = 3 * i + 1;
    tHeaderFriction1.iDirIndex = 1;
    tHeaderFriction1.pBrotherForcesHeaders[0] = &tHeaderNormal;
    tHeaderFriction1.pBrotherForcesHeaders[1] = &tHeaderFriction2;

    tHeaderFriction2.eForceType = EFT_FRICTION;			
    tHeaderFriction2.iMatrixAIndex = 3 * i + 2;
    tHeaderFriction2.iSourceMatrixAIndex = 3 * i + 2;
    tHeaderFriction2.iDirIndex = 2;
    tHeaderFriction2.pBrotherForcesHeaders[0] = &tHeaderNormal;
    tHeaderFriction2.pBrotherForcesHeaders[1] = &tHeaderFriction1;

    const CC_ContactPoint &tContact1 = _cContactPoints[i];

    tHeaderFriction2.fSlidingFrictionCoef = tHeaderFriction1.fSlidingFrictionCoef = 
      tHeaderNormal.fSlidingFrictionCoef = tContact1.fSlidingFrictionCoef;
  }

  //// create vector B and Ball
  if(MARINA_FAILURE ==_cVectorB.Init(_nNumberOfForces))
    return MARINA_FAILURE;
  //_cVectorBAll.Init(3 * _cContactPoints.Size());

  bNothingToDo = TRUE;
  //float fMaxSpeed = 0;
  for(unsigned int i = 0; i < nNumberOfPoints; i++)
  {
    const CC_ContactPoint &tContact = _cContactPoints[i];
    Vector3 cSpeed;
    Vector3 cSpeedTemp;

    if (tContact.pcBody1 != NULL)
    {
      cSpeedTemp = tContact.pcBody1->SpeedAtPoint(tContact.pos);
      cSpeed = cSpeedTemp;
    }

    if (tContact.pcBody2 != NULL)
    {			
      cSpeedTemp = tContact.pcBody2->SpeedAtPoint(tContact.pos);
      cSpeed -= cSpeedTemp;
    }

    //if (cSpeed.Size() > fMaxSpeed)
    //    fMaxSpeed = cSpeed.Size();

    for(unsigned int ii = 0; ii < 3; ii++)
    {
      //_cVectorBAll[3 * i + ii] = _cVectorB[3 * i + ii] = cContact.dir[ii]*(cAccel); 

      marina_real fSpeed = tContact.dir[ii]*(cSpeed);
      if (ii == 0 )
      {
        // estimate outgoing speed 
        marina_real fEpsilon = 1.0f + tContact.fRestitutionCoef;
        if(fSpeed >= 0.0f)
        {
          _cVectorB[3*i] = fSpeed; // am I right ?
        }
        else
        {
          // ingoing speed
          bNothingToDo = FALSE;

          //if (fSpeed > -0.10f)
          //    fEpsilon = 1.0f; 

          _cVectorB[3 * i] = fSpeed * fEpsilon;
        }

        //_cVectorBAll[3 * i] = _cVectorB[3 * i];
      }
      else
      {
        //_cVectorBAll[3 * i + ii] = fSpeed ;
        _cVectorB[3 * i + ii] = fSpeed ;
      }
    }

    // Check if contacs are dynamical.

    if (_cVectorB[3 * i + 1] * _cVectorB[3 * i + 1] + _cVectorB[3 * i + 2] * _cVectorB[3 * i + 2] > MAXIMAL_DYN_SPEED) // if dynamical contact;
    {
      Vector3 cDir(_cVectorB[3 * i + 1],_cVectorB[3 * i + 2],0);
      cDir.Normalize();

      (*_pcColumnHeaders)[3 * i + 1].cStartDir = cDir;
      (*_pcColumnHeaders)[3 * i + 2].cStartDir = cDir;
    }
  }

  if (bNothingToDo)
    return MARINA_OK;

  _cMatrixA.Init(_nNumberOfForces);

  for(unsigned int i = 0; i < nNumberOfPoints; i++)
  {           
    const CC_ContactPoint &tContact1 = _cContactPoints[i];
    if (tContact1.pcBody1 != NULL)
    {            
      for(unsigned int j = 0; j < nNumberOfPoints; j++)
      {
        const CC_ContactPoint &tContact2 = _cContactPoints[j];

        if (tContact1.pcBody1 == tContact2.pcBody1 || tContact1.pcBody1 == tContact2.pcBody2)
        {

          Vector3 cAccel;

          for(unsigned int jj = 0; jj < 3; jj++)
          {
            Vector3 dir = tContact2.dir[jj] * (tContact1.pcBody1 == tContact2.pcBody1 ? 1.0f : -1.0f);
            cAccel = tContact1.pcBody1->DiffAccelerationAtPointOnForceAtPoint(tContact1.pos, 
              dir, tContact2.pos) * _fNormalizedMass;

            for(unsigned int ii = 0; ii < 3; ii++)
            {

              _cMatrixA.AddMemberValue(3 * i + ii, 3 * j + jj, cAccel * tContact1.dir[ii]);

              //	if (3 * i + ii != 3 * j + jj)
              //		_cMatrixA.SetMemberValue(3 * j + jj, 3 * i + ii, cAccel * tContact1.dir[ii]);
            }
          }
        } 
      }
    }

    if (tContact1.pcBody2 != NULL)
    {

      for(unsigned int j = 0; j < nNumberOfPoints; j++)
      {
        const CC_ContactPoint &tContact2 = _cContactPoints[j];

        if (tContact1.pcBody2 == tContact2.pcBody1 || tContact1.pcBody2 == tContact2.pcBody2)
        {

          Vector3 cAccel;

          for(unsigned int jj = 0; jj < 3; jj++)
          {
            Vector3 dir = tContact2.dir[jj] * (tContact1.pcBody2 == tContact2.pcBody1 ? 1.0f : -1.0f);
            cAccel = tContact1.pcBody2->DiffAccelerationAtPointOnForceAtPoint(tContact1.pos, 
              dir, tContact2.pos) * _fNormalizedMass;

            for(unsigned int ii =  0; ii < 3; ii++)
            {
              _cMatrixA.AddMemberValue(3 * i + ii, 3 * j + jj, -cAccel * tContact1.dir[ii]);
              //	if (3 * i + ii != 3 * j + jj)
              //		_cMatrixA.SetMemberValue(3 * j + jj, 3 * i + ii, -cAccel * tContact1.dir[ii]);
            }
          }
        }
      }
    } 

  }

  // Desingularizace
  /*
  for(unsigned int i = 0; i < _nNumberOfForces/3; i++)
  {
  _cMatrixA.AddMemberValue(3 * i + 1,3 * i + 1,- DESINGULATOR);
  _cMatrixA.AddMemberValue(3 * i + 2,3 * i + 2,- DESINGULATOR);
  }*/

  //_cMatrixA = _cGroup.GetMatrix();
  _cSourceMatrixA = _cMatrixA;

  //// Prepare forces and accelerations
  if (MARINA_FAILURE == _cForces.Init(_nNumberOfForces))
    return MARINA_FAILURE;

  if (MARINA_FAILURE == _cAccelerations.Init(_nNumberOfForces))
    return MARINA_FAILURE;

  //// Prepare mapping
  _cMappingToColumnID.Clear();
  _cMappingToColumnID.Init(_nNumberOfForces);
  for(unsigned int i = 0; i < _nNumberOfForces; i++)
  {
    PTCOLUMNHEADER pTemp = &(_pcColumnHeaders->operator[](i));
    _cMappingToColumnID.Add(pTemp);
  }
  return MARINA_OK;
}

void CarCollisionsContactResolver::ApplyCalculatedForces(const Vector<marina_real>& cForces)
{
  /// The calculated impulses will be applied.
  /// The objects will be damaged according to impulse. 
  /// "owner" of dammage are objects itself, just for static objects it is second object.


  Vector3 cAveragePoint = VZero;
  Vector3 cAppliedImpulse = VZero;
  Vector3 cTorqueBody1 = VZero;
  CC_ContactPoint &tPoint = _cContactPoints[0]; // I assume that there just two bodies.
  Vector3 cCenterBody1(VFastTransform,tPoint.pcBody1->FutureVisualState().ModelToWorld(),tPoint.pcBody1->GetCenterOfMass()); 

  //Vector3 cCenterBody2 = VZero;

  _fAppliedImpulse = 0;
  for(unsigned int iForce = 0; iForce < _nNumberOfForces; iForce++)
  {                
    if (cForces[iForce] != 0)
    {             
      cForces[iForce] *= _fNormalizedMass;
      const TCOLUMNHEADER &tColumn = *(_cMappingToColumnID[iForce]);
      CC_ContactPoint &tPoint = _cContactPoints[tColumn.iContactPointID];

      _fAppliedImpulse += fabsf(cForces[iForce]);
      cAveragePoint += fabsf(cForces[iForce]) * tPoint.pos;

      Vector3 cForce = tPoint.dir[tColumn.iDirIndex] * (cForces[iForce]);
      cAppliedImpulse += cForce;
      cTorqueBody1 += (tPoint.pos - cCenterBody1).CrossProduct(cForce);

      /*           
      if (tPoint.pcBody1 != NULL)
      {
      //cForce = tPoint.dir[tColumn.iDirIndex] * (cForces[iForce]);            
      tPoint.pcBody1->ApplyImpulseAtPoint(cForce, tPoint.pos);
      }
      else
      {
      Assert(FALSE);
      }

      if (tPoint.pcBody2 != NULL)
      {
      //cForce = -tPoint.dir[tColumn.iDirIndex] * (cForces[iForce]); 
      //tPoint.pcBody2->AddImpulseNet
      tPoint.pcBody2->ApplyImpulseAtPoint(-cForce, tPoint.pos);
      } 
      */
    }
  }

  if (_fAppliedImpulse == 0)
    return;

  // ApplyImpulse    

  cAveragePoint /= _fAppliedImpulse;      
  _fAppliedImpulse = cAppliedImpulse.Size(); 

  Vector3 modelPos;

  // Do damage according to impulses.

  if (tPoint.pcBody1 != NULL)
  {
    // pcBody1 is always local
    tPoint.pcBody1->PositionWorldToModel(modelPos, cAveragePoint);
    tPoint.pcBody1->ApplyImpulseNetAware(cAppliedImpulse, cTorqueBody1);      
    tPoint.pcBody1->DamageOnImpulse(tPoint.pcBody1, _fAppliedImpulse, modelPos);
  }

  if (tPoint.pcBody2 != NULL)
  {
    Vector3 cCenterBody2(VFastTransform,tPoint.pcBody2->FutureVisualState().ModelToWorld(),tPoint.pcBody2->GetCenterOfMass()); 
    Vector3 cTorqueBody2 = -(cTorqueBody1 + (cCenterBody1 - cCenterBody2).CrossProduct(cAppliedImpulse));
    tPoint.pcBody2->PositionWorldToModel(modelPos, cAveragePoint);

    if (!tPoint.pcBody2->IsLocal() && dyn_cast<Car, EntityAI>(tPoint.pcBody2)) 
    {
      // use ApplyImpulseNetAware only to wake up - divide by 8
      tPoint.pcBody2->ApplyImpulseNetAware(-cAppliedImpulse/8, cTorqueBody2/8);  
      tPoint.pcBody2->ApplyImpulse(-cAppliedImpulse, cTorqueBody2);
    }
    else
    {
      // we collide with something else but car
      // ApplyImpulseNetAware handles active objects which receive impulses
      tPoint.pcBody2->ApplyImpulseNetAware(-cAppliedImpulse, cTorqueBody2);
      // DamageOnImpulse handles static objects (buildings)
      tPoint.pcBody2->DamageOnImpulse(tPoint.pcBody2, _fAppliedImpulse, modelPos);
      
      // only active objects should be processed here
      // local non-car:
      // ApplyImpulseNetAware calls ApplyImpulse, which applies impulse and does no damage
      // DamageOnImpulse handles damage
      
      // remote active objects:
      // ApplyImpulseNetAware transfers impulse as AddImpulse
      // DamageOnImpulse does nothing
      
      
    }
  }
  else
  {
    // only object, not EntityAI - damage it accordingly
    modelPos = Vector3(VFastTransform,tPoint.pcObjBody2->WorldInvTransform(tPoint.pcObjBody2->RenderVisualState()),tPoint.pos);
    
    // damage both local and remote objects
    
    Object::DoDamageResult result;
    tPoint.pcObjBody2->SimulateImpulseDamage(result,NULL,_fAppliedImpulse, modelPos);
    tPoint.pcObjBody2->ApplyDoDamage(result,NULL,RString(),SkinnedPosition());
  }

}

void CarCollisionsResolverMaxImpulse::Init( marina_real fSimulationTime,
                                            EntityAI * pcBody1,
                                            const CollisionBuffer& _pcCollBuffer,
                                            const unsigned int iFrom,
                                            const unsigned int iTo,
                                            float fMaxImpulse)
{
  _fMaxImpulse = fMaxImpulse;    

  CarCollisionsContactResolver::Init(fSimulationTime, pcBody1, _pcCollBuffer, iFrom, iTo);
}

void CarCollisionsResolverMaxImpulse::ApplyCalculatedForces(const Vector<marina_real>& cForces)
{

  /// Limit impulse to MaxImpulseValue
  _fAppliedImpulse = 0;
  for(unsigned int iForce = 0; iForce < _nNumberOfForces; iForce++)
  {
    cForces[iForce] *= _fNormalizedMass;
    _fAppliedImpulse += fabsf(cForces[iForce]);
  }

  float fImpulseCoef = 1;
  if (_fAppliedImpulse > _fMaxImpulse)
  {   
    fImpulseCoef = _fMaxImpulse / _fAppliedImpulse;
    _fAppliedImpulse = _fMaxImpulse;        
  }

  Vector3 cAveragePoint = VZero;
  for(unsigned int iForce = 0; iForce < _nNumberOfForces; iForce++)
  {
    if (cForces[iForce] != 0)
    {     
      const TCOLUMNHEADER &tColumn = *(_cMappingToColumnID[iForce]);
      CC_ContactPoint &tPoint = _cContactPoints[tColumn.iContactPointID];

      cForces[iForce] *= fImpulseCoef;
      cAveragePoint += fabsf(cForces[iForce]) * tPoint.pos;            

      Vector3 cForce;
      Vector3 cForceModel;
      Vector3 modelPos;
      if (tPoint.pcBody1 != NULL)
      {
        cForce = tPoint.dir[tColumn.iDirIndex] * (cForces[iForce]);            
        tPoint.pcBody1->ApplyImpulseAtPoint(cForce, tPoint.pos);                    
      }
      else
      {
        Assert(FALSE);
      }

      if (tPoint.pcBody2 != NULL)
      {
        cForce = -tPoint.dir[tColumn.iDirIndex] * (cForces[iForce]); 
        tPoint.pcBody2->ApplyImpulseAtPoint(cForce, tPoint.pos); 
      }        
    }
  }

  if (_fAppliedImpulse == 0)
    return;

  cAveragePoint /= _fAppliedImpulse;

  Vector3 modelPos;

  // Do damage according to impulses.
  CC_ContactPoint &tPoint = _cContactPoints[0]; // I assume that there just two bodies...

  if (tPoint.pcBody1 != NULL)
  {
    tPoint.pcBody1->PositionWorldToModel(modelPos, cAveragePoint);
    tPoint.pcBody1->DamageOnImpulse(tPoint.pcBody1, _fAppliedImpulse, modelPos);
  }
}
