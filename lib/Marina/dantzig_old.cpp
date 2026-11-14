#include "../wpch.hpp"

//#include "stdafx.h"
#include "marina_old.hpp"
#include "dantzig_old.hpp"
//#include "bodyGroup.h"
//#include <drawstuff/drawstuff.h>
//#include "f2c.h"

//#define DRAW_MARINA_FORCES
#undef DRAW_MARINA_FORCES

//#define min(a,b) ((a) < (b) ? (a) : (b))

const float DESINGULATOR = 0.0000001f;

unsigned long g_iDebugID = 1;
unsigned long g_iStopID = 0;

#define NUMERICAL_ERROR 0.00001f

__forceinline bool IsNumericalZero(float fValue)
{
  return ((fValue <= NUMERICAL_ERROR) && (fValue >= -NUMERICAL_ERROR));
}

// cLapack function: It solves Ax = y, for symetric semidefinite matrix 
/*
extern "C" 
{
int sposv_(char *uplo, integer *n, integer *nrhs, real *a, 
integer *lda, real *b, integer *ldb, integer *info);

int sgelss_(integer *m, integer *n, integer *nrhs, real *a, 
integer *lda, real *b, integer *ldb, real *s, real *rcond, integer *
rank, real *work, integer *lwork, integer *info);

int sgels_(char *trans, integer *m, integer *n, integer *
nrhs, real *a, integer *lda, real *b, integer *ldb, real *work, 
integer *lwork, integer *info);

int sgesv_(integer *n, integer *nrhs, real *a, integer *lda, 
integer *ipiv, real *b, integer *ldb, integer *info);

int dposv_(char *uplo, integer *n, integer *nrhs, doublereal 
*a, integer *lda, doublereal *b, integer *ldb, integer *info);

int dgelss_(integer *m, integer *n, integer *nrhs, 
doublereal *a, integer *lda, doublereal *b, integer *ldb, doublereal *
s, doublereal *rcond, integer *rank, doublereal *work, integer *lwork,
integer *info);

int dgels_(char *trans, integer *m, integer *n, integer *
nrhs, doublereal *a, integer *lda, doublereal *b, integer *ldb, 
doublereal *work, integer *lwork, integer *info);
}*/


void SolveNxN(unsigned int n, unsigned int lda, const float * pMatrix, float * pVector);
void SolveNxNOld(float * pVector);
void SolvingDone();
//void SerializeColumnHeader(FILE *fArchive, TCOLUMNHEADER& tColumnHeader, BOOL bSave);


__forceinline marina_real m_sqr(marina_real x)
{
  return x*x;
}

// Solves ax^2 + b^x + c = 0
// returns the lowest nonegative solution in x1. If does not exist any it returns a negative value.
// x2 is the second nonnegative solution. If does not exist any it returns a negative value.

void QvadruSolve(marina_real &x1, marina_real &x2, marina_real a, marina_real b, marina_real c)
{
  if (a == 0.0f)
  {
    x1 = -c/b;
    x2 = -1.0f;
    return;
  }

  marina_real fDisc = b*b - 4*a*c;
  if (fDisc < 0)
  {
    x1 = -1.0f;
    x2 = -1.0f;
    return;
  }

  fDisc = sqrtf(fDisc);

  x1 = (-b - fDisc) / (2 * a);
  x2 = (-b + fDisc) / (2 * a);

  marina_real x = umin<marina_real>(x1,x2);
  if (x != x1)
  {
    x2 = x1;
    x1 = x;
  }

  return;
}

BOOL DantzigAlgorithm::ProjectBIntoA()
{ 
  return FALSE;
}

BOOL DantzigAlgorithm::ResolveFirstCollision()
{
  marina_real fMaxAccel = 0.0f;
  unsigned int iMaxAccelIndex = 0;
  for(unsigned int i = 0; i < _nNumberOfForces; i++)
  {
    marina_real fNewAccel = _cVectorB[i];

    const TCOLUMNHEADER& tColumnHeader = *_cMappingToColumnID[i];
    if (tColumnHeader.eForceType != EFT_FRICTION)
    {
      // normal force
      if (fNewAccel < 0.0f && fMaxAccel > fNewAccel)
      {
        fMaxAccel = fNewAccel;
        iMaxAccelIndex = i;       
      }
    } 
  }

  if (IsNumericalZero(fMaxAccel))
  {
    //  Immediately terminate, nothing to do
    return FALSE;
  }

  SwitchCollisions(0, iMaxAccelIndex);

  _cAccelerations[0] = 0.0f; 

  // clamped
  _cForces[0] = -_cVectorB[0]/ (*_cMatrixA.GetPointer());

  _pnLastIndexInZone[EMZ_CLAMPED] = 1;
  _pnLastIndexInZone[EMZ_NOTCLAMPED] = 1;
  _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] = 1;    
  _pnLastIndexInZone[EMZ_SOLVING] = 1;
  //_pnLastIndexInZone[EMZ_FRICTIONCONST] = 1;
  _pnLastIndexInZone[EMZ_UNSOLVED] = _nNumberOfForces;
  _pnLastIndexInZone[EMZ_FRICTIONFROMNOTCLAMPED] = _nNumberOfForces;
  _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED2] = _nNumberOfForces;

  return TRUE;  
}

/*
void DantzigAlgorithm::BackUp()
{
_cMatrixABackUp = _cMatrixA;
_cForcesBackUp = _cForces;
_cAccelerationsBackUp = _cAccelerations;
_cMappingToColumnIDBackUp = _cMappingToColumnID;
memcpy(_pnLastIndexInZoneBackUp, _pnLastIndexInZone, sizeof(*_pnLastIndexInZoneBackUp) * EMZ_NUMBER);    
}

void DantzigAlgorithm::Restore()
{
_cMatrixA = _cMatrixABackUp;
_cForces = _cForcesBackUp;
_cAccelerations = _cAccelerationsBackUp;
_cMappingToColumnID = _cMappingToColumnIDBackUp;
memcpy(_pnLastIndexInZone, _pnLastIndexInZoneBackUp, sizeof(*_pnLastIndexInZoneBackUp) * EMZ_NUMBER);   
}*/


BOOL DantzigAlgorithm::ResolveNextCollision()
{
  /// Calculate acceleration at new collision
  /// Let's do pivoting and take the biggest one (It is not clear that it will help)
  /// The first it chooses all normal forces. When all of them are solved it starts with 
  /// friction forces.

  marina_real fMaxAccelNormal = 0.0f;
  marina_real fMaxAccelTangential = 0.0f;
  unsigned int iMaxAccelNormalIndex = 0;
  unsigned int iMaxAccelTangentialIndex = 0;

  ASSERT(_pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] == _pnLastIndexInZone[EMZ_SOLVING]);

  for(unsigned int i = _pnLastIndexInZone[EMZ_SOLVING]; i < _pnLastIndexInZone[EMZ_UNSOLVED]; i++)
  {
    const TCOLUMNHEADER& tColumnHeader = *_cMappingToColumnID[i];
    if (tColumnHeader.iNotUseInGlobalCycle < _iGlobalCycle) // TODO: If one of the friction was used refuse both
    {
      // was not moved into unsolved in last run
      marina_real fNewAccel = _cMatrixA.RowTimesVector(i, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) +                 
        _cVectorB[i];

      if (tColumnHeader.eForceType != EFT_FRICTION )
      {
        // normal force
        if (fNewAccel < 0.0f && fMaxAccelNormal > fNewAccel)
        {
          fMaxAccelNormal = fNewAccel;
          iMaxAccelNormalIndex = i;       
        }
      }
      else
      {

        unsigned int iFriction2Index = tColumnHeader.pBrotherForcesHeaders[1]->iMatrixAIndex ;
        if (iFriction2Index > i || iFriction2Index < _pnLastIndexInZone[EMZ_SOLVING])
        {                
          // is normal force clamped ?
          unsigned int iNormalForceIndex = tColumnHeader.pBrotherForcesHeaders[0]->iMatrixAIndex;
          if (iNormalForceIndex < _pnLastIndexInZone[EMZ_CLAMPED] && _cForces[iNormalForceIndex] > 0.0f)
          {
            marina_real fWholeAccel = fNewAccel * fNewAccel;
            if (iFriction2Index >= _pnLastIndexInZone[EMZ_SOLVING])
            {
              float fAccel2 = _cMatrixA.RowTimesVector(iFriction2Index, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) +                                
                _cVectorB[iFriction2Index];

              fWholeAccel += fAccel2 * fAccel2;
            }

            if (fabs(fMaxAccelTangential) < fabs(fWholeAccel)) // I am searching for negative one
            {
              fMaxAccelTangential = fWholeAccel;
              iMaxAccelTangentialIndex = i;
            }
          }
        }
      }
    }   
  }


  //DebugPoint();
  //LogEx("---------------Debug Point %d-----------------------",g_iDebugID);

#ifdef _DEBUG
  VerifySituation();
#endif

  fMaxAccelTangential = sqrtf(fMaxAccelTangential);
  if (IsNumericalZero(fMaxAccelNormal/10.0f) && (IsNumericalZero(fMaxAccelTangential/100.0f) || _iGlobalCycle >  _nNumberOfForces * 3 / 2 || 
    _pnLastIndexInZone[EMZ_CLAMPED] >= _nNumberOfBodies * 6 - 6) ) // 6 degrees of freedom per freebody.
  {
    //  Immediately terminate

    /*
    for(unsigned int iIndex = _pnLastIndexInZone[EMZ_CLAMPED]; iIndex < _pnLastIndexInZone[EMZ_NOTCLAMPED]; iIndex++)
    {
    if (_cMappingToColumnID[iIndex]->eForceType == EFT_STATIC && ChooseNewPedestalPoint(iIndex))
    { 
    return TRUE;
    }
    }

    for(unsigned int iIndex = _pnLastIndexInZone[EMZ_CLAMPED]; iIndex < _pnLastIndexInZone[EMZ_NOTCLAMPED]; iIndex++)
    {
    if (_cMappingToColumnID[iIndex]->eForceType == EFT_STATIC && ChooseNewPedestalPoint(iIndex))
    { 
    return TRUE;
    }
    }*/

    /*LogEx("qwerty: _iGlobalCycle %d, clamped %d, %f, Calculate diffs %d, %f, AverageMatrixSize %d, _iLineDep %d, %f", _iGlobalCycle, _pnLastIndexInZone[EMZ_CLAMPED], _iGlobalCycle / (float) _pnLastIndexInZone[EMZ_CLAMPED],
      _iCalledCalculateDiffs, _iCalledCalculateDiffs / (float) _pnLastIndexInZone[EMZ_CLAMPED],
      _iMatrixSize / (_iCalledCalculateDiffs == 0 ? 1 : _iCalledCalculateDiffs),
      _iLineDep, _iLineDep / (float) _pnLastIndexInZone[EMZ_CLAMPED]);*/
    return FALSE;
  }

  //  _nActual = _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED];

  //ASSERT(_pnLastIndexInZone[EMZ_CLAMPED] <= 6);
  //ASSERT(nNormalClamped <= 3);
  unsigned int nActual;
  if (!IsNumericalZero(fMaxAccelNormal/10.0f))
  {   
    //  SwitchCollisions(_nActual, iMaxAccelNormalIndex);
    //ASSERT(nNormalClamped < 3);

    nActual = iMaxAccelNormalIndex;
    _cAccelerations[iMaxAccelNormalIndex] = fMaxAccelNormal; //_cMatrixA.RowTimesVector( _nActual, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) + _cVectorB[_nActual];
  }
  else
  {
    //SwitchCollisions(_nActual, iMaxAccelTangentialIndex); 
    nActual = iMaxAccelTangentialIndex;


    // Check if the axes can be changed
    TCOLUMNHEADER& tHeaderFriction1 =  *(_cMappingToColumnID[iMaxAccelTangentialIndex]);
    const TCOLUMNHEADER& tHeaderFriction2 =  *(tHeaderFriction1.pBrotherForcesHeaders[1]);

    if (tHeaderFriction2.iMatrixAIndex >= _pnLastIndexInZone[EMZ_SOLVING])
    {
      BOOL  bUseAccelDir = TRUE; 
      if (tHeaderFriction1.cStartDir != VZero)
      {
        bUseAccelDir = FALSE;
        if (tHeaderFriction1.iDirIndex == 1)
          ChangeFrictionAxes(tHeaderFriction1, tHeaderFriction1.cStartDir);
        else
          ChangeFrictionAxes(tHeaderFriction1, Vector3(tHeaderFriction1.cStartDir[1], tHeaderFriction1.cStartDir[0],0));

        _cAccelerations[iMaxAccelTangentialIndex] = _cMatrixA.RowTimesVector(iMaxAccelTangentialIndex, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) +                     
          _cVectorB[iMaxAccelTangentialIndex];

        //float fTemp = _cMatrixA.RowTimesVector(tHeaderFriction2.iMatrixAIndex, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) +                     
        //   _cVectorB[tHeaderFriction2.iMatrixAIndex];

        float fTemp1 = _cAccelerations[iMaxAccelTangentialIndex]/fMaxAccelTangential;
        if (fabsf(fTemp1) < 0.4)
          bUseAccelDir = TRUE;
      }

      if (bUseAccelDir)
      {
        // change friction direction into accelerations direction 
        marina_real fAccel1 = _cMatrixA.RowTimesVector(tHeaderFriction1.iMatrixAIndex, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) +                    
          _cVectorB[tHeaderFriction1.iMatrixAIndex];
        marina_real fAccel2 = _cMatrixA.RowTimesVector(tHeaderFriction2.iMatrixAIndex, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) +                    
          _cVectorB[tHeaderFriction2.iMatrixAIndex];

        Vector3 cNewDir(fAccel1,fAccel2,0);
        cNewDir.Normalize();

        ChangeFrictionAxes(tHeaderFriction1, cNewDir);
        _cAccelerations[iMaxAccelTangentialIndex] = _cMatrixA.RowTimesVector(iMaxAccelTangentialIndex, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) +                    
          _cVectorB[iMaxAccelTangentialIndex];

#ifdef _DEBUG
        marina_real fTemp1 = _cMatrixA.RowTimesVector(tHeaderFriction1.iMatrixAIndex, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) 
          + _cVectorB[tHeaderFriction1.iMatrixAIndex];
        marina_real fTemp2 = _cMatrixA.RowTimesVector(tHeaderFriction2.iMatrixAIndex, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) 
          + _cVectorB[tHeaderFriction2.iMatrixAIndex];
        //variables never used - only intended for debugging
        (void)fTemp1,(void)fTemp2;
        //int iii = 8;
#endif

        //  ASSERT(IsNumericalZero(fMaxAccelTangential - (_cMatrixA.RowTimesVector(tHeaderFriction1.iMatrixAIndex, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) 
        //     + _cVectorB[tHeaderFriction1.iMatrixAIndex])));
      }
    }
    else
      _cAccelerations[iMaxAccelTangentialIndex] = _cMatrixA.RowTimesVector(iMaxAccelTangentialIndex, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) +                
      _cVectorB[iMaxAccelTangentialIndex];

  }

  //unsigned int iActualColumnID = _cMappingToColumnID[_nActual];
  /*
  _nActual = _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED];
  _cAccelerations[_nActual] = _cMatrixA.RowTimesVector( _nActual, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) + _cVectorB[_nActual];
  */  
  //  Serialize();
  _cForces[nActual] = 0.0f;

  _cReleaseList.Clear();
  if (DriveToZero(nActual,TRUE))
  {
#ifdef _DEBUG

    IsMatrixARegular();
#endif

    for(int i = 0; i < _cReleaseList.Size(); i++)
    {
      _cReleaseList[i]->bDeadMouse = TRUE;
      DriveToZero(_cReleaseList[i]->iMatrixAIndex, FALSE);
      _cReleaseList[i]->bDeadMouse = FALSE;
    }

    return TRUE;
  }
  else
  {
#ifdef _DEBUG
    IsMatrixARegular();
#endif        
    // co ted ? umbounded found
    if (_bAllowDynamicContact)
    {
      _bAllowDynamicContact = !_bAllowDynamicContact;
      return FALSE;
    }

    // let's solve it later
    return TRUE;
  }
}

BOOL DantzigAlgorithm::DriveToZero(unsigned int iMatrixIndex, // Drive to zero  with force.
                                    BOOL bNewForce) // if FALSE it moves frictin force between unsolved
{
  _iLocalCycle++;

  if (!bNewForce)
  {
    //LogEx("DriveToZeroFalse ");
  }

  TCOLUMNHEADER& tColumnHeader = *_cMappingToColumnID[iMatrixIndex];
  TCOLUMNHEADER * ptLastMoved = NULL;

  //LogEx("DriveToZero: %d", tColumnHeader.iSourceMatrixAIndex);
  /*Log("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  LogEx("DriveToZero %s, SourceMatrixIndex = %d, MatrixIndex = %d",
    BOOL_TO_STRING(bNewForce), tColumnHeader.iSourceMatrixAIndex, tColumnHeader.iMatrixAIndex);
  Log("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");*/


  //Move(iMatrixIndex, _pnLastIndexInZone[EMZ_NOTCLAMPED]);
  MoveToZoneEx(iMatrixIndex, EMZ_SOLVING, bNewForce ? EMZ_UNSOLVED : EMZ_CLAMPED);

  BOOL bRecalculate;

  do 
  {

    //LogEx("DantzingAlgoritm: nextstep %d", _iCycle);

    bRecalculate = FALSE;

    // TOOD: better handling
    if (_pnLastIndexInZone[EMZ_SOLVING] == _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED])
    {
      if (_cForces[tColumnHeader.iMatrixAIndex] != 0.0f) // something realy happend
        _iGlobalCycle++;

      if (!bNewForce)
      {
        ASSERT(bNewForce);
        //LogEx("DriveToZeroFalse End\n");
      }

      /*Log("---------------------------------------------------------------");
      Log("DriveToZero _pnLastIndexInZone[EMZ_SOLVING] == _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]");
      Log("---------------------------------------------------------------");*/

      return TRUE;
    }

    unsigned int nActual = _pnLastIndexInZone[EMZ_SOLVING] - 1;
    Vector<marina_real> cDiffForces;
    cDiffForces.Init(_nNumberOfForces);

    Vector<marina_real> cDiffAccel;
    cDiffAccel.Init(_nNumberOfForces);

    if (bNewForce)
      CalculateDiffs(cDiffForces, cDiffAccel, _cAccelerations[nActual] < 0.0);
    else
      CalculateDiffs(cDiffForces, cDiffAccel, _cForces[nActual] < 0.0);

    //LogEx("cDiffAccel[nActual] = %f, _cAccelerations[nActual] = %f",cDiffAccel[nActual],_cAccelerations[nActual]);
    //unsigned int iNormalForceIndex = 0;

    marina_real fSNew = -1.0f;
    BOOL bFrictionClamped = TRUE;

    if (!bNewForce)
    {
      fSNew = fabsf(_cForces[nActual]);
    }
    else
    {
      if (IsNumericalZero(cDiffAccel[nActual]/1000.0f)) // Because of instability it is better to pe more sceptic.
      {

        // the new force are lineary dependent with other forces.
        // TODO: better handling
        if (_cForces[nActual] == 0.0f)
        {
          // just try another one
          MoveToZoneEx(tColumnHeader.iMatrixAIndex, EMZ_UNSOLVED, EMZ_SOLVING);
          tColumnHeader.iNotUseInGlobalCycle = _iGlobalCycle;

          _iLineDep++;

          // Linearize condition
          marina_real fTemp = _cAccelerations[nActual];
          (void) fTemp;
          _cVectorB[nActual] -= _cAccelerations[nActual];
          _cAccelerations[nActual] = 0.0f;

          /*Log("---------------------------------------------------------------");
          LogEx("DriveToZero IsNumericalZero(cDiffAccel[nActual]/100.0), cAccel[nActual] = %f", fTemp);
          Log("---------------------------------------------------------------");*/
          return TRUE;
        }
        else
        {
          ASSERT(bNewForce);

          _iLocalCycle++;
          ASSERT(ptLastMoved != NULL)
            if (ptLastMoved->iMatrixAIndex < _pnLastIndexInZone[EMZ_CLAMPED])
            {


              // move it back between unclamped
              if (ptLastMoved->eForceType == EFT_STATIC)
              {
                MoveToZoneEx(ptLastMoved->iMatrixAIndex, EMZ_NOTCLAMPED, EMZ_CLAMPED);
              }
              else
              {
                MoveToZoneEx(ptLastMoved->iMatrixAIndex, EMZ_FRICTIONNOTCLAMPED, EMZ_CLAMPED);
                MoveToZoneEx(ptLastMoved->pBrotherForcesHeaders[1]->iMatrixAIndex, EMZ_FRICTIONNOTCLAMPED2);
                ASSERT(!ptLastMoved->pBrotherForcesHeaders[1]->bDeadMouse);

                _cMatrixA.CombinateColumns(ptLastMoved->pBrotherForcesHeaders[0]->iMatrixAIndex,ptLastMoved->iMatrixAIndex,
                  ptLastMoved->fSlidingFrictionCoef);
                _bLUDecompositionValid = FALSE;

                ptLastMoved->iNotClampedIndex = ptLastMoved->iDirIndex;
                ptLastMoved->pBrotherForcesHeaders[1]->iNotClampedIndex = 
                  ptLastMoved->pBrotherForcesHeaders[0]->iNotClampedIndex = ptLastMoved->iNotClampedIndex;         

              }

              /// change _cVectorB and move it into clamped

              //marina_real fTemp = _cAccelerations[nActual];
              _cVectorB[nActual] -= _cAccelerations[nActual];
              _cAccelerations[nActual] = 0.0f;

              MoveToZoneEx(nActual, EMZ_CLAMPED, EMZ_SOLVING);

              _iGlobalCycle++;

              /*Log("---------------------------------------------------------------");
              LogEx("DriveToZero IsNumericalZero(cDiffAccel[nActual]/100.0),_cForces[nActual] != 0.0f cAccel[nActual] = %f",fTemp);
              Log("---------------------------------------------------------------");*/
              return TRUE;
            }
        }
      }

      if (tColumnHeader.eForceType == EFT_FRICTION)
      {
        /// Step to make friction contact clamped
        fSNew = - _cAccelerations[nActual] / cDiffAccel[nActual];
        /*  if (IsNumericalZero(_cAccelerations[nActual]/10.0f) && IsNumericalZero(cDiffAccel[nActual]))
        {
        _cAccelerations[nActual] = 0.0f;
        MoveToZoneEx(nActual, EMZ_CLAMPED, EMZ_SOLVING);

        Log("DriveToZero: End");
        return TRUE;
        }       
        */      
        if (fSNew < 0.0f /*|| cDiffAccel[nActual] == 0.0f*/)
        {

          if (!_bAllowDynamicContact && _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] == _pnLastIndexInZone[EMZ_NOTCLAMPED])
          {
            // Numerical problem, solve it later
            ASSERT(FALSE);
            //Log("DriveToZero: End FALSE");
            Log("---------------------------------------------------------------");
            Log("nDriveToZero !_bAllowDynamicContact && _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] == _pnLastIndexInZone[EMZ_NOTCLAMPED]");
            Log("---------------------------------------------------------------");
            return FALSE;
          }

          fSNew = -1.0f;
        }

        // The normal force must be clamped. Calculate the step till friction reaches maximum value.
        ASSERT(tColumnHeader.fSlidingFrictionCoef >= 0.0f);

        TCOLUMNHEADER & tColumnHeaderNormal = *(_cMappingToColumnID[nActual]->pBrotherForcesHeaders[0]);
        ASSERT(tColumnHeaderNormal.iNotClampedIndex == 0);

        marina_real fS;
        CalculateMaximalAllowedStepForFrictionPoint(fS, tColumnHeaderNormal, cDiffForces);

        fSNew = umin<marina_real>(fSNew, fS);
        bFrictionClamped = (fS != fSNew); 

      }
      else
      {
        if (cDiffAccel[nActual]  <= 0.0f) 
        {

          if (!_bAllowDynamicContact && _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] == _pnLastIndexInZone[EMZ_NOTCLAMPED])
          {
            // Numerical problem
            ASSERT(FALSE);
            //Log("DriveToZero: End FALSE");
            Log("---------------------------------------------------------------");
            Log("DriveToZero !_bAllowDynamicContact && _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] == _pnLastIndexInZone[EMZ_NOTCLAMPED]");
            Log("---------------------------------------------------------------");
            return FALSE;
          }

          fSNew = -1.0f;
        }
        else
          fSNew = - _cAccelerations[nActual] / cDiffAccel[nActual];
      }
    }

    // Find if any collision need change between C <-> NC
    marina_real fSMin = -1.0f;
    unsigned int iCollision = _nNumberOfForces + 1;

    /*bRecalculate = */CalculateMaximalAllowedStep( fSMin, iCollision, cDiffForces, cDiffAccel);

    //LogEx("CalculateMaximalAllowedStep; fSMin = %f, iCollision = %d", fSMin, iCollision);

    if (fSMin < 0.0f && fSNew < 0.0f)
    { 
      // Dynamic case, unbound case
      //ASSERT(fSMin >= 0.0f || fSNew >= 0.0f);
      tColumnHeader.iNotUseInGlobalCycle = _iGlobalCycle + 1; //BEWARE It is more than needed
      _cReleaseList.Add(&tColumnHeader);

      //Log("DriveToZero: End FALSE");
      //Log("---------------------------------------------------------------");
      //Log("DriveToZero fSMin < 0.0f && fSNew < 0.0f");
      //Log("---------------------------------------------------------------");
      return TRUE;
    }
    //float fSNewBackUp = fSNew;

    if (fSNew != umin<marina_real>(fSNew, fSMin))
    {

      fSNew = fSMin;
      /*
      if ((iCollision < _pnLastIndexInZone[EMZ_CLAMPED] || !IsNumericalZero((_cAccelerations[nActual] + fSMin * cDiffAccel[nActual])/100.0f) || 
      (fSNew < 0.0f && fSMin >= 0.0f)) || !bNewForce)
      //if (fSNew != umin<marina_real>(fSNew, fSMin))
      fSNew = fSMin;
      else
      iCollision = _nNumberOfForces + 1;*/

    }                
    else
      iCollision = _nNumberOfForces + 1;


    if (fSNew != 0.0f)
    {

      _iLocalCycle++;
      // Drive to zero by coefficient fSNew
      // Clamped
      for (unsigned i = 0; i < _pnLastIndexInZone[EMZ_CLAMPED]; i++)
        _cForces[i] += fSNew * cDiffForces[i];

      for (unsigned i = _pnLastIndexInZone[EMZ_NOTCLAMPED]; i < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]; i++)
        _cForces[i] += fSNew * cDiffForces[i];


      // Not clamped
      for(unsigned i = _pnLastIndexInZone[EMZ_CLAMPED]; i < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED];i++)
        _cAccelerations[i] += fSNew * cDiffAccel[i];

      _cForces[nActual] += fSNew * cDiffForces[nActual];
      _cAccelerations[nActual] += fSNew * cDiffAccel[nActual];
    }

    if (iCollision > _nNumberOfForces )
    {     
      if (!bNewForce)
      {
        ASSERT(IsNumericalZero(_cForces[nActual]));
        _cForces[nActual] = 0.0f;
        //LogEx("DriveToZeroFalse End\n");

        DriveToZeroReleaseEnd(tColumnHeader);

        /*Log("---------------------------------------------------------------");
        Log("DriveToZeroFalse End, !bNewForce");
        Log("---------------------------------------------------------------");*/
        return TRUE;
      }

      return DriveToZeroEnd(tColumnHeader,bFrictionClamped);      
    }
    else
    {           
      bRecalculate = TRUE;
      // A contact need to be moved C <-> NC  


      TCOLUMNHEADER& tColumnHeaderMove = *(_cMappingToColumnID[iCollision]);
      ptLastMoved = _cMappingToColumnID[iCollision];

      //LogEx("Change collision: %d, %d",tColumnHeaderTemp.iSourceMatrixAIndex, tColumnHeaderTemp.iLastChangeCycle);

      if ((fSNew == 0.0f) && (tColumnHeaderMove.iLastChangeLocalCycle == _iLocalCycle/* - 1*/))
      {
        // force must be moved between unsolved   

        //unsigned int iTemp = tColumnHeaderMove.iMatrixAIndex;
        DriveToZeroRelease(tColumnHeaderMove);

        /*Log("---------------------------------------------------------------");
        LogEx("!DriveToZeroRelease(tColumnHeader) iMatrixAIndex = %d, iMatrixAIndexAfter = %d", iTemp, tColumnHeaderMove.iMatrixAIndex);
        Log("---------------------------------------------------------------");*/
      }
      else
      {
        DriveToZeroCFromToNC( tColumnHeader, tColumnHeaderMove);
      }     
    }

  } while (bRecalculate);

  //Log("DriveToZero: End");
  /*LogEx("DriveToZeroFalse WrongEnd\n");
  Log("---------------------------------------------------------------");
  Log("DriveToZero");
  Log("---------------------------------------------------------------");*/
  return TRUE;  
}

BOOL DantzigAlgorithm::DriveToZeroEnd(TCOLUMNHEADER &tColumnHeader, BOOL bFrictionClamped)
{
  if (_cForces[tColumnHeader.iMatrixAIndex] != 0.0f) // something realy happend
    _iGlobalCycle++;  

  // made contact fully something
  if (tColumnHeader.eForceType == EFT_FRICTION)
  { 
    unsigned int iNormalForceIndex = tColumnHeader.pBrotherForcesHeaders[0]->iMatrixAIndex;
    if (iNormalForceIndex < _pnLastIndexInZone[EMZ_CLAMPED])
    {       
      if (!bFrictionClamped)
      {
        // move it into "friction not clamped". 
        // determine clamped direction
        TCOLUMNHEADER & tHeaderBrotherFriction = * tColumnHeader.pBrotherForcesHeaders[1];
        ASSERT (tHeaderBrotherFriction.iMatrixAIndex < _pnLastIndexInZone[EMZ_NOTCLAMPED] || 
          tHeaderBrotherFriction.iMatrixAIndex >= _pnLastIndexInZone[EMZ_SOLVING]);
        ASSERT(!tHeaderBrotherFriction.bDeadMouse);

        // make friction not clamped
        TCOLUMNHEADER & tHeaderBrotherNormal = * tColumnHeader.pBrotherForcesHeaders[0];

        ASSERT(tHeaderBrotherNormal.iMatrixAIndex  < _pnLastIndexInZone[EMZ_CLAMPED]);               

        Vector3 cNewDir(0,0,0);

        cNewDir[0] = _cForces[tColumnHeader.iMatrixAIndex];
        cNewDir[1] = _cForces[tHeaderBrotherFriction.iMatrixAIndex];

        cNewDir.Normalize();

        ChangeFrictionAxes( tColumnHeader, cNewDir);                

        tColumnHeader.iNotClampedIndex = tColumnHeader.iDirIndex;
        tHeaderBrotherFriction.iNotClampedIndex = tColumnHeader.iDirIndex ;
        tHeaderBrotherNormal.iNotClampedIndex = tColumnHeader.iDirIndex;

        MoveToZoneEx(tColumnHeader.iMatrixAIndex, EMZ_FRICTIONNOTCLAMPED,EMZ_SOLVING);
        MoveToZoneEx(tHeaderBrotherFriction.iMatrixAIndex,EMZ_FRICTIONNOTCLAMPED2);

        _cMatrixA.CombinateColumns(tHeaderBrotherNormal.iMatrixAIndex,tColumnHeader.iMatrixAIndex,
          tHeaderBrotherNormal.fSlidingFrictionCoef);

        _bLUDecompositionValid = FALSE;



        _cForces[tHeaderBrotherFriction.iMatrixAIndex] = 0.0f;
        _cAccelerations[tHeaderBrotherFriction.iMatrixAIndex] = _cMatrixA.RowTimesVector(tHeaderBrotherFriction.iMatrixAIndex, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) +
          _cMatrixA.RowTimesVector(tHeaderBrotherFriction.iMatrixAIndex, _cForces, _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED], _pnLastIndexInZone[EMZ_SOLVING]) +
          _cVectorB[tHeaderBrotherFriction.iMatrixAIndex];                
      }
      else
      { 
        // move it into clamped                   
        ASSERT(IsNumericalZero(_cAccelerations[tColumnHeader.iMatrixAIndex]/100.0f));
        _cAccelerations[tColumnHeader.iMatrixAIndex] = 0.0f;
        MoveToZoneEx(tColumnHeader.iMatrixAIndex, EMZ_CLAMPED, EMZ_SOLVING);                    
      } 
      //Log("---------------------------------------------------------------");
      //Log("DriveToZero End");
      //Log("---------------------------------------------------------------");

      return TRUE;
    }

    if (iNormalForceIndex < _pnLastIndexInZone[EMZ_NOTCLAMPED])
    {
      ASSERT(IsNumericalZero(_cForces[tColumnHeader.iMatrixAIndex]));
      _cForces[tColumnHeader.iMatrixAIndex] = 0.0f;
      MoveToZoneEx(tColumnHeader.iMatrixAIndex, EMZ_FRICTIONFROMNOTCLAMPED, EMZ_SOLVING);
      //Log("---------------------------------------------------------------");
      //Log("DriveToZero End");
      //Log("---------------------------------------------------------------");

      return TRUE;
    }

    //if (iNormalForceIndex >= _pnLastIndexInZone[EMZ_NOTCLAMPED])
    {
      ASSERT(IsNumericalZero(_cForces[tColumnHeader.iMatrixAIndex]));
      _cForces[tColumnHeader.iMatrixAIndex] = 0.0f;
      MoveToZoneEx(tColumnHeader.iMatrixAIndex, EMZ_UNSOLVED, EMZ_SOLVING);
      Log("---------------------------------------------------------------");
      Log("DriveToZero End");
      Log("---------------------------------------------------------------");

      return TRUE;
    }
  }
  else
  {
    // normal forces
    // made contact fully clamped

    _cAccelerations[tColumnHeader.iMatrixAIndex] = 0.0f;
    MoveToZoneEx(tColumnHeader.iMatrixAIndex,EMZ_CLAMPED, EMZ_SOLVING);       

    //Log("---------------------------------------------------------------");
    //Log("DriveToZero End");
    //Log("---------------------------------------------------------------");

    return TRUE;
  }
}

void DantzigAlgorithm::DriveToZeroCFromToNC(TCOLUMNHEADER &tColumnHeaderActual, TCOLUMNHEADER &tColumnHeaderMove)
{
  switch(tColumnHeaderMove.eForceType)
  {
  case EFT_STATIC:
    {
      _cAccelerations[tColumnHeaderMove.iMatrixAIndex] = 0.0f;
      _cForces[tColumnHeaderMove.iMatrixAIndex] = 0.0f;
      // Normal forces
      if (tColumnHeaderMove.iMatrixAIndex < _pnLastIndexInZone[EMZ_CLAMPED])
      {
        // C -> NC  
        MoveToZoneEx(tColumnHeaderMove.iMatrixAIndex, EMZ_NOTCLAMPED, EMZ_CLAMPED);

        //ChooseNewPedestalPoint(tColumnHeaderTemp.iMatrixAIndex);
      }
      else
      {
        // NC -> C        
        MoveToZoneEx(tColumnHeaderMove.iMatrixAIndex, EMZ_CLAMPED, EMZ_NOTCLAMPED);           
      }     
      break;
    }
  case EFT_FRICTION:
    {
      if (tColumnHeaderMove.iMatrixAIndex < _pnLastIndexInZone[EMZ_CLAMPED])
      {
        // move it into NOTCLAMPED
        TCOLUMNHEADER & tHeaderBrotherNormal = * tColumnHeaderMove.pBrotherForcesHeaders[0];
        TCOLUMNHEADER & tHeaderBrotherFriction = * tColumnHeaderMove.pBrotherForcesHeaders[1];

        ASSERT(tHeaderBrotherNormal.iMatrixAIndex  < _pnLastIndexInZone[EMZ_CLAMPED]);
        ASSERT(tHeaderBrotherFriction.iMatrixAIndex  < _pnLastIndexInZone[EMZ_CLAMPED] || 
          tHeaderBrotherFriction.iMatrixAIndex >= _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]);

        ASSERT(!tHeaderBrotherNormal.bDeadMouse);
        ASSERT(!tHeaderBrotherFriction.bDeadMouse);


        Vector3 cNewDir(0,0,0);

        cNewDir[0] = _cForces[tColumnHeaderMove.iMatrixAIndex];  
        cNewDir[1] = _cForces[tHeaderBrotherFriction.iMatrixAIndex];

        cNewDir.Normalize();

        ChangeFrictionAxes( tColumnHeaderMove, cNewDir);                

        tColumnHeaderMove.iNotClampedIndex = tColumnHeaderMove.iDirIndex;
        tHeaderBrotherFriction.iNotClampedIndex = tColumnHeaderMove.iDirIndex ;
        tHeaderBrotherNormal.iNotClampedIndex = tColumnHeaderMove.iDirIndex;

        MoveToZoneEx(tColumnHeaderMove.iMatrixAIndex, EMZ_FRICTIONNOTCLAMPED,EMZ_CLAMPED);

        MoveToZoneEx(tHeaderBrotherFriction.iMatrixAIndex,EMZ_FRICTIONNOTCLAMPED2);

        _cMatrixA.CombinateColumns(tHeaderBrotherNormal.iMatrixAIndex,tColumnHeaderMove.iMatrixAIndex,
          tHeaderBrotherNormal.fSlidingFrictionCoef);

        _bLUDecompositionValid = FALSE;

        _cAccelerations[tColumnHeaderMove.iMatrixAIndex] = 0.0f;

        ASSERT(IsNumericalZero(_cForces[tHeaderBrotherFriction.iMatrixAIndex]));
        _cForces[tHeaderBrotherFriction.iMatrixAIndex] = 0.0f;                                


        //_cAccelerations[tHeaderBrotherFriction.iMatrixAIndex] = _cMatrixA.RowTimesVector(tHeaderBrotherFriction.iMatrixAIndex, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) +
        //    _cMatrixA.RowTimesVector(tHeaderBrotherFriction.iMatrixAIndex, _cForces, _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED], _pnLastIndexInZone[EMZ_SOLVING]) +
        //    _cVectorB[tHeaderBrotherFriction.iMatrixAIndex]; 
        // The value of acceleration is unimportant.


        break;
      }

      //if (iCollision < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED])
      {
        ASSERT(tColumnHeaderMove.iMatrixAIndex < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] || tColumnHeaderMove.iMatrixAIndex >= _pnLastIndexInZone[EMZ_NOTCLAMPED]);

        // move it into CLAMPED
        TCOLUMNHEADER & tHeaderBrotherNormal = * tColumnHeaderMove.pBrotherForcesHeaders[0];
        TCOLUMNHEADER & tHeaderBrotherFriction = * tColumnHeaderMove.pBrotherForcesHeaders[1];


        ASSERT(tHeaderBrotherNormal.iMatrixAIndex  < _pnLastIndexInZone[EMZ_CLAMPED]);                
        ASSERT(tHeaderBrotherFriction.iMatrixAIndex  >= _pnLastIndexInZone[EMZ_FRICTIONFROMNOTCLAMPED]);

        ASSERT(!tHeaderBrotherNormal.bDeadMouse);
        ASSERT(!tHeaderBrotherFriction.bDeadMouse);

        RevertColumnInMatrixA(tHeaderBrotherNormal.iMatrixAIndex);

        // Create clamped friction force in the direction of accel, when the actual point get clamped
        tColumnHeaderMove.iNotClampedIndex = 0;
        tHeaderBrotherFriction.iNotClampedIndex = 0;
        tHeaderBrotherNormal.iNotClampedIndex = 0;  

        MoveToZoneEx(tColumnHeaderMove.iMatrixAIndex, EMZ_CLAMPED, EMZ_FRICTIONNOTCLAMPED);
        ASSERT(IsNumericalZero(_cAccelerations[tColumnHeaderMove.iMatrixAIndex]/10000));
        _cAccelerations[tColumnHeaderMove.iMatrixAIndex] = 0.0f;

        //LogEx("_cForces[tColumnHeaderMove.iMatrixAIndex] = %f,_cForces[tHeaderBrotherNormal.iMatrixAIndex] = %f", _cForces[tColumnHeaderMove.iMatrixAIndex],_cForces[tHeaderBrotherNormal.iMatrixAIndex]  );
        ASSERT(_cForces[tColumnHeaderMove.iMatrixAIndex] >= 0.0f || IsNumericalZero(_cForces[tColumnHeaderMove.iMatrixAIndex] / 100.0f));

        _cForces[tColumnHeaderMove.iMatrixAIndex] = _cForces[tHeaderBrotherNormal.iMatrixAIndex] * tHeaderBrotherNormal.fSlidingFrictionCoef;

        _cForces[tHeaderBrotherFriction.iMatrixAIndex] = 0.0f;
        /* if (_cForces[tHeaderBrotherFriction.iMatrixAIndex] != 0.0f)
        MoveToZoneEx(tHeaderBrotherFriction.iMatrixAIndex, EMZ_FRICTIONCONST, EMZ_FRICTIONNOTCLAMPED);
        else*/
        MoveToZoneEx(tHeaderBrotherFriction.iMatrixAIndex, EMZ_UNSOLVED, EMZ_FRICTIONNOTCLAMPED2);
      }

      break;
    }
  default:
    ASSERT(FALSE);
  }
}

BOOL DantzigAlgorithm::DriveToZeroRelease(TCOLUMNHEADER &tColumnHeaderRelease)
{
  //tColumnHeaderRelease.iNotUseInGlobalCycle = _iGlobalCycle + 1;

  switch(tColumnHeaderRelease.eForceType)
  {
  case EFT_STATIC:
    {
      _cAccelerations[tColumnHeaderRelease.iMatrixAIndex] = 0.0f;
      _cForces[tColumnHeaderRelease.iMatrixAIndex] = 0.0f;
      MoveToZoneEx(tColumnHeaderRelease.iMatrixAIndex, EMZ_UNSOLVED);

      return TRUE;
      //ChooseNewPedestalPoint(tColumnHeaderMove.iMatrixAIndex);
      break;
    }
  case EFT_FRICTION:
    {
      TCOLUMNHEADER& tHeaderBrotherNormal = *(tColumnHeaderRelease.pBrotherForcesHeaders[0]);                            
      TCOLUMNHEADER & tHeaderBrotherFriction = * tColumnHeaderRelease.pBrotherForcesHeaders[1];

      ASSERT(_cForces[tHeaderBrotherFriction.iMatrixAIndex] == 0.0f);

      if (tColumnHeaderRelease.iMatrixAIndex >= _pnLastIndexInZone[EMZ_NOTCLAMPED])
      {
        ASSERT(tColumnHeaderRelease.iMatrixAIndex < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]);

        // Create force in direction of not clamped                
        RevertColumnInMatrixA(tHeaderBrotherNormal.iMatrixAIndex);                

        tColumnHeaderRelease.iNotClampedIndex = 0;
        tHeaderBrotherFriction.iNotClampedIndex = 0;
        tHeaderBrotherNormal.iNotClampedIndex = 0;                                        

        ASSERT(_cForces[tColumnHeaderRelease.iMatrixAIndex] >= 0.0f);
        _cForces[tColumnHeaderRelease.iMatrixAIndex] = _cForces[tHeaderBrotherNormal.iMatrixAIndex] * tColumnHeaderRelease.fSlidingFrictionCoef;                                
        _cForces[tHeaderBrotherFriction.iMatrixAIndex] = 0.0f;

        MoveToZoneEx(tHeaderBrotherFriction.iMatrixAIndex, EMZ_UNSOLVED, EMZ_FRICTIONNOTCLAMPED2);
        //tHeaderBrotherFriction.iNotUseInGlobalCycle = _iGlobalCycle + 1;

        MoveToZoneEx(tColumnHeaderRelease.iMatrixAIndex, EMZ_UNSOLVED, EMZ_FRICTIONNOTCLAMPED);
        //tColumnHeaderRelease.iNotUseInGlobalCycle = _iGlobalCycle + 1;            
      }
      else
      {            
        ASSERT(tColumnHeaderRelease.pBrotherForcesHeaders[1]->iMatrixAIndex >= _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] &&
          tColumnHeaderRelease.pBrotherForcesHeaders[1]->iMatrixAIndex < _pnLastIndexInZone[EMZ_FRICTIONFROMNOTCLAMPED]);

        MoveToZoneEx(tHeaderBrotherFriction.iMatrixAIndex, EMZ_UNSOLVED, EMZ_FRICTIONNOTCLAMPED);
        //tHeaderBrotherFriction.iNotUseInGlobalCycle = _iGlobalCycle + 1;

        MoveToZoneEx(tColumnHeaderRelease.iMatrixAIndex, EMZ_UNSOLVED, EMZ_CLAMPED);
        //tColumnHeaderRelease.iNotUseInGlobalCycle = _iGlobalCycle + 1;  
      }

      _cReleaseList.Add(&tColumnHeaderRelease);
      return TRUE;          
      break;

    }
  default:
    ASSERT(FALSE);                     
  }

  return TRUE;
}

void DantzigAlgorithm::DriveToZeroReleaseEnd(TCOLUMNHEADER &tColumnHeaderRelease)
{    
  if (tColumnHeaderRelease.eForceType == EFT_STATIC)
  {
    MoveToZoneEx(tColumnHeaderRelease.iMatrixAIndex, EMZ_UNSOLVED, EMZ_SOLVING);
  }
  else
  {                
    if (tColumnHeaderRelease.pBrotherForcesHeaders[0]->iMatrixAIndex >= _pnLastIndexInZone[EMZ_CLAMPED] &&
      tColumnHeaderRelease.pBrotherForcesHeaders[0]->iMatrixAIndex < _pnLastIndexInZone[EMZ_NOTCLAMPED])
      MoveToZoneEx(tColumnHeaderRelease.iMatrixAIndex, EMZ_FRICTIONFROMNOTCLAMPED, EMZ_SOLVING);
    else
      MoveToZoneEx(tColumnHeaderRelease.iMatrixAIndex, EMZ_UNSOLVED, EMZ_SOLVING);
  }

  return;
}

void DantzigAlgorithm::CalculateMaximalAllowedStepForFrictionPoint(marina_real& fSMin, const TCOLUMNHEADER &tHeaderNormal, const Vector<marina_real>& cDiffForces)
{
  // Configuration is valid if:
  // ax^2 + bx + c <= 0

  //marina_real a,b,c;
  marina_real a,b,c;
  a = b = c = 0.0f;

  marina_real fFrictionCoefSqr = m_sqr(tHeaderNormal.fSlidingFrictionCoef);
  unsigned int iIndex = tHeaderNormal.iMatrixAIndex;

  a -= m_sqr(cDiffForces[iIndex]) * fFrictionCoefSqr;
  b -= 2 * cDiffForces[iIndex] * _cForces[iIndex] * fFrictionCoefSqr;
  c -= m_sqr(_cForces[iIndex]) * fFrictionCoefSqr;

  iIndex = tHeaderNormal.pBrotherForcesHeaders[0]->iMatrixAIndex;

  if (!tHeaderNormal.pBrotherForcesHeaders[0]->bDeadMouse)
  {    
    a += m_sqr(cDiffForces[iIndex]);
    b += 2 * cDiffForces[iIndex] * _cForces[iIndex];
    c += m_sqr(_cForces[iIndex]);
  }

  iIndex = tHeaderNormal.pBrotherForcesHeaders[1]->iMatrixAIndex;

  if (!tHeaderNormal.pBrotherForcesHeaders[1]->bDeadMouse)
  { 
    a += m_sqr(cDiffForces[iIndex]);
    b += 2 * cDiffForces[iIndex] * _cForces[iIndex];
    c += m_sqr(_cForces[iIndex]);
  }

  marina_real fSMin1,fSMin2;
  QvadruSolve(fSMin1 , fSMin2, a, b, c);
  fSMin = fSMin1;



  if (fSMin >= 0.0f)
  {

    if (IsNumericalZero(fSMin/10000)) // Is it goes to or from zero ?
    {
      marina_real fTestPoint = fSMin + ( (fSMin2 < 0.0f ? 1.0f : fSMin2) - fSMin) / 4.0f; 
      // If in TestPoint configuration is correct it goes from zero.
      marina_real fFrac = 0.0f; 

      iIndex = tHeaderNormal.pBrotherForcesHeaders[1]->iMatrixAIndex;
      if (!tHeaderNormal.pBrotherForcesHeaders[1]->bDeadMouse)
      {
        fFrac += m_sqr(_cForces[iIndex] + fTestPoint * cDiffForces[iIndex]);
      }

      iIndex = tHeaderNormal.pBrotherForcesHeaders[0]->iMatrixAIndex;
      if (!tHeaderNormal.pBrotherForcesHeaders[0]->bDeadMouse)
      {
        fFrac += m_sqr(_cForces[iIndex] + fTestPoint * cDiffForces[iIndex]);
      }

      iIndex = tHeaderNormal.iMatrixAIndex;
      fFrac /= m_sqr(_cForces[iIndex] + fTestPoint * cDiffForces[iIndex]);

      if (fFrac <= fFrictionCoefSqr)
      {
        // yes it goes from zero
        fSMin = fSMin2;
      }
    }
    else
    {
      if (c > 0.0f) // situation is not valid 
        fSMin = 0.0f; 
    }
  }
  else       
  {
    if (c > 0.0f) // situation is not valid 
      fSMin = 0.0f;        
  }


}



void Solver2x2(marina_real * pMatrix, const unsigned int nDim, marina_real * pVector)
{
  if (fabs(pMatrix[0]) < fabs(pMatrix[nDim]))
  {
    pMatrix[1] = pMatrix[1] * pMatrix[nDim] - pMatrix[0] * pMatrix[nDim + 1];
    //  ASSERT(pMatrix[3] != 0,0f);


    float fTemp = pVector[1];
    pVector[1] = pVector[0] * pMatrix[nDim] - pVector[1] * pMatrix[0];

    pVector[1] /= pMatrix[1];

    pVector[0] = fTemp - pMatrix[nDim + 1] * pVector[1];
    pVector[0] /= pMatrix[nDim];
  }
  else
  {
    pMatrix[nDim + 1] = pMatrix[1] * pMatrix[nDim] - pMatrix[0] * pMatrix[nDim + 1];
    //  ASSERT(pMatrix[3] != 0,0f);

    pVector[1] = pVector[0] * pMatrix[nDim] - pVector[1] * pMatrix[0];

    pVector[1] /= pMatrix[nDim + 1];

    pVector[0] -= pMatrix[1] * pVector[1];
    pVector[0] /= pMatrix[0];
  }
}


void DantzigAlgorithm::CalculateDiffs(Vector<marina_real>& cDiffForces, Vector<marina_real>& cDiffAccel, BOOL bPositiveForce)
{     
  _iCalledCalculateDiffs++;
  _iMatrixSize += _pnLastIndexInZone[EMZ_CLAMPED];

  // Prepare rigth side
  unsigned int nActual = _pnLastIndexInZone[EMZ_SOLVING] - 1;
  _cMatrixA.GetColumn(cDiffForces, nActual, nActual + 1 /* _pnLastIndexInZone[EMZ_CLAMPED]*/);

  //const TCOLUMNHEADER& tColumnHeader = *_cMappingToColumnID[nActual];
  if (bPositiveForce)
  {
    // In case with friction and positive acceleration the friction force must be negative
    cDiffForces.Negative(nActual + 1);
  }

  // Get copy of "forcies"
  Vector<marina_real> cLastColumn;
  cLastColumn = cDiffForces;
  cLastColumn.Negative(nActual + 1);

  switch (_pnLastIndexInZone[EMZ_CLAMPED])
  {
  case 0:
    break;
  case 1: cDiffForces[0] /= _cMatrixA.GetMemberValue(0,0);
    break;
  case 2:
    { 
      marina_real * pTempMatrix = new marina_real[_cMatrixA.GetDim() * 2];

      memcpy( pTempMatrix, _cMatrixA.GetPointer(), _cMatrixA.GetDim() * 2 * sizeof(marina_real));     

      Solver2x2(pTempMatrix, _cMatrixA.GetDim(), cDiffForces.GetPointer());


      delete pTempMatrix;
    }
    break;

  default:
    {
      if(!_bLUDecompositionValid)
      {            
        SolveNxN(_pnLastIndexInZone[EMZ_CLAMPED], _cMatrixA.GetDim(), _cMatrixA.GetPointer(), cDiffForces.GetPointer());
        _bLUDecompositionValid = TRUE;
      }
      else
      {            
        SolveNxNOld(cDiffForces.GetPointer());
      }
    }

  }




  // Nonclaped forces are 0, actual force is 1.

  // Calculate diff accelerations
  _cMatrixA.MultiplyByShortVectorFromTo(cDiffAccel, cDiffForces, _pnLastIndexInZone[EMZ_CLAMPED], 
    _pnLastIndexInZone[EMZ_CLAMPED], _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] - 1);

  for(unsigned int i = _pnLastIndexInZone[EMZ_CLAMPED]; i < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]; i++)
    cDiffAccel[i] += cLastColumn[i];

  //_cMatrixA.MultiplyByShortVectorFromTo(cDiffAccel, cDiffForces, _pnLastIndexInZone[EMZ_CLAMPED], 
  //  _nActual, _nActual);

  cDiffAccel[nActual] = _cMatrixA.RowTimesVector(nActual,cDiffForces, _pnLastIndexInZone[EMZ_CLAMPED]);

  cDiffAccel[nActual] += cLastColumn[nActual];
  cDiffForces[nActual] = (bPositiveForce) ? 1.0f : -1.0f;

  // calculate DiffForces for not clamped 
  for(unsigned int i = 0; i < _pnLastIndexInZone[EMZ_CLAMPED]; i++)
  {
    const TCOLUMNHEADER &tHeaderNormal = *(_cMappingToColumnID[i]);
    if (tHeaderNormal.eForceType == EFT_STATIC && tHeaderNormal.iNotClampedIndex != 0)
    {

      ASSERT(tHeaderNormal.pBrotherForcesHeaders[tHeaderNormal.iNotClampedIndex - 1]->iMatrixAIndex >= _pnLastIndexInZone[EMZ_NOTCLAMPED] &&
        tHeaderNormal.pBrotherForcesHeaders[tHeaderNormal.iNotClampedIndex - 1]->iMatrixAIndex < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]);

      cDiffForces[tHeaderNormal.pBrotherForcesHeaders[tHeaderNormal.iNotClampedIndex - 1]->iMatrixAIndex] = 
        cDiffForces[i] * tHeaderNormal.fSlidingFrictionCoef;

    }
  }



#ifdef _DEBUG

  /*  if (_pnLastIndexInZone[EMZ_CLAMPED] > 0)
  {

  _cMatrixA.MultiplyByShortVectorFromTo(cDiffAccel, cDiffForces, _pnLastIndexInZone[EMZ_CLAMPED], 
  0,_pnLastIndexInZone[EMZ_CLAMPED] - 1);

  for(unsigned int iii = 0; iii <_pnLastIndexInZone[EMZ_CLAMPED]; iii++)
  {
  cDiffAccel[iii] += cLastColumn[iii];
  if (!IsNumericalZero(cDiffAccel[iii]/10.0f))
  {
  int i = 3;
  }
  //  ASSERT(IsNumericalZero(cDiffAccel[iii]/10.0f));
  }
  }
  */
#endif
}

BOOL DantzigAlgorithm::CalculateMaximalAllowedStep( marina_real& fSMin, unsigned int &iMinIndex, 
                                                    Vector<marina_real>& cDiffForces, Vector<marina_real>& cDiffAccel)
{

  // Clamped
  unsigned int iIndex = 0;
  for (; iIndex < _pnLastIndexInZone[EMZ_CLAMPED]; iIndex++)
  {
    TCOLUMNHEADER& tColumnHeaderTemp = *_cMappingToColumnID[iIndex];    

    marina_real fS = -1.0f;

    if (tColumnHeaderTemp.eForceType == EFT_FRICTION)
    {
      // the second friction must be also clamped or unsolved
      ASSERT(tColumnHeaderTemp.pBrotherForcesHeaders[1]->iMatrixAIndex < _pnLastIndexInZone[EMZ_CLAMPED] ||
        tColumnHeaderTemp.pBrotherForcesHeaders[1]->iMatrixAIndex >= _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]);

      // the normal force must be clamped
      ASSERT(tColumnHeaderTemp.pBrotherForcesHeaders[0]->iMatrixAIndex < _pnLastIndexInZone[EMZ_CLAMPED]);

      CalculateMaximalAllowedStepForFrictionPoint( fS, *(tColumnHeaderTemp.pBrotherForcesHeaders[0]), cDiffForces);
    }
    else
    {
      // The normal forces
      if (cDiffForces[iIndex] < 0.0f)
      {
        if (_cForces[iIndex] == 0)
        {
          // This collision was switch C<->NC last time 
          // Mark it like a unsolved, 
          fS = 0.0f;      
        }
        else
        {

          fS = -_cForces[iIndex] / cDiffForces[iIndex];
          if (fS < 0.0f)
            fS = 0.0f;
        }
      }
    }

    if ( fSMin != umin<marina_real>(fS, fSMin))
    {
      fSMin = fS;
      iMinIndex = iIndex;
    }
  }

  // Not clamped 
  for(; iIndex < _pnLastIndexInZone[EMZ_NOTCLAMPED];iIndex++)
  {
#ifdef _DEBUG
    TCOLUMNHEADER& tColumnHeaderTemp = *_cMappingToColumnID[iIndex];  
    ASSERT(tColumnHeaderTemp.eForceType == EFT_STATIC);
#endif

    // here are only normal forces
    if (cDiffAccel[iIndex] < 0.0f && !IsNumericalZero(cDiffAccel[iIndex]/100.0f))
    {
      marina_real fS = -1.0f;
      if (_cAccelerations[iIndex] == 0.0f)
      {
        // This iIndex was switched C<->NC last time 
        // Mark it like a unsolved, 
        fS = 0.0f;        
      }
      else
      {           
        fS = -_cAccelerations[iIndex] / cDiffAccel[iIndex];
        if (fS < 0.0f)
          fS = 0.0f;
      }

      if ( fSMin != umin<marina_real>(fS, fSMin))
      {
        fSMin = fS;
        iMinIndex = iIndex;
      }
    }
  }

  // Not clamped friction forces
  for(; iIndex < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED];iIndex++)
  {
#ifdef _DEBUG   
    TCOLUMNHEADER& tColumnHeaderTemp = *_cMappingToColumnID[iIndex];  
    ASSERT(tColumnHeaderTemp.eForceType == EFT_FRICTION);

    const TCOLUMNHEADER& tBrotherFrictionHeader = *(tColumnHeaderTemp.pBrotherForcesHeaders[1]);

    unsigned int iBrotherFrictionForceIndex = tBrotherFrictionHeader.iMatrixAIndex;

    ASSERT(iBrotherFrictionForceIndex >= _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]); 
#endif

    marina_real fAccelInNotClampedDir = _cAccelerations[iIndex];

    marina_real fDiffAccelInNotClampedDir = cDiffAccel[iIndex];

    marina_real fS = -1.0f; 
    if (!IsNumericalZero(fDiffAccelInNotClampedDir/100.0f) && fDiffAccelInNotClampedDir > 0.0f)
    {        
      fS = - fAccelInNotClampedDir / fDiffAccelInNotClampedDir;
      if (fS < 0.0f)
        fS =  0.0f;
    }

    if ( fSMin != umin<marina_real>(fS, fSMin))
    {
      fSMin = fS;
      iMinIndex = iIndex;
    } 
  }

  return FALSE;
}

void DantzigAlgorithm::SwitchCollisions(unsigned int iColl1, unsigned int iColl2)
{
  if (iColl1 == iColl2)
    return;

  _cMatrixA.SwitchCoordinates(iColl1, iColl2);

  marina_real fTemp = _cVectorB[iColl1];
  _cVectorB[iColl1] = _cVectorB[iColl2];
  _cVectorB[iColl2] = fTemp;

  fTemp = _cForces[iColl1];
  _cForces[iColl1] = _cForces[iColl2];
  _cForces[iColl2] = fTemp;

  fTemp = _cAccelerations[iColl1];
  _cAccelerations[iColl1] = _cAccelerations[iColl2];
  _cAccelerations[iColl2] = fTemp;

  PTCOLUMNHEADER pcTemp = _cMappingToColumnID[iColl1];
  _cMappingToColumnID[iColl1] = _cMappingToColumnID[iColl2];
  _cMappingToColumnID[iColl2] = pcTemp;

  _cMappingToColumnID[iColl1]->iMatrixAIndex = iColl1;
  _cMappingToColumnID[iColl2]->iMatrixAIndex = iColl2;
}


void DantzigAlgorithm::MoveToZoneEx(unsigned int iIndex, unsigned int iToZone, unsigned int iFromZone /*= 0*/)
{
  while( iIndex >= _pnLastIndexInZone[iFromZone])
    iFromZone ++;

  if (iFromZone == iToZone)
    return;  // trivial case

  TCOLUMNHEADER& tColumnHeader = *_cMappingToColumnID[iIndex];

 /* LogEx("MoveToZoneEx: SourceMatrixIndex = %d, MatrixIndex = %d, iFromZone = %d, iToZone = %d",
    tColumnHeader.iSourceMatrixAIndex, tColumnHeader.iMatrixAIndex, iFromZone, iToZone);*/

  switch (tColumnHeader.eForceType)
  {
  case EFT_FRICTION:
    {     
      MoveToZone(iIndex, iToZone, iFromZone);
      tColumnHeader.iLastChangeLocalCycle = _iLocalCycle;
      break;
    }
  case EFT_STATIC:
    {
      if (iFromZone == EMZ_CLAMPED)
      {


        if (tColumnHeader.iNotClampedIndex != 0)
        {
          // not clamped friction in point
          RevertColumnInMatrixA(iIndex);
          tColumnHeader.iNotClampedIndex = 0;
          tColumnHeader.pBrotherForcesHeaders[0]->iNotClampedIndex = 0;
          tColumnHeader.pBrotherForcesHeaders[1]->iNotClampedIndex = 0;                     
        }

        if (!tColumnHeader.pBrotherForcesHeaders[0]->bDeadMouse)
          _cForces[tColumnHeader.pBrotherForcesHeaders[0]->iMatrixAIndex] = 0.0f;

        if (!tColumnHeader.pBrotherForcesHeaders[1]->bDeadMouse)
          _cForces[tColumnHeader.pBrotherForcesHeaders[1]->iMatrixAIndex] = 0.0f;

        if (iToZone == EMZ_NOTCLAMPED)
        {
          if (!tColumnHeader.pBrotherForcesHeaders[0]->bDeadMouse)
            MoveToZone(tColumnHeader.pBrotherForcesHeaders[0]->iMatrixAIndex,EMZ_FRICTIONFROMNOTCLAMPED);

          if (!tColumnHeader.pBrotherForcesHeaders[1]->bDeadMouse)
            MoveToZone(tColumnHeader.pBrotherForcesHeaders[1]->iMatrixAIndex,EMZ_FRICTIONFROMNOTCLAMPED);
        }

        if (iToZone == EMZ_UNSOLVED)
        {
          if (!tColumnHeader.pBrotherForcesHeaders[0]->bDeadMouse)
            MoveToZone(tColumnHeader.pBrotherForcesHeaders[0]->iMatrixAIndex,EMZ_UNSOLVED);

          if (!tColumnHeader.pBrotherForcesHeaders[1]->bDeadMouse)
            MoveToZone(tColumnHeader.pBrotherForcesHeaders[1]->iMatrixAIndex,EMZ_UNSOLVED);
        }
      }

      if (iFromZone == EMZ_NOTCLAMPED)
      {
        if (!tColumnHeader.pBrotherForcesHeaders[0]->bDeadMouse)
          MoveToZone(tColumnHeader.pBrotherForcesHeaders[0]->iMatrixAIndex,EMZ_UNSOLVED);

        if (!tColumnHeader.pBrotherForcesHeaders[1]->bDeadMouse)
          MoveToZone(tColumnHeader.pBrotherForcesHeaders[1]->iMatrixAIndex,EMZ_UNSOLVED);
      }

      MoveToZone(tColumnHeader.iMatrixAIndex, iToZone, iFromZone);
      tColumnHeader.iLastChangeLocalCycle = _iLocalCycle;

      /*
      if (iToZone == EMZ_NOTCLAMPED)
      {
      ChooseNewPedestalPoint(tColumnHeader.iMatrixAIndex);  
      }*/


      break;
    }
  default:
    ASSERT(FALSE);
  }

  /*LogEx("MovedToZoneEx: SourceMatrixIndex = %d, MatrixIndex = %d, iFromZone = %d, iToZone = %d, _pnLastIndexInZone[EMZ_CLAMPED] = %d",
    tColumnHeader.iSourceMatrixAIndex, tColumnHeader.iMatrixAIndex, iFromZone, iToZone, _pnLastIndexInZone[EMZ_CLAMPED]);*/

  return;
}

void DantzigAlgorithm::MoveToZone(unsigned int iIndex, unsigned int iToZone, unsigned int iFromZone /*= 0*/)
{

  while( iIndex >= _pnLastIndexInZone[iFromZone])
    iFromZone ++;

  if (iFromZone == iToZone)
    return;  // trivial case

 /* LogEx("MoveToZone:   SourceMatrixIndex = %d, MatrixIndex = %d, iFromZone = %d, iToZone = %d",
    _cMappingToColumnID[iIndex]->iSourceMatrixAIndex, iIndex, iFromZone, iToZone);*/

  if (iFromZone == EMZ_CLAMPED || iToZone == EMZ_CLAMPED)
    _bLUDecompositionValid = FALSE;

  // semitrivial cases
  if (iFromZone + 1 == iToZone)
  {
    SwitchCollisions(iIndex, _pnLastIndexInZone[iFromZone] - 1);
    _pnLastIndexInZone[iFromZone]--;
    return;
  }

  if (iFromZone == iToZone + 1)
  {
    SwitchCollisions(iIndex, _pnLastIndexInZone[iToZone]);
    _pnLastIndexInZone[iToZone]++;
    return;
  }

  // nontrivial case
  if (iFromZone < iToZone)
  {
    // move down all form iIndex + 1 to _pnLastIndexInZone[iToZone]
    unsigned int iToIndex = _pnLastIndexInZone[iToZone] - 1;

    if (iIndex != iToIndex)
    {
      _cVectorB.MoveUp(iIndex, iToIndex);
      _cForces.MoveUp(iIndex, iToIndex);
      _cAccelerations.MoveUp(iIndex, iToIndex);
      _cMatrixA.MoveUp(iIndex, iToIndex);

      _cMappingToColumnID.MoveUp(iIndex, iToIndex);
    }

    for(unsigned int iZone = iFromZone; iZone < iToZone; iZone++ )
      _pnLastIndexInZone[iZone]--;    

    for(unsigned int iIndexTemp = iIndex; iIndexTemp <= iToIndex; iIndexTemp++ )
      _cMappingToColumnID[iIndexTemp]->iMatrixAIndex = iIndexTemp;


  }
  else
  {
    // move up all form iIndex + 1 to _pnLastIndexInZone[iToZone]
    unsigned int iToIndex = _pnLastIndexInZone[iToZone];

    if (iIndex != iToIndex)
    {
      _cVectorB.MoveDown(iIndex, iToIndex );
      _cForces.MoveDown(iIndex, iToIndex );
      _cAccelerations.MoveDown(iIndex, iToIndex );
      _cMatrixA.MoveDown(iIndex, iToIndex );
      _cMappingToColumnID.MoveDown(iIndex, iToIndex);
    }

    for(unsigned int iZone = iToZone; iZone < iFromZone; iZone++ )
      _pnLastIndexInZone[iZone]++;    

    for(unsigned int iIndexTemp = iToIndex; iIndexTemp <= iIndex; iIndexTemp++ )
      _cMappingToColumnID[iIndexTemp]->iMatrixAIndex = iIndexTemp;
  }
}

void DantzigAlgorithm::RevertColumnInMatrixA(unsigned int iIndex)
{
  unsigned int iSourceIndex = _cMappingToColumnID[iIndex]->iSourceMatrixAIndex;


  for(unsigned int iIndexTemp = 0; iIndexTemp < _nNumberOfForces; iIndexTemp++)
    _cMatrixA.SetMemberValue( iIndexTemp, iIndex, 
    _cSourceMatrixA.GetMemberValue(_cMappingToColumnID[iIndexTemp]->iSourceMatrixAIndex,iSourceIndex));

  _bLUDecompositionValid = FALSE;

}


void DantzigAlgorithm::ChangeFrictionAxes(TCOLUMNHEADER &tHeaderFriction1, const Vector3& cNewDir1)
{
  TCOLUMNHEADER &tHeaderFriction2 = *(tHeaderFriction1.pBrotherForcesHeaders[1]);

  // the friction must not be clamped and both must be in the same zone
  //ASSERT((tHeaderFriction1.iMatrixAIndex < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] && tHeaderFriction2.iMatrixAIndex < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]) ||
  //    (tHeaderFriction1.iMatrixAIndex >= _pnLastIndexInZone[EMZ_SOLVING] && tHeaderFriction2.iMatrixAIndex >= _pnLastIndexInZone[EMZ_SOLVING]));

  // projection matrix
  marina_real pfProjectionMatrix[4];

  pfProjectionMatrix[0] = cNewDir1[0];
  pfProjectionMatrix[1] = cNewDir1[1];
  pfProjectionMatrix[2] = cNewDir1[1];
  pfProjectionMatrix[3] = -cNewDir1[0];

  // update matrixes
  _cMatrixA.CombineCoordinates(tHeaderFriction1.iMatrixAIndex, tHeaderFriction2.iMatrixAIndex, pfProjectionMatrix);
  _cSourceMatrixA.CombineCoordinates(tHeaderFriction1.iSourceMatrixAIndex, tHeaderFriction2.iSourceMatrixAIndex, pfProjectionMatrix);
  //_bLUDecompositionValid = FALSE; BE WARE not needed but be ware.

  // update vectors
  marina_real fIn1, fIn2;

  fIn1 = _cVectorB[tHeaderFriction1.iMatrixAIndex];
  fIn2 = _cVectorB[tHeaderFriction2.iMatrixAIndex];

  _cVectorB[tHeaderFriction1.iMatrixAIndex] = pfProjectionMatrix[0] * fIn1 + pfProjectionMatrix[1] * fIn2;
  _cVectorB[tHeaderFriction2.iMatrixAIndex] = pfProjectionMatrix[2] * fIn1 + pfProjectionMatrix[3] * fIn2;

  fIn1 = _cForces[tHeaderFriction1.iMatrixAIndex];
  fIn2 = _cForces[tHeaderFriction2.iMatrixAIndex];

  _cForces[tHeaderFriction1.iMatrixAIndex] = pfProjectionMatrix[0] * fIn1 + pfProjectionMatrix[1] * fIn2;
  _cForces[tHeaderFriction2.iMatrixAIndex] = pfProjectionMatrix[2] * fIn1 + pfProjectionMatrix[3] * fIn2;

  fIn1 = _cAccelerations[tHeaderFriction1.iMatrixAIndex];
  fIn2 = _cAccelerations[tHeaderFriction2.iMatrixAIndex];

  _cAccelerations[tHeaderFriction1.iMatrixAIndex] = pfProjectionMatrix[0] * fIn1 + pfProjectionMatrix[1] * fIn2;
  _cAccelerations[tHeaderFriction2.iMatrixAIndex] = pfProjectionMatrix[2] * fIn1 + pfProjectionMatrix[3] * fIn2;

  // update ContactPoint

  Vector3 cNewRealDir = ContactPointDir(tHeaderFriction1.iContactPointID, tHeaderFriction1.iDirIndex) * pfProjectionMatrix[0] + 
    ContactPointDir(tHeaderFriction1.iContactPointID, tHeaderFriction2.iDirIndex) * pfProjectionMatrix[1];

  ContactPointDir(tHeaderFriction1.iContactPointID, tHeaderFriction2.iDirIndex) = ContactPointDir(tHeaderFriction1.iContactPointID, tHeaderFriction1.iDirIndex) * pfProjectionMatrix[2] +
    ContactPointDir(tHeaderFriction1.iContactPointID, tHeaderFriction2.iDirIndex) * pfProjectionMatrix[3];

  ContactPointDir(tHeaderFriction1.iContactPointID, tHeaderFriction1.iDirIndex) = cNewRealDir;

  // update ColumnHeaders

  ASSERT(tHeaderFriction1.iNotClampedIndex == 0);
  ASSERT(tHeaderFriction2.iNotClampedIndex == 0);
  ASSERT(tHeaderFriction1.cStartDir == tHeaderFriction2.cStartDir);

  marina_real fTemp = pfProjectionMatrix[0] * tHeaderFriction1.cStartDir[(int)tHeaderFriction1.iDirIndex - 1] + pfProjectionMatrix[1] * tHeaderFriction1.cStartDir[(int) tHeaderFriction2.iDirIndex - 1];
  tHeaderFriction1.cStartDir[(int)tHeaderFriction2.iDirIndex - 1] = pfProjectionMatrix[2] * tHeaderFriction1.cStartDir[(int) tHeaderFriction1.iDirIndex - 1] + pfProjectionMatrix[3] * tHeaderFriction1.cStartDir[(int) tHeaderFriction2.iDirIndex - 1];
  tHeaderFriction1.cStartDir[(int)tHeaderFriction1.iDirIndex - 1] = fTemp;

  tHeaderFriction2.cStartDir = tHeaderFriction1.cStartDir;
}

int DantzigAlgorithm::Resolve()
{
  //DebugPoint();
  //InitLogging();

  BOOL bAllowDynamic; 

  //if (_cGroup.IsFrozen())
  //    return MARINA_OK; //NOTHING TO DO.

  do 
  {

    Done();

    BOOL bNothingToDo;
    if (MARINA_FAILURE == Initialize(bNothingToDo))
      return MARINA_FAILURE;         
    if (bNothingToDo)
    {
      Done();
      return MARINA_OK;
    }

    bAllowDynamic = _bAllowDynamicContact;
    _bLUDecompositionValid = FALSE;

    _iLocalCycle = _iGlobalCycle = 1;

    /// Vector B can lay out outside from the space defined by culumns of the matrix A.
    /// Here we present very simple and time consuming method how to project vector b into 
    /// Matrix A space. On other sidem if we will be lucy all forces can be possitiove
    //  if (! ProjectBIntoA())
    { 

      //// Calculation with first one. it must be solved by hand.
      if (!ResolveFirstCollision())
        return MARINA_OK; // nothing to do


      //// Calculate othrers
      while (/*_pnLastIndexInZone[EMZ_UNSOLVED] != _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] &&*/ ResolveNextCollision())
      {}      
    }

  } while /*(bAllowDynamic && !_bAllowDynamicContact)*/ (FALSE);

  //Serialize();

  /*  for(unsigned int ii = _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]; ii < _pnLastIndexInZone[EMZ_UNSOLVED]; ii = _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED])
  {
  const TCOLUMNHEADER& tColumnHeader = _cColumnHeaders[_cMappingToColumnID[ii]];
  tColumnHeader.pPedestal->MoveFrictionToClamped(_cMappingToColumnID[ii]);
  MoveToZone(ii, EMZ_CLAMPED, EMZ_UNSOLVED);
  }

  RecalculateClamped();
  */
  //Serialize();

  /*
  for(unsigned int i = 0; i < _pnLastIndexInZone[EMZ_CLAMPED]; i++)
  {
  marina_real fNewAccel = _cMatrixA.RowTimesVector(i, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) + _cVectorB[i];
  if (!IsNumericalZero(fNewAccel))
  {
  RecalculateClamped();
  break;
  }
  }*/


  RecalculateClamped();
  // Still some acceleration < 0, because of numerical fluctuations. 
  // Apply harmonic potencial to force bodies to stay in the middle of the tolerance.
  // ApplyBindForcesAtClamped();


  //// Apply forces to bodies
  ApplyCalculatedForces(_cForces);

  //// Apply correction  forces
  //ApplyCorrectionForces(fSimulationTime); 

  Done();
  //DoneLogging();

  return MARINA_OK;
}

void DantzigAlgorithm::RecalculateClamped()
{

  if (_pnLastIndexInZone[EMZ_CLAMPED] == 0)
    return; //nothing to do.

  //    ASSERT(_cForces[0] < 50);
  for(unsigned int iForce = 0; iForce < _pnLastIndexInZone[EMZ_CLAMPED]; iForce++)
  {
    _cForces[iForce] = - _cVectorB[iForce];
  }

  if (_bLUDecompositionValid)
    SolveNxNOld(_cForces.GetPointer());
  else
    SolveNxN(_pnLastIndexInZone[EMZ_CLAMPED], _cMatrixA.GetDim(), _cMatrixA.GetPointer(), _cForces.GetPointer());

  // SetUp forces for NonClamped Friction.
  for(unsigned int iForce = _pnLastIndexInZone[EMZ_NOTCLAMPED]; iForce < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]; iForce++)
  {
    TCOLUMNHEADER & tColumnHeader = *_cMappingToColumnID[iForce];    
    _cForces[iForce] = _cForces[tColumnHeader.pBrotherForcesHeaders[0]->iMatrixAIndex] * tColumnHeader.fSlidingFrictionCoef * 
      fSign(_cForces[iForce]);
  }

#ifdef _DEBUG

  //  for(unsigned int i = 0; i < _pnLastIndexInZone[EMZ_CLAMPED]; i++)
  //  {
  //    marina_real fNewAccel = _cMatrixA.RowTimesVector(i, _cForces, _pnLastIndexInZone[EMZ_CLAMPED]) + _cVectorB[i];
  //    if (!IsNumericalZero(fNewAccel/10.0f))
  //    {
  //      int i =3;
  //  
  //    }
  //  }

#endif

}

void DantzigAlgorithm::Done()
{
  delete _pcColumnHeaders; // indexes are columns in the source matrix A. 
  _pcColumnHeaders = NULL;
  _cMappingToColumnID.Clear(); // indexes are coordinates used in _cMatrixA, values are columns IDs
  SolvingDone();
}

BOOL DantzigAlgorithm::IsMatrixARegular()
{
  if (_pnLastIndexInZone[EMZ_CLAMPED] == 1)
    return TRUE;

  Vector<marina_real> cLeftVector;
  cLeftVector.Init(_pnLastIndexInZone[EMZ_CLAMPED] - 1);

  _cMatrixA.GetColumn(cLeftVector,_pnLastIndexInZone[EMZ_CLAMPED] - 1,_pnLastIndexInZone[EMZ_CLAMPED] - 1);

  SolveNxN(_pnLastIndexInZone[EMZ_CLAMPED] - 1, _cMatrixA.GetDim(), _cMatrixA.GetPointer(), cLeftVector.GetPointer());

  marina_real fResult = _cMatrixA.RowTimesVector(_pnLastIndexInZone[EMZ_CLAMPED] - 1,cLeftVector, _pnLastIndexInZone[EMZ_CLAMPED] - 1);

  fResult -= _cMatrixA.GetMemberValue(_pnLastIndexInZone[EMZ_CLAMPED] - 1,_pnLastIndexInZone[EMZ_CLAMPED] - 1);
  //ASSERT(!IsNumericalZero(fResult));

  return !IsNumericalZero(fResult);
}

BOOL DantzigAlgorithm::VerifySituation()
{
  // All clamped normal forces must be >= 0
  // All clamped friction forces must have thier normal forces clamped
  unsigned int i = 0;
  for(; i < _pnLastIndexInZone[EMZ_CLAMPED]; i++)
  {
    TCOLUMNHEADER tHeader = *(_cMappingToColumnID[i]);
    if (tHeader.eForceType == EFT_STATIC)
    {
      if (_cForces[i] < 0.0 && !IsNumericalZero(_cForces[i]))
      {            
        ASSERT(FALSE);
        return FALSE;
      }

      if (tHeader.iNotClampedIndex != 0)
      {

        if (tHeader.pBrotherForcesHeaders[tHeader.iNotClampedIndex - 1]->iMatrixAIndex >= _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED] ||
          tHeader.pBrotherForcesHeaders[tHeader.iNotClampedIndex - 1]->iMatrixAIndex < _pnLastIndexInZone[EMZ_NOTCLAMPED])
        {
          ASSERT(FALSE);
          return FALSE;            
        }  


      }
    }
    else
      if (tHeader.eForceType == EFT_FRICTION)
      {
        if (tHeader.pBrotherForcesHeaders[0]->iMatrixAIndex >=  _pnLastIndexInZone[EMZ_CLAMPED])
        {
          ASSERT(FALSE);
          return FALSE;
        }

        if (tHeader.pBrotherForcesHeaders[1]->iMatrixAIndex <  _pnLastIndexInZone[EMZ_CLAMPED])
        {
          /*marina_real fFrictionForce = _cForces[i] *  _cForces[i] +
          _cForces[tHeader.pBrotherForcesHeaders[1]->iMatrixAIndex] * _cForces[tHeader.pBrotherForcesHeaders[1]->iMatrixAIndex];
          fFrictionForce = sqrt(fFrictionForce);

          if (fFrictionForce > _cForces[tHeader.pBrotherForcesHeaders[0]->iMatrixAIndex] * tHeader.fSlidingFrictionCoef &&
          IsNumericalZero(fFrictionForce - _cForces[tHeader.pBrotherForcesHeaders[0]->iMatrixAIndex] * tHeader.fSlidingFrictionCoef))
          {                    
          ASSERT(FALSE);
          return FALSE;
          }
          */
        }
        else
        {                
          if (tHeader.pBrotherForcesHeaders[1]->iMatrixAIndex < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED])
          {
            ASSERT(FALSE);
            return FALSE;
          }

          /* if (_cForces[i] > _cForces[tHeader.pBrotherForcesHeaders[0]->iMatrixAIndex] * tHeader.fSlidingFrictionCoef 
          && !IsNumericalZero(_cForces[i] - _cForces[tHeader.pBrotherForcesHeaders[0]->iMatrixAIndex] * tHeader.fSlidingFrictionCoef))
          {                    
          ASSERT(FALSE);
          return FALSE;
          }
          */
        }
      }

  }

  // All nonclamped normal accelerations must be >= 0
  for(; i < _pnLastIndexInZone[EMZ_NOTCLAMPED]; i++)
  {
    TCOLUMNHEADER tHeader = *(_cMappingToColumnID[i]);
    if (tHeader.eForceType != EFT_STATIC)// && _cAccelerations[i] < 0.0)
    {
      ASSERT(FALSE);
      return FALSE;
    }
  }

  for(; i < _pnLastIndexInZone[EMZ_FRICTIONNOTCLAMPED]; i++)
  {
    TCOLUMNHEADER tHeader = *(_cMappingToColumnID[i]);
    if (tHeader.eForceType != EFT_FRICTION) 
    {
      ASSERT(FALSE);
      return FALSE;
    }

    if (tHeader.pBrotherForcesHeaders[0]->iNotClampedIndex == 0)
    {
      ASSERT(FALSE);
      return FALSE;
    }

    if (tHeader.pBrotherForcesHeaders[0]->iMatrixAIndex >= _pnLastIndexInZone[EMZ_CLAMPED])
    {
      ASSERT(FALSE);
      return FALSE;
    }

    if (tHeader.pBrotherForcesHeaders[1]->iMatrixAIndex < _pnLastIndexInZone[EMZ_FRICTIONFROMNOTCLAMPED])
    {
      ASSERT(FALSE);
      return FALSE;
    }

  }

  return TRUE;
}

void DantzigAlgorithm::Serialize()
{
  FILE *fArchive = fopen("C:\\temp\\RestResolver.ser","w");

  SerializeChild(fArchive);

  fprintf(fArchive,"_cMappingToColumnID: %u\n", _cMappingToColumnID.Size());
  for(int i = 0; i < _cMappingToColumnID.Size(); i++)
  {
    fprintf(fArchive,"%u\n", _cMappingToColumnID[i]);
  }

  /*  fprintf(fArchive,"_cColumnHeaders: %u\n", _cColumnHeaders.GetSize());
  for(unsigned int i = 0; i < _cColumnHeaders.GetSize(); i++)
  {
  SerializeColumnHeader(fArchive, _cColumnHeaders[i], TRUE);    
  }
  */
  fprintf(fArchive,"_cForces: %u\n", _cForces.GetDim());
  for(unsigned int i = 0; i < _cForces.GetDim(); i++)
  {
    fprintf(fArchive,"%10.8lf\n", _cForces[i]); 
  }

  fclose(fArchive);
}


#include <El/Credits/credits.hpp>

REGISTER_CREDITS("Matrix Template Library", "2001-2005 The Trustees of Indiana University",STANDARD_DISCLAIMER)
REGISTER_CREDITS("Matrix Template Library", "1998-2001 University of Notre Dame",STANDARD_DISCLAIMER)
