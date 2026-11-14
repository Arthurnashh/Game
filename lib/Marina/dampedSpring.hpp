#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DAMPEDSPRING_HPP__
#define __DAMPEDSPRING_HPP__

/** Spring with linear force on stiffness */
class Spring
{
protected:
  float _stiffness;  /// Spring stiffness 
  float _steadyLength; /// Length of the spring without force.

public:
  Spring() : _stiffness(0),_steadyLength(0) {};
  Spring(float stiffness, float steadyLength): _stiffness(stiffness), _steadyLength(steadyLength){};

  /** Returns force size according to spring length 
    * @param length - actual spring length. (BEWARE it is not spring length diff)
    * @return force in spring. Positive if its direction is "outside" the spring.
     */
  inline float Force(float length) const {return _stiffness * (_steadyLength - length);};


  /** Returns Energy accumulated in spring
  * @param length - actual spring length. (BEWARE it is not spring length diff)
  * @return energy in spring.
  */
  inline float Energy(float length) const {return 0.5 * _stiffness * (_steadyLength - length) * (_steadyLength - length);};    
};

/** Spring with damping. Damping is depending on speed of the spring length change. */

class DampedSpring : public Spring
{
  float _dampingLin;    /// Linear term of damping 
  float _dampingQuad;   /// Quadratical term of damping   

public:
  DampedSpring() : _dampingLin(0), _dampingQuad(0) {};

  /** Initialization. */
  inline void Init(float steadyLength, float maxForce, float effectiveMass); 

  /** Returns the damping force */
  inline float Damping(float fSpeed) const;

  ClassIsSimpleZeroed(DampedSpring);
};


/** Returns the damping force. It depends quadratically on speed.
* @param speed - speed of spring length change. Positive if spring is abbreviates.
* @return damping force in spring. Positive if its direction is "outside" the spring.
*/
float DampedSpring::Damping(float speed) const 
{
  return -_dampingLin * speed - _dampingQuad * speed * fabsf(speed);
}


void DampedSpring::Init(float steadyLength, float maxForce, float effectiveMass)
{
  _steadyLength = steadyLength;
  _stiffness = maxForce / _steadyLength;   
  _dampingQuad = effectiveMass / 2 / steadyLength;
  _dampingLin = effectiveMass * 4; 
}

/** Spring with damping and mass. Damping is depending on speed of the spring length change. */

class DampedSpringWithMass : public DampedSpring
{
  typedef DampedSpring base;
  
  float _effectiveMass; /// Spring usually is connected to some body. Effective mass is its approximative mass.
  float _invEffectiveMass; /// Inverse of  _effectiveMass
  
public:

  /** Initialization. */
  inline void Init(float steadyLength, float maxForce, float effectiveMass); 
  
  /** Returns the force in spring plus damping. */
  inline float DampedForce(float diffLength, float speed) const ;

  /** Returns the force in spring plus damping. Avoid positive work from damping.*/
  inline float DampedForce(float diffLength, float speed, float simTime) const ;

  /** Returns the force in spring plus damping. There is also external force on spring.*/
  inline float DampedForce(float diffLength, float speed, float simTime, float force) const ;  
};


void DampedSpringWithMass::Init(float steadyLength, float maxForce, float effectiveMass)
{
  base::Init(steadyLength,maxForce,effectiveMass);
  _invEffectiveMass = 1.0f / effectiveMass;
  _effectiveMass = effectiveMass;
}

/** Returns the force in spring plus damping. 
  * @param diffLength - difference from spring steady length. Positive if spring is shorter than steady length.
  * @param speed - speed of spring length change. Positive if spring is abbreviates.
  * @param simTime - simulation time.
  * @return force from spring. Positive if its direction is "outside" the spring.
  */
float DampedSpringWithMass::DampedForce(float diffLength,float speed, float simTime) const 
{
  float length = _steadyLength - diffLength;

  float springForce = Spring::Force(length);
  float dampingForce = Damping(speed);

  // If speed changes sign, because of dumping force reduce dumping force.
  if (fabsf(dampingForce) * _invEffectiveMass * simTime > fabsf(speed))
  {
    dampingForce = -speed / simTime / _invEffectiveMass;
  }

  return springForce + dampingForce;
}


/** Returns the force in spring plus damping. 
* @param diffLength - difference from spring steady length. Positive if spring is shorter than steady length.
* @param speed - speed of spring length change. Positive if spring is abbreviates.
* @return force in spring. Positive if its direction is "outside" the spring.
*/
float DampedSpringWithMass::DampedForce(float diffLength, float speed) const 
{
  float length = _steadyLength - diffLength;
  return Spring::Force(length) + Damping(speed);
}

/** Returns the force in spring plus damping. The result force should produce more stable simulation. On other hand
  * function works slowly than other DampedForce functions.
* @param diffLength - difference from spring steady length. Positive if spring is shorter than steady length.
* @param speed - speed of spring length change. Positive if spring is abbreviates.
* @param simTime - simulation time.
* @param force - external force on spring.
* @return force in spring. Positive if its direction is "outside" the spring.
*/
float DampedSpringWithMass::DampedForce(float diffLength, float speed, float simTime, float force) const 
{
  // simulate it
  float globalForce = 0;

  const float maxSimTimeInv = 1000;  //TODO these constants needs tuning
  const float maxSimSteps = 50;  //TODO these constants needs tuning

  int simSteps = toIntCeil(simTime * maxSimTimeInv);
  saturateMin(simSteps, toInt(maxSimSteps));
  float timeStep = simTime/simSteps;

  for(int i = 0; i < simSteps; i++)
  {
    float springForce = DampedForce(diffLength, speed, timeStep);
    globalForce += springForce;

    speed += (springForce - force) * _invEffectiveMass * timeStep;
    diffLength -= speed * timeStep;
    if (diffLength < 0)
      diffLength = 0;

    if (diffLength > _steadyLength)
    {
      diffLength = _steadyLength;            
    }
  }

  return globalForce / simSteps;
}

#endif //__DAMPEDSPRING_HPP__
