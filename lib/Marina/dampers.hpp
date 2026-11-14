#ifdef _MSC_VER
#pragma once
#endif

#ifndef __MARINA_DAMPERS_HPP__
#define __MARINA_DAMPERS_HPP__

#include "../animSource.hpp"
#include <Es/Common/delegate.hpp>

#include "dampedSpring.hpp"

/// damper animation - links bones to damper simulation
class AnimationSourceDamper: public AnimationSource
{
  typedef AnimationSource base;
  
  friend class DamperHolder;
  friend class DamperForces;
  
  /// damper identification
  int _index;
  /// which bone is this one bound to
  SkeletonIndex _bone;
  /// some dampers are unused - not attached to any landcontact points
  /** Such dampers should not be simulated at all*/
  bool _unused;
  /// dampers may provide explicit bone name
  RStringB _boneName;
  
  public:
  AnimationSourceDamper(const RStringB &name, int index);
  AnimationSourceDamper(const RStringB &name, int index, const RStringB &selection);
  //@{ AnimationSource implementation
  virtual float GetPhase(const ObjectVisualState &vs, const Animated *obj) const;
  virtual void SetPhaseWanted(Animated *obj, float phase) {}
  //@}
};

/// container for damper properties - type (shared between instances)
class DamperHolder
{
  friend class DamperState;  
  friend class DamperForces;
  
  /// total count of dampers
  AutoArray< InitPtr<AnimationSourceDamper> > _sources;
	/// conversion from land contact to dampers
	Buffer<int> _whichWheelContact;
	/// convertion from geometry to dampers
	Buffer<int> _whichWheelGeometry;
  /// Damped spring representing dampers
	Buffer<DampedSpring> _springs;
	
	//@{ simulation properties
	float _maxAmplitude;  
  float _stiffnesScale;
  float _dampingScale; 
	//@}
	
  /// true if at least one damper is used
  bool _used; 

  /// total count of dampers
  public:
  DamperHolder();
  
  AnimationSource *CreateAnimationSource();
  AnimationSource *CreateAnimationSource(ParamEntryPar entry);

  void Init(float maxAmplitude=0.1f, float stiffnesScale = 1, float dampingScale = 1);

  // function returns initial difference from dampers middle to equilibrium pos.
  float GetInitPos() const;
 
  void InitSources();
  void InitLandcontact(
    LODShape *shape, int level,
    const AnimationHolder &anims, const AnimationSourceHolder &sources
  );
  void InitGeometry(
    LODShape *shape, int level,
    const AnimationHolder &anims, const AnimationSourceHolder &sources
  );
  
  void Clear();

  /// check if given vertex is a wheel or not
  bool CheckWheel(int vertexIndex, bool landContact) const;

  /// Returns true if at least one damper is used (connected to landcontact)
  bool IsUsed() const {return _used;};
	int Count() const {return _sources.Size();}
};

/// per-instance information for dampers corresponding to DamperHolder
class DamperState
{
  Buffer<float> _pos;
  
  friend class DamperForces;
  
  public:
  void Init(const DamperHolder &type);

  //@{ array style access to damper positions  
  float operator [] (int i) const {return _pos[i];}
  int Count() const {return _pos.Length();}
  //@}
  /// total energy stored in dampers
  float Energy(const DamperHolder &type) const;

  void Interpolate(DamperState const& t1state, float t, DamperState& interpolatedResult) const;
};

/// simulation support for DamperState - temporary only
class DamperForces
{
  bool _storage[32];
  StaticArrayAuto<bool> _processedDampers;
			
  public:  
  DamperForces(const DamperHolder &type);  


  /// Process damper simulation.
  /**
  @param under Returned value is how much was not processed
  @param vertexIndex index of the vertex
  @param landContact is the vertex from landcontact or geometry?
  */
  void SimulateContact(
    float &forceUp, float &frictionUp, DamperState &state,
    const DamperHolder &type, int vertexIndex, bool landContact,
    float &under,
    float pointSpeedUp
  );

  void Finish(DamperState &state, const DamperHolder &type, float deltaT);
};

#endif
