#include "wpch.hpp"

#include "object.hpp"
#include "rtAnimation.hpp"
#include "IKengine.hpp"

#include <El/ParamFile/paramFile.hpp>

void IKEngine::CalculateBones(AutoArray<Matrix4>& matrices, 
                    Matrix4Val boneStart,  // start  bone cannot be changed
                    Matrix4& boneEnd, // If bone is too faraway outside 
                    float blendFactor) // blending factor >= 1 - full IK, <= no IK
                    const
{
  // actual position of the points
  Vector3 p0act = boneStart * _p0Src; 
  Vector3 p2actOld = matrices[matrices.Size() - 1] * _p2Src;
  Vector3 p2act = boneEnd * _p2Src;

  // current direction
  Vector3 dirOld = p2actOld - p0act;
  dirOld.Normalize();

  // desired direction
  Vector3 dir = p2act - p0act;
  dir.Normalize();

  /// Read vDir from data
  Vector3 p1actOld = matrices[_jointBone] * _p1Src;
  Vector3 vDirOld = p0act - p1actOld;
  vDirOld -= dirOld * (vDirOld * dirOld);
  
  if (vDirOld.SquareSize() < _l0 * _l0 * 0.17f * 0.17f)  // sin 10 
  {
    vDirOld = matrices[_jointBone].Rotate( _vDirSrc);
    vDirOld -= dirOld * (vDirOld * dirOld);   
  }  

  vDirOld.Normalize();

  // transform it to new dir
  Vector3 rootAxe = dirOld.CrossProduct(dir);

  Vector3 vDir;
  if (rootAxe.SquareSize() < 0.0001f)
  {
    //use the same
    vDir = vDirOld;
  }
  else
  {       
    rootAxe.Normalize();
    Matrix3 old;
    old.SetDirectionAside(rootAxe.CrossProduct(dirOld));
    old.SetDirectionUp(rootAxe);
    old.SetDirection(dirOld);

    Matrix3 diffMat;
    diffMat.SetDirectionAside(rootAxe.CrossProduct(dir));
    diffMat.SetDirectionUp(rootAxe);
    diffMat.SetDirection(dir);

    vDir = diffMat * (old.InverseRotation() * vDirOld);

    Assert(vDir *  dir < 0.0001f); // cendos - moved here from below, it seems it signals assert every time when rootAxe length is below limit
  }

  //Assert(vDir *  dir < 0.0001f);
  
  Vector3  wristRot = boneEnd.Rotate( _vDirSrc);

  // recalculate bones
  float l = (p2act - p0act).Size();
  float k = (_l0*_l0 - _l1*_l1 + l*l)/ ( 2*l*l);
  if (_l0 < k*l + 0.0001f)
    k = _l0/l - 0.0001f; // numerical barrier

  Vector3 p1act = p0act + k * l * dir - vDir * sqrtf(_l0*_l0  - k*k*l*l);    
  Vector3 dir01act = p1act - p0act;

  /// first implementation do not solve rotations
  Vector3 animDir = matrices[0].Rotate(_vDirSrc);

  if (blendFactor>=1.0f)
  {
    //calculate new transform of bone
  for(int i = 0; i <= _jointBone; i++)
  {
    Matrix4 newTrans;
    newTrans.SetDirectionAndUp(dir01act, animDir * (1 - _rotationFrac[i]) +  _rotationFrac[i] * vDir);      
    newTrans.SetPosition(p0act);  
    matrices[i] = newTrans * _srcInv01;    
  }
  }
  else
  {
    // blend new IK transform and original bone matrix
    for(int i = 0; i <= _jointBone; i++)
    {
      Matrix4 newTrans;
      newTrans.SetDirectionAndUp(dir01act, animDir * (1 - _rotationFrac[i]) +  _rotationFrac[i] * vDir);      
      newTrans.SetPosition(p0act);  
      matrices[i] = matrices[i]*(1-blendFactor) + newTrans * _srcInv01 * blendFactor;
    }
  }

  Vector3 dir21act = p2act - p1act;
  if (blendFactor>=1.0f)
  {
  for(int i = _jointBone + 1; i < matrices.Size(); i++)
  {
    Matrix4 newTrans;
    newTrans.SetDirectionAndUp(dir21act, vDir * (1 - _rotationFrac[i]) +  _rotationFrac[i] * wristRot);
    newTrans.SetPosition(p1act);  
    matrices[i] = newTrans * _srcInv12;
  }
}
  else
  {
    for(int i = _jointBone + 1; i < matrices.Size(); i++)
    {
      Matrix4 newTrans;
      newTrans.SetDirectionAndUp(dir21act, vDir * (1 - _rotationFrac[i]) +  _rotationFrac[i] * wristRot);
      newTrans.SetPosition(p1act);  

      matrices[i] = matrices[i]*(1-blendFactor) + newTrans * _srcInv12 * blendFactor;
    }
  }
}

void IKEngine::Init(Vector3Val p0src, Vector3Val p1src, Vector3Val p2src, Vector3Val jointDir, int jointBone, const AutoArray<float>& rotationFrac)
{ 
  _p0Src = p0src;
  _p1Src = p1src;
  _p2Src = p2src;

  _vDirSrc = jointDir;

  _jointBone = jointBone;

  _rotationFrac = rotationFrac;

  _srcInv01.SetDirectionAndUp(p1src - p0src, _vDirSrc);
  _srcInv01.SetPosition(p0src);
  _srcInv01 = _srcInv01.InverseRotation();

  _srcInv12.SetDirectionAndUp(p2src - p1src, _vDirSrc);
  _srcInv12.SetPosition(p1src);
  _srcInv12 = _srcInv12.InverseRotation();

  _l0 = (p1src - p0src).Size();
  _l1 = (p2src - p1src).Size();
}


void IKAnimation::CombineTransform(Matrix4Array &matrices, LODShape *lShape, int level, float blendFactor, const IKAnimationParam& par ) const
{
  // Get the shape and make sure it already exists
  DoAssert(lShape->IsLevelLocked(level));
  const Shape *shape = lShape->GetLevelLocked(level);

  AutoArray<Matrix4> matricesForIK;  

  int nMatrices = _bones.Size();
  matricesForIK.Realloc(nMatrices,nMatrices);
  
  for(int i = 0; i < _bones.Size(); i++)
  {
    const SubSkeletonIndexSet &indxs = shape->GetSubSkeletonIndexFromSkeletonIndex(_bones[i]);
    if (indxs.Size() > 0)
    {       
      matricesForIK.Add(matrices[GetSubSkeletonIndex(indxs[0])]);     
    }        
    else
      return; /// nothing to animate
  }  

  Matrix4 endBone = par.GetEndBoneTrans(matrices, shape, _endBone);

  const SubSkeletonIndexSet &startBoneIndxs = shape->GetSubSkeletonIndexFromSkeletonIndex(_startBone);
  Assert(startBoneIndxs.Size() > 0);
  Matrix4 startBone = matrices[GetSubSkeletonIndex(startBoneIndxs[0])];

  // do ik calculation
  CalculateBones(matricesForIK, startBone, endBone, blendFactor);

  //set bones...
  for(int i = 0; i < _bones.Size(); i++)
  {
    const SubSkeletonIndexSet &indxs = shape->GetSubSkeletonIndexFromSkeletonIndex(_bones[i]);
    if (indxs.Size() > 0)
    {        
      matrices[GetSubSkeletonIndex(indxs[0])] = matricesForIK[i];        
      REPLICATE_FIRST_BONE(matrices, indxs);
    }      
  }

  const SubSkeletonIndexSet &endBoneIndxs = shape->GetSubSkeletonIndexFromSkeletonIndex(_endBone);
  if (blendFactor>=1.0f)
  {
  if (endBoneIndxs.Size() > 0)
  {        
    matrices[GetSubSkeletonIndex(endBoneIndxs[0])] = endBone;        
    REPLICATE_FIRST_BONE(matrices, endBoneIndxs);
  }      
  }
  else
  {
    if (endBoneIndxs.Size() > 0)
    {        
      int index = GetSubSkeletonIndex(endBoneIndxs[0]);
      matrices[index] = matrices[index]*(1-blendFactor) + endBone*blendFactor;        
      REPLICATE_FIRST_BONE(matrices, endBoneIndxs);
    }  
  }

  par.SetAfterEndBoneTrans(matrices, shape, endBone,_endBone, _afterEndBone,blendFactor);
}

/**
@right determine if we are setting up the right or left hand
*/
void IKHandAnimation::Init(LODShape *lShape, bool right, const ParamEntry &par)
{
  _bones.Resize(0); // reset arrays, this function can be called more that once
  _afterEndBone.Resize(0);

  AutoArray<float> rotationFrac;
  ParamEntryVal armToElbow  =  par >> (right ? "rightArmToElbow" :"leftArmToElbow");
  int n = armToElbow.GetSize() / 2;
  _bones.Realloc(n);
  for(int i = 0; i < n; i++)
  {
    RString boneName = armToElbow[2*i];
    boneName.Lower();
    SkeletonIndex boneIndx = lShape->FindBone(boneName);
    if (SkeletonIndexValid(boneIndx))
    {
      _bones.Add(boneIndx);
      rotationFrac.Add(armToElbow[2 *i + 1]);
    }
    else
    {
      RptF("IK: unknown bone %s in %s",cc_cast(boneName),cc_cast(par.GetContext()));
    }

  }

  int jointBone = n - 1;

  ParamEntryVal armFromElbow  =  par >> (right ? "rightArmFromElbow" :"leftArmFromElbow");
  n = armFromElbow.GetSize() / 2;
  for(int i = 0; i < n; i++)
  {
    RString boneName = armFromElbow[2*i];
    boneName.Lower();
    SkeletonIndex boneIndx = lShape->FindBone(boneName);
    if (SkeletonIndexValid(boneIndx))
    {
      _bones.Add(boneIndx);
      rotationFrac.Add(armFromElbow[2 *i + 1]);
    }
    else
    {
      RptF("IK: unknown bone %s in %s",cc_cast(boneName),cc_cast(par.GetContext()));
    }
  }

  ParamEntryVal leftHand  =  par >> (right ?  "rightHand" : "leftHand");
  n = leftHand.GetSize();
  _afterEndBone.Realloc(n);
  for(int i = 0; i < n; i++)
  {
    RString boneName = leftHand[i];
    boneName.Lower();
    SkeletonIndex boneIndx = lShape->FindBone(boneName);
    //DoAssert(GetSkeletonIndex(boneIndx) >= 0);
    if (GetSkeletonIndex(boneIndx) >= 0)
    {
      _afterEndBone.Add(boneIndx);
    }
    else
    {
      RptF("IK: unknown bone %s in %s",cc_cast(boneName),cc_cast(par.GetContext()));
    }
  }

  RString endBoneName = par >> (right ? "rightWrist" : "leftWrist");
  endBoneName.Lower();
  _endBone =lShape->FindBone(endBoneName);

  RString startBoneName = par >> (right ? "rightShoulder" : "leftShoulder");
  startBoneName.Lower();
  _startBone =lShape->FindBone(startBoneName);

  if (SkeletonIndexValid(_startBone) && SkeletonIndexValid(_endBone))
  {
    ParamEntryVal armPoints  =  par >> (right ? "rightArmPoints" : "leftArmPoints");

    DoAssert(armPoints.IsArray() && armPoints.GetSize() == 4);

    Vector3 p0src = lShape->MemoryPoint(armPoints[0].operator RString()); 
    Vector3 p1src = lShape->MemoryPoint(armPoints[1].operator RString()); 
    Vector3 p1dir = lShape->MemoryPoint(armPoints[2].operator RString()) - p1src; 
    p1dir.Normalize();
    Vector3 p2src = lShape->MemoryPoint(armPoints[3].operator RString()); 

    base::Init(p0src, p1src, p2src, p1dir, jointBone, rotationFrac);
  }
}

void IKLegsAnimation::Init(LODShape *lShape, bool right, const ParamEntry &par)
{
  _bones.Resize(0); // reset arrays, this function can be called more that once
  _afterEndBone.Resize(0);

  AutoArray<float> rotationFrac;
  ParamEntryVal LegToKnee  =  par >> (right ? "rightLegToKnee" :"leftLegToKnee");
  int n = LegToKnee.GetSize() / 2;
  for(int i = 0; i < n; i++)
  {
    RString boneName = LegToKnee[2*i];
    boneName.Lower();
    SkeletonIndex boneIndx = lShape->FindBone(boneName);
    if (SkeletonIndexValid(boneIndx))
    {
      _bones.Add(boneIndx);
      rotationFrac.Add(LegToKnee[2 *i + 1]);
    }
    else
    {
      RptF("IK: unknown bone %s in %s",cc_cast(boneName),cc_cast(par.GetContext()));
    }
  }

  int jointBone = n - 1;

  ParamEntryVal legFromKnee  =  par >> (right ? "rightLegFromKnee" :"leftLegFromKnee");
  n = legFromKnee.GetSize() / 2;
  for(int i = 0; i < n; i++)
  {
    RString boneName = legFromKnee[2*i];
    boneName.Lower();
    SkeletonIndex boneIndx = lShape->FindBone(boneName);
    if (SkeletonIndexValid(boneIndx))
    {
      _bones.Add(boneIndx);
      rotationFrac.Add(legFromKnee[2 *i + 1]);
    }
    else
    {
      RptF("IK: unknown bone %s in %s",cc_cast(boneName),cc_cast(par.GetContext()));
    }
  }

  ParamEntryVal leftFoot  =  par >> (right ?  "rightFoot" : "leftFoot");
  n = leftFoot.GetSize();
  for(int i = 0; i < n; i++)
  {
    RString boneName = leftFoot[i];
    boneName.Lower();
    SkeletonIndex boneIndx = lShape->FindBone(boneName);
    if (SkeletonIndexValid(boneIndx))
    {
      _afterEndBone.Add(boneIndx);
    }
    else
    {
      RptF("IK: unknown bone %s in %s",cc_cast(boneName),cc_cast(par.GetContext()));
    }
  }

  RString endBoneName = par >> (right ? "rightHeel" : "leftHeel");
  endBoneName.Lower();
  _endBone =lShape->FindBone(endBoneName);

  RString startBoneName = par >> (right ? "rightHip" : "leftHip");
  startBoneName.Lower();
  _startBone =lShape->FindBone(startBoneName);

  if (SkeletonIndexValid(_startBone) && SkeletonIndexValid(_endBone))
  {
    ParamEntryVal legPoints  =  par >> (right ? "rightLegPoints" : "leftLegPoints");

    DoAssert(legPoints.IsArray() && legPoints.GetSize() == 4);

    Vector3 p0src = lShape->MemoryPoint(legPoints[0].operator RString()); 
    Vector3 p1src = lShape->MemoryPoint(legPoints[1].operator RString()); 
    Vector3 p1dir = lShape->MemoryPoint(legPoints[2].operator RString()) - p1src; 
    p1dir.Normalize();
    Vector3 p2src = lShape->MemoryPoint(legPoints[3].operator RString()); 

    base::Init(p0src, p1src, p2src, p1dir, jointBone, rotationFrac);
  }
}

Matrix4 AnimationRTIKParam::GetEndBoneTrans(const Matrix4Array &matrices, const Shape *shape, SkeletonIndex endBone) const 
{
  const SubSkeletonIndexSet &leadBoneSet = shape->GetSubSkeletonIndexFromSkeletonIndex(_leadBone);
  if (leadBoneSet.Size() == 0)
    return M4Identity;

  // AddPreloadCount was used here before, which seems wrong - see also SetAfterEndBoneTrans
  // the animation is loaded permanently, we cannot manipulate its load count, as it would not be MT safe anyway

  // find diff transform...    
  Matrix4 trans = matrices[GetSubSkeletonIndex(leadBoneSet[0])] * _handAnim->GetMatrix(0, _leadBone).InverseGeneral();
  trans = trans* _handAnim->GetMatrix(0, endBone);
  //_handAnim->ReleaseLoadCount();

  return trans;
}

void AnimationRTIKParam::SetAfterEndBoneTrans( Matrix4Array &matrices, const Shape *shape, Matrix4& endBone, SkeletonIndex endBoneIndx, const AutoArray<SkeletonIndex> &afterEndBone, float blendFactor) const
{
  Matrix4 trans = endBone * _handAnim->GetMatrix(0, endBoneIndx).InverseGeneral();
  if (blendFactor>=1.0f)
  {
  for(int i = 0; i < afterEndBone.Size();i++)
  {
    const SubSkeletonIndexSet &ssis = shape->GetSubSkeletonIndexFromSkeletonIndex(afterEndBone[i]);
    if (ssis.Size()> 0)
    {
      matrices[GetSubSkeletonIndex(ssis[0])] = trans *  _handAnim->GetMatrix(0, afterEndBone[i]);
      REPLICATE_FIRST_BONE(matrices, ssis);
    }
  }
}
  else
  {
    for(int i = 0; i < afterEndBone.Size();i++)
    {
      const SubSkeletonIndexSet &ssis = shape->GetSubSkeletonIndexFromSkeletonIndex(afterEndBone[i]);
      if (ssis.Size()> 0)
      {
        int index = GetSubSkeletonIndex(ssis[0]);
        matrices[index] = matrices[index]*(1-blendFactor) + trans * _handAnim->GetMatrix(0, afterEndBone[i]) * blendFactor;
        REPLICATE_FIRST_BONE(matrices, ssis);
      }
    }
  }
}


Matrix4 AnimationRTIKParamPilot::GetEndBoneTrans(const Matrix4Array &matrices, const Shape *shape, SkeletonIndex endBone) const 
{
  const SubSkeletonIndexSet &leadBoneSet = shape->GetSubSkeletonIndexFromSkeletonIndex(endBone);
  if (leadBoneSet.Size()==0) 
    return M4Identity;

  return _leadTrans * matrices[GetSubSkeletonIndex(leadBoneSet[0])] ;
}

void AnimationRTIKParamPilot::SetAfterEndBoneTrans( Matrix4Array &matrices, const Shape *shape, Matrix4& endBone, SkeletonIndex endBoneIndx,const AutoArray<SkeletonIndex> &afterEndBone, float blendFactor) const
{
  if (blendFactor>=1.0f)
  {
    for(int i = 0; i < afterEndBone.Size();i++)
    {
      const SubSkeletonIndexSet &ssis = shape->GetSubSkeletonIndexFromSkeletonIndex(afterEndBone[i]);
      if (ssis.Size()> 0)
      {
        int index = GetSubSkeletonIndex(ssis[0]);
        matrices[index] = _leadTrans * matrices[index];
        REPLICATE_FIRST_BONE(matrices, ssis);
      }
    }
  }
  else
  {
    for(int i = 0; i < afterEndBone.Size();i++)
    {
      const SubSkeletonIndexSet &ssis = shape->GetSubSkeletonIndexFromSkeletonIndex(afterEndBone[i]);
      if (ssis.Size()> 0)
      {
        int index = GetSubSkeletonIndex(ssis[0]);
        matrices[index] = matrices[index]*(1-blendFactor) +  _leadTrans * matrices[index] * blendFactor;
        REPLICATE_FIRST_BONE(matrices, ssis);
      }
    }
  }
}

