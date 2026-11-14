#ifdef _MSC_VER
#pragma once
#endif

#ifndef __IKENGINE_HPP__
#define __IKENGINE_HPP__


class IKEngine
{
private:
  float _l0;
  float _l1;  

  Vector3 _p0Src;
  Vector3 _p1Src;
  Vector3 _p2Src;
  Vector3 _vDirSrc;

  int _jointBone; 
  AutoArray<float> _rotationFrac;
  Matrix4 _srcInv01;
  Matrix4 _srcInv12;

public:
  virtual ~IKEngine() {};

  void CalculateBones(AutoArray<Matrix4>& matrices, 
                      Matrix4Val boneStart,  // start  bone cannot be changed   
                      Matrix4& boneEnd, // If bone is too faraway outside                       
                      float blendFactor) const; // blending factor >= 1 - full IK, <= no IK

  void Init(Vector3Val p0src, 
    Vector3Val p1src, 
    Vector3Val p2src, 
    Vector3Val jointDir,
    int jointBone, 
    const AutoArray<float>& rotationFrac);
};


/// parameter interface
class IKAnimationParam
{
public:
  virtual ~IKAnimationParam() {};

  virtual Matrix4 GetEndBoneTrans(const Matrix4Array &matrices, const Shape *shape, SkeletonIndex endBone) const = 0;
  virtual void SetAfterEndBoneTrans(Matrix4Array &matrices, const Shape *shape, Matrix4& endBone, SkeletonIndex endBoneIndx,const AutoArray<SkeletonIndex> &afterEndBone, float blendFactor) const = 0;
};

/// IK setup - define bones meaning
class IKAnimation : public IKEngine
{
protected:
  /// all bones participating in the IK (affected by the IK or weapon)
  AutoArray<SkeletonIndex> _bones;

  /// wrist bone (affected by IK, closest to the target position)
  SkeletonIndex _endBone;
  
  /// shoulder bone (affected by IK, most distant from the target position)
  SkeletonIndex _startBone;
  
  /// hand bones (directly driven by the weapon)
  AutoArray<SkeletonIndex> _afterEndBone;
 
public:
  void CombineTransform(Matrix4Array &matrices, LODShape *lShape, int level, float blendFactor, const IKAnimationParam& par ) const;
  bool Valid() const {return SkeletonIndexValid(_startBone) && SkeletonIndexValid(_endBone);}
};

class IKHandAnimation : public IKAnimation
{
  typedef IKAnimation base;
public:
  void Init(LODShape *lShape, bool right, const ParamEntry &par);  
  
};

class IKLegsAnimation : public IKAnimation
{
  typedef IKAnimation base;
public:
  void Init(LODShape *lShape, bool right, const ParamEntry &par);  

};

/// IK parameters - connecting the IK to the weapon
class AnimationRTIKParam : public IKAnimationParam
{
protected:
  /// hand position (defined by the weapon)
  Ref<AnimationRT> _handAnim; 
  /// weapon bone ("leading" the animation)
  SkeletonIndex _leadBone;
public:
  AnimationRTIKParam(Ref<AnimationRT> handAnim, const SkeletonIndex& leadBone) : _handAnim(handAnim), _leadBone(leadBone) {};

  //@{ implement IKAnimationParam
  virtual Matrix4 GetEndBoneTrans(const Matrix4Array &matrices, const Shape *shape, SkeletonIndex endBone) const;  
  virtual void SetAfterEndBoneTrans( Matrix4Array &matrices, const Shape *shape, Matrix4& endBone, SkeletonIndex endBoneIndx, const AutoArray<SkeletonIndex> &afterEndBone, float blendFactor) const;  
  //@}
};

class AnimationRTIKParamPilot : public IKAnimationParam
{
protected:
  /// pilot hand position (on stick/wheel drive)
  Matrix4 _leadTrans;
public:
  AnimationRTIKParamPilot(const Matrix4 &leadTrans) : _leadTrans(leadTrans) {};

  //@{ implement IKAnimationParam
  virtual Matrix4 GetEndBoneTrans(const Matrix4Array &matrices, const Shape *shape, SkeletonIndex endBone) const;  
  virtual void SetAfterEndBoneTrans( Matrix4Array &matrices, const Shape *shape, Matrix4& endBone, SkeletonIndex endBoneIndx,const AutoArray<SkeletonIndex> &afterEndBone, float blendFactor) const;  
  //@}
};

#endif
