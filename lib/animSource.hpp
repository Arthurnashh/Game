#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ANIM_SOURCE_HPP
#define _ANIM_SOURCE_HPP

#include <Es/Common/delegate.hpp>
#include <El/ParamFile/paramFile.hpp>
#include <El/ParamArchive/paramArchive.hpp>

#include "animation.hpp"
#include "object.hpp"  // ObjectVisualState

class Animated
{
public:
  Animated() {}
  virtual ~Animated() {}
};

/// interface for obtaining the value for animation controller
class AnimationSource: public RefCount
{
  RStringB _name;
public:
  explicit AnimationSource(const RStringB &name):_name(name){}
  const RStringB &GetName() const {return _name;}
  virtual void Init(Animated *obj) {}
  virtual float GetPhase(const ObjectVisualState &vs, const Animated *obj) const = 0;
  virtual void SetPhaseWanted(Animated *obj, float phase) = 0;
  /// index of animation instance (in _animationStates array)
  virtual int GetStateIndex() const {return -1;}
  virtual bool Simulate(Animated *obj, float deltaT) const {return false;}
  virtual Matrix4 GetAnimMatrix(const ObjectVisualState &vs, const AnimationType *type, const Animated *obj, int level) const
  {
    return type->GetAnimMatrix(level,GetPhase(vs,obj));
  }
};


/// animation state stored for Entity (per-instance data)
class AnimationInstance
{
protected:
  float _phase;
  float _phaseWanted;
    
public:
  AnimationInstance();
  
  float GetPhase() const {return _phase;}
  float GetPhaseWanted() const {return _phaseWanted;}

  void SetPhase(float phase) {_phase = phase;}
  void SetPhaseWanted(float phase) {_phaseWanted = phase;}

  LSError Serialize(ParamArchive &ar)
  {
    CHECK(ar.Serialize("phase", _phase, 1, 0))
    CHECK(ar.Serialize("phaseWanted ", _phaseWanted, 1, 0))
    return LSOK;
  }
};

TypeIsMovable(AnimationInstance)

class AnimatedType
{
public:
  /// various types may provide various animation source values
  virtual AnimationSource *CreateAnimationSource(const AnimationType *type, RStringB source);
};

/// base for deriving property handling for animations
class AnimatedMemberFunc0
{
public:
  virtual float operator () (const Animated *type) const = 0;
};

/// creative function for a non-virtual Entity member function pointer with 0 arguments
template <class Type>
AnimatedMemberFunc0 *CreateAnimatedMemberFunc0(float (Type::*member)() const)
{
  return new MembFunc0<Type,float,Animated,AnimatedMemberFunc0>(member);
};

template <class Type, class ConfigType>
AnimatedMemberFunc0 *CreateAnimatedMemberFunc0WithConfig(float (Type::*member)(ConfigType config) const, ConfigType config)
{
  return new MembFunc0WithConfig<Type,float,ConfigType, Animated,AnimatedMemberFunc0>(member, config);
};

/// base for deriving property handling for animations
class AnimatedMemberFuncVisualState
{
public:
  virtual float operator () (const Animated *type, ObjectVisualState const& vs) const = 0;
};

/// creative function for a non-virtual Entity member function pointer with a "ObjectVisualState const&" argument
template <class Type>
AnimatedMemberFuncVisualState *CreateAnimatedMemberFuncVisualState(float (Type::*member)(ObjectVisualState const&) const)
{
  return new ConstMembFunc1<Type,float,ObjectVisualState const&,Animated,AnimatedMemberFuncVisualState>(member);
};


class AnimationHolder;

class AnimationSourceHolder
{
protected:
  RefArray<AnimationSource> _animSource;
  int _nAnimStates;

public:
  //@{ array style enumeration
  int Size() const {return _animSource.Size();}
  AnimationSource *operator [] (int i) const {return _animSource[i];}
  //@}
  void Clear() {_animSource.Clear(); _nAnimStates = 0;}
  void Compact() {_animSource.Compact();}
  int NStates() const {return _nAnimStates;}
  int AddState() {return _nAnimStates++;}

  /// create animation source corresponding to an animation list
  void Load(AnimatedType *type, const AnimationHolder &animations);

  /// find animation source based on the controller name
  int FindName(const char *name) const;

  /// find slot in which given source exists
  int Find(const AnimationSource *source) const {return _animSource.FindKey(source);}
  /// helper - create animation source value directly from a member function
  template <class Type>
  AnimationSource *CreateAnimationSource(float (Type::*member)() const)
  {
    return CreateAnimationSource(CreateAnimatedMemberFunc0(member));
  }

  template <class Type>
  AnimationSource *CreateAnimationSource(float (Type::*member)(ObjectVisualState const&) const)
  {
    return CreateAnimationSource(CreateAnimatedMemberFuncVisualState(member));
  }

  /// wrapper - create animation source value
  AnimationSource *CreateAnimationSource(AnimatedMemberFuncVisualState *member);

  bool PrepareShapeDrawMatrices(Matrix4Array &matrices, LODShape *shape, int level, const ObjectVisualState &vs, const Animated *obj, const AnimationHolder &animations) const;
};

#endif
