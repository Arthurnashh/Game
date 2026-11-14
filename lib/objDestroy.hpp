#ifdef _MSC_VER
#pragma once
#endif

#ifndef _OBJ_DESTROY_HPP
#define _OBJ_DESTROY_HPP

class Skeleton;
class AnimationRT;

/// information about skeletal animation destruction style
class ObjectDestroyAnim: public RefCount
{
  struct AnimInfo
  {
    Ref<AnimationRT> _anim;
    float _probab;
	  float _duration;
    
    ClassIsMovableZeroed(AnimInfo)
  };
	AutoArray<AnimInfo> _animations;
	
	public:
	int Select(float kind) const;
	ObjectDestroyAnim(LODShape *shape, ParamEntryPar cls);
	~ObjectDestroyAnim();
	
	float GetAnimTime(float kind) const;
	/// if animation is not valid, return NULL - used as pass-through filter
	ObjectDestroyAnim *Validate()
	{
	  return _animations.Size()>0 ? this : NULL;
	}

  void PrepareShapeDrawMatrices(
    Matrix4Array &matrices, int level,
    float time, float kind, const Shape *shape, const Skeleton *skeleton
  );
};

#endif
