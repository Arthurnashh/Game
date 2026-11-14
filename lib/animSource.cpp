#include "wpch.hpp"
#include "animSource.hpp"
#include "rtAnimation.hpp"

#include "global.hpp"
#include "vehicle.hpp"  // to know that Entity inherits from Animated

extern int JumpySim;

/// animation depends only on entity's current visual state
class AnimationSourceMemberFuncVisualState: public AnimationSource
{
  typedef AnimationSource base;
  SRef<AnimatedMemberFuncVisualState> _member;

public:
  AnimationSourceMemberFuncVisualState(AnimatedMemberFuncVisualState *member, const RStringB &name);

  float GetPhase(const ObjectVisualState &vs, const Animated *obj) const
  {
    return (*_member)(obj, vs);
  }
  void SetPhaseWanted(Animated *obj, float phase){}
};

AnimationSourceMemberFuncVisualState::AnimationSourceMemberFuncVisualState(AnimatedMemberFuncVisualState *member, const RStringB &name)
:base(name),_member(member)
{
}


/// animation depends on time only
class AnimationSourceTime: public AnimationSource
{
  typedef AnimationSource base;
public:
  AnimationSourceTime(const RStringB &name):base(name){}
  float GetPhase(const ObjectVisualState &vs, const Animated *obj) const
  {
    return Glob.time.toFloat();
  }
  void SetPhaseWanted(Animated *obj, float phase){}
};

/// animation depends on game time only
class AnimationSourceClockHour : public AnimationSource
{
  typedef AnimationSource base;
public:
  AnimationSourceClockHour(const RStringB &name):base(name){}
  float GetPhase(const ObjectVisualState &vs, const Animated *obj) const
  {
    float timeOfDay = Glob.clock.GetTimeOfDay();
    if (timeOfDay >= 1.0f) timeOfDay--;
    Assert(timeOfDay >= 0 && timeOfDay < 1.0);

    return fmod(2.0f * timeOfDay, 1.0f);
  }
  void SetPhaseWanted(Animated *obj, float phase){}
};

/// animation depends on game time only
class AnimationSourceClockMinute : public AnimationSource
{
  typedef AnimationSource base;
public:
  AnimationSourceClockMinute(const RStringB &name):base(name){}
  float GetPhase(const ObjectVisualState &vs, const Animated *obj) const
  {
    float timeOfDay = Glob.clock.GetTimeOfDay();
    if (timeOfDay >= 1.0f) timeOfDay--;
    Assert(timeOfDay >= 0 && timeOfDay < 1.0);

    return fmod(24.0f * timeOfDay, 1.0f);
  }
  void SetPhaseWanted(Animated *obj, float phase){}
};

/// animation depends on game time only
class AnimationSourceClockSecond : public AnimationSource
{
  typedef AnimationSource base;
public:
  AnimationSourceClockSecond(const RStringB &name):base(name){}
  float GetPhase(const ObjectVisualState &vs, const Animated *obj) const
  {
    float timeOfDay = Glob.clock.GetTimeOfDay();
    if (timeOfDay >= 1.0f) timeOfDay--;
    Assert(timeOfDay >= 0 && timeOfDay < 1.0);

    float sec = fmod(24.0f * 60.0f * timeOfDay, 1.0f);
    return (1.0f / 60.0f) * toIntFloor(60.0 * sec);
  }
  void SetPhaseWanted(Animated *obj, float phase){}
};


AnimationSource* AnimationSourceHolder::CreateAnimationSource(AnimatedMemberFuncVisualState* member)
{
  return new AnimationSourceMemberFuncVisualState(member,RStringB());
}


int AnimationSourceHolder::FindName(const char *name) const
{
  // note: _animSource may be partially constructed and may contain NULLs
  for (int i=0; i<_animSource.Size(); i++)
  {
    if (!_animSource[i]) continue;
    if (!strcmpi(_animSource[i]->GetName(),name)) return i;
  }
  return -1;
}

DEF_RSB(user)

void AnimationSourceHolder::Load(AnimatedType *type, const AnimationHolder &animations)
{
  Clear();
  _animSource.Realloc(animations.Size());
  _animSource.Resize(animations.Size());
  for (int i = 0; i<animations.Size(); i++)
  {
    RStringB source = animations[i]->GetSource();
    if (source.GetLength()>0)
      _animSource[i] = type->CreateAnimationSource(animations[i], source);
    else
      _animSource[i] = type->CreateAnimationSource(animations[i], RSB(user));
  }
  _animSource.Compact();
}

bool AnimationSourceHolder::PrepareShapeDrawMatrices(Matrix4Array &matrices, LODShape *shape, int level, const ObjectVisualState &vs, const Animated *obj, const AnimationHolder &animations) const
{
  const Skeleton *sk = shape->GetSkeleton();
  Assert(sk);

  if (animations.Size()>0)
  {
    MATRIX_4_ARRAY(parentMatrix,128);
    // TODO: remove parentValid;
    AUTO_STATIC_ARRAY(bool,parentValid,128);
    parentMatrix.Resize(sk->NBones());
    parentValid.Resize(sk->NBones());
    for (int i=0; i<sk->NBones(); i++) parentValid[i] = false;

    // Get the shape and make sure it already exists
    // we are inside of the MT, cannot use ShapeUsed here
    DoAssert(shape->IsLevelLocked(level));
    const Shape *lShape = shape->GetLevelLocked(level);
    // traverse subskeleton hierarchy, this guarantees only needed bones are computed (including their parents)
    for (int sst = 0; sst<lShape->GetSubSkeletonTreeSize(); sst++)
    {
      SkeletonIndex bone = lShape->GetSubSkeletonTreeBone(sst);

      Assert(!parentValid[GetSkeletonIndex(bone)]);
      const AutoArray<int> &animIndices = animations.BoneToAnimation(level,bone);
      Matrix4 baseAnim = MIdentity;

      for (int i=0; i<animIndices.Size(); i++)
      {
        int animIndex = animIndices[i];
        const AnimationType *animType = animations[animIndex];
        // another bone may be using this one as a parent
        // or same skeleton bone may be used by multiple subskeleton indices
        Matrix4 animMatrix = _animSource[animIndex]->GetAnimMatrix(vs,animType, obj,level);
        baseAnim = animMatrix * baseAnim;
      }
      
      // parent was almost certainly already calculated
      SkeletonIndex parent = sk->GetParent(bone);
      if (parent!=SkeletonIndexNone)
      {
        Assert(parentValid[GetSkeletonIndex(parent)]);
        baseAnim = parentMatrix[GetSkeletonIndex(parent)] * baseAnim;
      }
        
      parentMatrix[GetSkeletonIndex(bone)] = baseAnim;
      parentValid[GetSkeletonIndex(bone)] = true;
      // replicate to all necessary subskeleton bones
      const SubSkeletonIndexSet &ssis = lShape->GetSubSkeletonIndexFromSkeletonIndex(bone);
      for (int i=0; i<ssis.Size(); i++)
      {
        matrices[GetSubSkeletonIndex(ssis[i])] = baseAnim;
      }
    }
  }
  return true;
}

AnimationSource *AnimatedType::CreateAnimationSource(const AnimationType *type, RStringB source)
{
  if (!strcmpi(source,"time"))
    return new AnimationSourceTime(source);
  else if (!strcmpi(source,"clockHour"))
    return new AnimationSourceClockHour(source);
  else if (!strcmpi(source,"clockMinute"))
    return new AnimationSourceClockMinute(source);
  else if (!strcmpi(source,"clockSecond"))
    return new AnimationSourceClockSecond(source);
  else
    return NULL;
}

AnimationInstance::AnimationInstance()
{
  _phase = _phaseWanted = 0;
}

