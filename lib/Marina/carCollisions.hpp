#ifdef _MSC_VER
#pragma once
#endif

#ifndef __CARCOLLISIONS_H__
#define __CARCOLLISIONS_H__

#include "../wpch.hpp"
#include "dantzig_old.hpp"


struct CC_ContactPoint // contact point used by marina physical engine
{	
  Vector3 pos; // world space position
  Vector3 dir[3]; // direction out from pedestal.pcBody1, direction x and y from pedestal

  //float under; // how much under surface

  float fSlidingFrictionCoef;
  float fRestitutionCoef;
  EntityAI * pcBody1;   // must not be NULL
  EntityAI * pcBody2;   // is NULL for "Static" objects
  Object * pcObjBody2;  // not NULL for "Static" objects
};

TypeIsMovable(CC_ContactPoint);

typedef AutoArray<CC_ContactPoint> CC_ContactArray;


/**
 * Calculates impulses for a collision of two objects,
 * It dammages both objects according to the calculated impulses.
 */
class CarCollisionsContactResolver : public DantzigAlgorithm
{
protected:
  marina_real _fSimulationTime; 
  //Vector<marina_real> _cVectorBAll;
  marina_real _fNormalizedMass;
  marina_real _fAppliedImpulse;
  CC_ContactArray _cContactPoints;

  /** 
  * Function prepares A matrix and B vector and it initializates all staff needed in algorithm.
  */
  int Initialize(BOOL &bNothingToDo); 

  /** 
  * Not used at the moment.
  */
  BOOL ChooseNewPedestalPoint(unsigned int iIndex) {return TRUE;};

  /** 
  * Function applies calculated impulses onto bodies.
  * @param cForces - (in) Impulses.
  */
  virtual void ApplyCalculatedForces( const Vector<marina_real>& cForces);

  // Debug function
  void SerializeChild(FILE *fArchive) {};

  Vector3& ContactPointDir(const unsigned int iContactPointID, const unsigned int iIndex) 
    {return _cContactPoints[iContactPointID].dir[iIndex];};

public:
  CarCollisionsContactResolver(): _fAppliedImpulse(0), DantzigAlgorithm() {};


  void Init( marina_real fSimulationTime,             //< Time step.
    EntityAI * _pcBody1,                     //< Body 1 involved in collision
    const CollisionBuffer& _pcCollBuffer,    //< Buffer with contacts, relevant are only according to following values.
    const unsigned int iFrom,                //< Indexes in _pcCollBuffer. zero-based
    const unsigned int iTo);                  //< one-based


  float GetAppliedImpulse() const {return _fAppliedImpulse;}               
};

/**
* Calculates impulses for a collision of two objects,
* The agregate impulse is limited by given maximal value.
* It dammages only pcBody1,
*/
class CarCollisionsResolverMaxImpulse : public CarCollisionsContactResolver
{
protected:
  float _fMaxImpulse;   

  virtual void ApplyCalculatedForces( const Vector<marina_real>& cForces);
public:
  CarCollisionsResolverMaxImpulse() : CarCollisionsContactResolver() {};
  void Init( marina_real fSimulationTime,             //< Time step.
    EntityAI * _pcBody1,                     //< Body 1 involved in collision
    const CollisionBuffer& _pcCollBuffer,    //< Buffer with contacts, relevant are only according to following values.
    const unsigned int iFrom,                //< Indexes in _pcCollBuffer. zero-based
    const unsigned int iTo,                  //< one-based
    float fMaxImpulse);                      //< The maximal value of agregate impulse              
};

#endif //__CARCOLLISIONS_H__
